#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/watchdog-v1-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/watchdog-v1-probe.log}"
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
    tick_ms: int = 200


BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
PROBE_LOG = pathlib.Path(require_env("PROBE_LOG"))
PROCESS_RESTART_EXIT_CODE = 31

CASES = (
    ProbeCase("healthy_feed_eligible"),
    ProbeCase("warning_only_before_safe"),
    ProbeCase("stale_to_process_restart"),
    ProbeCase("warning_recovery_after_heartbeat_resume"),
    ProbeCase("resource_monitor_rss_threshold"),
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


def wait_for_substring(lines: list[str], needle: str, timeout: float, start_index: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if needle in output_since(lines, start_index):
            return
        time.sleep(0.05)
    raise RuntimeError(f"timed out waiting for '{needle}'")


def ensure_absent(lines: list[str], needle: str, start_index: int) -> None:
    if needle in output_since(lines, start_index):
        raise RuntimeError(f"unexpected '{needle}' observed")


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command '{command}'")
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def wait_for_exit(process: subprocess.Popen[str], expected: int, timeout: float, label: str) -> int:
    deadline = time.time() + timeout
    while time.time() < deadline:
        code = process.poll()
        if code is not None:
            if code != expected:
                raise RuntimeError(f"{label}: expected exit {expected}, got {code}")
            return code
        time.sleep(0.1)
    raise RuntimeError(f"{label}: timed out waiting for exit {expected}")


def append_obc_output(log_handle, lines: list[str], process: subprocess.Popen[str]) -> str:
    output = "".join(lines)
    log_handle.write("\n--- OBC stdout ---\n")
    log_handle.write(output)
    log_handle.write(f"\n--- OBC returncode={process.poll()} ---\n")
    return output


def last_matching_line(text: str, prefix: str) -> str:
    lines = [line for line in text.splitlines() if line.startswith(prefix)]
    if not lines:
        raise RuntimeError(f"no line starting with '{prefix}' observed")
    return lines[-1]


def assert_contains(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise RuntimeError(f"{label}: expected '{needle}' in:\n{text}")


def status_snapshot(obc: subprocess.Popen[str], lines: list[str], start_index: int) -> str:
    send_command(obc, "watchdog status")
    time.sleep(0.75)
    text = output_since(lines, start_index)
    assert_contains(text, "watchdog aggregate=", "status snapshot")
    return text


def full_status_snapshot(obc: subprocess.Popen[str], lines: list[str], start_index: int) -> str:
    send_command(obc, "status")
    time.sleep(0.75)
    text = output_since(lines, start_index)
    assert_contains(text, "boot resetCause=", "full status snapshot")
    assert_contains(text, "watchdog aggregate=", "full status snapshot")
    return text


def start_case_processes(
    case: ProbeCase,
    case_dir: pathlib.Path,
    case_log_path: pathlib.Path,
    port_seed: int,
    reset_runtime: bool = True,
    log_mode: str = "w",
    ground_link_mode: str = "direct-tcp",
    startup_delay: float = 1.6,
):
    runtime_root = case_dir / "runtime"
    if reset_runtime and runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)

    csp_sub_port = find_free_port_pair(56420 + port_seed, 1000)
    csp_pub_port = csp_sub_port + 1000
    radio_port = find_free_port(17220 + port_seed)
    gds_port = find_free_port(18220 + port_seed)

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

    log_handle = case_log_path.open(log_mode, encoding="utf-8", buffering=1)
    hold_server: Optional[HoldServer] = None
    if ground_link_mode == "direct-tcp":
        hold_server = HoldServer(gds_port)
        hold_server.start()
    processes: list[subprocess.Popen[str]] = []
    processes.append(
        subprocess.Popen(
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
    )
    processes.append(
        subprocess.Popen(
            [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "80.00"],
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
    )
    processes.append(
        subprocess.Popen(
            [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
    )
    processes.append(
        subprocess.Popen(
            [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
            stdin=subprocess.DEVNULL,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
        )
    )

    time.sleep(1.0)
    obc_command = [
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
        ground_link_mode,
    ]
    if ground_link_mode == "direct-tcp":
        obc_command.extend(["--gds-host", "127.0.0.1", "--gds-port", str(gds_port)])
    obc_command.extend(
        [
            "--runtime-root",
            str(runtime_root),
            "--persistent-root",
            str(runtime_root / "persistent-data"),
            "--staging-root",
            str(runtime_root / "staging"),
            "--tick-ms",
            str(case.tick_ms),
        ]
    )
    obc = subprocess.Popen(
        obc_command,
        env=env,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
        cwd=str(runtime_root),
    )
    processes.append(obc)
    lines: list[str] = []
    reader_thread = collect_output(obc, lines)
    time.sleep(startup_delay)
    return log_handle, processes, obc, lines, reader_thread, hold_server


def finish_case(log_handle, processes, obc, lines, reader_thread, hold_server) -> str:
    try:
        if obc.poll() is None and obc.stdin is not None:
            try:
                obc.stdin.write("quit\n")
                obc.stdin.flush()
                obc.stdin.close()
            except BrokenPipeError:
                pass
        if reader_thread is not None:
            reader_thread.join(timeout=2.0)
        output = append_obc_output(log_handle, lines, obc)
        return output
    finally:
        terminate_all(processes)
        if hold_server is not None:
            hold_server.stop()
        log_handle.close()


def run_healthy_case(case: ProbeCase, case_dir: pathlib.Path, summary: list[str], port_seed: int) -> None:
    case_log = case_dir / f"{case.name}.log"
    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(case, case_dir, case_log, port_seed)
    try:
        start_index = len(lines)
        snapshot = status_snapshot(obc, lines, start_index)
        watchdog_line = last_matching_line(snapshot, "watchdog aggregate=")
        mode_line = last_matching_line(snapshot, "mode=")
        assert_contains(watchdog_line, "aggregate=HEALTHY", case.name)
        assert_contains(watchdog_line, "recovery=NONE", case.name)
        assert_contains(watchdog_line, "feedEligible=yes", case.name)
        for source_name in ("EPS_BRIDGE", "EPS_FDIR", "MODE_SAFETY", "COMM_CONTROLLER"):
            assert_contains(snapshot, f"watchdog source={source_name} enabled=yes state=HEALTHY", case.name)
        summary.append(f"  {case.name}: mode={mode_line.split()[0].split('=')[1]} watchdog=HEALTHY feedEligible=yes log={case_log}")
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)


def run_warning_case(case: ProbeCase, case_dir: pathlib.Path, summary: list[str], port_seed: int) -> None:
    case_log = case_dir / f"{case.name}.log"
    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(case, case_dir, case_log, port_seed)
    try:
        send_command(obc, "watchdog config comm-controller on 2 8 10")
        time.sleep(0.2)
        start_index = len(lines)
        send_command(obc, "watchdog suppress comm-controller on")
        wait_for_substring(lines, "Watchdog source COMM_CONTROLLER", 2.5, start_index)
        wait_for_substring(lines, "warning at age", 2.5, start_index)
        time.sleep(0.35)
        ensure_absent(lines, "fault latched", start_index)
        snapshot = status_snapshot(obc, lines, start_index)
        watchdog_line = last_matching_line(snapshot, "watchdog aggregate=")
        source_line = last_matching_line(snapshot, "watchdog source=COMM_CONTROLLER")
        assert_contains(watchdog_line, "aggregate=WARNING", case.name)
        assert_contains(watchdog_line, "recovery=WARNING", case.name)
        assert_contains(watchdog_line, "feedEligible=yes", case.name)
        assert_contains(source_line, "state=WARNING", case.name)
        assert_contains(source_line, "probeSuppressed=yes", case.name)
        summary.append(f"  {case.name}: aggregate=WARNING feedEligible=yes source=COMM_CONTROLLER log={case_log}")
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)


def run_process_restart_case(case: ProbeCase, case_dir: pathlib.Path, summary: list[str], port_seed: int) -> None:
    case_log = case_dir / f"{case.name}.log"
    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(
        case, case_dir, case_log, port_seed, ground_link_mode="disabled", startup_delay=0.3
    )
    try:
        wait_for_substring(lines, "runtime started", 2.0, 0)
        send_command(obc, "mode idle")
        wait_for_substring(lines, "mode=IDLE ", 3.0, 0)
        send_command(obc, "watchdog config comm-controller on 1 1 100")
        time.sleep(0.2)
        start_index = len(lines)
        send_command(obc, "watchdog suppress comm-controller on")
        wait_for_substring(lines, "Watchdog source COMM_CONTROLLER", 2.0, start_index)
        wait_for_substring(lines, "fault latched at age", 2.5, start_index)
        exit_code = wait_for_exit(obc, PROCESS_RESTART_EXIT_CODE, 4.0, case.name)
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)

    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(
        case, case_dir, case_log, port_seed, reset_runtime=False, log_mode="a"
    )
    try:
        snapshot = full_status_snapshot(obc, lines, 0)
        assert_contains(snapshot, "boot resetCause=RECOVERY_WATCHDOG", case.name)
        assert_contains(snapshot, "bootCount=2", case.name)
        assert_contains(snapshot, "lastRecoverySource=WATCHDOG_COMM_CONTROLLER", case.name)
        assert_contains(snapshot, "lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT", case.name)
        assert_contains(snapshot, "watchdog aggregate=HEALTHY", case.name)
        summary.append(
            f"  {case.name}: stale->PROCESS_RESTART exit={exit_code} resetCause=RECOVERY_WATCHDOG "
            f"lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT log={case_log}"
        )
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)


def run_warning_recovery_case(case: ProbeCase, case_dir: pathlib.Path, summary: list[str], port_seed: int) -> None:
    case_log = case_dir / f"{case.name}.log"
    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(case, case_dir, case_log, port_seed)
    try:
        send_command(obc, "watchdog config comm-controller on 2 8 10")
        time.sleep(0.2)
        start_index = len(lines)
        send_command(obc, "watchdog suppress comm-controller on")
        wait_for_substring(lines, "Watchdog source COMM_CONTROLLER", 2.5, start_index)
        wait_for_substring(lines, "warning at age", 2.5, start_index)
        recover_index = len(lines)
        send_command(obc, "watchdog suppress comm-controller off")
        wait_for_substring(lines, "recovered from WARNING", 2.5, recover_index)
        time.sleep(0.35)
        snapshot = status_snapshot(obc, lines, recover_index)
        watchdog_line = last_matching_line(snapshot, "watchdog aggregate=")
        source_line = last_matching_line(snapshot, "watchdog source=COMM_CONTROLLER")
        assert_contains(watchdog_line, "aggregate=HEALTHY", case.name)
        assert_contains(watchdog_line, "recovery=NONE", case.name)
        assert_contains(watchdog_line, "feedEligible=yes", case.name)
        assert_contains(source_line, "state=HEALTHY", case.name)
        summary.append(f"  {case.name}: warning->recovered aggregate=HEALTHY feedEligible=yes log={case_log}")
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)


def run_resource_case(case: ProbeCase, case_dir: pathlib.Path, summary: list[str], port_seed: int) -> None:
    case_log = case_dir / f"{case.name}.log"
    log_handle, processes, obc, lines, reader_thread, hold_server = start_case_processes(case, case_dir, case_log, port_seed)
    try:
        start_index = len(lines)
        send_command(obc, "health enable on")
        time.sleep(0.2)
        send_command(obc, "health threshold rss 0.01")
        wait_for_substring(lines, "RSS ", 2.0, start_index)
        wait_for_substring(lines, "exceeded threshold", 2.0, start_index)
        summary.append(f"  {case.name}: SYS_LOW_MEMORY observed after hosted rss threshold update log={case_log}")
    finally:
        finish_case(log_handle, processes, obc, lines, reader_thread, hold_server)


def main() -> int:
    PROBE_TMP_DIR.mkdir(parents=True, exist_ok=True)
    summary_lines = [f"watchdog-v1-probe: PASS log={PROBE_LOG}"]

    run_healthy_case(CASES[0], PROBE_TMP_DIR / CASES[0].name, summary_lines, 0)
    run_warning_case(CASES[1], PROBE_TMP_DIR / CASES[1].name, summary_lines, 20)
    run_process_restart_case(CASES[2], PROBE_TMP_DIR / CASES[2].name, summary_lines, 40)
    run_warning_recovery_case(CASES[3], PROBE_TMP_DIR / CASES[3].name, summary_lines, 60)
    run_resource_case(CASES[4], PROBE_TMP_DIR / CASES[4].name, summary_lines, 80)

    text = "\n".join(summary_lines) + "\n"
    PROBE_LOG.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
PY
