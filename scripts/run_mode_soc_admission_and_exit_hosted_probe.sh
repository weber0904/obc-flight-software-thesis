#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/mode-soc-admission-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/mode-soc-admission-hosted-probe.log}"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
PROBE_LOG="${PROBE_LOG}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import shutil
import signal
import subprocess
import threading
import time
from dataclasses import dataclass
from typing import Optional


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


@dataclass(frozen=True)
class ProbeCase:
    name: str
    initial_soc: float
    commands: tuple[str, ...]
    expected_final_mode: str
    expected_fragments: tuple[str, ...] = ()
    forbidden_after_fragments: tuple[tuple[str, str], ...] = ()
    restart_soc: Optional[float] = None
    restart_expected_fragment: Optional[str] = None
    restart_delay_sec: float = 3.0


bin_dir = pathlib.Path(require_env("BIN_DIR"))
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
probe_log = pathlib.Path(require_env("PROBE_LOG"))

cases = (
    ProbeCase(
        "safe_to_idle_accept",
        60.0,
        ("mode idle",),
        "IDLE",
        ("SYS_MODE_CHANGE : System mode changed to IDLE (1)",),
    ),
    ProbeCase(
        "safe_to_idle_reject_boundary",
        50.0,
        ("mode idle",),
        "SAFE",
        ("SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from SAFE (0) to IDLE (1) reason 4",),
    ),
    ProbeCase(
        "idle_to_payload_accept",
        71.0,
        ("mode idle", "mode payload"),
        "PAYLOAD",
        ("SYS_MODE_CHANGE : System mode changed to PAYLOAD (3)",),
    ),
    ProbeCase(
        "idle_to_payload_reject_boundary",
        70.0,
        ("mode idle", "mode payload"),
        "IDLE",
        ("SYS_MODE_TRANSITION_REJECTED : System mode transition rejected from IDLE (1) to PAYLOAD (3) reason 4",),
    ),
    ProbeCase(
        "idle_to_ttc_unchanged",
        60.0,
        ("mode idle", "mode ttc"),
        "TTC",
        ("SYS_MODE_CHANGE : System mode changed to TTC (4)",),
    ),
    ProbeCase(
        "payload_to_idle_automatic",
        71.0,
        ("mode idle", "mode payload"),
        "IDLE",
        ("SYS_MODE_CHANGE : System mode changed to PAYLOAD (3)", "SYS_MODE_CHANGE : System mode changed to IDLE (1)"),
        restart_soc=59.0,
        restart_expected_fragment=(
            "MODE_SAFETY_TRANSITION : Mode safety transition PAYLOAD (3) -> IDLE (1) at SOC 59.000000"
        ),
    ),
    ProbeCase(
        "payload_to_safe_priority",
        71.0,
        ("mode idle", "mode payload"),
        "SAFE",
        ("SYS_MODE_CHANGE : System mode changed to PAYLOAD (3)", "SYS_MODE_CHANGE : System mode changed to SAFE (0)"),
        restart_soc=39.0,
        restart_expected_fragment=(
            "MODE_SAFETY_TRANSITION : Mode safety transition PAYLOAD (3) -> SAFE (0) at SOC 39.000000"
        ),
        forbidden_after_fragments=(
            (
                "SYS_MODE_CHANGE : System mode changed to PAYLOAD (3)",
                "SYS_MODE_CHANGE : System mode changed to IDLE (1)",
            ),
        ),
    ),
)


def cleanup_port(port: int, expected_command_substring: str) -> None:
    result = subprocess.run(
        ["lsof", "-nP", f"-iTCP:{port}", "-sTCP:LISTEN", "-Fpct"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return

    pid: Optional[str] = None
    command = ""
    for raw_line in result.stdout.splitlines():
        if raw_line.startswith("p"):
            pid = raw_line[1:]
            command = ""
        elif raw_line.startswith("c"):
            command = raw_line[1:]
        elif raw_line.startswith("t") and raw_line[1:] == "IPv4":
            if pid and expected_command_substring in command:
                try:
                    os.kill(int(pid), signal.SIGTERM)
                except OSError:
                    pass
            pid = None
            command = ""


def collect_output(process: subprocess.Popen[str], lines: list[str]) -> threading.Thread:
    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def append_obc_output(log_handle, lines: list[str], process: subprocess.Popen[str]) -> str:
    output = "".join(lines)
    log_handle.write("\n--- OBC stdout ---\n")
    log_handle.write(output)
    log_handle.write(f"\n--- OBC returncode={process.poll()} ---\n")
    return output


def terminate(processes: list[subprocess.Popen[str]]) -> None:
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
    for process in reversed(processes):
        if process.poll() is None:
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5.0)


def mode_lines(output: str) -> list[str]:
    return [line for line in output.splitlines() if line.startswith("mode=")]


def wait_for_fragment(lines: list[str], fragment: str, timeout_sec: float, start_index: int = 0) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in "".join(lines[start_index:]):
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for fragment {fragment!r}")


def start_eps(env: dict[str, str], initial_soc: float, log_handle) -> subprocess.Popen[str]:
    return subprocess.Popen(
        [str(bin_dir / "eps_simulator"), "--node-id", "2", "--initial-soc", f"{initial_soc:.2f}"],
        env=env,
        stdin=subprocess.DEVNULL,
        stdout=log_handle,
        stderr=subprocess.STDOUT,
        text=True,
    )


def run_case(index: int, case: ProbeCase, summary_lines: list[str]) -> None:
    runtime_root = probe_tmp_dir / f"runtime-{case.name}"
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)

    csp_sub_port = 56320 + index
    csp_pub_port = 57320 + index
    radio_port = 17120 + index
    case_log = probe_tmp_dir / f"{case.name}.log"

    cleanup_port(csp_sub_port, "csp_zmq")
    cleanup_port(csp_pub_port, "csp_zmq")
    cleanup_port(radio_port, "radio_mock")
    time.sleep(0.3)

    env = os.environ.copy()
    env["CSP_TRANSPORT"] = "zmqhub"
    env["CSP_HUB_HOST"] = "127.0.0.1"
    env["CSP_HUB_SUB_PORT"] = str(csp_sub_port)
    env["CSP_HUB_PUB_PORT"] = str(csp_pub_port)
    env["EPS_CSP_NODE_ID"] = "2"
    env["ADCS_CSP_NODE_ID"] = "3"

    processes: list[subprocess.Popen[str]] = []
    obc_lines: list[str] = []
    reader_thread: Optional[threading.Thread] = None
    with case_log.open("w", encoding="utf-8", buffering=1) as log_handle:
        try:
            processes.append(
                subprocess.Popen(
                    [
                        str(bin_dir / "csp_zmqproxy"),
                        "-s",
                        f"tcp://0.0.0.0:{csp_sub_port}",
                        "-p",
                        f"tcp://0.0.0.0:{csp_pub_port}",
                    ],
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            )
            eps_process = start_eps(env, case.initial_soc, log_handle)
            processes.append(eps_process)
            processes.append(
                subprocess.Popen(
                    [str(bin_dir / "radio_mock_server"), "--port", str(radio_port)],
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            )

            time.sleep(1.0)
            obc = subprocess.Popen(
                [
                    str(bin_dir / "OBC"),
                    "--comm",
                    "tcp",
                    "--comm-host",
                    "127.0.0.1",
                    "--comm-port",
                    str(radio_port),
                    "--radio-protocol",
                    "mock-text",
                    "--ground-link",
                    "disabled",
                    "--runtime-root",
                    str(runtime_root),
                    "--persistent-root",
                    str(runtime_root / "persistent-data"),
                    "--staging-root",
                    str(runtime_root / "staging"),
                    "--tick-ms",
                    "100",
                ],
                env=env,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
            )
            processes.append(obc)
            reader_thread = collect_output(obc, obc_lines)
            time.sleep(1.5)

            assert obc.stdin is not None
            for command in case.commands:
                if obc.poll() is not None:
                    raise RuntimeError(f"{case.name}: OBC exited before command '{command}'")
                obc.stdin.write(command + "\n")
                obc.stdin.flush()
                time.sleep(1.0)

            if case.restart_soc is not None:
                restart_start_index = len(obc_lines)
                eps_process.terminate()
                try:
                    eps_process.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    eps_process.kill()
                    eps_process.wait(timeout=5.0)
                processes.remove(eps_process)
                replacement = start_eps(env, case.restart_soc, log_handle)
                processes.append(replacement)
                if case.restart_expected_fragment is None:
                    raise RuntimeError(f"{case.name}: restart probe case is missing restart_expected_fragment")
                wait_for_fragment(
                    obc_lines,
                    case.restart_expected_fragment,
                    case.restart_delay_sec + 4.0,
                    start_index=restart_start_index,
                )
                time.sleep(case.restart_delay_sec)

            if obc.poll() is not None:
                raise RuntimeError(f"{case.name}: OBC exited before final status")

            obc.stdin.write("status\n")
            obc.stdin.flush()
            time.sleep(1.0)
            obc.stdin.write("quit\n")
            obc.stdin.flush()
            obc.stdin.close()

            graceful_shutdown = True
            try:
                obc.wait(timeout=10.0)
            except subprocess.TimeoutExpired:
                graceful_shutdown = False
            if reader_thread is not None:
                reader_thread.join(timeout=2.0)

            output = append_obc_output(log_handle, obc_lines, obc)
            if obc.returncode is not None and obc.returncode != 0:
                raise RuntimeError(f"{case.name}: OBC exited with {obc.returncode}")

            observed = mode_lines(output)
            if not observed:
                raise RuntimeError(f"{case.name}: no mode status lines observed")
            final_mode_line = observed[-1]
            expected_line = f"mode={case.expected_final_mode} "
            if expected_line not in final_mode_line:
                raise RuntimeError(f"{case.name}: expected final {case.expected_final_mode}, got '{final_mode_line}'")
            for fragment in case.expected_fragments:
                if fragment not in output:
                    raise RuntimeError(f"{case.name}: missing expected fragment {fragment!r}")
            for marker, fragment in case.forbidden_after_fragments:
                marker_index = output.find(marker)
                if marker_index == -1:
                    raise RuntimeError(f"{case.name}: missing ordering marker {marker!r}")
                if output.find(fragment, marker_index + len(marker)) != -1:
                    raise RuntimeError(
                        f"{case.name}: observed forbidden fragment {fragment!r} after marker {marker!r}"
                    )

            summary_lines.append(
                f"{case.name}: initial_soc={case.initial_soc:.2f} final={case.expected_final_mode} "
                f"shutdown={'graceful' if graceful_shutdown else 'terminated'} log={case_log}"
            )
        finally:
            terminate(processes)


summary: list[str] = []
try:
    for index, probe_case in enumerate(cases):
        run_case(index, probe_case, summary)
except Exception as exc:
    probe_log.write_text("\n".join(summary + [f"FAIL: {exc}"]) + "\n", encoding="utf-8")
    raise

probe_log.write_text("\n".join(summary) + "\n", encoding="utf-8")
print(f"mode_soc_admission_and_exit_hosted_probe: PASS log={probe_log}")
for line in summary:
    print(f"  {line}")
PY
