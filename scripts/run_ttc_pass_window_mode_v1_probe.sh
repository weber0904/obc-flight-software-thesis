#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/ttc-pass-window-mode-v1.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/ttc-pass-window-mode-v1-probe.log}"
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
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
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
    ttc_enabled: bool
    loss_timeout_sec: int
    window_start_offset_sec: int
    window_end_offset_sec: int
    wait_after_setup_sec: float
    expected_final_mode: str
    expected_status_pairs: tuple[tuple[str, str], ...]
    expected_fragments: tuple[str, ...] = ()
    forbidden_fragments: tuple[str, ...] = ()
    manual_ttc_entry: bool = False
    manual_idle_exit: bool = False
    restart_soc: Optional[float] = None
    wait_for_fragment_before_restart: Optional[str] = None


BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
PROBE_LOG = pathlib.Path(require_env("PROBE_LOG"))

BASE_DATETIME = datetime(2026, 5, 13, 12, 0, 0, tzinfo=timezone.utc)
BASE_EPOCH = int(BASE_DATETIME.timestamp())

CASES = (
    ProbeCase(
        name="disabled-no-entry",
        initial_soc=60.0,
        ttc_enabled=False,
        loss_timeout_sec=0,
        window_start_offset_sec=1,
        window_end_offset_sec=5,
        wait_after_setup_sec=2.5,
        expected_final_mode="IDLE",
        expected_status_pairs=(
            ("enabled", "no"),
            ("windowConfigured", "yes"),
            ("windowActive", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryCount", "0"),
            ("exitCount", "0"),
        ),
        forbidden_fragments=("TTC_POLICY_ENTRY_REQUEST",),
    ),
    ProbeCase(
        name="enabled-auto-entry",
        initial_soc=60.0,
        ttc_enabled=True,
        loss_timeout_sec=10,
        window_start_offset_sec=1,
        window_end_offset_sec=6,
        wait_after_setup_sec=3.5,
        expected_final_mode="TTC",
        expected_status_pairs=(
            ("enabled", "yes"),
            ("windowConfigured", "yes"),
            ("windowActive", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "yes"),
            ("entryReason", "WINDOW_ACTIVE"),
            ("entryCount", "1"),
            ("exitCount", "0"),
        ),
        expected_fragments=(
            "TTC_POLICY_ENTRY_REQUEST",
            "SYS_MODE_CHANGE : System mode changed to TTC (4)",
        ),
    ),
    ProbeCase(
        name="window-end-auto-exit",
        initial_soc=60.0,
        ttc_enabled=True,
        loss_timeout_sec=10,
        window_start_offset_sec=1,
        window_end_offset_sec=4,
        wait_after_setup_sec=5.5,
        expected_final_mode="IDLE",
        expected_status_pairs=(
            ("enabled", "yes"),
            ("windowConfigured", "yes"),
            ("windowActive", "no"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryReason", "WINDOW_ACTIVE"),
            ("exitReason", "WINDOW_INACTIVE"),
            ("entryCount", "1"),
            ("exitCount", "1"),
        ),
        expected_fragments=(
            "TTC_POLICY_ENTRY_REQUEST",
            "TTC_POLICY_EXIT_REQUEST : TTC policy requested TTC exit reason 4",
        ),
    ),
    ProbeCase(
        name="comm-loss-recovery-precedence",
        initial_soc=60.0,
        ttc_enabled=True,
        loss_timeout_sec=1,
        window_start_offset_sec=1,
        window_end_offset_sec=8,
        wait_after_setup_sec=0.5,
        expected_final_mode="SAFE",
        expected_status_pairs=(
            ("enabled", "yes"),
            ("windowConfigured", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryReason", "WINDOW_ACTIVE"),
            ("entryCount", "1"),
            ("exitCount", "0"),
        ),
        expected_fragments=(
            "TTC_POLICY_ENTRY_REQUEST",
            "COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available 0",
            "RECOVERY_INCIDENT_OPENED : Recovery incident COMM_PRIMARY_UNAVAILABLE",
            "SYS_MODE_CHANGE : System mode changed to SAFE (0)",
        ),
        forbidden_fragments=("TTC_POLICY_EXIT_REQUEST",),
    ),
    ProbeCase(
        name="safety-precedence",
        initial_soc=60.0,
        ttc_enabled=True,
        loss_timeout_sec=10,
        window_start_offset_sec=1,
        window_end_offset_sec=8,
        wait_after_setup_sec=4.5,
        expected_final_mode="SAFE",
        expected_status_pairs=(
            ("enabled", "yes"),
            ("windowConfigured", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryReason", "WINDOW_ACTIVE"),
            ("entryCount", "1"),
            ("exitCount", "0"),
        ),
        expected_fragments=(
            "TTC_POLICY_ENTRY_REQUEST",
            "Mode safety transition TTC (4) -> SAFE (0)",
        ),
        forbidden_fragments=("TTC_POLICY_EXIT_REQUEST",),
        restart_soc=39.0,
        wait_for_fragment_before_restart="SYS_MODE_CHANGE : System mode changed to TTC (4)",
    ),
    ProbeCase(
        name="manual-ttc-policy-coexistence",
        initial_soc=60.0,
        ttc_enabled=False,
        loss_timeout_sec=0,
        window_start_offset_sec=1,
        window_end_offset_sec=8,
        wait_after_setup_sec=2.5,
        expected_final_mode="IDLE",
        expected_status_pairs=(
            ("enabled", "no"),
            ("windowConfigured", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryReason", "NONE"),
            ("exitReason", "TTC_DISABLED"),
            ("entryCount", "0"),
            ("exitCount", "1"),
        ),
        expected_fragments=(
            "SYS_MODE_CHANGE : System mode changed to TTC (4)",
            "TTC_POLICY_EXIT_REQUEST : TTC policy requested TTC exit reason 2",
        ),
        forbidden_fragments=("TTC_POLICY_ENTRY_REQUEST",),
        manual_ttc_entry=True,
    ),
    ProbeCase(
        name="manual-idle-coexistence",
        initial_soc=60.0,
        ttc_enabled=True,
        loss_timeout_sec=10,
        window_start_offset_sec=1,
        window_end_offset_sec=8,
        wait_after_setup_sec=2.5,
        expected_final_mode="IDLE",
        expected_status_pairs=(
            ("enabled", "yes"),
            ("windowConfigured", "yes"),
            ("windowActive", "yes"),
            ("gpsValid", "yes"),
            ("ttcActive", "no"),
            ("entryReason", "WINDOW_ACTIVE"),
            ("exitReason", "NONE"),
            ("entryCount", "1"),
            ("exitCount", "0"),
        ),
        expected_fragments=("TTC_POLICY_ENTRY_REQUEST",),
        manual_idle_exit=True,
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


class HoldOpenTcpServer:
    def __init__(self, host: str, port: int):
        self._host = host
        self._port = port
        self._stop_event = threading.Event()
        self._ready_event = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._listen_socket: Optional[socket.socket] = None
        self._client_sockets: list[socket.socket] = []
        self._lock = threading.Lock()

    def start(self) -> None:
        if self._thread is not None:
            return

        def run() -> None:
            listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listen_socket.bind((self._host, self._port))
            listen_socket.listen(4)
            listen_socket.settimeout(0.2)
            self._listen_socket = listen_socket
            self._ready_event.set()
            try:
                while not self._stop_event.is_set():
                    try:
                        client, _ = listen_socket.accept()
                    except socket.timeout:
                        continue
                    except OSError:
                        break
                    client.settimeout(0.2)
                    with self._lock:
                        self._client_sockets.append(client)
                    threading.Thread(target=self._drain_client, args=(client,), daemon=True).start()
            finally:
                try:
                    listen_socket.close()
                except OSError:
                    pass

        self._thread = threading.Thread(target=run, daemon=True)
        self._thread.start()
        if not self._ready_event.wait(timeout=5.0):
            raise RuntimeError(f"timed out starting dummy ground-link server on {self._host}:{self._port}")

    def _drain_client(self, client: socket.socket) -> None:
        try:
            while not self._stop_event.is_set():
                try:
                    data = client.recv(512)
                except socket.timeout:
                    continue
                except OSError:
                    break
                if not data:
                    break
        finally:
            try:
                client.close()
            except OSError:
                pass
            with self._lock:
                if client in self._client_sockets:
                    self._client_sockets.remove(client)

    def stop(self) -> None:
        self._stop_event.set()
        if self._listen_socket is not None:
            try:
                self._listen_socket.close()
            except OSError:
                pass
            self._listen_socket = None
        with self._lock:
            clients = list(self._client_sockets)
            self._client_sockets.clear()
        for client in clients:
            try:
                client.close()
            except OSError:
                pass
        if self._thread is not None:
            self._thread.join(timeout=5.0)
            self._thread = None


def make_sentence(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"${payload}*{checksum:02X}"


def write_replay_file(path: pathlib.Path, sentence_count: int) -> None:
    lines = []
    for second in range(sentence_count):
        dt = BASE_DATETIME + timedelta(seconds=second)
        payload = (
            f"GPRMC,{dt:%H%M%S},A,4807.038,N,01131.000,E,022.4,084.4,{dt:%d%m%y},003.1,W"
        )
        lines.append(make_sentence(payload))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def collect_output(process: subprocess.Popen[str], lines: list[str], log_handle) -> threading.Thread:
    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)
            log_handle.write(line)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


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


def append_output(log_handle, lines: list[str], process: subprocess.Popen[str]) -> str:
    output = "".join(lines)
    log_handle.write("\n--- OBC stdout ---\n")
    log_handle.write(output)
    log_handle.write(f"\n--- OBC returncode={process.poll()} ---\n")
    return output


def wait_for_fragment(lines: list[str], fragment: str, timeout_sec: float, start_index: int = 0) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in "".join(lines[start_index:]):
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for fragment {fragment!r}")


def mode_lines(output: str) -> list[str]:
    return [line for line in output.splitlines() if line.startswith("mode=")]


def ttc_lines(output: str) -> list[str]:
    return [line for line in output.splitlines() if line.startswith("ttc ")]


def parse_status_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in line.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        fields[key] = value
    return fields


def latest_matching_line(lines: list[str], prefix: str) -> Optional[str]:
    for line in reversed(lines):
        if line.startswith(prefix):
            return line
    return None


def issue_command(
    process: subprocess.Popen[str],
    lines: list[str],
    log_handle,
    case_name: str,
    command: str,
    delay_sec: float = 1.1,
) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"{case_name}: OBC exited before command {command!r}")
    assert process.stdin is not None
    log_handle.write(f"\n>>> {command}\n")
    process.stdin.write(command + "\n")
    process.stdin.flush()
    time.sleep(delay_sec)
    if process.poll() is not None:
        log_handle.write("\n--- buffered OBC stdout before failure ---\n")
        log_handle.write("".join(lines))
        raise RuntimeError(f"{case_name}: OBC exited after command {command!r}")


def wait_for_status_pair(lines: list[str], prefix: str, key: str, expected_value: str, timeout_sec: float) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        line = latest_matching_line(lines, prefix)
        if line is not None and parse_status_fields(line).get(key) == expected_value:
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {prefix} {key}={expected_value}")


def ensure_mode(
    process: subprocess.Popen[str],
    lines: list[str],
    log_handle,
    case_name: str,
    mode_name: str,
    timeout_sec: float,
) -> None:
    deadline = time.time() + timeout_sec
    mode_prefix = f"mode={mode_name.upper()} "
    while time.time() < deadline:
        issue_command(process, lines, log_handle, case_name, f"mode {mode_name}")
        line = latest_matching_line(lines, "mode=")
        if line is not None and mode_prefix in line:
            return
        time.sleep(0.5)
    raise RuntimeError(f"{case_name}: timed out driving mode {mode_name}")


def start_eps(env: dict[str, str], initial_soc: float, log_handle) -> subprocess.Popen[str]:
    return subprocess.Popen(
        [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", f"{initial_soc:.2f}"],
        env=env,
        stdin=subprocess.DEVNULL,
        stdout=log_handle,
        stderr=subprocess.STDOUT,
        text=True,
    )


def run_case(index: int, case: ProbeCase, summary: list[str]) -> None:
    runtime_root = PROBE_TMP_DIR / f"runtime-{case.name}"
    if runtime_root.exists():
        shutil.rmtree(runtime_root)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)

    case_dir = PROBE_TMP_DIR / case.name
    case_dir.mkdir(parents=True, exist_ok=True)
    case_log = case_dir / "case.log"
    replay_file = case_dir / "ttc-pass-window-probe.nmea"
    write_replay_file(replay_file, 16)

    csp_sub_port = 56420 + index
    csp_pub_port = 57420 + index
    radio_port = 17220 + index
    gds_port = 18220 + index

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
    env["OBC_GPS_SOURCE_MODE"] = "replay"
    env["OBC_GPS_REPLAY_FILE"] = str(replay_file)

    processes: list[subprocess.Popen[str]] = []
    obc_lines: list[str] = []
    reader_thread: Optional[threading.Thread] = None
    ground_link_server = HoldOpenTcpServer("127.0.0.1", gds_port)

    with case_log.open("w", encoding="utf-8", buffering=1) as log_handle:
        try:
            final_status_captured = False
            ground_link_server.start()
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
                )
            )
            eps_process = start_eps(env, case.initial_soc, log_handle)
            processes.append(eps_process)
            processes.append(
                subprocess.Popen(
                    [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
                    env=env,
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            )
            processes.append(
                subprocess.Popen(
                    [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
                    stdin=subprocess.DEVNULL,
                    stdout=log_handle,
                    stderr=subprocess.STDOUT,
                    text=True,
                )
            )

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
                    "direct-tcp",
                    "--gds-host",
                    "127.0.0.1",
                    "--gds-port",
                    str(gds_port),
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
            reader_thread = collect_output(obc, obc_lines, log_handle)
            wait_for_fragment(obc_lines, "Type 'help' for commands.", 20.0)
            issue_command(obc, obc_lines, log_handle, case.name, "eps get")
            wait_for_fragment(obc_lines, "EPS_STATUS_RECEIVED", 10.0)
            issue_command(obc, obc_lines, log_handle, case.name, "status")
            wait_for_status_pair(obc_lines, "eps ", "soc", f"{case.initial_soc:.2f}", 10.0)

            ensure_mode(obc, obc_lines, log_handle, case.name, "idle", 10.0)
            issue_command(obc, obc_lines, log_handle, case.name, "gps get")
            issue_command(obc, obc_lines, log_handle, case.name, "gps get")

            if case.ttc_enabled:
                issue_command(
                    obc,
                    obc_lines,
                    log_handle,
                    case.name,
                    f"ttc config on {case.loss_timeout_sec}",
                )
            else:
                issue_command(obc, obc_lines, log_handle, case.name, "ttc config off 0")

            start_epoch = BASE_EPOCH + case.window_start_offset_sec
            end_epoch = BASE_EPOCH + case.window_end_offset_sec
            issue_command(
                obc,
                obc_lines,
                log_handle,
                case.name,
                f"ttc window set {start_epoch} {end_epoch}",
            )

            if case.manual_ttc_entry:
                ensure_mode(obc, obc_lines, log_handle, case.name, "ttc", 10.0)
            if case.manual_idle_exit:
                wait_for_fragment(obc_lines, "TTC_POLICY_ENTRY_REQUEST", 15.0)
                ensure_mode(obc, obc_lines, log_handle, case.name, "idle", 10.0)

            if case.name == "comm-loss-recovery-precedence":
                wait_for_fragment(obc_lines, "SYS_MODE_CHANGE : System mode changed to TTC (4)", 15.0)
                log_handle.write("\n>>> stop dummy ground-link server\n")
                ground_link_server.stop()
                wait_for_fragment(
                    obc_lines,
                    "SYS_MODE_CHANGE : System mode changed to SAFE (0)",
                    10.0,
                )
                issue_command(obc, obc_lines, log_handle, case.name, "status", delay_sec=0.4)
                final_status_captured = True

            if case.restart_soc is not None:
                if case.wait_for_fragment_before_restart is not None:
                    wait_for_fragment(obc_lines, case.wait_for_fragment_before_restart, 15.0)
                if eps_process.poll() is None:
                    eps_process.terminate()
                    eps_process.wait(timeout=5.0)
                processes.remove(eps_process)
                log_handle.write(f"\n>>> restart eps_simulator initial_soc={case.restart_soc:.2f}\n")
                eps_process = start_eps(env, case.restart_soc, log_handle)
                processes.append(eps_process)

            if not final_status_captured:
                time.sleep(case.wait_after_setup_sec)
                issue_command(obc, obc_lines, log_handle, case.name, "status")
                time.sleep(0.5)
            if reader_thread is not None:
                reader_thread.join(timeout=2.0)

            output = append_output(log_handle, obc_lines, obc)

            for fragment in case.expected_fragments:
                if fragment not in output:
                    raise RuntimeError(f"{case.name}: missing fragment {fragment!r}; see {case_log}")
            for fragment in case.forbidden_fragments:
                if fragment in output:
                    raise RuntimeError(f"{case.name}: observed forbidden fragment {fragment!r}; see {case_log}")

            observed_modes = mode_lines(output)
            if not observed_modes:
                raise RuntimeError(f"{case.name}: no mode lines observed; see {case_log}")
            final_mode_line = observed_modes[-1]
            expected_mode_prefix = f"mode={case.expected_final_mode} "
            if expected_mode_prefix not in final_mode_line:
                raise RuntimeError(
                    f"{case.name}: expected final mode {case.expected_final_mode}, got {final_mode_line!r}; see {case_log}"
                )

            observed_ttc = ttc_lines(output)
            if not observed_ttc:
                raise RuntimeError(f"{case.name}: no TTC status lines observed; see {case_log}")
            final_ttc_fields = parse_status_fields(observed_ttc[-1])
            for key, expected_value in case.expected_status_pairs:
                actual_value = final_ttc_fields.get(key)
                if actual_value != expected_value:
                    raise RuntimeError(
                        f"{case.name}: expected TTC status {key}={expected_value}, got {actual_value}; see {case_log}"
                    )

            summary.append(
                f"case-{case.name}=PASS mode={case.expected_final_mode} "
                f"shutdown=terminated log={case_log}"
            )
        finally:
            ground_link_server.stop()
            terminate(processes)


summary_lines: list[str] = []
try:
    for index, case in enumerate(CASES):
        run_case(index, case, summary_lines)
except Exception as exc:
    PROBE_LOG.write_text(
        "\n".join(summary_lines + [f"formal-verdict=ttc-pass-window-mode-v1", f"FAIL: {exc}"]) + "\n",
        encoding="utf-8",
    )
    raise

PROBE_LOG.write_text(
    "\n".join(["ttc-pass-window-mode-v1-probe: PASS", "formal-verdict=ttc-pass-window-mode-v1"] + summary_lines) + "\n",
    encoding="utf-8",
)
print(f"ttc-pass-window-mode-v1-probe: PASS log={PROBE_LOG}")
print("formal-verdict=ttc-pass-window-mode-v1")
for line in summary_lines:
    print(line)
PY
