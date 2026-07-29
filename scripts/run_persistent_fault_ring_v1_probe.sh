#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/persistent-fault-ring-v1.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/persistent-fault-ring-v1-probe.log}"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
PROBE_LOG="${PROBE_LOG}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import re
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

READY_TIMEOUT = 8.0
REBOOT_TIMEOUT = 10.0
FAULT_TIMEOUT = 3.0


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
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def output_since(lines: list[str], start_index: int) -> str:
    return "".join(lines[start_index:])


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command {command!r}")
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def wait_for_reboot_termination(process: subprocess.Popen[str], timeout: float, label: str) -> int:
    deadline = time.time() + timeout
    while time.time() < deadline:
        code = process.poll()
        if code is not None:
            if code == 0:
                raise RuntimeError(f"{label}: expected non-zero reboot-equivalent termination, got 0")
            return code
        time.sleep(0.1)
    raise RuntimeError(f"{label}: timed out waiting for reboot-equivalent termination")


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
    delay: float = 0.25,
) -> str:
    deadline = time.time() + timeout
    last_status = ""
    while time.time() < deadline:
        last_status = request_status(process, lines)
        if all(item in last_status for item in required):
            return last_status
        time.sleep(delay)
    raise RuntimeError(f"timed out waiting for status containing {required}; last status:\n{last_status}")


def request_fault_history(process: subprocess.Popen[str], lines: list[str], limit: int, timeout: float = FAULT_TIMEOUT) -> str:
    start_index = len(lines)
    send_command(process, f"fault history {limit}")
    deadline = time.time() + timeout
    last_size = -1
    stable_cycles = 0
    while time.time() < deadline:
        text = output_since(lines, start_index)
        if "fault total=" not in text:
            time.sleep(0.05)
            continue
        size = len(text)
        if size == last_size:
            stable_cycles += 1
        else:
            stable_cycles = 0
            last_size = size
        if stable_cycles >= 3:
            return text
        time.sleep(0.05)
    raise RuntimeError(f"timed out waiting for fault history output:\n{output_since(lines, start_index)}")


def write_output(log_handle, lines: list[str], process: Optional[subprocess.Popen[str]], label: str) -> None:
    log_handle.write(f"\n--- {label} stdout ---\n")
    log_handle.write("".join(lines))
    log_handle.write(f"\n--- {label} returncode={(process.poll() if process is not None else 'none')} ---\n")


@dataclass
class HistorySnapshot:
    total_records: int
    returned_records: int
    active_copy: str
    generation: int
    max_boot_count: int
    text: str


def parse_fault_history(text: str) -> HistorySnapshot:
    header_match = re.search(
        r"fault total=(?P<total>\d+) returned=(?P<returned>\d+) activeCopy=(?P<copy>[A-Z_]+) generation=(?P<generation>\d+)",
        text,
    )
    if header_match is None:
        raise RuntimeError(f"missing fault history header:\n{text}")

    boot_counts = [int(match.group(1)) for match in re.finditer(r"bootCount=(\d+)", text)]
    if not boot_counts:
        raise RuntimeError(f"missing bootCount fields in fault history:\n{text}")

    return HistorySnapshot(
        total_records=int(header_match.group("total")),
        returned_records=int(header_match.group("returned")),
        active_copy=header_match.group("copy"),
        generation=int(header_match.group("generation")),
        max_boot_count=max(boot_counts),
        text=text,
    )


@dataclass
class Stack:
    log_handle: object

    def __post_init__(self) -> None:
        self.csp_sub_port = find_free_port_pair(56520, 1000)
        self.csp_pub_port = self.csp_sub_port + 1000
        self.radio_port = find_free_port(17320)
        self.gds_port = find_free_port(50320)
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

    def start(self) -> None:
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

    def stop(self) -> None:
        for process in reversed(self.processes):
            terminate_process(process)
        self.processes.clear()
        self.csp_process = None
        self.eps_process = None
        self.adcs_process = None


class HoldServer:
    def __init__(self, port: int) -> None:
        self.port = port
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._server: Optional[socket.socket] = None
        self._conn: Optional[socket.socket] = None

    def start(self) -> None:
        self.stop()
        self._stop.clear()
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind(("127.0.0.1", self.port))
        server.listen(1)
        server.settimeout(0.2)
        self._server = server

        def run() -> None:
            while not self._stop.is_set():
                try:
                    conn, _ = server.accept()
                except socket.timeout:
                    continue
                except OSError:
                    break
                self._conn = conn
                conn.settimeout(0.2)
                try:
                    while not self._stop.is_set():
                        try:
                            data = conn.recv(4096)
                        except socket.timeout:
                            continue
                        except OSError:
                            break
                        if not data:
                            break
                finally:
                    try:
                        conn.close()
                    except OSError:
                        pass
                    self._conn = None

        self._thread = threading.Thread(target=run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._conn is not None:
            try:
                self._conn.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                self._conn.close()
            except OSError:
                pass
            self._conn = None
        if self._server is not None:
            try:
                self._server.close()
            except OSError:
                pass
            self._server = None
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None


def ensure_runtime_root(runtime_root: pathlib.Path) -> None:
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)


def start_obc(stack: Stack, runtime_root: pathlib.Path, ground_link_mode: str = "direct-tcp"):
    lines: list[str] = []
    command = [
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
        ground_link_mode,
    ]
    if ground_link_mode == "direct-tcp":
        command.extend(["--gds-host", "127.0.0.1", "--gds-port", str(stack.gds_port)])
    command.extend(
        [
            "--runtime-root",
            str(runtime_root),
            "--persistent-root",
            str(runtime_root / "persistent-data"),
            "--staging-root",
            str(runtime_root / "staging"),
            "--tick-ms",
            "500",
        ]
    )
    process = subprocess.Popen(
        command,
        env=stack.env(),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        cwd=str(runtime_root),
    )
    thread = collect_output(process, lines)
    wait_for_status_contains(process, lines, ["boot resetCause=", "recovery activeCount="], READY_TIMEOUT)
    return process, lines, thread


def stop_obc(
    process: Optional[subprocess.Popen[str]],
    thread: Optional[threading.Thread],
    log_handle,
    lines: list[str],
    label: str,
) -> None:
    if process is not None and process.poll() is None:
        try:
            send_command(process, "quit")
            assert process.stdin is not None
            process.stdin.close()
            process.wait(timeout=5.0)
        except Exception:
            terminate_process(process)
    if thread is not None:
        thread.join(timeout=2.0)
    write_output(log_handle, lines, process, label)


def corrupt_copy(path: pathlib.Path) -> None:
    if not path.exists():
        raise RuntimeError(f"active fault-ring copy does not exist: {path}")
    with path.open("r+b") as handle:
        handle.seek(0)
        handle.write(b"\x00")
        handle.flush()


def main() -> None:
    runtime_root = PROBE_TMP_DIR / "runtime"
    recovery_root = runtime_root / "persistent-data" / "recovery"
    ensure_runtime_root(runtime_root)

    summary_lines: list[str] = []
    stack = Stack(PROBE_LOG.open("w", encoding="utf-8", buffering=1))
    log_handle = stack.log_handle
    hold_server = HoldServer(stack.gds_port)
    hold_server.start()
    stack.start()

    try:
        obc = None
        lines: list[str] = []
        thread: Optional[threading.Thread] = None

        try:
            obc, lines, thread = start_obc(stack, runtime_root, ground_link_mode="disabled")
            wait_for_status_contains(obc, lines, ["bootCount=1", "mode=SAFE "], READY_TIMEOUT)
            reboot_exit = wait_for_reboot_termination(obc, REBOOT_TIMEOUT, "comm fdir reboot")
            summary_lines.append(f"  comm_fdir_reboot_exit: code={reboot_exit}")
        finally:
            if thread is not None:
                thread.join(timeout=2.0)
            write_output(log_handle, lines, obc, "comm-fdir-reboot-cycle")

        obc, lines, thread = start_obc(stack, runtime_root)
        before_history = HistorySnapshot(0, 0, "", 0, 0, "")
        try:
            wait_for_status_contains(
                obc,
                lines,
                [
                    "boot resetCause=RECOVERY_COMM_FDIR",
                    "bootCount=2",
                    "lastRecoverySource=COMM_PRIMARY_UNAVAILABLE",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                ],
                READY_TIMEOUT,
            )
            before_history = parse_fault_history(request_fault_history(obc, lines, 12))
            if "kind=BOOT_OBSERVED" not in before_history.text or "kind=REBOOT_ISSUED" not in before_history.text:
                raise RuntimeError(f"missing expected persistent fault records before corruption:\n{before_history.text}")
            if before_history.max_boot_count != 2:
                raise RuntimeError(f"expected bootCount 2 before corruption, got {before_history.max_boot_count}")
        finally:
            stop_obc(obc, thread, log_handle, lines, "same-root-relaunch-before-corruption")

        active_copy_path = recovery_root / (
            "fault-ring-a.bin" if before_history.active_copy == "COPY_A" else "fault-ring-b.bin"
        )
        corrupt_copy(active_copy_path)

        obc, lines, thread = start_obc(stack, runtime_root)
        after_history = HistorySnapshot(0, 0, "", 0, 0, "")
        try:
            wait_for_status_contains(
                obc,
                lines,
                [
                    "bootCount=3",
                    "lastRecoverySource=COMM_PRIMARY_UNAVAILABLE",
                    "lastRecoveryLevel=R6_OBC_REBOOT",
                ],
                READY_TIMEOUT,
            )
            after_history = parse_fault_history(request_fault_history(obc, lines, 12))
            if "kind=BOOT_OBSERVED" not in after_history.text or "kind=REBOOT_ISSUED" not in after_history.text:
                raise RuntimeError(f"missing expected persistent fault records after corruption fallback:\n{after_history.text}")
            if after_history.max_boot_count != 3:
                raise RuntimeError(f"expected bootCount 3 after corruption fallback, got {after_history.max_boot_count}")
            if after_history.generation != before_history.generation:
                raise RuntimeError(
                    "corruption fallback should keep generation flat across the extra reboot; "
                    f"before={before_history.generation} after={after_history.generation}"
                )
            if after_history.total_records != before_history.total_records:
                raise RuntimeError(
                    "corruption fallback should keep record count flat across the extra reboot; "
                    f"before={before_history.total_records} after={after_history.total_records}"
                )
            summary_lines.append(
                "  same_root_relaunch: "
                f"generation={before_history.generation} total={before_history.total_records} "
                f"maxBootCountBefore={before_history.max_boot_count} maxBootCountAfter={after_history.max_boot_count}"
            )
            summary_lines.append(
                "  dual_copy_fallback: "
                f"corrupted={active_copy_path.name} beforeActive={before_history.active_copy} "
                f"afterActive={after_history.active_copy} generationStayed={after_history.generation}"
            )
            log_handle.write("\n--- fault history before corruption ---\n")
            log_handle.write(before_history.text)
            log_handle.write("\n--- fault history after corruption fallback ---\n")
            log_handle.write(after_history.text)
        finally:
            stop_obc(obc, thread, log_handle, lines, "same-root-relaunch-after-corruption")
    finally:
        stack.stop()
        hold_server.stop()
        log_handle.close()

    print(f"persistent-fault-ring-v1 probe PASS log={PROBE_LOG}")
    for line in summary_lines:
        print(line)


if __name__ == "__main__":
    main()
PY
