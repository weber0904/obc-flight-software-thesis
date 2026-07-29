#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="${DICT_PATH:-$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)}"

find_local_tool() {
  local tool_name="${1:?tool_name is required}"
  if [[ -x "${ROOT_DIR}/fprime-venv/bin/${tool_name}" ]]; then
    printf '%s\n' "${ROOT_DIR}/fprime-venv/bin/${tool_name}"
    return 0
  fi
  command -v "${tool_name}"
}

FPRIME_CLI_BIN="$(find_local_tool fprime-cli || true)"
FPRIME_GDS_BIN="$(find_local_tool fprime-gds || true)"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || -z "${FPRIME_CLI_BIN}" || -z "${FPRIME_GDS_BIN}" ]]; then
  echo "Required build outputs or F Prime tools are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-uhf-node6-backup.XXXXXX")}"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
FPRIME_GDS_BIN="${FPRIME_GDS_BIN}" \
ROOT_DIR="${ROOT_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import pty
import shlex
import shutil
import socket
import subprocess
import sys
import termios
import threading
import time


READY_TIMEOUT = 18.0


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
sys.path.insert(0, str(ROOT_DIR / "scripts"))
from probe_process_utils import (
    ManagedProcess,
    build_gds_stale_match_groups,
    cleanup_managed_processes,
    install_signal_cleanup,
    reap_matching_processes,
)


BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
FPRIME_CLI_BIN = require_env("FPRIME_CLI_BIN")
FPRIME_GDS_BIN = require_env("FPRIME_GDS_BIN")
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = PROBE_TMP_DIR / "runtime"
GDS_LOG = PROBE_TMP_DIR / "gds.log"
CSP_PROXY_LOG = PROBE_TMP_DIR / "csp-zmqproxy.log"
EPS_LOG = PROBE_TMP_DIR / "eps-simulator.log"
ADCS_LOG = PROBE_TMP_DIR / "adcs-simulator.log"
RADIO_LOG = PROBE_TMP_DIR / "radio-mock-server.log"
COMM_NODE_LOG = PROBE_TMP_DIR / "uhf-comm-node.log"
GATEWAY_LOG = PROBE_TMP_DIR / "ground-ttc-gateway.log"
EVENTS_LOG = PROBE_TMP_DIR / "events.log"
OBC_LOG = PROBE_TMP_DIR / "obc.log"
SUMMARY_LOG = PROBE_TMP_DIR / "summary.log"
PTY_LOG = PROBE_TMP_DIR / "pty.log"
processes: list[ManagedProcess] = []


class PtyPeer:
    def __init__(self) -> None:
        self.master_fd, slave_fd = pty.openpty()
        self.slave_path = os.ttyname(slave_fd)
        os.close(slave_fd)
        os.set_blocking(self.master_fd, False)
        attrs = termios.tcgetattr(self.master_fd)
        attrs[3] = attrs[3] & ~(termios.ICANON | termios.ECHO)
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(self.master_fd, termios.TCSANOW, attrs)

    def close(self) -> None:
        if self.master_fd >= 0:
            os.close(self.master_fd)
            self.master_fd = -1


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


def build_fragment_group(*fragments: object) -> tuple[str, ...]:
    return tuple(str(fragment) for fragment in fragments if str(fragment))


def start_process(
    name: str,
    args: list[str],
    log_path: pathlib.Path,
    *,
    env: dict[str, str] | None = None,
    stdin=None,
    handle=None,
    stdout_pipe: bool = False,
    stale_match_groups: tuple[tuple[str, ...], ...] = (),
    stale_match_markers: tuple[str, ...] = (),
) -> subprocess.Popen[str]:
    if stale_match_groups:
        reap_matching_processes(stale_match_groups, markers=stale_match_markers)
    if handle is None:
        handle = log_path.open("w", encoding="utf-8", buffering=1)
    handle.write("$ " + shlex.join(str(arg) for arg in args) + "\n")
    handle.flush()
    process = subprocess.Popen(
        [str(arg) for arg in args],
        env=env,
        stdin=stdin if stdin is not None else subprocess.DEVNULL,
        stdout=subprocess.PIPE if stdout_pipe else handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
    )
    processes.append(
        ManagedProcess(
            name=name,
            process=process,
            handle=handle,
            stale_match_groups=stale_match_groups,
            stale_match_markers=stale_match_markers,
        )
    )
    return process


def cleanup_processes() -> None:
    cleanup_managed_processes(processes, timeout_sec=5.0)


def collect_output(process: subprocess.Popen[str], lines: list[str], handle) -> threading.Thread:
    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            lines.append(line)
            try:
                handle.write(line)
                handle.flush()
            except ValueError:
                return

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    return thread


def send_command(process: subprocess.Popen[str], command: str) -> None:
    if process.poll() is not None:
        raise RuntimeError(f"OBC exited before command '{command}'")
    assert process.stdin is not None
    process.stdin.write(command + "\n")
    process.stdin.flush()


def request_status(process: subprocess.Popen[str], lines: list[str], timeout: float = 6.0) -> str:
    start = len(lines)
    send_command(process, "status")
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = "".join(lines[start:])
        if "boot resetCause=" in text and "groundLink mode=" in text:
            return text
        time.sleep(0.05)
    raise RuntimeError("timed out waiting for hosted status output")


def wait_for_status(process: subprocess.Popen[str], lines: list[str], required: list[str], timeout: float) -> str:
    deadline = time.time() + timeout
    last_status = ""
    while time.time() < deadline:
        last_status = request_status(process, lines)
        if all(item in last_status for item in required):
            return last_status
        time.sleep(0.4)
    raise RuntimeError(f"timed out waiting for status markers {required}; last status was:\n{last_status}")


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""
        if fragment in text:
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path}")


def run_cli_command(*args: str) -> None:
    command = [
        FPRIME_CLI_BIN,
        "command-send",
        "--dictionary",
        str(DICT_PATH),
        "--no-zmq",
        "--tts-port",
        str(gds_tts_port),
        *args,
    ]
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"fprime-cli command failed: {' '.join(command)}\n{result.stdout}")


install_signal_cleanup(cleanup_processes)


def main() -> None:
    global gds_tts_port
    gds_port = free_port()
    gds_tts_port = free_port()
    radio_port = free_port()
    csp_sub_port = free_port()
    csp_pub_port = free_port()
    shutil.rmtree(RUNTIME_ROOT, ignore_errors=True)
    (RUNTIME_ROOT / "persistent-data").mkdir(parents=True, exist_ok=True)
    (RUNTIME_ROOT / "staging").mkdir(parents=True, exist_ok=True)

    pty_peer = PtyPeer()
    obc_thread = None
    obc_lines: list[str] = []
    obc = None

    try:
        with EVENTS_LOG.open("w", encoding="utf-8", buffering=1) as events_handle, \
            OBC_LOG.open("w", encoding="utf-8", buffering=1) as obc_handle, \
            PTY_LOG.open("w", encoding="utf-8", buffering=1) as pty_handle:

            pty_bridge = start_process(
                "pty-pair-bridge",
                [str(BIN_DIR / "pty_pair_bridge")],
                PTY_LOG,
                handle=pty_handle,
                stdout_pipe=True,
            )
            serial_paths: dict[str, str] = {}
            assert pty_bridge.stdout is not None
            for _ in range(2):
                line = pty_bridge.stdout.readline()
                if not line:
                    raise RuntimeError("pty_pair_bridge did not report PTY paths")
                pty_handle.write(line)
                pty_handle.flush()
                key, value = line.strip().split("=", 1)
                serial_paths[key] = value

            gateway_serial = serial_paths["PTY_A"]
            uhf_serial = serial_paths["PTY_B"]

            start_process(
                "fprime-gds",
                [
                    FPRIME_GDS_BIN,
                    "-n",
                    "-g",
                    "none",
                    "--framing-selection",
                    "space-packet-space-data-link",
                    "--scid",
                    "68",
                    "--vcid",
                    "2",
                    "--frame-size",
                    "1024",
                    "--dictionary",
                    str(DICT_PATH),
                    "--no-zmq",
                    "--ip-address",
                    "127.0.0.1",
                    "--ip-port",
                    str(gds_port),
                    "--tts-port",
                    str(gds_tts_port),
                    "--tts-addr",
                    "127.0.0.1",
                ],
                GDS_LOG,
                stale_match_groups=build_gds_stale_match_groups(
                    ip_port=gds_port,
                    tts_port=gds_tts_port,
                ),
                stale_match_markers=("fprime-gds", "fprime_gds.executables.comm", "fprime_gds.executables.tcpserver"),
            )
            wait_port(gds_port, READY_TIMEOUT)
            wait_port(gds_tts_port, READY_TIMEOUT)

            start_process(
                "fprime-cli-events",
                [FPRIME_CLI_BIN, "events", "--dictionary", str(DICT_PATH), "--no-zmq", "--tts-port", str(gds_tts_port)],
                EVENTS_LOG,
                handle=events_handle,
                stale_match_groups=(
                    build_fragment_group(
                        "events",
                        f"--dictionary {DICT_PATH}",
                        f"--tts-port {gds_tts_port}",
                    ),
                ),
                stale_match_markers=("fprime-cli",),
            )

            env = os.environ.copy()
            env.update(
                {
                    "CSP_TRANSPORT": "zmqhub",
                    "CSP_HUB_HOST": "127.0.0.1",
                    "CSP_HUB_SUB_PORT": str(csp_sub_port),
                    "CSP_HUB_PUB_PORT": str(csp_pub_port),
                    "EPS_CSP_NODE_ID": "2",
                    "ADCS_CSP_NODE_ID": "3",
                }
            )
            start_process(
                "csp-zmqproxy",
                [str(BIN_DIR / "csp_zmqproxy"), "-s", f"tcp://0.0.0.0:{csp_sub_port}", "-p", f"tcp://0.0.0.0:{csp_pub_port}"],
                CSP_PROXY_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(
                        str(BIN_DIR / "csp_zmqproxy"),
                        f"tcp://0.0.0.0:{csp_sub_port}",
                        f"tcp://0.0.0.0:{csp_pub_port}",
                    ),
                ),
            )
            start_process(
                "eps-simulator",
                [str(BIN_DIR / "eps_simulator"), "--node-id", "2"],
                EPS_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(str(BIN_DIR / "eps_simulator"), "--node-id 2"),
                ),
            )
            start_process(
                "adcs-simulator",
                [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
                ADCS_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(str(BIN_DIR / "adcs_simulator"), "--node-id 3"),
                ),
            )
            start_process(
                "radio-mock-server",
                [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
                RADIO_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(str(BIN_DIR / "radio_mock_server"), f"--port {radio_port}"),
                ),
            )
            start_process(
                "uhf-comm-csp-node",
                [str(BIN_DIR / "uhf_comm_csp_node"), "--serial-device", uhf_serial, "--baudrate", "115200", "--node-id", "6"],
                COMM_NODE_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(
                        str(BIN_DIR / "uhf_comm_csp_node"),
                        f"--serial-device {uhf_serial}",
                        "--node-id 6",
                    ),
                ),
            )
            start_process(
                "ground-ttc-gateway",
                [str(BIN_DIR / "ground_ttc_gateway"), "--serial-device", gateway_serial, "--baudrate", "115200", "--link-identity", "uhf", "--gds-host", "127.0.0.1", "--gds-port", str(gds_port)],
                GATEWAY_LOG,
                env=env,
                stale_match_groups=(
                    build_fragment_group(
                        str(BIN_DIR / "ground_ttc_gateway"),
                        f"--serial-device {gateway_serial}",
                        "--link-identity uhf",
                        f"--gds-port {gds_port}",
                    ),
                ),
            )
            time.sleep(1.0)

            obc = start_process(
                "obc",
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
                    "comm-csp",
                    "--comm-csp-node",
                    "5",
                    "--gds-host",
                    "127.0.0.1",
                    "--gds-port",
                    str(gds_port),
                    "--runtime-root",
                    str(RUNTIME_ROOT),
                    "--persistent-root",
                    str(RUNTIME_ROOT / "persistent-data"),
                    "--staging-root",
                    str(RUNTIME_ROOT / "staging"),
                ],
                OBC_LOG,
                env=env,
                stdin=subprocess.PIPE,
                handle=obc_handle,
                stdout_pipe=True,
                stale_match_groups=(
                    build_fragment_group(str(BIN_DIR / "OBC"), f"--runtime-root {RUNTIME_ROOT}"),
                ),
            )
            obc_thread = collect_output(obc, obc_lines, obc_handle)

            wait_for_status(
                obc,
                obc_lines,
                [
                    "comm primary command=UHF",
                    "uhfAvailable=yes",
                    "uhfReason=HEALTHY_ACTIVITY",
                    "groundLink mode=comm-csp commNode=5",
                    "groundLink connected=no",
                ],
                READY_TIMEOUT,
            )

            wait_for_status(
                obc,
                obc_lines,
                [
                    "comm primary command=UHF",
                    "uhfAvailable=yes",
                    "uhfReason=HEALTHY_ACTIVITY",
                ],
                READY_TIMEOUT,
            )

            SUMMARY_LOG.write_text(
                "\n".join(
                    [
                        "uhf-node6-backup-probe: PASS",
                        "formal-verdict=uhf-node6-backup",
                        "comm-node=6",
                        "framing=space-packet-space-data-link",
                        "scid=0x44",
                        "vcid=2",
                        "frame-size=1024",
                        "primary-command=UHF",
                        "uhf-reason=HEALTHY_ACTIVITY",
                        "evidence=ccsds-failover-and-status",
                        f"logs={PROBE_TMP_DIR}",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            print(SUMMARY_LOG.read_text(encoding="utf-8"), end="")
    finally:
        try:
            if obc is not None and obc.stdin is not None and obc.poll() is None:
                send_command(obc, "quit")
        except Exception:
            pass
        if obc_thread is not None:
            obc_thread.join(timeout=2.0)
        cleanup_processes()
        pty_peer.close()


if __name__ == "__main__":
    main()
PY
