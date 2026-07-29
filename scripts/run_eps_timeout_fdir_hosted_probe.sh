#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/eps-timeout-fdir-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/eps-timeout-fdir-hosted-probe.log}"
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
import socket
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
    expect_fault: bool
    expect_recovery: bool
    expected_final_mode: str


BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
PROBE_LOG = pathlib.Path(require_env("PROBE_LOG"))

CASES = (
    ProbeCase(
        name="transient_two_failures_no_safe",
        expect_fault=False,
        expect_recovery=False,
        expected_final_mode="IDLE",
    ),
    ProbeCase(
        name="threshold_crossing_to_safe",
        expect_fault=True,
        expect_recovery=False,
        expected_final_mode="SAFE",
    ),
    ProbeCase(
        name="recovery_after_fault_clear",
        expect_fault=True,
        expect_recovery=True,
        expected_final_mode="SAFE",
    ),
)

RETRY_1 = "EPS FDIR retrying after consecutive failure count 1"
RETRY_2 = "EPS FDIR retrying after consecutive failure count 2"
FAULT_EVENT = "EPS FDIR fault latched at failure count 3"
RECOVERY_EVENT = "EPS FDIR fault cleared after recovery from failure count"
SAFE_MODE_EVENT = "System mode changed to SAFE"
EPS_STATUS_EVENT = "EPS_STATUS_RECEIVED"
RETRY_TIMEOUT = 5.0
FAULT_TIMEOUT = 7.0
RECOVERY_TIMEOUT = 6.0
ACTIVE_PROCESSES: list[subprocess.Popen[str]] = []
ACTIVE_CASE_NAME = ""


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


def port_is_available(port: int) -> bool:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind(("0.0.0.0", port))
        return True
    except OSError:
        return False
    finally:
        sock.close()


def find_free_port(start: int) -> int:
    candidate = start
    while not port_is_available(candidate):
        candidate += 1
    return candidate


def find_free_port_pair(first_port: int, second_offset: int) -> int:
    candidate = first_port
    while (not port_is_available(candidate)) or (not port_is_available(candidate + second_offset)):
        candidate += 1
    return candidate


def terminate_process(process: Optional[subprocess.Popen[str]]) -> None:
    if process is None or process.poll() is not None:
        return
    try:
        os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    except OSError:
        process.terminate()
    try:
        process.wait(timeout=5.0)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        except OSError:
            process.kill()
        process.wait(timeout=5.0)


def terminate_all(processes: list[subprocess.Popen[str]]) -> None:
    for process in reversed(processes):
        terminate_process(process)


def handle_termination(signum, _frame) -> None:
    terminate_all(ACTIVE_PROCESSES)
    signal_name = signal.Signals(signum).name
    message = f"eps_timeout_fdir_hosted_probe: received {signal_name}"
    print(message, file=sys.stderr, flush=True)
    raise SystemExit(128 + signum)


def collect_output(process: subprocess.Popen[str], lines: list[str]) -> threading.Thread:
    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def output_since(lines: list[str], start_index: int) -> str:
    return "".join(lines[start_index:])


def write_obc_output(log_handle, lines: list[str], process: Optional[subprocess.Popen[str]]) -> str:
    output = "".join(lines)
    log_handle.write("\n--- OBC stdout ---\n")
    log_handle.write(output)
    log_handle.write(f"\n--- OBC returncode={(process.poll() if process is not None else 'none')} ---\n")
    return output


def wait_for_substring(lines: list[str], needle: str, timeout: float, start_index: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if needle in output_since(lines, start_index):
            return
        time.sleep(0.05)
    raise RuntimeError(f"timed out waiting for '{needle}'")


def wait_for_absence(lines: list[str], needle: str, timeout: float, start_index: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if needle in output_since(lines, start_index):
            raise RuntimeError(f"unexpected '{needle}' observed")
        time.sleep(0.05)


def request_status(process: subprocess.Popen[str], lines: list[str], timeout: float = 4.0) -> str:
    start_index = len(lines)
    send_command(process, "status")
    deadline = time.time() + timeout
    latest = ""
    while time.time() < deadline:
        text = output_since(lines, start_index)
        if text:
            latest = text
        if (
            "mode=" in text
            and "boot resetCause=" in text
            and "recovery activeCount=" in text
            and "adcs fdir transportOk=" in text
        ):
            return text
        time.sleep(0.05)
    if latest:
        return latest
    raise RuntimeError("timed out waiting for hosted status output")


def wait_for_status_contains(
    process: subprocess.Popen[str],
    lines: list[str],
    required: list[str],
    timeout: float,
    delay: float = 0.35,
) -> str:
    deadline = time.time() + timeout
    last_status = ""
    while time.time() < deadline:
        last_status = request_status(process, lines)
        if all(item in last_status for item in required):
            return last_status
        time.sleep(delay)
    raise RuntimeError(f"timed out waiting for status containing {required}; last status:\n{last_status}")


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command '{command}'")
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def extract_last_mode_line(text: str) -> str:
    mode_lines = [line for line in text.splitlines() if line.startswith("mode=")]
    if not mode_lines:
        raise RuntimeError("no mode lines observed")
    return mode_lines[-1]


def start_eps_process(env: dict[str, str], log_handle) -> subprocess.Popen[str]:
    return subprocess.Popen(
        [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "80.00"],
        env=env,
        stdin=subprocess.DEVNULL,
        stdout=log_handle,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )


def ensure_idle_mode(obc: subprocess.Popen[str], lines: list[str]) -> None:
    wait_for_status_contains(obc, lines, ["mode=SAFE ", "eps pdu=", "adcs mode=IDLE"], timeout=8.0)
    send_command(obc, "mode idle")
    wait_for_status_contains(obc, lines, ["mode=IDLE ", "eps pdu=", "adcs mode=IDLE"], timeout=6.0)


def run_case(index: int, case: ProbeCase, summary_lines: list[str]) -> None:
    global ACTIVE_CASE_NAME
    runtime_root = PROBE_TMP_DIR / f"runtime-{case.name}"
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)

    csp_sub_port = find_free_port_pair(56320 + (index * 10), 1000)
    csp_pub_port = csp_sub_port + 1000
    radio_port = find_free_port(17120 + (index * 10))
    case_log = PROBE_TMP_DIR / f"{case.name}.log"

    cleanup_port(csp_sub_port, "csp_zmq")
    cleanup_port(csp_pub_port, "csp_zmq")
    cleanup_port(radio_port, "radio_mock")
    time.sleep(0.2)

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
    csp_process: Optional[subprocess.Popen[str]] = None
    eps_process: Optional[subprocess.Popen[str]] = None
    adcs_process: Optional[subprocess.Popen[str]] = None
    obc: Optional[subprocess.Popen[str]] = None

    with case_log.open("w", encoding="utf-8", buffering=1) as log_handle:
        try:
            ACTIVE_CASE_NAME = case.name
            ACTIVE_PROCESSES.clear()
            csp_process = subprocess.Popen(
                [
                    str(BIN_DIR / "csp_zmqproxy"),
                    "-s",
                    f"tcp://0.0.0.0:{csp_sub_port}",
                    "-p",
                    f"tcp://0.0.0.0:{csp_pub_port}",
                ],
                stdin=subprocess.DEVNULL,
                stdout=log_handle,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
            processes.append(csp_process)
            ACTIVE_PROCESSES.append(csp_process)
            eps_process = start_eps_process(env, log_handle)
            processes.append(eps_process)
            ACTIVE_PROCESSES.append(eps_process)
            adcs_process = subprocess.Popen(
                [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=log_handle,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
            processes.append(adcs_process)
            ACTIVE_PROCESSES.append(adcs_process)
            radio_process = subprocess.Popen(
                [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
                stdin=subprocess.DEVNULL,
                stdout=log_handle,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
            processes.append(radio_process)
            ACTIVE_PROCESSES.append(radio_process)

            time.sleep(1.0)
            obc = subprocess.Popen(
                [
                    str(BIN_DIR / "OBC"),
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
                    "1000",
                ],
                env=env,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
            )
            processes.append(obc)
            ACTIVE_PROCESSES.append(obc)
            reader_thread = collect_output(obc, obc_lines)

            ensure_idle_mode(obc, obc_lines)

            outage_index = len(obc_lines)
            terminate_process(csp_process)
            terminate_process(eps_process)
            terminate_process(adcs_process)
            time.sleep(0.2)

            if case.expect_fault:
                wait_for_substring(obc_lines, RETRY_1, timeout=RETRY_TIMEOUT, start_index=outage_index)
                wait_for_substring(obc_lines, RETRY_2, timeout=RETRY_TIMEOUT, start_index=outage_index)
                wait_for_substring(obc_lines, FAULT_EVENT, timeout=FAULT_TIMEOUT, start_index=outage_index)
                wait_for_substring(obc_lines, SAFE_MODE_EVENT, timeout=FAULT_TIMEOUT, start_index=outage_index)
            else:
                wait_for_substring(obc_lines, RETRY_1, timeout=RETRY_TIMEOUT, start_index=outage_index)
                wait_for_substring(obc_lines, RETRY_2, timeout=RETRY_TIMEOUT, start_index=outage_index)
                wait_for_absence(obc_lines, FAULT_EVENT, timeout=0.05, start_index=outage_index)

            if case.expect_recovery or not case.expect_fault:
                restart_index = len(obc_lines)
                csp_process = subprocess.Popen(
                    [
                        str(BIN_DIR / "csp_zmqproxy"),
                        "-s",
                        f"tcp://0.0.0.0:{csp_sub_port}",
                        "-p",
                        f"tcp://0.0.0.0:{csp_pub_port}",
                    ],
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                    start_new_session=True,
                )
                processes.append(csp_process)
                ACTIVE_PROCESSES.append(csp_process)
                eps_process = start_eps_process(env, log_handle)
                processes.append(eps_process)
                ACTIVE_PROCESSES.append(eps_process)
                adcs_process = subprocess.Popen(
                    [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
                    env=env,
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                    start_new_session=True,
                )
                processes.append(adcs_process)
                ACTIVE_PROCESSES.append(adcs_process)
                wait_for_substring(obc_lines, EPS_STATUS_EVENT, timeout=RECOVERY_TIMEOUT, start_index=restart_index)

            if not case.expect_fault:
                wait_for_absence(obc_lines, FAULT_EVENT, timeout=1.5, start_index=outage_index)

            if case.expect_recovery:
                wait_for_substring(obc_lines, RECOVERY_EVENT, timeout=RECOVERY_TIMEOUT, start_index=outage_index)

            send_command(obc, "status")
            time.sleep(1.0)
            send_command(obc, "quit")
            assert obc.stdin is not None
            obc.stdin.close()

            try:
                obc.wait(timeout=10.0)
            except subprocess.TimeoutExpired:
                obc.terminate()
                obc.wait(timeout=5.0)

            if reader_thread is not None:
                reader_thread.join(timeout=2.0)

            output = write_obc_output(log_handle, obc_lines, obc)

            if obc.returncode not in (0, None):
                raise RuntimeError(f"{case.name}: OBC exited with {obc.returncode}")

            case_output = output_since(obc_lines, outage_index)
            if case_output.count(FAULT_EVENT) != (1 if case.expect_fault else 0):
                raise RuntimeError(
                    f"{case.name}: expected fault event count {(1 if case.expect_fault else 0)}, got {case_output.count(FAULT_EVENT)}"
                )
            if case.expect_fault and case_output.count(SAFE_MODE_EVENT) != 1:
                raise RuntimeError(f"{case.name}: expected exactly one SAFE mode-change event")
            if case.expect_recovery and case_output.count(RECOVERY_EVENT) != 1:
                raise RuntimeError(f"{case.name}: expected exactly one recovery event")
            if not case.expect_fault and RECOVERY_EVENT in case_output:
                raise RuntimeError(f"{case.name}: unexpected recovery event without fault")

            final_mode_line = extract_last_mode_line(output)
            if f"mode={case.expected_final_mode} " not in final_mode_line:
                raise RuntimeError(
                    f"{case.name}: expected final mode {case.expected_final_mode}, got '{final_mode_line}'"
                )

            if case.expect_fault and not case.expect_recovery:
                if "eps unavailable" not in output:
                    raise RuntimeError(f"{case.name}: expected eps unavailable after latched fault while simulator stayed down")
            else:
                if "eps unavailable" in final_mode_line:
                    raise RuntimeError(f"{case.name}: final status unexpectedly reports unavailable EPS")
                final_eps_lines = [line for line in output.splitlines() if line.startswith("eps ")]
                if not final_eps_lines or final_eps_lines[-1] == "eps unavailable":
                    raise RuntimeError(f"{case.name}: expected final EPS status after simulator recovery")

            summary_lines.append(
                f"  {case.name}: final={case.expected_final_mode} fault={case.expect_fault} recovery={case.expect_recovery} log={case_log}"
            )
        except Exception:
            if reader_thread is not None:
                reader_thread.join(timeout=2.0)
            write_obc_output(log_handle, obc_lines, obc)
            raise
        finally:
            terminate_all(processes)
            ACTIVE_PROCESSES.clear()
            ACTIVE_CASE_NAME = ""


def main() -> int:
    signal.signal(signal.SIGTERM, handle_termination)
    signal.signal(signal.SIGINT, handle_termination)
    summary_lines = [f"eps_timeout_fdir_hosted_probe: PASS log={PROBE_LOG}"]
    with PROBE_LOG.open("w", encoding="utf-8", buffering=1) as handle:
        for index, case in enumerate(CASES):
            run_case(index, case, summary_lines)
        for line in summary_lines:
            handle.write(line + "\n")
    print("\n".join(summary_lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
PY
