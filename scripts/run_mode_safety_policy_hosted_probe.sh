#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/mode-safety-policy-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/mode-safety-policy-hosted-probe.log}"
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
import sys
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
    forbidden_mode: Optional[str] = None


bin_dir = pathlib.Path(require_env("BIN_DIR"))
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
probe_log = pathlib.Path(require_env("PROBE_LOG"))

cases = (
    ProbeCase("safe_to_hell", 9.0, ("status",), "HELL"),
    ProbeCase("idle_to_safe", 39.0, ("mode idle", "status"), "SAFE"),
    ProbeCase("payload_to_safe", 39.0, ("mode idle", "mode payload", "status"), "SAFE"),
    ProbeCase("ttc_to_safe", 39.0, ("mode idle", "mode ttc", "status"), "SAFE"),
    ProbeCase("safe_high_soc_remains_safe", 60.0, ("status",), "SAFE", "IDLE"),
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


def run_case(index: int, case: ProbeCase, summary_lines: list[str]) -> None:
    runtime_root = probe_tmp_dir / f"runtime-{case.name}"
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)

    csp_sub_port = 56280 + index
    csp_pub_port = 57280 + index
    radio_port = 17080 + index
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
            processes.append(
                subprocess.Popen(
                    [str(bin_dir / "eps_simulator"), "--node-id", "2", "--initial-soc", f"{case.initial_soc:.2f}"],
                    env=env,
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            )
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
            try:
                for command in case.commands:
                    if obc.poll() is not None:
                        if reader_thread is not None:
                            reader_thread.join(timeout=2.0)
                        append_obc_output(log_handle, obc_lines, obc)
                        raise RuntimeError(f"{case.name}: OBC exited before command '{command}'; see {case_log}")
                    obc.stdin.write(command + "\n")
                    obc.stdin.flush()
                    time.sleep(1.0)
                if obc.poll() is not None:
                    if reader_thread is not None:
                        reader_thread.join(timeout=2.0)
                    append_obc_output(log_handle, obc_lines, obc)
                    raise RuntimeError(f"{case.name}: OBC exited before final status; see {case_log}")
                obc.stdin.write("status\n")
                obc.stdin.flush()
                time.sleep(1.0)
                if obc.poll() is not None:
                    if reader_thread is not None:
                        reader_thread.join(timeout=2.0)
                    append_obc_output(log_handle, obc_lines, obc)
                    raise RuntimeError(f"{case.name}: OBC exited before quit; see {case_log}")
                obc.stdin.write("quit\n")
                obc.stdin.flush()
                obc.stdin.close()
            except BrokenPipeError as exc:
                if reader_thread is not None:
                    reader_thread.join(timeout=2.0)
                append_obc_output(log_handle, obc_lines, obc)
                raise RuntimeError(f"{case.name}: OBC stdin closed while sending commands; see {case_log}") from exc

            graceful_shutdown = True
            try:
                obc.wait(timeout=10.0)
            except subprocess.TimeoutExpired:
                graceful_shutdown = False
            if reader_thread is not None:
                reader_thread.join(timeout=2.0)

            output = append_obc_output(log_handle, obc_lines, obc)

            if obc.returncode is not None and obc.returncode != 0:
                raise RuntimeError(f"{case.name}: OBC exited with {obc.returncode}; see {case_log}")

            observed = mode_lines(output)
            if not observed:
                raise RuntimeError(f"{case.name}: no mode status lines observed; see {case_log}")
            final_mode_line = observed[-1]
            expected_line = f"mode={case.expected_final_mode} "
            if expected_line not in final_mode_line:
                raise RuntimeError(
                    f"{case.name}: expected final {case.expected_final_mode}, got '{final_mode_line}'; see {case_log}"
                )
            if case.forbidden_mode is not None and any(f"mode={case.forbidden_mode} " in line for line in observed):
                raise RuntimeError(f"{case.name}: observed forbidden mode {case.forbidden_mode}; see {case_log}")

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
print(f"mode_safety_policy_hosted_probe: PASS log={probe_log}")
for line in summary:
    print(f"  {line}")
PY
