#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" || true)"
if [[ -z "${DICT_PATH}" ]]; then
  echo "Dictionary not found. Run fprime-util build first." >&2
  exit 1
fi

find_local_tool() {
  local tool_name="${1:?tool_name is required}"
  if [[ -x "${ROOT_DIR}/fprime-venv/bin/${tool_name}" ]]; then
    printf '%s\n' "${ROOT_DIR}/fprime-venv/bin/${tool_name}"
    return 0
  fi
  command -v "${tool_name}"
}

FPRIME_CLI_BIN="$(find_local_tool fprime-cli || true)"
if [[ -z "${FPRIME_CLI_BIN}" ]]; then
  echo "fprime-cli not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-$(obc_default_remote_workspace_dir)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_COMM_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE:-can1}"
CSP_TRANSPORT="${CSP_TRANSPORT:-socketcan}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
CAN_BITRATE="${CAN_BITRATE:-500000}"
CAN_DBITRATE="${CAN_DBITRATE:-2000000}"
CAN_RESTART_MS="${CAN_RESTART_MS:-100}"
PREPARE_CAN_INTERFACES="${PREPARE_CAN_INTERFACES:-1}"
SSH_CONNECT_TIMEOUT_SEC="${SSH_CONNECT_TIMEOUT_SEC:-15}"
PREPARE_REMOTE_RUNTIME="${PREPARE_REMOTE_RUNTIME:-1}"
COMM_SOCKETCAN_FILE_PROBE_MODE="${COMM_SOCKETCAN_FILE_PROBE_MODE:-file-downlink}"
DIAGNOSTIC_TTC_CYCLES="${DIAGNOSTIC_TTC_CYCLES:-3}"
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES:-20}"
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS:-500}"
LAB_SERIAL_STARTUP_DELAY_SEC="${LAB_SERIAL_STARTUP_DELAY_SEC:-12}"
COMMAND_ATTEMPTS="${COMMAND_ATTEMPTS:-12}"
HK_TARGET_OCCUPIED_SLOTS="${HK_TARGET_OCCUPIED_SLOTS:-2}"
HK_CAPTURE_MAX_ATTEMPTS="${HK_CAPTURE_MAX_ATTEMPTS:-32}"
HK_CAPTURE_DELAY_SEC="${HK_CAPTURE_DELAY_SEC:-0.8}"
FILE_DOWNLINK_TIMEOUT_SEC="${FILE_DOWNLINK_TIMEOUT_SEC:-60}"
FILE_DOWNLINK_COMMAND_ATTEMPTS="${FILE_DOWNLINK_COMMAND_ATTEMPTS:-3}"
FILE_DOWNLINK_START_TIMEOUT_SEC="${FILE_DOWNLINK_START_TIMEOUT_SEC:-20}"
CAN_CAPTURE_TIMEOUT_SEC="${CAN_CAPTURE_TIMEOUT_SEC:-25}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/comm-csp-socketcan-file-downlink}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-csp-socketcan-file-downlink-gds-downlink}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-csp-socketcan-file-downlink.XXXXXX")}"

if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "This probe requires CSP_TRANSPORT=socketcan." >&2
  exit 2
fi
if [[ "${CSP_CAN_PROMISC}" != "0" ]]; then
  echo "Governed acceptance must use CSP_CAN_PROMISC=0." >&2
  exit 2
fi
if [[ "${PREPARE_CAN_INTERFACES}" != "0" && "${PREPARE_CAN_INTERFACES}" != "1" ]]; then
  echo "PREPARE_CAN_INTERFACES must be 0 or 1." >&2
  exit 2
fi
if [[ "${PREPARE_REMOTE_RUNTIME}" != "0" && "${PREPARE_REMOTE_RUNTIME}" != "1" ]]; then
  echo "PREPARE_REMOTE_RUNTIME must be 0 or 1." >&2
  exit 2
fi
if [[ "${COMM_SOCKETCAN_FILE_PROBE_MODE}" != "file-downlink" && "${COMM_SOCKETCAN_FILE_PROBE_MODE}" != "ttc-prereq" ]]; then
  echo "COMM_SOCKETCAN_FILE_PROBE_MODE must be file-downlink or ttc-prereq." >&2
  exit 2
fi
if ! [[ "${HK_TARGET_OCCUPIED_SLOTS}" =~ ^[1-9][0-9]*$ && \
  "${HK_CAPTURE_MAX_ATTEMPTS}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_COMMAND_ATTEMPTS}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_START_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${CAN_CAPTURE_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${SSH_CONNECT_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${DIAGNOSTIC_TTC_CYCLES}" =~ ^[1-9][0-9]*$ && \
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" =~ ^[0-9]+$ && \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" =~ ^[0-9]+$ && \
  "${COMMAND_ATTEMPTS}" =~ ^[1-9][0-9]*$ ]]; then
  echo "Numeric probe settings are invalid." >&2
  exit 2
fi
if [[ ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial device not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi
obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
if [[ "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" == "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_CSP_CAN_DEVICE and SUBSYSTEM_SIM_COMM_CAN_DEVICE must differ." >&2
  exit 2
fi

port_is_available() {
  local port="${1:?port is required}"
  python3 - "${port}" <<'PY'
import socket
import sys

port = int(sys.argv[1])
for socket_type in (socket.SOCK_STREAM, socket.SOCK_DGRAM):
    sock = socket.socket(socket.AF_INET, socket_type)
    try:
        sock.bind(("0.0.0.0", port))
    except OSError:
        sys.exit(1)
    finally:
        sock.close()
PY
}

find_free_port_pair() {
  local first_port="${1:?first port is required}"
  local second_offset="${2:?offset is required}"
  local candidate="${first_port}"
  while ! port_is_available "${candidate}" || ! port_is_available "$((candidate + second_offset))"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

if [[ -z "${GDS_PORT:-}" ]]; then
  GDS_PORT="$(find_free_port_pair 51900 1)"
fi
if [[ -z "${GDS_TTS_PORT:-}" ]]; then
  GDS_TTS_PORT="$((GDS_PORT + 1))"
fi

mkdir -p "${PROBE_TMP_DIR}"
rm -rf "${GDS_FILE_STORAGE_DIR}"
mkdir -p "${GDS_FILE_STORAGE_DIR}"

python3 - \
  "${ROOT_DIR}" \
  "${BIN_DIR}" \
  "${DICT_PATH}" \
  "${FPRIME_CLI_BIN}" \
  "${PROBE_TMP_DIR}" \
  "${GDS_FILE_STORAGE_DIR}" \
  "${OBC_SSH_TARGET}" \
  "${SUBSYSTEM_SIM_SSH_TARGET}" \
  "${RPI_REMOTE_DIR}" \
  "${SUBSYSTEM_SIM_REMOTE_DIR}" \
  "${HOST_SERIAL_DEVICE}" \
  "${SUBSYSTEM_SIM_COMM_DEVICE}" \
  "${COMM_BAUDRATE}" \
  "${COMM_CSP_NODE}" \
  "${EPS_CSP_NODE_ID}" \
  "${ADCS_CSP_NODE_ID}" \
  "${OBC_CSP_CAN_DEVICE}" \
  "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
  "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" \
  "${CSP_TRANSPORT}" \
  "${CSP_CAN_PROMISC}" \
  "${CAN_BITRATE}" \
  "${CAN_DBITRATE}" \
  "${CAN_RESTART_MS}" \
  "${PREPARE_CAN_INTERFACES}" \
  "${SSH_CONNECT_TIMEOUT_SEC}" \
  "${PREPARE_REMOTE_RUNTIME}" \
  "${COMM_SOCKETCAN_FILE_PROBE_MODE}" \
  "${DIAGNOSTIC_TTC_CYCLES}" \
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" \
  "${LAB_SERIAL_STARTUP_DELAY_SEC}" \
  "${COMMAND_ATTEMPTS}" \
  "${HK_TARGET_OCCUPIED_SLOTS}" \
  "${HK_CAPTURE_MAX_ATTEMPTS}" \
  "${HK_CAPTURE_DELAY_SEC}" \
  "${FILE_DOWNLINK_TIMEOUT_SEC}" \
  "${FILE_DOWNLINK_COMMAND_ATTEMPTS}" \
  "${FILE_DOWNLINK_START_TIMEOUT_SEC}" \
  "${CAN_CAPTURE_TIMEOUT_SEC}" \
  "${RUNTIME_ROOT}" \
  "${GDS_PORT}" \
  "${GDS_TTS_PORT}" <<'PY'
import csv
import filecmp
import hashlib
import io
import os
import pathlib
import re
import shlex
import subprocess
import sys
import time

(
    root_dir,
    bin_dir,
    dict_path,
    fprime_cli_bin,
    probe_tmp_dir,
    gds_file_storage_dir,
    obc_ssh_target,
    subsystem_ssh_target,
    rpi_remote_dir,
    subsystem_remote_dir,
    host_serial_device,
    subsystem_serial_device,
    comm_baudrate,
    comm_csp_node,
    eps_csp_node_id,
    adcs_csp_node_id,
    obc_can_device,
    subsystem_eps_adcs_can_device,
    subsystem_comm_can_device,
    csp_transport,
    csp_can_promisc,
    can_bitrate,
    can_dbitrate,
    can_restart_ms,
    prepare_can_interfaces,
    ssh_connect_timeout_sec,
    prepare_remote_runtime_flag,
    probe_mode,
    diagnostic_ttc_cycles,
    preamble_lines,
    preamble_delay_ms,
    startup_delay_sec,
    command_attempts,
    target_occupied_slots,
    capture_max_attempts,
    capture_delay_sec,
    file_timeout_sec,
    file_command_attempts,
    file_start_timeout_sec,
    can_capture_timeout_sec,
    runtime_root,
    gds_port,
    gds_tts_port,
) = sys.argv[1:]

command_attempts = int(command_attempts)
diagnostic_ttc_cycles = int(diagnostic_ttc_cycles)
target_occupied_slots = int(target_occupied_slots)
capture_max_attempts = int(capture_max_attempts)
capture_delay_sec = float(capture_delay_sec)
file_timeout_sec = int(file_timeout_sec)
file_command_attempts = int(file_command_attempts)
file_start_timeout_sec = int(file_start_timeout_sec)
can_capture_timeout_sec = int(can_capture_timeout_sec)
startup_delay_sec = float(startup_delay_sec)

bringup_log = os.path.join(probe_tmp_dir, "can-bringup.log")
ground_gds_log = os.path.join(probe_tmp_dir, "ground-gds.log")
subsystem_stack_log = os.path.join(probe_tmp_dir, "subsystem-can-comm-stack.log")
gateway_log = os.path.join(probe_tmp_dir, "gateway.log")
obc_log = os.path.join(probe_tmp_dir, "obc.log")
cli_command_log = os.path.join(probe_tmp_dir, "command-send.log")
cli_events_log = os.path.join(probe_tmp_dir, "events.log")
cli_channels_log = os.path.join(probe_tmp_dir, "channels.log")
subsystem_pre_stats_log = os.path.join(probe_tmp_dir, "subsystem-pre-stats.log")
subsystem_post_stats_log = os.path.join(probe_tmp_dir, "subsystem-post-stats.log")
obc_pre_stats_log = os.path.join(probe_tmp_dir, "obc-pre-stats.log")
obc_post_stats_log = os.path.join(probe_tmp_dir, "obc-post-stats.log")
subsystem_parentdev_log = os.path.join(probe_tmp_dir, "subsystem-parentdev.log")
subsystem_eps_adcs_candump_log = os.path.join(probe_tmp_dir, "subsystem-eps-adcs.candump.log")
subsystem_comm_candump_log = os.path.join(probe_tmp_dir, "subsystem-comm.candump.log")
obc_candump_log = os.path.join(probe_tmp_dir, "obc.candump.log")
snapshot_dir = pathlib.Path(probe_tmp_dir) / "source-snapshots"
snapshot_dir.mkdir(parents=True, exist_ok=True)


def shq(value: str) -> str:
    return shlex.quote(str(value))


def ssh_command(target: str, script: str):
    return [
        "ssh",
        "-o",
        "BatchMode=yes",
        "-o",
        f"ConnectTimeout={ssh_connect_timeout_sec}",
        target,
        "/bin/bash -lc " + shq(script),
    ]


def ssh_cmd(target: str, script: str, *, stdout=None, stderr=None, check=True, text=True):
    return subprocess.run(ssh_command(target, script), stdout=stdout, stderr=stderr, check=check, text=text)


def ssh_capture(target: str, script: str) -> str:
    result = ssh_cmd(target, script, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"ssh command failed on {target}: {result.stderr.strip()}")
    return result.stdout


def start_ssh_process(target: str, script: str, handle):
    return subprocess.Popen(
        ssh_command(target, script),
        stdin=subprocess.DEVNULL,
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )


def start_process(args, *, env=None, handle=None, stdin=None):
    return subprocess.Popen(
        args,
        env=env,
        stdin=stdin if stdin is not None else subprocess.DEVNULL,
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )


def sanitize_file(path: str) -> str:
    if not os.path.exists(path):
        return ""
    with open(path, "rb") as handle:
        return handle.read().replace(b"\0", b"\n").decode("utf-8", errors="replace")


def log_position(path: str) -> int:
    return len(sanitize_file(path))


def log_since(path: str, position: int) -> str:
    return sanitize_file(path)[position:]


def append_file(path: str, text: str):
    with open(path, "a", encoding="utf-8") as handle:
        handle.write(text)


def bring_up_can(target: str, remote_dir: str, can_device: str):
    script = (
        f"set -euo pipefail; cd {shq(remote_dir)}; "
        f"sudo -n ip link set {shq(can_device)} down >/dev/null 2>&1 || true; "
        f"sudo -n ip link set {shq(can_device)} up type can bitrate {shq(can_bitrate)} "
        f"dbitrate {shq(can_dbitrate)} restart-ms {shq(can_restart_ms)} fd on"
    )
    with open(bringup_log, "a", encoding="utf-8") as handle:
        handle.write(f"Preparing {target}:{can_device}\n")
        ssh_cmd(target, script, stdout=handle, stderr=subprocess.STDOUT)


def prepare_remote_runtime():
    script = f"set -euo pipefail; rm -rf {shq(runtime_root)}; mkdir -p {shq(runtime_root)}"
    ssh_cmd(obc_ssh_target, script, check=True)


def capture_stats(target: str, devices, output_path: str):
    chunks = []
    for device in devices:
        chunks.append(f"echo '==== {shq(device)} ===='")
        chunks.append(f"ip -details -statistics link show dev {shq(device)}")
    script = "set -euo pipefail; " + "; ".join(chunks)
    with open(output_path, "w", encoding="utf-8") as handle:
        ssh_cmd(target, script, stdout=handle, stderr=subprocess.STDOUT, check=False)


def start_candump(target: str, device: str, output_path: str):
    script = f"set -euo pipefail; timeout {can_capture_timeout_sec} candump {shq(device)} || test $? -eq 124"
    handle = open(output_path, "w", encoding="utf-8", buffering=1)
    return handle, start_ssh_process(target, script, handle)


def start_remote_subsystem_stack(handle):
    remote_script = f"""
set -euo pipefail
cd {shq(subsystem_remote_dir)}
source scripts/_common.sh
BIN_DIR="$(obc_find_native_bin_dir "$PWD")"
if [[ -z "${{BIN_DIR}}" ]]; then echo 'Build output not found on subsystem.local.' >&2; exit 1; fi
PIDS="$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(eps_simulator|adcs_simulator|comm_csp_node)' || true)"
if [[ -n "${{PIDS}}" ]]; then kill ${{PIDS}} >/dev/null 2>&1 || true; sleep 1; fi
cleanup() {{ jobs -p | xargs -r kill >/dev/null 2>&1 || true; wait || true; }}
trap cleanup EXIT INT TERM
echo 'Starting subsystem EPS/ADCS + COMM SocketCAN stack'
echo '  EPS/ADCS CAN : {subsystem_eps_adcs_can_device}'
echo '  COMM CAN     : {subsystem_comm_can_device}'
echo '  COMM serial  : {subsystem_serial_device} @ {comm_baudrate}'
echo '  COMM node    : {comm_csp_node}'
CSP_TRANSPORT={shq(csp_transport)} CSP_CAN_DEVICE={shq(subsystem_eps_adcs_can_device)} CSP_CAN_PROMISC={shq(csp_can_promisc)} "${{BIN_DIR}}/eps_simulator" --node-id {shq(eps_csp_node_id)} &
CSP_TRANSPORT={shq(csp_transport)} CSP_CAN_DEVICE={shq(subsystem_eps_adcs_can_device)} CSP_CAN_PROMISC={shq(csp_can_promisc)} "${{BIN_DIR}}/adcs_simulator" --node-id {shq(adcs_csp_node_id)} &
CSP_TRANSPORT={shq(csp_transport)} CSP_CAN_DEVICE={shq(subsystem_comm_can_device)} CSP_CAN_PROMISC={shq(csp_can_promisc)} "${{BIN_DIR}}/comm_csp_node" --serial-device {shq(subsystem_serial_device)} --baudrate {shq(comm_baudrate)} --node-id {shq(comm_csp_node)} &
wait
"""
    return start_ssh_process(subsystem_ssh_target, remote_script, handle)


def require_running(process, name: str):
    if process.poll() is not None:
        raise RuntimeError(f"{name} exited early with code {process.returncode}")


def wait_for_log(path: str, predicate, timeout_sec: float, description: str):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        text = sanitize_file(path)
        if predicate(text):
            return text
        time.sleep(0.3)
    raise RuntimeError(f"Timed out waiting for {description} in {path}")


def wait_for_csp_ping(process, node: str, timeout_sec: float, description: str):
    success_fragment = f"CSP ping node {node} success 1"
    for _ in range(command_attempts):
        ping_log_start = log_position(obc_log)
        send_obc_command(process, f"csp ping {node}", 0.5)
        try:
            return wait_for_log(obc_log, lambda text: success_fragment in text[ping_log_start:], timeout_sec, description)
        except RuntimeError:
            time.sleep(0.5)
    raise RuntimeError(f"Timed out waiting for {description} in {obc_log}")


def parse_bus_off_counts(stats_text: str):
    counts = []
    lines = stats_text.splitlines()
    for index, line in enumerate(lines):
        columns = line.split()
        if "bus-off" not in columns:
            continue
        bus_off_index = columns.index("bus-off")
        for values_line in lines[index + 1 : index + 4]:
            values = values_line.split()
            if len(values) <= bus_off_index:
                continue
            if all(re.fullmatch(r"[0-9]+", value) for value in values[: len(columns)]):
                counts.append(int(values[bus_off_index]))
                break
    return counts


def stats_section(stats_text: str, device: str) -> str:
    marker = f"==== {device} ===="
    start = stats_text.find(marker)
    if start < 0:
        return ""
    next_marker = stats_text.find("\n==== ", start + len(marker))
    if next_marker < 0:
        return stats_text[start:]
    return stats_text[start:next_marker]


def require_can_stats_healthy(stats_text: str, device: str, description: str):
    section = stats_section(stats_text, device)
    if not section:
        raise RuntimeError(f"{description} stats did not include {device}")
    if "ERROR-ACTIVE" not in section:
        raise RuntimeError(f"{description} {device} did not report ERROR-ACTIVE")
    bus_off_counts = parse_bus_off_counts(section)
    if not bus_off_counts:
        raise RuntimeError(f"{description} {device} did not include CAN bus-off counters")
    if any(count != 0 for count in bus_off_counts):
        raise RuntimeError(f"{description} {device} reported nonzero bus-off")


def has_candump_frame(path: str, device: str) -> bool:
    text = sanitize_file(path)
    return re.search(rf"^\s*{re.escape(device)}\s+[0-9A-Fa-f]{{3,8}}\s+\[\d+\]\s+", text, re.MULTILINE) is not None


def send_obc_command(process, command: str, delay_sec: float = 0.5):
    if process.stdin is None:
        raise RuntimeError("OBC process stdin is not available")
    process.stdin.write(command + "\n")
    process.stdin.flush()
    time.sleep(delay_sec)


def wait_for_ground_link_connected(description: str):
    for _ in range(command_attempts):
        status_log_start = log_position(obc_log)
        send_obc_command(obc, "status", 1.0)
        if "groundLink connected=yes" in log_since(obc_log, status_log_start):
            return
        time.sleep(0.5)
    raise RuntimeError(f"Timed out waiting for connected COMM ground link before {description}")


def run_cli_command(command_name: str, *extra_args):
    wait_for_ground_link_connected(command_name)
    command = [
        fprime_cli_bin,
        "command-send",
        "--dictionary",
        dict_path,
        "--no-zmq",
        "--tts-port",
        gds_tts_port,
        command_name,
        *extra_args,
    ]
    with open(cli_command_log, "a", encoding="utf-8") as handle:
        handle.write("$ " + " ".join(shlex.quote(part) for part in command) + "\n")
        handle.flush()
        result = subprocess.run(command, stdout=handle, stderr=subprocess.STDOUT, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"fprime-cli command failed: {' '.join(command)}")


def start_cli_listener(handle, subcommand: str, *extra_args):
    return start_process(
        [
            fprime_cli_bin,
            subcommand,
            "--dictionary",
            dict_path,
            "--no-zmq",
            "--tts-port",
            gds_tts_port,
            *extra_args,
        ],
        handle=handle,
    )


def received_downlink_path(dest_name: str) -> pathlib.Path:
    return pathlib.Path(gds_file_storage_dir) / "fprime-downlink" / dest_name


def received_downlink_dir() -> pathlib.Path:
    return pathlib.Path(gds_file_storage_dir) / "fprime-downlink"


def source_snapshot_path(dest_name: str) -> pathlib.Path:
    return snapshot_dir / dest_name


def fetch_remote_file(remote_path: str, snapshot_name: str) -> pathlib.Path:
    snapshot_path = source_snapshot_path(snapshot_name)
    result = subprocess.run(
        ssh_command(obc_ssh_target, f"set -euo pipefail; cat {shq(remote_path)}"),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        stderr = result.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(f"Failed to fetch remote source file {remote_path}: {stderr}")
    snapshot_path.write_bytes(result.stdout)
    return snapshot_path


def list_remote_data_product_files():
    script = (
        f"set -euo pipefail; "
        f"if [[ -d {shq(runtime_root + '/data-products')} ]]; then "
        f"find {shq(runtime_root + '/data-products')} -maxdepth 1 -type f -name 'Dp_*.fdp' | sort; "
        f"fi"
    )
    text = ssh_capture(obc_ssh_target, script)
    return [line.strip() for line in text.splitlines() if line.strip()]


def select_remote_data_product_files():
    files = list_remote_data_product_files()
    attempts = 0
    while len(files) < 1 and attempts < capture_max_attempts:
        attempts += 1
        time.sleep(capture_delay_sec)
        files = list_remote_data_product_files()
    if len(files) < 1:
        raise RuntimeError(
            f"Only {len(files)} official remote .fdp files appeared after {attempts} waits; "
            "needed at least one file for file downlink"
        )
    return files[:target_occupied_slots], attempts


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def wait_for_matching_file(source_path: pathlib.Path, received_path: pathlib.Path, timeout_sec: int):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if source_path.is_file() and received_path.is_file() and filecmp.cmp(source_path, received_path, shallow=False):
            return sha256_file(received_path), received_path.stat().st_size
        time.sleep(0.5)
    raise RuntimeError(f"Timed out waiting for byte-matching downlink: source={source_path} received={received_path}")


def remove_received_file(received_path: pathlib.Path):
    try:
        received_path.unlink()
    except FileNotFoundError:
        pass


def remove_received_fdp_files():
    for path in received_downlink_dir().glob("*.fdp"):
        remove_received_file(path)


def snapshot_remote_source_files(remote_source_paths, label_prefix: str):
    snapshots = []
    for index, remote_source_path in enumerate(remote_source_paths, start=1):
        source_name = pathlib.Path(remote_source_path).name
        snapshot_name = f"{label_prefix}-{index}-{source_name}"
        snapshots.append(
            {
                "source_path": remote_source_path,
                "snapshot_path": fetch_remote_file(remote_source_path, snapshot_name),
            }
        )
    return snapshots


def wait_for_any_matching_file(expected_snapshots, timeout_sec: int):
    deadline = time.time() + timeout_sec
    expected_map = {
        entry["snapshot_path"]: entry["snapshot_path"].read_bytes() for entry in expected_snapshots
    }
    while time.time() < deadline:
        for received_path in sorted(received_downlink_dir().glob("*.fdp")):
            if not received_path.is_file():
                continue
            payload = received_path.read_bytes()
            for entry in expected_snapshots:
                if payload == expected_map[entry["snapshot_path"]]:
                    return entry, received_path, sha256_file(received_path), received_path.stat().st_size
        time.sleep(0.5)
    expected_names = ", ".join(pathlib.Path(entry["source_path"]).name for entry in expected_snapshots)
    raise RuntimeError(f"Timed out waiting for any byte-matching remote .fdp downlink among [{expected_names}]")


def run_start_xmit_catalog():
    errors = []
    for argument in ("1", "NO_WAIT", "Fw.Wait.NO_WAIT"):
        try:
            run_cli_command("OBCApp.dpCatalog.START_XMIT_CATALOG", "--arguments", argument)
            return
        except RuntimeError as exc:
            errors.append(str(exc))
    raise RuntimeError("START_XMIT_CATALOG failed with all enum argument forms: " + " | ".join(errors))


def downlink_data_product(remote_source_paths, snapshot_label: str):
    remove_received_fdp_files()
    last_error = None
    for attempt in range(1, file_command_attempts + 1):
        expected_snapshots = snapshot_remote_source_files(remote_source_paths, f"{snapshot_label}-attempt-{attempt}")
        try:
            run_cli_command("OBCApp.dpCatalog.BUILD_CATALOG")
            wait_for_log(
                obc_log,
                lambda text: "CatalogBuildComplete" in text,
                file_start_timeout_sec,
                "CatalogBuildComplete after DpCatalog.BUILD_CATALOG",
            )
            run_start_xmit_catalog()
            wait_for_log(
                obc_log,
                lambda text: "SendingProduct" in text,
                file_start_timeout_sec,
                "SendingProduct after DpCatalog.START_XMIT_CATALOG",
            )
            return wait_for_any_matching_file(expected_snapshots, file_timeout_sec)
        except RuntimeError as exc:
            last_error = exc
            if attempt < file_command_attempts:
                remove_received_fdp_files()
                time.sleep(2.0)
    assert last_error is not None
    raise last_error


def send_eps_set_pdu(enabled: bool):
    run_cli_command("OBCApp.epsBridge.EPS_SET_PDU", "--arguments", "2", "true" if enabled else "false")


def send_adcs_set_mode(mode: str):
    run_cli_command("OBCApp.adcsBridge.ADCS_SET_MODE", "--arguments", mode)


def drive_eps_pdu(enabled: bool, expected_pdu: int) -> bool:
    for _ in range(command_attempts):
        attempt_log_start = log_position(obc_log)
        send_eps_set_pdu(enabled)
        time.sleep(1.0)
        send_obc_command(obc, "eps get", 0.5)
        obc_text = log_since(obc_log, attempt_log_start)
        if re.search(rf"eps soc=.* pdu={expected_pdu}", obc_text):
            return True
        time.sleep(1.0)
    return False


def drive_adcs_mode(mode: str) -> bool:
    for _ in range(command_attempts):
        attempt_log_start = log_position(obc_log)
        send_adcs_set_mode(mode)
        time.sleep(1.0)
        send_obc_command(obc, "adcs get", 0.5)
        obc_text = log_since(obc_log, attempt_log_start)
        if f"adcs mode={mode}" in obc_text:
            return True
        time.sleep(1.0)
    return False


def require_event_channel_observations(event_start: int, channel_start: int, expected_commands: int, description: str):
    try:
        wait_for_log(
            cli_channels_log,
            lambda text: "GROUND_LINK_TX_BYTES" in text[channel_start:],
            20.0,
            f"GROUND_LINK_TX_BYTES telemetry through fprime-cli channels for {description}",
        )
    except RuntimeError:
        pass

    events_delta = log_since(cli_events_log, event_start)
    channels_delta = log_since(cli_channels_log, channel_start)
    dispatched = events_delta.count("OpCodeDispatched")
    completed = events_delta.count("OpCodeCompleted")
    command_event_count = dispatched + completed
    if command_event_count < expected_commands or "GROUND_LINK_TX_BYTES" not in channels_delta:
        raise RuntimeError(
            f"TT&C downlink did not produce required fprime-cli event/channel observations for {description}; "
            f"dispatched={dispatched} completed={completed}"
        )


def run_ttc_cycle(cycle_index: int):
    event_start = log_position(cli_events_log)
    channel_start = log_position(cli_channels_log)

    send_obc_command(obc, "status", 1.0)
    wait_for_csp_ping(obc, comm_csp_node, 3.0, f"COMM CSP ping success cycle {cycle_index}")
    wait_for_csp_ping(obc, eps_csp_node_id, 3.0, f"EPS CSP ping success cycle {cycle_index}")
    wait_for_csp_ping(obc, adcs_csp_node_id, 3.0, f"ADCS CSP ping success cycle {cycle_index}")

    reset_eps = drive_eps_pdu(False, 3)
    reset_adcs = drive_adcs_mode("DETUMBLE")
    if not reset_eps or not reset_adcs:
        raise RuntimeError(f"TT&C reset commands did not produce required OBC readback in cycle {cycle_index}")

    target_eps = drive_eps_pdu(True, 7)
    target_adcs = drive_adcs_mode("POINTING")
    if not target_eps or not target_adcs:
        raise RuntimeError(f"TT&C commands did not produce required OBC readback in cycle {cycle_index}")

    send_obc_command(obc, "status", 1.0)
    require_event_channel_observations(event_start, channel_start, 2, f"cycle {cycle_index}")
    return {
        "cycle": cycle_index,
        "comm_ping": True,
        "eps_ping": True,
        "adcs_ping": True,
        "reset_readback": True,
        "target_readback": True,
        "event_channel": True,
    }


processes = []
open_handles = []
stage1_pass = False
stage2_pass = False
comm_ping_pass = False
eps_ping_pass = False
adcs_ping_pass = False
reset_readback_pass = False
target_eps_readback_pass = False
target_adcs_readback_pass = False
candump_exit_codes = {}
selected_slots = []
matched_source_path = None
matched_source_snapshot = None
matched_received_path = None
matched_received_hash = ""
matched_received_size = 0
capture_attempts_used = 0
ttc_cycles_required = diagnostic_ttc_cycles if probe_mode == "ttc-prereq" else 1
ttc_cycles_passed = 0
ttc_cycle_results = []

try:
    if prepare_can_interfaces == "1":
        bring_up_can(subsystem_ssh_target, subsystem_remote_dir, subsystem_eps_adcs_can_device)
        bring_up_can(subsystem_ssh_target, subsystem_remote_dir, subsystem_comm_can_device)
        bring_up_can(obc_ssh_target, rpi_remote_dir, obc_can_device)
    else:
        append_file(bringup_log, "PREPARE_CAN_INTERFACES=0; using existing CAN link state.\n")

    if prepare_remote_runtime_flag == "1":
        prepare_remote_runtime()
    else:
        append_file(bringup_log, "PREPARE_REMOTE_RUNTIME=0; preserving existing OBC runtime tree.\n")
    capture_stats(subsystem_ssh_target, [subsystem_eps_adcs_can_device, subsystem_comm_can_device], subsystem_pre_stats_log)
    capture_stats(obc_ssh_target, [obc_can_device], obc_pre_stats_log)
    with open(subsystem_parentdev_log, "w", encoding="utf-8") as handle:
        ssh_cmd(
            subsystem_ssh_target,
            (
                f"set -euo pipefail; cd {shq(subsystem_remote_dir)}; source scripts/_common.sh; "
                f"printf 'eps_adcs=%s parentdev=%s\\n' {shq(subsystem_eps_adcs_can_device)} \"$(obc_can_parentdev {shq(subsystem_eps_adcs_can_device)})\"; "
                f"printf 'comm=%s parentdev=%s\\n' {shq(subsystem_comm_can_device)} \"$(obc_can_parentdev {shq(subsystem_comm_can_device)})\""
            ),
            stdout=handle,
            stderr=subprocess.STDOUT,
        )

    with open(ground_gds_log, "w", encoding="utf-8", buffering=1) as gds_handle, \
        open(subsystem_stack_log, "w", encoding="utf-8", buffering=1) as subsystem_handle, \
        open(gateway_log, "w", encoding="utf-8", buffering=1) as gateway_handle, \
        open(obc_log, "w", encoding="utf-8", buffering=1) as obc_handle, \
        open(cli_events_log, "w", encoding="utf-8", buffering=1) as events_handle, \
        open(cli_channels_log, "w", encoding="utf-8", buffering=1) as channels_handle:

        ground_env = os.environ.copy()
        ground_env["GDS_PORT"] = gds_port
        ground_env["GDS_TTS_PORT"] = gds_tts_port
        ground_env["GDS_FILE_STORAGE_DIR"] = gds_file_storage_dir
        ground_gds = start_process(["bash", os.path.join(root_dir, "scripts/run_ground_gds_only_stack.sh")], env=ground_env, handle=gds_handle)
        processes.append(("ground_gds", ground_gds))

        subsystem_stack = start_remote_subsystem_stack(subsystem_handle)
        processes.append(("subsystem_can_comm_stack", subsystem_stack))

        wait_for_log(
            ground_gds_log,
            lambda text: f"Server connected to 0.0.0.0:{gds_port}" in text
            and f"Client connected to 0.0.0.0:{gds_tts_port}" in text,
            20.0,
            "GDS TCP and TTS listeners to start",
        )
        wait_for_log(
            subsystem_stack_log,
            lambda text: "Starting subsystem EPS/ADCS + COMM SocketCAN stack" in text,
            10.0,
            "subsystem stack to start",
        )
        for name, process in processes:
            require_running(process, name)

        obc_can_handle, obc_can_process = start_candump(obc_ssh_target, obc_can_device, obc_candump_log)
        subsystem_eps_adcs_handle, subsystem_eps_adcs_process = start_candump(
            subsystem_ssh_target, subsystem_eps_adcs_can_device, subsystem_eps_adcs_candump_log
        )
        subsystem_comm_handle, subsystem_comm_process = start_candump(
            subsystem_ssh_target, subsystem_comm_can_device, subsystem_comm_candump_log
        )
        open_handles.extend([obc_can_handle, subsystem_eps_adcs_handle, subsystem_comm_handle])
        processes.extend(
            [
                ("obc_candump", obc_can_process),
                ("subsystem_eps_adcs_candump", subsystem_eps_adcs_process),
                ("subsystem_comm_candump", subsystem_comm_process),
            ]
        )

        gateway = start_process(
            [
                os.path.join(bin_dir, "ground_ttc_gateway"),
                "--serial-device",
                host_serial_device,
                "--baudrate",
                comm_baudrate,
                "--gds-host",
                "127.0.0.1",
                "--gds-port",
                gds_port,
                "--serial-tx-preamble-lines",
                preamble_lines,
                "--serial-tx-preamble-delay-ms",
                preamble_delay_ms,
            ],
            handle=gateway_handle,
        )
        processes.append(("ground_ttc_gateway", gateway))

        obc_env = os.environ.copy()
        obc_env["OBC_SSH_TARGET"] = obc_ssh_target
        obc_env["RPI_REMOTE_DIR"] = rpi_remote_dir
        obc_env["CSP_TRANSPORT"] = csp_transport
        obc_env["OBC_CSP_CAN_DEVICE"] = obc_can_device
        obc_env["CSP_CAN_PROMISC"] = csp_can_promisc
        obc_env["GDS_PORT"] = "0"
        obc_env["GROUND_LINK_MODE"] = "comm-csp"
        obc_env["COMM_CSP_NODE"] = comm_csp_node
        obc_env["MANAGE_AUTOSTART"] = "1"
        obc_env["KILL_EXISTING_PIDS"] = "1"
        obc_env["RUNTIME_ROOT"] = runtime_root
        obc = start_process(
            ["bash", os.path.join(root_dir, "scripts/run_rpi_can_csp_stack.sh")],
            env=obc_env,
            handle=obc_handle,
            stdin=subprocess.PIPE,
        )
        processes.append(("rpi_can_csp_stack", obc))

        wait_for_log(
            obc_log,
            lambda text: "OBC runtime started" in text and f"Ground link via COMM CSP node: {comm_csp_node}" in text,
            max(startup_delay_sec + 20.0, 40.0),
            "OBC runtime and COMM ground link mode to start",
        )
        time.sleep(startup_delay_sec)
        for name, process in processes:
            if "candump" not in name:
                require_running(process, name)

        events_listener = start_cli_listener(events_handle, "events", "--search", "OpCode")
        channels_listener = start_cli_listener(channels_handle, "channels", "--search", "GROUND_LINK_TX_BYTES")
        processes.append(("cli_events_listener", events_listener))
        processes.append(("cli_channels_listener", channels_listener))
        time.sleep(1.0)

        wait_for_ground_link_connected("TT&C prerequisite")
        for cycle_index in range(1, ttc_cycles_required + 1):
            ttc_cycle_results.append(run_ttc_cycle(cycle_index))
            ttc_cycles_passed += 1

        comm_ping_pass = True
        eps_ping_pass = True
        adcs_ping_pass = True
        reset_readback_pass = True
        target_eps_readback_pass = True
        target_adcs_readback_pass = True
        stage1_pass = True
        stage2_pass = True

        if probe_mode == "file-downlink":
            run_cli_command("OBCApp.hkTrendProductProducer.HK_TREND_FLUSH")
            wait_for_log(
                obc_log,
                lambda text: "HK_TREND_PRODUCT_WRITTEN" in text,
                max(file_start_timeout_sec, 8.0),
                "HK_TREND_PRODUCT_WRITTEN after HK_TREND_FLUSH",
            )
            selected_slots, capture_attempts_used = select_remote_data_product_files()
            matched_entry, matched_received_path, matched_received_hash, matched_received_size = downlink_data_product(
                selected_slots,
                "source-dp",
            )
            matched_source_path = matched_entry["source_path"]
            matched_source_snapshot = str(matched_entry["snapshot_path"])

        for process_name in ("cli_events_listener", "cli_channels_listener"):
            for name, process in processes:
                if name == process_name and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5.0)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5.0)

    for name, process in list(processes):
        if "candump" in name and process.poll() is None:
            try:
                process.wait(timeout=can_capture_timeout_sec + 5)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=5.0)
        if "candump" in name:
            candump_exit_codes[name] = process.returncode
    for handle in open_handles:
        handle.close()

    capture_stats(subsystem_ssh_target, [subsystem_eps_adcs_can_device, subsystem_comm_can_device], subsystem_post_stats_log)
    capture_stats(obc_ssh_target, [obc_can_device], obc_post_stats_log)
finally:
    if not os.path.exists(subsystem_post_stats_log):
        try:
            capture_stats(subsystem_ssh_target, [subsystem_eps_adcs_can_device, subsystem_comm_can_device], subsystem_post_stats_log)
        except Exception as exc:
            append_file(subsystem_post_stats_log, f"failed to capture subsystem post stats: {exc}\n")
    if not os.path.exists(obc_post_stats_log):
        try:
            capture_stats(obc_ssh_target, [obc_can_device], obc_post_stats_log)
        except Exception as exc:
            append_file(obc_post_stats_log, f"failed to capture OBC post stats: {exc}\n")
    for _, process in reversed(processes):
        if process.poll() is None:
            process.terminate()
    for _, process in reversed(processes):
        if process.poll() is None:
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5.0)
    for handle in open_handles:
        try:
            handle.close()
        except Exception:
            pass
    ssh_cmd(
        obc_ssh_target,
        "PIDS=$(pgrep -f 'build-fprime-automatic-native/bin/Linux/OBC' || true); "
        "if [[ -n \"${PIDS}\" ]]; then kill ${PIDS} >/dev/null 2>&1 || true; fi",
        check=False,
    )
    ssh_cmd(
        subsystem_ssh_target,
        "PIDS=$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(eps_simulator|adcs_simulator|comm_csp_node)' || true); "
        "if [[ -n \"${PIDS}\" ]]; then kill ${PIDS} >/dev/null 2>&1 || true; fi",
        check=False,
    )

obc_text = sanitize_file(obc_log)
events_text = sanitize_file(cli_events_log)
channels_text = sanitize_file(cli_channels_log)
subsystem_stats = sanitize_file(subsystem_post_stats_log)
obc_stats = sanitize_file(obc_post_stats_log)

if not comm_ping_pass:
    raise RuntimeError("COMM node ping did not pass")
if not eps_ping_pass or not adcs_ping_pass:
    raise RuntimeError("EPS/ADCS ping did not pass")
if not reset_readback_pass:
    raise RuntimeError("TT&C reset commands did not produce required OBC readback")
if not stage1_pass:
    raise RuntimeError("TT&C commands did not produce required OBC readback")
if not stage2_pass:
    raise RuntimeError("TT&C downlink did not produce required fprime-cli event/channel observations")
if ttc_cycles_passed != ttc_cycles_required:
    raise RuntimeError(f"TT&C prerequisite passed {ttc_cycles_passed} cycles, required {ttc_cycles_required}")
if probe_mode == "file-downlink" and not matched_received_hash:
    raise RuntimeError("File downlink did not produce the required official .fdp match")
if "groundLink mode=comm-csp" not in obc_text or f"commNode={comm_csp_node}" not in obc_text:
    raise RuntimeError("OBC status did not report COMM-backed ground-link mode")
if "groundLink connected=yes" not in obc_text:
    raise RuntimeError("OBC status did not report connected COMM ground link")
for path, process_name, device, description in (
    (obc_candump_log, "obc_candump", obc_can_device, "OBC CAN capture"),
    (
        subsystem_eps_adcs_candump_log,
        "subsystem_eps_adcs_candump",
        subsystem_eps_adcs_can_device,
        "subsystem EPS/ADCS CAN capture",
    ),
    (subsystem_comm_candump_log, "subsystem_comm_candump", subsystem_comm_can_device, "subsystem COMM CAN capture"),
):
    exit_code = candump_exit_codes.get(process_name)
    if exit_code is None:
        raise RuntimeError(f"{description} process did not complete")
    if exit_code != 0:
        raise RuntimeError(f"{description} process failed with exit code {exit_code}")
    if not os.path.exists(path) or os.path.getsize(path) == 0:
        raise RuntimeError(f"{description} is empty")
    if not has_candump_frame(path, device):
        raise RuntimeError(f"{description} does not contain candump frame lines")
require_can_stats_healthy(obc_stats, obc_can_device, "OBC CAN stats")
require_can_stats_healthy(subsystem_stats, subsystem_eps_adcs_can_device, "subsystem EPS/ADCS CAN stats")
require_can_stats_healthy(subsystem_stats, subsystem_comm_can_device, "subsystem COMM CAN stats")

print("comm-csp-socketcan-file-downlink-probe: PASS")
print(f"probe-mode={probe_mode}")
if probe_mode == "file-downlink":
    print("formal-verdict=file-downlink")
else:
    print("diagnostic-verdict=ttc-prereq")
print("link-mode=physical-socketcan")
print(f"log-dir={probe_tmp_dir}")
print(f"host-serial-device={host_serial_device}")
print(f"subsystem-serial-device={subsystem_serial_device}")
print(f"baudrate={comm_baudrate}")
print(f"obc-can-device={obc_can_device}")
print(f"subsystem-eps-adcs-can-device={subsystem_eps_adcs_can_device}")
print(f"subsystem-comm-can-device={subsystem_comm_can_device}")
print(f"can-bitrate={can_bitrate}")
print(f"can-dbitrate={can_dbitrate}")
print(f"comm-node={comm_csp_node}")
print(f"gds-port={gds_port}")
print(f"gds-tts-port={gds_tts_port}")
print(f"gds-file-storage-dir={gds_file_storage_dir}")
print(f"runtime-root={runtime_root}")
print(f"serial-tx-preamble-lines={preamble_lines}")
print(f"serial-tx-preamble-delay-ms={preamble_delay_ms}")
print(f"startup-delay-sec={startup_delay_sec:g}")
print(f"bounded-ttc-command-attempts={command_attempts}")
print(f"file-downlink-command-attempts={file_command_attempts}")
print(f"file-start-timeout-sec={file_start_timeout_sec}")
print(f"file-byte-match-timeout-sec={file_timeout_sec}")
print(f"can-capture-timeout-sec={can_capture_timeout_sec}")
print(f"prepare-can-interfaces={prepare_can_interfaces}")
print(f"ssh-connect-timeout-sec={ssh_connect_timeout_sec}")
print(f"prepare-remote-runtime={prepare_remote_runtime_flag}")
print("stage0-ttc-prerequisite=PASS")
print(f"ttc-prereq-cycles-required={ttc_cycles_required}")
print(f"ttc-prereq-cycles-passed={ttc_cycles_passed}")
for result in ttc_cycle_results:
    print(f"ttc-cycle=PASS cycle={result['cycle']}")
if probe_mode == "file-downlink":
    print(f"source-product-sample-limit={target_occupied_slots}")
    print(f"source-product-wait-attempts={capture_attempts_used}")
    print(f"source-product-observed-count={len(selected_slots)}")
    print(
        f"downlinked-product=PASS source={matched_source_path} "
        f"source-snapshot={matched_source_snapshot} received={matched_received_path} "
        f"bytes={matched_received_size} sha256={matched_received_hash}"
    )
print("comm-ping=PASS")
print("eps-adcs-ping=PASS")
print("reset-command-readback=PASS")
print("stage1-command-readback=PASS")
print("stage2-event-channel-downlink=PASS")
for line in obc_text.splitlines():
    if any(fragment in line for fragment in ("groundLink mode=", "groundLink connected=", "eps soc=", "adcs mode=", "csp ping response=")):
        print(line)
PY
