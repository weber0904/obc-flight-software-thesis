#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route2-ttc-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/route2-mode-ttc-entry-hosted.log}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
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
from datetime import datetime, timedelta, timezone


ROOT_DIR = pathlib.Path(os.environ["ROOT_DIR"])
BIN_DIR = pathlib.Path(os.environ["BIN_DIR"])
PROBE_TMP_DIR = pathlib.Path(os.environ["PROBE_TMP_DIR"])
PROBE_LOG = pathlib.Path(os.environ["PROBE_LOG"])
sys.path.insert(0, str(ROOT_DIR / "scripts/chapter5_routes/lib"))

from eps_control_client import set_soc  # noqa: E402


def cleanup_port(port: int, expected_command_substring: str) -> None:
    result = subprocess.run(
        ["lsof", "-nP", f"-iTCP:{port}", "-sTCP:LISTEN", "-Fpct"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return
    pid = None
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
    def __init__(self, host: str, port: int) -> None:
        self.host = host
        self.port = port
        self._server: socket.socket | None = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()

    def start(self) -> None:
        self.stop()
        self._stop.clear()
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((self.host, self.port))
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

        self._thread = threading.Thread(target=run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._server is not None:
            try:
                self._server.close()
            except OSError:
                pass
            self._server = None
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None


def write_replay_file(path: pathlib.Path) -> None:
    def make_sentence(payload: str) -> str:
        checksum = 0
        for ch in payload:
            checksum ^= ord(ch)
        return f"${payload}*{checksum:02X}"

    base = datetime.now(timezone.utc) + timedelta(seconds=2)
    sentences: list[str] = []
    for index in range(12):
        moment = base + timedelta(seconds=index)
        hhmmss = moment.strftime("%H%M%S")
        ddmmyy = moment.strftime("%d%m%y")
        payload = f"GPRMC,{hhmmss}.00,A,2503.7135,N,12133.5335,E,0.0,0.0,{ddmmyy},0.0,E"
        sentences.append(make_sentence(payload))
    path.write_text("\n".join(sentences) + "\n", encoding="utf-8")


def collect_output(process: subprocess.Popen[str], lines: list[str], handle) -> threading.Thread:
    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)
            handle.write(line)
            handle.flush()
    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def start_pty_bridge(handle, processes: list[subprocess.Popen[str]]) -> tuple[str, str]:
    bridge = subprocess.Popen(
        [str(BIN_DIR / "pty_pair_bridge")],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    if bridge.stdout is None:
        raise RuntimeError("pty_pair_bridge did not expose stdout")
    paths: dict[str, str] = {}
    for _ in range(2):
        line = bridge.stdout.readline()
        if not line:
            raise RuntimeError("pty_pair_bridge did not report PTY paths")
        handle.write(line)
        handle.flush()
        key, value = line.strip().split("=", 1)
        paths[key] = value
    processes.append(bridge)
    return paths["PTY_A"], paths["PTY_B"]


def wait_for_fragment(lines: list[str], fragment: str, timeout_sec: float, start_index: int = 0) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in "".join(lines[start_index:]):
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for fragment {fragment!r}")


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command {command!r}")
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def request_status(process: subprocess.Popen[str], lines: list[str], timeout: float = 12.0) -> str:
    start = len(lines)
    send_command(process, "status")
    deadline = time.time() + timeout
    last = ""
    while time.time() < deadline:
        text = "".join(lines[start:])
        last = text
        if "mode=" in text and "ttc " in text and "adcs mode=" in text and "groundLink connected=" in text:
            return text
        time.sleep(0.05)
    return last


def send_command_and_wait(
    process: subprocess.Popen[str], lines: list[str], command: str, fragment: str, timeout: float
) -> None:
    start = len(lines)
    send_command(process, command)
    wait_for_fragment(lines, fragment, timeout, start)


def extract_status_field(status_text: str, key: str) -> str:
    for token in status_text.replace("\n", " ").split():
        if token.startswith(f"{key}="):
            return token.split("=", 1)[1]
    raise RuntimeError(f"missing status field {key!r} in:\n{status_text}")


def wait_for_status_contains(process: subprocess.Popen[str], lines: list[str], required: list[str], timeout: float) -> str:
    deadline = time.time() + timeout
    last = ""
    while time.time() < deadline:
        last = request_status(process, lines)
        if all(fragment in last for fragment in required):
            return last
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for status fragments {required}; last status:\n{last}")


def terminate_all(processes: list[subprocess.Popen[str]]) -> None:
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


runtime_root = PROBE_TMP_DIR / "runtime"
replay_file = PROBE_TMP_DIR / "ttc-route2-replay.nmea"
control_socket = PROBE_TMP_DIR / "eps-control.sock"
if runtime_root.exists():
    shutil.rmtree(runtime_root)
(runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)
write_replay_file(replay_file)
control_socket.unlink(missing_ok=True)

csp_sub_port = 56540
csp_pub_port = 57540
radio_port = 17340
gds_port = 18340
cleanup_port(csp_sub_port, "csp_zmq")
cleanup_port(csp_pub_port, "csp_zmq")
cleanup_port(radio_port, "radio_mock")

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
ground_link_server = HoldOpenTcpServer("127.0.0.1", gds_port)

with PROBE_LOG.open("w", encoding="utf-8", buffering=1) as handle:
    try:
        _, uhf_serial = start_pty_bridge(handle, processes)
        ground_link_server.start()
        processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "csp_zmqproxy"), "-s", f"tcp://0.0.0.0:{csp_sub_port}", "-p", f"tcp://0.0.0.0:{csp_pub_port}"],
                stdin=subprocess.DEVNULL,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
            )
        )
        processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "9.00", "--control-socket", str(control_socket)],
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
            )
        )
        processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
            )
        )
        processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "uhf_comm_csp_node"), "--serial-device", uhf_serial, "--baudrate", "115200", "--node-id", "6"],
                env=env,
                stdin=subprocess.DEVNULL,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
            )
        )
        processes.append(
            subprocess.Popen(
                [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
                stdin=subprocess.DEVNULL,
                stdout=handle,
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
        reader = collect_output(obc, obc_lines, handle)
        wait_for_fragment(obc_lines, "Type 'help' for commands.", 20.0)
        wait_for_fragment(obc_lines, "Mode safety transition SAFE (0) -> HELL (2)", 20.0)

        print(set_soc(control_socket, 16.0))
        wait_for_fragment(obc_lines, "Mode safety transition HELL (2) -> SAFE (0)", 20.0)

        print(set_soc(control_socket, 60.0))
        time.sleep(2.0)
        send_command_and_wait(obc, obc_lines, "mode idle", "mode response=0", 20.0)
        wait_for_fragment(obc_lines, "System mode changed to IDLE (1)", 20.0)
        send_command(obc, "ttc config on 10")
        wait_for_fragment(obc_lines, "TTC policy config updated enabled 1 loss timeout 10", 20.0)
        gps_unix = int(time.time())
        send_command(obc, f"ttc window set {max(0, gps_unix - 30)} {gps_unix + 60}")

        wait_for_fragment(obc_lines, "TTC policy requested TTC entry reason 1", 40.0)
        wait_for_fragment(obc_lines, "System mode changed to TTC (4)", 40.0)
        wait_for_fragment(obc_lines, "ADCS mode changed to POINTING (2)", 40.0)

        send_command(obc, "quit")
        assert obc.stdin is not None
        obc.stdin.close()
        reader.join(timeout=2.0)
        if obc.wait(timeout=10.0) != 0:
            raise RuntimeError(f"OBC exited with {obc.returncode}")

        print(f"chapter5-route2-mode-ttc-entry-hosted: PASS log={PROBE_LOG}")
        print("mode-path=SAFE->HELL->SAFE->IDLE->TTC with TTC auto-entry and ADCS POINTING readback")
    finally:
        ground_link_server.stop()
        terminate_all(processes)
PY
