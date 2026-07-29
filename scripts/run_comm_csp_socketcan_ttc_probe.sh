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
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES:-20}"
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS:-500}"
LAB_SERIAL_STARTUP_DELAY_SEC="${LAB_SERIAL_STARTUP_DELAY_SEC:-12}"
COMMAND_ATTEMPTS="${COMMAND_ATTEMPTS:-12}"
CAN_CAPTURE_TIMEOUT_SEC="${CAN_CAPTURE_TIMEOUT_SEC:-25}"
PATH_WARMUP_SEC="${PATH_WARMUP_SEC:-12}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-csp-socketcan-ttc-gds-downlink}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-csp-socketcan-ttc.XXXXXX")}"

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

mkdir -p "${PROBE_TMP_DIR}" "${GDS_FILE_STORAGE_DIR}"

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
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" \
  "${LAB_SERIAL_STARTUP_DELAY_SEC}" \
  "${COMMAND_ATTEMPTS}" \
  "${CAN_CAPTURE_TIMEOUT_SEC}" \
  "${PATH_WARMUP_SEC}" \
  "${GDS_PORT}" \
  "${GDS_TTS_PORT}" <<'PY'
import os
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
    preamble_lines,
    preamble_delay_ms,
    startup_delay_sec,
    command_attempts,
    can_capture_timeout_sec,
    path_warmup_sec,
    gds_port,
    gds_tts_port,
) = sys.argv[1:]

command_attempts = int(command_attempts)
can_capture_timeout_sec = int(can_capture_timeout_sec)
path_warmup_sec = float(path_warmup_sec)
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


def shq(value: str) -> str:
    return shlex.quote(str(value))


def ssh_cmd(target: str, script: str, *, stdout=None, stderr=None, check=True):
    command = [
        "ssh",
        "-o",
        "BatchMode=yes",
        "-o",
        "ConnectTimeout=5",
        target,
        "/bin/bash -lc " + shq(script),
    ]
    return subprocess.run(command, stdout=stdout, stderr=stderr, check=check, text=True)


def start_ssh_process(target: str, script: str, handle):
    return subprocess.Popen(
        [
            "ssh",
            "-o",
            "BatchMode=yes",
            "-o",
            "ConnectTimeout=5",
            target,
            "/bin/bash -lc " + shq(script),
        ],
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
            return wait_for_log(
                obc_log,
                lambda text: success_fragment in text[ping_log_start:],
                timeout_sec,
                description,
            )
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


def send_eps_set_pdu(enabled: bool):
    run_cli_command("OBCApp.epsBridge.EPS_SET_PDU", "--arguments", "2", "true" if enabled else "false")


def send_adcs_set_mode(mode: str):
    run_cli_command("OBCApp.adcsBridge.ADCS_SET_MODE", "--arguments", mode)


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


processes = []
open_handles = []
stage1_pass = False
stage2_pass = False
comm_ping_pass = False
eps_ping_pass = False
adcs_ping_pass = False
reset_readback_pass = False
candump_exit_codes = {}
try:
    if prepare_can_interfaces == "1":
        bring_up_can(subsystem_ssh_target, subsystem_remote_dir, subsystem_eps_adcs_can_device)
        bring_up_can(subsystem_ssh_target, subsystem_remote_dir, subsystem_comm_can_device)
        bring_up_can(obc_ssh_target, rpi_remote_dir, obc_can_device)
    else:
        append_file(bringup_log, "PREPARE_CAN_INTERFACES=0; using existing CAN link state.\n")

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
        ground_gds = start_process(
            ["bash", os.path.join(root_dir, "scripts/run_ground_gds_only_stack.sh")],
            env=ground_env,
            handle=gds_handle,
        )
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
        obc_env["RUNTIME_ROOT"] = f"{rpi_remote_dir}/runtime/comm-csp-socketcan-ttc"
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
            max(startup_delay_sec, 20.0),
            "OBC runtime and COMM ground link mode to start",
        )
        time.sleep(path_warmup_sec)
        for name, process in processes:
            if "candump" not in name:
                require_running(process, name)

        events_listener = start_cli_listener(events_handle, "events", "--search", "OpCode")
        channels_listener = start_cli_listener(channels_handle, "channels", "--search", "GROUND_LINK_TX_BYTES")
        processes.append(("cli_events_listener", events_listener))
        processes.append(("cli_channels_listener", channels_listener))
        time.sleep(1.0)

        wait_for_ground_link_connected("TT&C control prerequisite")
        wait_for_csp_ping(obc, comm_csp_node, 3.0, "COMM CSP ping success")
        comm_ping_pass = True
        wait_for_csp_ping(obc, eps_csp_node_id, 3.0, "EPS CSP ping success")
        eps_ping_pass = True
        wait_for_csp_ping(obc, adcs_csp_node_id, 3.0, "ADCS CSP ping success")
        adcs_ping_pass = True

        # Force a known opposite state first so the formal command effects are
        # not satisfied by simulator state left over from a previous lab run.
        reset_eps_readback_pass = drive_eps_pdu(False, 3)
        reset_adcs_readback_pass = drive_adcs_mode("DETUMBLE")
        reset_readback_pass = reset_eps_readback_pass and reset_adcs_readback_pass

        if not reset_readback_pass:
            raise RuntimeError("TT&C reset commands did not produce required OBC readback")

        target_eps_readback_pass = drive_eps_pdu(True, 7)
        target_adcs_readback_pass = drive_adcs_mode("POINTING")
        stage1_pass = target_eps_readback_pass and target_adcs_readback_pass

        send_obc_command(obc, "status", 1.0)
        try:
            wait_for_log(
                cli_channels_log,
                lambda text: "GROUND_LINK_TX_BYTES" in text,
                10.0,
                "GROUND_LINK_TX_BYTES telemetry through fprime-cli channels",
            )
        except RuntimeError:
            pass

        events_text = sanitize_file(cli_events_log)
        channels_text = sanitize_file(cli_channels_log)
        stage2_pass = (
            events_text.count("OpCodeDispatched") >= 2
            and events_text.count("OpCodeCompleted") >= 2
            and "GROUND_LINK_TX_BYTES" in channels_text
        )

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
            capture_stats(
                subsystem_ssh_target,
                [subsystem_eps_adcs_can_device, subsystem_comm_can_device],
                subsystem_post_stats_log,
            )
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
for stats_text, description in ((subsystem_stats, "subsystem CAN stats"), (obc_stats, "OBC CAN stats")):
    if "ERROR-ACTIVE" not in stats_text:
        raise RuntimeError(f"{description} did not report ERROR-ACTIVE")
    bus_off_counts = parse_bus_off_counts(stats_text)
    if not bus_off_counts:
        raise RuntimeError(f"{description} did not include CAN bus-off counters")
    if any(count != 0 for count in bus_off_counts):
        raise RuntimeError(f"{description} reported nonzero bus-off")

print("comm-csp-socketcan-ttc-probe: PASS")
print("formal-verdict=bounded-ttc")
print("link-mode=physical-socketcan")
print(f"log-dir={probe_tmp_dir}")
print(f"host-serial-device={host_serial_device}")
print(f"subsystem-serial-device={subsystem_serial_device}")
print(f"obc-can-device={obc_can_device}")
print(f"subsystem-eps-adcs-can-device={subsystem_eps_adcs_can_device}")
print(f"subsystem-comm-can-device={subsystem_comm_can_device}")
print(f"comm-node={comm_csp_node}")
print(f"gds-port={gds_port}")
print(f"gds-tts-port={gds_tts_port}")
print(f"serial-tx-preamble-lines={preamble_lines}")
print(f"serial-tx-preamble-delay-ms={preamble_delay_ms}")
print(f"prepare-can-interfaces={prepare_can_interfaces}")
print(f"comm-ping=PASS")
print(f"eps-adcs-ping=PASS")
print("reset-command-readback=PASS")
print("stage1-command-readback=PASS")
print("stage2-event-channel-downlink=PASS")
for line in obc_text.splitlines():
    if any(fragment in line for fragment in ("groundLink mode=", "groundLink connected=", "eps soc=", "adcs mode=", "csp ping response=")):
        print(line)
PY
