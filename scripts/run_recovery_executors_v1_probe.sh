#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/recovery-executors-v1.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/recovery-executors-v1-probe.log}"
RECOVERY_EXECUTOR_SCOPE="${RECOVERY_EXECUTOR_SCOPE:-full}"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
PROBE_LOG="${PROBE_LOG}" \
RECOVERY_EXECUTOR_SCOPE="${RECOVERY_EXECUTOR_SCOPE}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import pty
import shutil
import signal
import socket
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


BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
PROBE_LOG = pathlib.Path(require_env("PROBE_LOG"))
PROBE_SCOPE = os.environ.get("RECOVERY_EXECUTOR_SCOPE", "full").strip().lower()

PROCESS_RESTART_EXIT_CODE = 31
WATCHDOG_EXIT_CODE = 32
EPS_FAULT_TIMEOUT = 10.0
REBOOT_TIMEOUT = 10.0
READY_TIMEOUT = 8.0


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


def collect_output(process: subprocess.Popen[str], lines: list[str]) -> threading.Thread:
    def reader() -> None:
        stream = getattr(process, "_output_reader", None)
        if stream is None:
            assert process.stdout is not None
            stream = process.stdout
        for line in stream:
            lines.append(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def output_since(lines: list[str], start_index: int) -> str:
    return "".join(lines[start_index:])


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command '{command}'")
    control = getattr(process, "_control_writer", None)
    if control is not None:
        control.write(command + "\n")
        control.flush()
        return
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def wait_for_process_exit(process: subprocess.Popen[str], expected: int, timeout: float, label: str) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        code = process.poll()
        if code is not None:
            if code != expected:
                raise RuntimeError(f"{label}: expected exit {expected}, got {code}")
            return
        time.sleep(0.1)
    raise RuntimeError(f"{label}: timed out waiting for exit {expected}")


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise RuntimeError(f"{label}: expected '{needle}' in:\n{text}")


def request_status(process: subprocess.Popen[str], lines: list[str], timeout: float = 3.0) -> str:
    start_index = len(lines)
    send_command(process, "status")
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = output_since(lines, start_index)
        if "boot resetCause=" in text and "recovery activeCount=" in text and "watchdog aggregate=" in text:
            return text
        time.sleep(0.05)
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


def write_output(log_handle, lines: list[str], process: Optional[subprocess.Popen[str]], label: str) -> None:
    log_handle.write(f"\n--- {label} stdout ---\n")
    log_handle.write("".join(lines))
    log_handle.write(f"\n--- {label} returncode={(process.poll() if process is not None else 'none')} ---\n")


@dataclass
class Stack:
    name: str
    port_seed: int
    log_handle: object

    def __post_init__(self) -> None:
        self.csp_sub_port = find_free_port_pair(56320 + self.port_seed, 1000)
        self.csp_pub_port = self.csp_sub_port + 1000
        self.radio_port = find_free_port(17120 + self.port_seed)
        cleanup_port(self.csp_sub_port, "csp_zmq")
        cleanup_port(self.csp_pub_port, "csp_zmq")
        cleanup_port(self.radio_port, "radio_mock")
        self.processes: list[subprocess.Popen[str]] = []
        self.csp_process: Optional[subprocess.Popen[str]] = None
        self.eps_process: Optional[subprocess.Popen[str]] = None
        self.adcs_process: Optional[subprocess.Popen[str]] = None

    def env(self) -> dict[str, str]:
        env = os.environ.copy()
        env["CSP_TRANSPORT"] = "zmqhub"
        env["CSP_HUB_HOST"] = "127.0.0.1"
        env["CSP_HUB_SUB_PORT"] = str(self.csp_sub_port)
        env["CSP_HUB_PUB_PORT"] = str(self.csp_pub_port)
        env["EPS_CSP_NODE_ID"] = "2"
        env["ADCS_CSP_NODE_ID"] = "3"
        return env

    def start_proxy_and_simulators(self) -> None:
        env = self.env()
        self.csp_process = subprocess.Popen(
            [
                str(BIN_DIR / "csp_zmqproxy"),
                "-s",
                f"tcp://0.0.0.0:{self.csp_sub_port}",
                "-p",
                f"tcp://0.0.0.0:{self.csp_pub_port}",
            ],
            stdin=subprocess.DEVNULL,
            stdout=self.log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
        self.processes.append(self.csp_process)
        self.start_eps()
        self.adcs_process = subprocess.Popen(
            [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=self.log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
        self.processes.append(self.adcs_process)
        self.processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "radio_mock_server"), "--port", str(self.radio_port)],
                stdin=subprocess.DEVNULL,
                stdout=self.log_handle,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
        )
        time.sleep(1.0)

    def start_eps(self) -> None:
        env = self.env()
        self.eps_process = subprocess.Popen(
            [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "80.00"],
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=self.log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
        self.processes.append(self.eps_process)

    def stop_eps(self) -> None:
        terminate_process(self.eps_process)
        self.eps_process = None

    def start_adcs(self) -> None:
        env = self.env()
        self.adcs_process = subprocess.Popen(
            [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=self.log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
        self.processes.append(self.adcs_process)

    def stop_adcs(self) -> None:
        terminate_process(self.adcs_process)
        self.adcs_process = None

    def stop_all(self) -> None:
        for process in reversed(self.processes):
            terminate_process(process)
        self.processes.clear()
        self.csp_process = None
        self.eps_process = None
        self.adcs_process = None


def ensure_runtime_root(runtime_root: pathlib.Path) -> None:
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)


def start_obc(stack: Stack, runtime_root: pathlib.Path, tick_ms: int = 500):
    lines: list[str] = []
    master_fd, slave_fd = pty.openpty()
    process = subprocess.Popen(
        [
            str(BIN_DIR / "OBC"),
            "--comm",
            "tcp",
            "--comm-host",
            "127.0.0.1",
            "--comm-port",
            str(stack.radio_port),
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
            str(tick_ms),
        ],
        env=stack.env(),
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        text=True,
        bufsize=1,
        cwd=str(runtime_root),
        start_new_session=True,
    )
    os.close(slave_fd)
    process._output_reader = os.fdopen(os.dup(master_fd), "r", buffering=1, encoding="utf-8", errors="replace")
    process._control_writer = os.fdopen(os.dup(master_fd), "w", buffering=1, encoding="utf-8", errors="replace")
    os.close(master_fd)
    thread = collect_output(process, lines)
    wait_for_status_contains(process, lines, ["boot resetCause=", "recovery activeCount="], READY_TIMEOUT)
    return process, lines, thread


def stop_obc(process: Optional[subprocess.Popen[str]], thread: Optional[threading.Thread], log_handle, lines: list[str], label: str) -> None:
    if process is not None and process.poll() is None:
        try:
            send_command(process, "quit")
            control = getattr(process, "_control_writer", None)
            if control is not None:
                control.close()
            elif process.stdin is not None:
                process.stdin.close()
            process.wait(timeout=5.0)
        except Exception:
            terminate_process(process)
    if process is not None:
        reader = getattr(process, "_output_reader", None)
        if reader is not None:
            reader.close()
        control = getattr(process, "_control_writer", None)
        if control is not None and not control.closed:
            control.close()
    if thread is not None:
        thread.join(timeout=2.0)
    write_output(log_handle, lines, process, label)


def run_watchdog_r2_case(log_handle, summary_lines: list[str]) -> None:
    runtime_root = PROBE_TMP_DIR / "watchdog-r2-runtime"
    ensure_runtime_root(runtime_root)
    stack = Stack("watchdog-r2", 0, log_handle)
    stack.start_proxy_and_simulators()

    try:
        obc, lines, thread = start_obc(stack, runtime_root)
        try:
            wait_for_status_contains(obc, lines, ["bootCount=1"], READY_TIMEOUT)
            send_command(obc, "watchdog config eps-bridge on 1 1 100")
            wait_for_status_contains(obc, lines, ["watchdog source=EPS_BRIDGE", "warn/safe/suppress=1/1/100"], READY_TIMEOUT)
            send_command(obc, "watchdog suppress eps-bridge on")
            wait_for_process_exit(obc, WATCHDOG_EXIT_CODE, REBOOT_TIMEOUT, "watchdog suppression reboot")
        finally:
            if thread is not None:
                thread.join(timeout=2.0)
            write_output(log_handle, lines, obc, "watchdog-r2-first-boot")

        obc, lines, thread = start_obc(stack, runtime_root)
        try:
            restart_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "boot resetCause=RECOVERY_WATCHDOG",
                    "bootCount=2",
                    "consecutiveResetCount=1",
                    "lastRecoverySource=WATCHDOG_EPS_BRIDGE",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                ],
                READY_TIMEOUT,
            )
            summary_lines.append(
                "  watchdog_suppression_reboot_truth: "
                "firstFaultExit=32 resetCause=RECOVERY_WATCHDOG "
                "lastRecoveryLevel=R6_OBC_REBOOT bootCount=2 consecutive=1"
            )
            log_handle.write("\n--- watchdog r2 status snapshots ---\n")
            log_handle.write(restart_status)
        finally:
            stop_obc(obc, thread, log_handle, lines, "watchdog-r2-second-boot")
    finally:
        stack.stop_all()


def run_watchdog_case(log_handle, summary_lines: list[str]) -> None:
    runtime_root = PROBE_TMP_DIR / "watchdog-runtime"
    ensure_runtime_root(runtime_root)
    stack = Stack("watchdog", 0, log_handle)
    stack.start_proxy_and_simulators()

    try:
        for cycle in range(1, 4):
            obc, lines, thread = start_obc(stack, runtime_root)
            try:
                wait_for_status_contains(obc, lines, ["mode=SAFE "], READY_TIMEOUT)
                wait_for_status_contains(obc, lines, ["bootCount=" + str(cycle)], READY_TIMEOUT)
                send_command(obc, "watchdog config eps-bridge on 1 1 1")
                wait_for_status_contains(obc, lines, ["watchdog source=EPS_BRIDGE", "warn/safe/suppress=1/1/1"], READY_TIMEOUT)
                send_command(obc, "watchdog suppress eps-bridge on")
                wait_for_process_exit(obc, WATCHDOG_EXIT_CODE, REBOOT_TIMEOUT, f"watchdog reboot cycle {cycle}")
            finally:
                if thread is not None:
                    thread.join(timeout=2.0)
                write_output(log_handle, lines, obc, f"watchdog-cycle-{cycle}")

        obc, lines, thread = start_obc(stack, runtime_root)
        try:
            initial_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "boot resetCause=RECOVERY_WATCHDOG",
                    "bootCount=4",
                    "consecutiveResetCount=3",
                    "safeFallbackRequired=yes",
                    "lastRecoverySource=WATCHDOG_EPS_BRIDGE",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                ],
                READY_TIMEOUT,
            )
            assert_contains(initial_status, "mode=SAFE ", "watchdog safe fallback boot")
            summary_lines.append(
                "  watchdog_reboot_truth_and_safe_fallback: "
                "cycles=3 rebootExit=32 finalBootCount=4 initialConsecutive=3 "
                "initialSafeFallback=yes"
            )
            log_handle.write("\n--- watchdog status snapshots ---\n")
            log_handle.write(initial_status)
        finally:
            stop_obc(obc, thread, log_handle, lines, "watchdog-final-boot")
    finally:
        stack.stop_all()


def run_eps_case(log_handle, summary_lines: list[str]) -> None:
    runtime_root = PROBE_TMP_DIR / "eps-runtime"
    ensure_runtime_root(runtime_root)
    stack = Stack("eps", 80, log_handle)
    stack.start_proxy_and_simulators()

    try:
        obc, lines, thread = start_obc(stack, runtime_root, tick_ms=1000)
        try:
            wait_for_status_contains(obc, lines, ["bootCount=1"], READY_TIMEOUT)

            stack.stop_eps()
            first_fault_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "mode=SAFE ",
                    "recovery activeCount=1",
                    "activeSource=EPS_TIMEOUT",
                    "currentLevel=R3_RESET_SUBSYSTEM_INTERFACE",
                    "highestLevel=R3_RESET_SUBSYSTEM_INTERFACE",
                    "lastAction=SUBSYSTEM_INTERFACE_RESET",
                ],
                EPS_FAULT_TIMEOUT,
            )

            wait_for_process_exit(obc, WATCHDOG_EXIT_CODE, EPS_FAULT_TIMEOUT, "eps relatch reboot")
        finally:
            if thread is not None:
                thread.join(timeout=2.0)
            write_output(log_handle, lines, obc, "eps-first-boot")

        stack.start_eps()
        obc, lines, thread = start_obc(stack, runtime_root, tick_ms=1000)
        try:
            reboot_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "boot resetCause=RECOVERY_EPS_TIMEOUT",
                    "bootCount=2",
                    "consecutiveResetCount=1",
                    "lastRecoverySource=EPS_TIMEOUT",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                    "recovery activeCount=0",
                ],
                READY_TIMEOUT,
            )
            summary_lines.append(
                "  eps_shared_executor_relatch_reboot: "
                "firstFault=R3_RESET recoveryClear=no secondFaultReboot=32 "
                "resetCause=RECOVERY_EPS_TIMEOUT bootCount=2 consecutive=1"
            )
            log_handle.write("\n--- eps status snapshots ---\n")
            log_handle.write(first_fault_status)
            log_handle.write(reboot_status)
        finally:
            stop_obc(obc, thread, log_handle, lines, "eps-second-boot")
    finally:
        stack.stop_all()


def run_adcs_case(log_handle, summary_lines: list[str]) -> None:
    runtime_root = PROBE_TMP_DIR / "adcs-runtime"
    ensure_runtime_root(runtime_root)
    stack = Stack("adcs", 160, log_handle)
    stack.start_proxy_and_simulators()

    try:
        obc, lines, thread = start_obc(stack, runtime_root, tick_ms=1000)
        try:
            wait_for_status_contains(obc, lines, ["bootCount=1"], READY_TIMEOUT)

            stack.stop_adcs()
            first_fault_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "mode=SAFE ",
                    "recovery activeCount=1",
                    "activeSource=ADCS_POLL_TRANSPORT",
                    "currentLevel=R3_RESET_SUBSYSTEM_INTERFACE",
                    "highestLevel=R3_RESET_SUBSYSTEM_INTERFACE",
                    "lastAction=SUBSYSTEM_INTERFACE_RESET",
                    "pendingProcessRestart=no",
                    "pendingReboot=no",
                ],
                EPS_FAULT_TIMEOUT,
            )

            if PROBE_SCOPE == "adcs-first-fault-only":
                summary_lines.append(
                    "  adcs_first_fault_reset: "
                    "currentLevel=R3_RESET_SUBSYSTEM_INTERFACE mode=SAFE activeSource=ADCS_POLL_TRANSPORT"
                )
                send_command(obc, "quit")
                if obc.stdin is not None:
                    obc.stdin.close()
                obc.wait(timeout=5.0)
                return

            stack.start_adcs()
            cleared_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "mode=SAFE ",
                    "recovery activeCount=0",
                    "activeSource=NONE",
                    "boot resetCause=UNKNOWN",
                ],
                EPS_FAULT_TIMEOUT,
            )

            stack.stop_adcs()
            wait_for_process_exit(obc, WATCHDOG_EXIT_CODE, EPS_FAULT_TIMEOUT, "adcs relatch reboot")
        finally:
            if thread is not None:
                thread.join(timeout=2.0)
            write_output(log_handle, lines, obc, "adcs-first-boot")

        stack.start_adcs()
        obc, lines, thread = start_obc(stack, runtime_root, tick_ms=1000)
        try:
            reboot_status = wait_for_status_contains(
                obc,
                lines,
                [
                    "boot resetCause=RECOVERY_ADCS_FDIR",
                    "bootCount=2",
                    "consecutiveResetCount=1",
                    "lastRecoverySource=ADCS_POLL_TRANSPORT",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                    "recovery activeCount=0",
                ],
                READY_TIMEOUT,
            )
            summary_lines.append(
                "  adcs_shared_executor_r3_reset: "
                "firstFault=R3_RESET recoveryClear=yes secondFaultReboot=32 "
                "resetCause=RECOVERY_ADCS_FDIR bootCount=2 consecutive=1"
            )
            log_handle.write("\n--- adcs status snapshots ---\n")
            log_handle.write(first_fault_status)
            log_handle.write(cleared_status)
            log_handle.write(reboot_status)
        finally:
            stop_obc(obc, thread, log_handle, lines, "adcs-second-boot")
    finally:
        stack.stop_all()


summary_lines: list[str] = []
with PROBE_LOG.open("w", encoding="utf-8", buffering=1) as log_handle:
    if PROBE_SCOPE == "full":
        run_watchdog_r2_case(log_handle, summary_lines)
        run_watchdog_case(log_handle, summary_lines)
        run_eps_case(log_handle, summary_lines)
        run_adcs_case(log_handle, summary_lines)
    elif PROBE_SCOPE == "eps-only":
        run_eps_case(log_handle, summary_lines)
    elif PROBE_SCOPE == "adcs-first-fault-only":
        run_adcs_case(log_handle, summary_lines)
    else:
        raise RuntimeError(
            "RECOVERY_EXECUTOR_SCOPE must be one of: full, eps-only, adcs-first-fault-only"
        )

print(f"recovery-executors-v1 probe PASS log={PROBE_LOG}")
for line in summary_lines:
    print(line)
PY
