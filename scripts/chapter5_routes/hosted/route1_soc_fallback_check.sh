#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route1-soc-hosted.XXXXXX")}"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/route1-soc-fallback-hosted.log}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
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
import sys
import threading
import time


ROOT_DIR = pathlib.Path(os.environ["ROOT_DIR"])
BIN_DIR = pathlib.Path(os.environ["BIN_DIR"])
PROBE_TMP_DIR = pathlib.Path(os.environ["PROBE_TMP_DIR"])
PROBE_LOG = pathlib.Path(os.environ["PROBE_LOG"])
sys.path.insert(0, str(ROOT_DIR / "scripts"))
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


def wait_for_fragment(lines: list[str], fragment: str, timeout_sec: float, start_index: int = 0) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in "".join(lines[start_index:]):
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for fragment {fragment!r}")


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
        if "mode=" in text and "groundLink connected=" in text:
            return text
        time.sleep(0.05)
    return last


def wait_for_status_contains(process: subprocess.Popen[str], lines: list[str], required: list[str], timeout: float) -> str:
    deadline = time.time() + timeout
    last = ""
    while time.time() < deadline:
        last = request_status(process, lines)
        if all(fragment in last for fragment in required):
            return last
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for status fragments {required}; last status:\n{last}")


def parse_eps_soc(status_text: str) -> float | None:
    match = re.search(r"eps soc=(\d+(?:\.\d+)?)", status_text)
    if match is None:
        return None
    return float(match.group(1))


def wait_for_status_state(
    process: subprocess.Popen[str],
    lines: list[str],
    *,
    mode: str,
    expected_soc: float,
    adcs_mode: str | None = None,
    timeout: float,
    soc_tolerance: float = 0.5,
) -> str:
    deadline = time.time() + timeout
    last = ""
    while time.time() < deadline:
        last = request_status(process, lines)
        if f"mode={mode}" not in last:
            time.sleep(0.2)
            continue
        if adcs_mode is not None and f"adcs mode={adcs_mode}" not in last:
            time.sleep(0.2)
            continue
        observed_soc = parse_eps_soc(last)
        if observed_soc is None:
            time.sleep(0.2)
            continue
        if abs(observed_soc - expected_soc) <= soc_tolerance:
            return last
        time.sleep(0.2)
    raise RuntimeError(
        "timed out waiting for status state "
        f"mode={mode} expected_soc={expected_soc:.2f} adcs_mode={adcs_mode or '<any>'}; last status:\n{last}"
    )


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
control_socket = PROBE_TMP_DIR / "eps-control.sock"
if runtime_root.exists():
    shutil.rmtree(runtime_root)
(runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)
control_socket.unlink(missing_ok=True)


def cleanup_owned_sequence_aliases() -> None:
    """Remove only aliases generated for this probe's runtime root."""
    owned_root = runtime_root.resolve()
    for pattern in (".adm-*", ".stg-*"):
        for candidate in ROOT_DIR.glob(pattern):
            if not candidate.is_symlink():
                continue
            try:
                candidate.resolve().relative_to(owned_root)
            except ValueError:
                continue
            candidate.unlink()


csp_sub_port = 56520
csp_pub_port = 57520
radio_port = 17320
gds_port = 18320
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
                [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "80.00", "--control-socket", str(control_socket)],
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
        wait_for_status_state(obc, obc_lines, mode="SAFE", expected_soc=80.0, adcs_mode="IDLE", timeout=30.0)

        send_command(obc, "mode idle")
        wait_for_fragment(obc_lines, "System mode changed to IDLE (1)", 20.0)
        send_command(obc, "mode payload")
        wait_for_fragment(obc_lines, "System mode changed to PAYLOAD (3)", 20.0)
        wait_for_status_state(obc, obc_lines, mode="PAYLOAD", expected_soc=80.0, timeout=20.0)

        print(set_soc(control_socket, 59.0))
        wait_for_fragment(obc_lines, "Mode safety transition PAYLOAD (3) -> IDLE (1)", 20.0)
        wait_for_status_state(obc, obc_lines, mode="IDLE", expected_soc=59.0, timeout=20.0)

        start = len(obc_lines)
        send_command(obc, "mode payload")
        wait_for_fragment(obc_lines, "System mode transition rejected from IDLE (1) to PAYLOAD (3) reason 4", 20.0, start)

        print(set_soc(control_socket, 39.0))
        wait_for_fragment(obc_lines, "Mode safety transition IDLE (1) -> SAFE (0)", 20.0)
        wait_for_status_state(obc, obc_lines, mode="SAFE", expected_soc=39.0, timeout=20.0)

        send_command(obc, "quit")
        assert obc.stdin is not None
        obc.stdin.close()
        reader.join(timeout=2.0)
        if obc.wait(timeout=10.0) != 0:
            raise RuntimeError(f"OBC exited with {obc.returncode}")

        print(f"chapter5-route1-soc-fallback-hosted: PASS log={PROBE_LOG}")
        print("fallback-path=PAYLOAD->IDLE then IDLE->SAFE with live EPS control socket")
    finally:
        ground_link_server.stop()
        terminate_all(processes)
        cleanup_owned_sequence_aliases()
PY
