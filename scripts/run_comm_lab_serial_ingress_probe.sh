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

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
REMOTE_CSP_HUB_HOST="${REMOTE_CSP_HUB_HOST:-$(obc_default_remote_carrier_host)}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
RUN_STAGE0_PREFLIGHT="${RUN_STAGE0_PREFLIGHT:-0}"
RUN_STAGE0_ACQUISITION="${RUN_STAGE0_ACQUISITION:-0}"
RUN_STAGE2_DOWNLINK="${RUN_STAGE2_DOWNLINK:-1}"
PREPARE_SUBSYSTEM_WORKSPACE="${PREPARE_SUBSYSTEM_WORKSPACE:-0}"
RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO="${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO:-0}"
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES:-20}"
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS:-500}"
LAB_SERIAL_STARTUP_DELAY_SEC="${LAB_SERIAL_STARTUP_DELAY_SEC:-}"
LAB_SERIAL_COMMAND_ATTEMPTS="${LAB_SERIAL_COMMAND_ATTEMPTS:-6}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-lab-serial-ingress-gds-downlink}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-lab-serial-ingress-runtime}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-lab-serial-ingress.XXXXXX")}"
STAGE0_PREFLIGHT_LOG="${STAGE0_PREFLIGHT_LOG:-${PROBE_TMP_DIR}/stage0-uart-preflight.log}"
STAGE0_ACQUISITION_LOG="${STAGE0_ACQUISITION_LOG:-${PROBE_TMP_DIR}/stage0-acquisition.log}"
GROUND_GDS_LOG="${GROUND_GDS_LOG:-${PROBE_TMP_DIR}/ground-gds.log}"
GATEWAY_LOG="${GATEWAY_LOG:-${PROBE_TMP_DIR}/gateway.log}"
REMOTE_COMM_NODE_LOG="${REMOTE_COMM_NODE_LOG:-${PROBE_TMP_DIR}/remote-comm-node.log}"
OBC_LOG="${OBC_LOG:-${PROBE_TMP_DIR}/obc.log}"
CLI_COMMAND_LOG="${CLI_COMMAND_LOG:-${PROBE_TMP_DIR}/command-send.log}"
CLI_EVENTS_LOG="${CLI_EVENTS_LOG:-${PROBE_TMP_DIR}/events.log}"
CLI_CHANNELS_LOG="${CLI_CHANNELS_LOG:-${PROBE_TMP_DIR}/channels.log}"
OBC_SANITIZED_LOG="${OBC_SANITIZED_LOG:-${OBC_LOG}.sanitized}"
EVENTS_SANITIZED_LOG="${EVENTS_SANITIZED_LOG:-${CLI_EVENTS_LOG}.sanitized}"
CHANNELS_SANITIZED_LOG="${CHANNELS_SANITIZED_LOG:-${CLI_CHANNELS_LOG}.sanitized}"
REMOTE_COMM_NODE_PID_FILE="${REMOTE_COMM_NODE_PID_FILE:-/tmp/obc-comm-lab-serial-ingress-${RANDOM}-$$.pid}"
COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS:-0}"
COMM_NODE_INGRESS_TRACE_FULL="${COMM_NODE_INGRESS_TRACE_FULL:-0}"

usage() {
  cat >&2 <<EOF
Usage:
  HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_comm_lab_serial_ingress_probe.sh

Environment:
  HOST_SERIAL_DEVICE              macOS serial endpoint connected to subsystem.local
  SUBSYSTEM_SIM_COMM_DEVICE       subsystem.local serial endpoint connected to macOS
  COMM_BAUDRATE                   serial baudrate, default 115200
  COMM_CSP_NODE                   COMM CSP node id, default 4
  REMOTE_CSP_HUB_HOST             macOS address reachable from subsystem.local, default detected LAN host
  RUN_STAGE0_PREFLIGHT            run strict no-preamble macOS-initiated UART preflight before TT&C when set to 1
  RUN_STAGE0_ACQUISITION          run subsystem-origin acquisition precondition when set to 1
  RUN_STAGE2_DOWNLINK             attempt events/channels downlink after uplink when set to 1
  PREPARE_SUBSYSTEM_WORKSPACE     sync/bootstrap subsystem workspace before the probe when set to 1
  RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO run remote comm_csp_node through sudo -n when set to 1
  COMM_NODE_INGRESS_DIAGNOSTICS  enable remote comm node ingress diagnostics when set to 1
  COMM_NODE_INGRESS_TRACE_FULL    emit full ingress hex trace on the remote comm node when set to 1
  LAB_SERIAL_TX_PREAMBLE_LINES    gateway serial acquisition preamble lines before GDS traffic, default 20
  LAB_SERIAL_TX_PREAMBLE_DELAY_MS delay between gateway preamble lines, default 500
  LAB_SERIAL_STARTUP_DELAY_SEC    startup wait before commands, default derived from preamble duration
  LAB_SERIAL_COMMAND_ATTEMPTS     bounded attempts for each uplink command before Stage 1 STOP, default 6
EOF
}

require_bool() {
  local name="$1"
  local value="$2"
  case "${value}" in
    0|1) ;;
    *)
      echo "${name} must be 0 or 1." >&2
      exit 2
      ;;
  esac
}

port_is_available() {
  local port="${1:?port is required}"
  ! lsof -nP -iTCP:"${port}" >/dev/null 2>&1
}

find_free_port() {
  local candidate="${1:?starting port is required}"
  while ! port_is_available "${candidate}"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
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

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  usage
  exit 2
fi

require_bool RUN_STAGE0_PREFLIGHT "${RUN_STAGE0_PREFLIGHT}"
require_bool RUN_STAGE0_ACQUISITION "${RUN_STAGE0_ACQUISITION}"
require_bool RUN_STAGE2_DOWNLINK "${RUN_STAGE2_DOWNLINK}"
require_bool PREPARE_SUBSYSTEM_WORKSPACE "${PREPARE_SUBSYSTEM_WORKSPACE}"
require_bool RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO "${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO}"
require_bool COMM_NODE_INGRESS_DIAGNOSTICS "${COMM_NODE_INGRESS_DIAGNOSTICS}"
require_bool COMM_NODE_INGRESS_TRACE_FULL "${COMM_NODE_INGRESS_TRACE_FULL}"

if ! [[ "${LAB_SERIAL_TX_PREAMBLE_LINES}" =~ ^[0-9]+$ && "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" =~ ^[0-9]+$ && "${LAB_SERIAL_COMMAND_ATTEMPTS}" =~ ^[1-9][0-9]*$ ]]; then
  echo "LAB_SERIAL_TX_PREAMBLE_LINES/LAB_SERIAL_TX_PREAMBLE_DELAY_MS must be non-negative integers and LAB_SERIAL_COMMAND_ATTEMPTS must be a positive integer." >&2
  exit 2
fi

if [[ -z "${LAB_SERIAL_STARTUP_DELAY_SEC}" ]]; then
  LAB_SERIAL_STARTUP_DELAY_SEC="$(( (LAB_SERIAL_TX_PREAMBLE_LINES * LAB_SERIAL_TX_PREAMBLE_DELAY_MS + 999) / 1000 + 2 ))"
  if [[ "${LAB_SERIAL_STARTUP_DELAY_SEC}" -lt 5 ]]; then
    LAB_SERIAL_STARTUP_DELAY_SEC=5
  fi
fi

if [[ ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial device not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi

if [[ -z "${GDS_PORT:-}" ]]; then
  GDS_PORT="$(find_free_port_pair 50360 1)"
fi
if [[ -z "${GDS_TTS_PORT:-}" ]]; then
  GDS_TTS_PORT="$((GDS_PORT + 1))"
fi
if [[ -z "${CSP_HUB_SUB_PORT:-}" ]]; then
  CSP_HUB_SUB_PORT="$(find_free_port_pair 56660 1000)"
fi
if [[ -z "${CSP_HUB_PUB_PORT:-}" ]]; then
  CSP_HUB_PUB_PORT="$((CSP_HUB_SUB_PORT + 1000))"
fi
if [[ -z "${RADIO_PORT:-}" ]]; then
  RADIO_PORT="$(find_free_port 17260)"
fi

mkdir -p "${PROBE_TMP_DIR}" "${GDS_FILE_STORAGE_DIR}"
rm -rf "${RUNTIME_ROOT}"
mkdir -p "${RUNTIME_ROOT}"

if [[ "${PREPARE_SUBSYSTEM_WORKSPACE}" == "1" ]]; then
  bash "${ROOT_DIR}/scripts/sync_subsystem_sim_workspace.sh"
  bash "${ROOT_DIR}/scripts/bootstrap_subsystem_sim_workspace.sh"
fi

if [[ "${RUN_STAGE0_PREFLIGHT}" == "1" ]]; then
  HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
  SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE}" \
  COMM_BAUDRATE="${COMM_BAUDRATE}" \
  PREPARE_SUBSYSTEM_WORKSPACE=0 \
  PROBE_TMP_DIR="${PROBE_TMP_DIR}/stage0-uart-preflight" \
  bash "${ROOT_DIR}/scripts/run_subsystem_comm_uart_link_probe.sh" >"${STAGE0_PREFLIGHT_LOG}" 2>&1
fi

if [[ "${RUN_STAGE0_ACQUISITION}" == "1" ]]; then
  HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
  SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE}" \
  COMM_BAUDRATE="${COMM_BAUDRATE}" \
  PREPARE_SUBSYSTEM_WORKSPACE=0 \
  PROBE_TMP_DIR="${PROBE_TMP_DIR}/stage0-acquisition" \
  bash "${ROOT_DIR}/scripts/run_comm_lab_serial_acquisition_probe.sh" >"${STAGE0_ACQUISITION_LOG}" 2>&1
fi

python3 - \
  "${ROOT_DIR}" \
  "${BIN_DIR}" \
  "${DICT_PATH}" \
  "${FPRIME_CLI_BIN}" \
  "${SUBSYSTEM_SIM_SSH_TARGET}" \
  "${SUBSYSTEM_SIM_REMOTE_DIR}" \
  "${HOST_SERIAL_DEVICE}" \
  "${SUBSYSTEM_SIM_COMM_DEVICE}" \
  "${COMM_BAUDRATE}" \
  "${COMM_CSP_NODE}" \
  "${EPS_CSP_NODE_ID}" \
  "${ADCS_CSP_NODE_ID}" \
  "${CSP_HUB_HOST}" \
  "${REMOTE_CSP_HUB_HOST}" \
  "${CSP_HUB_SUB_PORT}" \
  "${CSP_HUB_PUB_PORT}" \
  "${GDS_PORT}" \
  "${GDS_TTS_PORT}" \
  "${RADIO_PORT}" \
  "${RUN_STAGE0_PREFLIGHT}" \
  "${RUN_STAGE0_ACQUISITION}" \
  "${RUN_STAGE2_DOWNLINK}" \
  "${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO}" \
  "${COMM_NODE_INGRESS_DIAGNOSTICS}" \
  "${COMM_NODE_INGRESS_TRACE_FULL}" \
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" \
  "${LAB_SERIAL_STARTUP_DELAY_SEC}" \
  "${LAB_SERIAL_COMMAND_ATTEMPTS}" \
  "${REMOTE_COMM_NODE_PID_FILE}" \
  "${RUNTIME_ROOT}" \
  "${GDS_FILE_STORAGE_DIR}" \
  "${PROBE_TMP_DIR}" \
  "${GROUND_GDS_LOG}" \
  "${GATEWAY_LOG}" \
  "${REMOTE_COMM_NODE_LOG}" \
  "${OBC_LOG}" \
  "${CLI_COMMAND_LOG}" \
  "${CLI_EVENTS_LOG}" \
  "${CLI_CHANNELS_LOG}" <<'PY'
import os
import re
import shlex
import subprocess
import sys
import time

MAX_SANITIZED_LOG_BYTES = 2 * 1024 * 1024

(
    root_dir,
    bin_dir,
    dict_path,
    fprime_cli_bin,
    subsystem_ssh_target,
    subsystem_remote_dir,
    host_serial_device,
    subsystem_serial_device,
    baudrate,
    comm_csp_node,
    eps_csp_node_id,
    adcs_csp_node_id,
    local_csp_hub_host,
    remote_csp_hub_host,
    csp_hub_sub_port,
    csp_hub_pub_port,
    gds_port,
    gds_tts_port,
    radio_port,
    run_stage0_preflight,
    run_stage0_acquisition,
    run_stage2_downlink,
    run_subsystem_comm_node_with_sudo,
    comm_node_ingress_diagnostics,
    comm_node_ingress_trace_full,
    lab_serial_tx_preamble_lines,
    lab_serial_tx_preamble_delay_ms,
    lab_serial_startup_delay_sec,
    lab_serial_command_attempts,
    remote_comm_node_pid_file,
    runtime_root,
    gds_file_storage_dir,
    probe_tmp_dir,
    ground_gds_log,
    gateway_log,
    remote_comm_node_log,
    obc_log,
    cli_command_log,
    cli_events_log,
    cli_channels_log,
) = sys.argv[1:41]

run_stage2 = run_stage2_downlink == "1"
command_attempts = int(lab_serial_command_attempts)
sanitized_log_cache = {}


def shq(value):
    return shlex.quote(str(value))


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


def sanitize_file(path):
    try:
        stat_result = os.stat(path)
    except FileNotFoundError:
        return ""
    cache_key = (stat_result.st_mtime_ns, stat_result.st_size)
    cached = sanitized_log_cache.get(path)
    if cached and cached[0] == cache_key:
        return cached[1]

    with open(path, "rb") as handle:
        if stat_result.st_size > MAX_SANITIZED_LOG_BYTES:
            handle.seek(stat_result.st_size - MAX_SANITIZED_LOG_BYTES)
        text = handle.read().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    sanitized_log_cache[path] = (cache_key, text)
    return text


def wait_for_log_fragment(path, fragment, timeout_sec):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in sanitize_file(path):
            return True
        time.sleep(0.2)
    return False


def wait_for_log_regex(path, pattern, timeout_sec):
    compiled = re.compile(pattern)
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if compiled.search(sanitize_file(path)):
            return True
        time.sleep(0.5)
    return False


def require_running(process, name):
    if process.poll() is not None:
        raise RuntimeError(f"{name} exited early with code {process.returncode}")


def run_cli_command(command_name, *extra_args):
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
        result = subprocess.run(command, check=False, stdout=handle, stderr=subprocess.STDOUT, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"fprime-cli command failed: {' '.join(command)}")


def send_eps_command():
    run_cli_command("OBCApp.epsBridge.EPS_SET_PDU", "--arguments", "2", "true")
    time.sleep(1.0)


def send_adcs_command():
    run_cli_command("OBCApp.adcsBridge.ADCS_SET_MODE", "--arguments", "POINTING")
    time.sleep(1.0)


def start_cli_listener(handle, subcommand, *extra_args):
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


def send_obc_command(obc_process, command_text, delay_sec=0.5):
    assert obc_process.stdin is not None
    obc_process.stdin.write(command_text + "\n")
    obc_process.stdin.flush()
    time.sleep(delay_sec)


def start_remote_comm_node(handle):
    runner_prefix = "sudo -n env " if run_subsystem_comm_node_with_sudo == "1" else "env "
    remote_script = f"""
set -euo pipefail
cd {shq(subsystem_remote_dir)}
source scripts/_common.sh
BIN_DIR="$(obc_find_native_bin_dir "$PWD" || true)"
if [[ -z "${{BIN_DIR}}" ]]; then
  echo "Build output not found on subsystem host. Run scripts/bootstrap_subsystem_sim_workspace.sh first." >&2
  exit 1
fi
if [[ ! -e {shq(subsystem_serial_device)} ]]; then
  echo "Subsystem serial device not found: {subsystem_serial_device}" >&2
  exit 1
fi
SERIAL_REALPATH="$(readlink -f {shq(subsystem_serial_device)} || true)"
if [[ ! -r {shq(subsystem_serial_device)} || ! -w {shq(subsystem_serial_device)} ]]; then
  if [[ {shq(run_subsystem_comm_node_with_sudo)} != "1" ]]; then
    echo "Subsystem serial device is not readable/writable by $(id -un); set RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO=1 or fix device permissions." >&2
    ls -l {shq(subsystem_serial_device)} "${{SERIAL_REALPATH:-{subsystem_serial_device}}}" 2>&1 || true
    exit 1
  fi
fi
cleanup() {{
  rm -f {shq(remote_comm_node_pid_file)} >/dev/null 2>&1 || true
}}
trap cleanup EXIT INT TERM HUP
{runner_prefix} \\
  CSP_TRANSPORT=zmqhub \\
  CSP_HUB_HOST={shq(remote_csp_hub_host)} \\
  CSP_HUB_SUB_PORT={shq(csp_hub_sub_port)} \\
  CSP_HUB_PUB_PORT={shq(csp_hub_pub_port)} \\
  COMM_NODE_INGRESS_DIAGNOSTICS={shq(comm_node_ingress_diagnostics)} \\
  COMM_NODE_INGRESS_TRACE_FULL={shq(comm_node_ingress_trace_full)} \\
  "${{BIN_DIR}}/comm_csp_node" \\
  --serial-device {shq(subsystem_serial_device)} \\
  --baudrate {shq(baudrate)} \\
  --node-id {shq(comm_csp_node)} &
REMOTE_COMM_PID=$!
echo "${{REMOTE_COMM_PID}}" >{shq(remote_comm_node_pid_file)}
wait "${{REMOTE_COMM_PID}}"
"""
    return start_process(
        [
            "ssh",
            "-o",
            "BatchMode=yes",
            "-o",
            "ConnectTimeout=5",
            subsystem_ssh_target,
            "/bin/bash -lc " + shq(remote_script),
        ],
        handle=handle,
    )


def cleanup_remote_comm_node():
    remote_script = f"""
set -euo pipefail
pid_file={shq(remote_comm_node_pid_file)}
serial_device={shq(subsystem_serial_device)}
if [[ -f "${{pid_file}}" ]]; then
  pid="$(cat "${{pid_file}}")"
  kill "${{pid}}" 2>/dev/null || true
  rm -f "${{pid_file}}"
fi
pkill -f "[c]omm_csp_node .*--serial-device ${{serial_device}}" 2>/dev/null || true
"""
    subprocess.run(
        [
            "ssh",
            "-o",
            "BatchMode=yes",
            "-o",
            "ConnectTimeout=5",
            subsystem_ssh_target,
            "/bin/bash -lc " + shq(remote_script),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
        text=True,
    )


processes = []
stage1_pass = False
stage2_pass = False
eps_readback_pass = False
adcs_readback_pass = False
try:
    with open(ground_gds_log, "w", encoding="utf-8", buffering=1) as gds_handle, \
        open(gateway_log, "w", encoding="utf-8", buffering=1) as gateway_handle, \
        open(remote_comm_node_log, "w", encoding="utf-8", buffering=1) as remote_comm_handle, \
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

        obc_env = os.environ.copy()
        obc_env["RADIO_PORT"] = radio_port
        obc_env["GDS_PORT"] = "0"
        obc_env["RUNTIME_ROOT"] = runtime_root
        obc_env["GROUND_LINK_MODE"] = "comm-csp"
        obc_env["COMM_CSP_NODE"] = comm_csp_node
        obc_env["CSP_HUB_HOST"] = local_csp_hub_host
        obc_env["CSP_HUB_SUB_PORT"] = csp_hub_sub_port
        obc_env["CSP_HUB_PUB_PORT"] = csp_hub_pub_port
        obc_env["EPS_CSP_NODE_ID"] = eps_csp_node_id
        obc_env["ADCS_CSP_NODE_ID"] = adcs_csp_node_id
        obc_env["CSP_PROXY_BIND_HOST"] = "0.0.0.0"
        obc_env["CSP_MANAGE_PROXY"] = "1"
        obc = start_process(
            ["bash", os.path.join(root_dir, "scripts/run_dev_stack.sh")],
            env=obc_env,
            handle=obc_handle,
            stdin=subprocess.PIPE,
        )
        processes.append(("run_dev_stack", obc))

        time.sleep(2.0)
        remote_comm = start_remote_comm_node(remote_comm_handle)
        processes.append(("remote_comm_csp_node", remote_comm))

        gateway = start_process(
            [
                os.path.join(bin_dir, "ground_ttc_gateway"),
                "--serial-device",
                host_serial_device,
                "--baudrate",
                baudrate,
                "--gds-host",
                "127.0.0.1",
                "--gds-port",
                gds_port,
                "--serial-tx-preamble-lines",
                lab_serial_tx_preamble_lines,
                "--serial-tx-preamble-delay-ms",
                lab_serial_tx_preamble_delay_ms,
            ],
            handle=gateway_handle,
        )
        processes.append(("ground_ttc_gateway", gateway))

        time.sleep(float(lab_serial_startup_delay_sec))
        for name, process in processes:
            require_running(process, name)

        events_listener = None
        channels_listener = None
        if run_stage2:
            events_listener = start_cli_listener(events_handle, "events", "--search", "OpCode")
            channels_listener = start_cli_listener(channels_handle, "channels", "--search", "GROUND_LINK_TX_BYTES")
            processes.append(("cli_events_listener", events_listener))
            processes.append(("cli_channels_listener", channels_listener))
            time.sleep(1.0)

        send_obc_command(obc, "status", 0.5)
        for attempt in range(command_attempts):
            current_obc_text = sanitize_file(obc_log)
            eps_readback_pass = eps_readback_pass or re.search(r"eps soc=.* pdu=7", current_obc_text) is not None
            adcs_readback_pass = adcs_readback_pass or "adcs mode=POINTING" in current_obc_text

            if not eps_readback_pass and not adcs_readback_pass:
                if attempt % 2 == 0:
                    send_eps_command()
                    send_adcs_command()
                else:
                    send_adcs_command()
                    send_eps_command()
            elif not eps_readback_pass:
                send_eps_command()
            elif not adcs_readback_pass:
                send_adcs_command()

            for _ in range(6):
                send_obc_command(obc, "eps get", 0.4)
                send_obc_command(obc, "adcs get", 0.4)
                current_obc_text = sanitize_file(obc_log)
                eps_readback_pass = eps_readback_pass or re.search(r"eps soc=.* pdu=7", current_obc_text) is not None
                adcs_readback_pass = adcs_readback_pass or "adcs mode=POINTING" in current_obc_text
                if eps_readback_pass and adcs_readback_pass:
                    stage1_pass = True
                    break
                time.sleep(0.8)

            if stage1_pass:
                break

        send_obc_command(obc, "status", 0.5)
        time.sleep(2.0)

        if stage1_pass and run_stage2:
            dispatched_count = sanitize_file(cli_events_log).count("OpCodeDispatched")
            completed_count = sanitize_file(cli_events_log).count("OpCodeCompleted")
            channels_seen = "GROUND_LINK_TX_BYTES" in sanitize_file(cli_channels_log)
            stage2_pass = dispatched_count >= 2 and completed_count >= 2 and channels_seen

        send_obc_command(obc, "quit", 0.0)
        try:
            obc.wait(timeout=20.0)
        except subprocess.TimeoutExpired:
            pass

        if not stage1_pass:
            missing = []
            if not eps_readback_pass:
                missing.append("EPS pdu=7")
            if not adcs_readback_pass:
                missing.append("ADCS mode=POINTING")
            raise RuntimeError("Stage 1 uplink ingress did not produce required OBC readback: " + ", ".join(missing))
finally:
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
    cleanup_remote_comm_node()

obc_text = sanitize_file(obc_log)
events_text = sanitize_file(cli_events_log)
channels_text = sanitize_file(cli_channels_log)

print("comm-lab-serial-ingress-probe: PASS")
print("formal-verdict=" + ("bounded-ttc" if stage2_pass else "uplink-ingress"))
print("stage0-uart-preflight=" + ("PASS" if run_stage0_preflight == "1" else "SKIPPED"))
print("stage0-acquisition=" + ("PASS" if run_stage0_acquisition == "1" else "SKIPPED"))
print("stage1-uplink-ingress=PASS")
if run_stage2:
    print("stage2-downlink=" + ("PASS" if stage2_pass else "STOPPED"))
else:
    print("stage2-downlink=SKIPPED")
print(f"host-serial-device={host_serial_device}")
print(f"subsystem-serial-device={subsystem_serial_device}")
print(f"baudrate={baudrate}")
print(f"comm-node={comm_csp_node}")
print(f"local-csp-hub-host={local_csp_hub_host}")
print(f"remote-csp-hub-host={remote_csp_hub_host}")
print(f"csp-hub-sub-port={csp_hub_sub_port}")
print(f"csp-hub-pub-port={csp_hub_pub_port}")
print(f"gds-port={gds_port}")
print(f"gds-tts-port={gds_tts_port}")
print(f"radio-port={radio_port}")
print(f"lab-serial-tx-preamble-lines={lab_serial_tx_preamble_lines}")
print(f"lab-serial-tx-preamble-delay-ms={lab_serial_tx_preamble_delay_ms}")
print(f"lab-serial-startup-delay-sec={lab_serial_startup_delay_sec}")
print(f"lab-serial-command-attempts={lab_serial_command_attempts}")
print(f"log-dir={probe_tmp_dir}")
print("=== OBC Readback ===")
for line in obc_text.splitlines():
    if any(fragment in line for fragment in ("Ground link via COMM CSP node:", "groundLink mode=", "groundLink connected=", "eps soc=", "adcs mode=")):
        print(line)
print("=== fprime-cli Events ===")
for line in events_text.splitlines():
    if "OpCodeDispatched" in line or "OpCodeCompleted" in line:
        print(line)
print("=== fprime-cli Channels ===")
for line in channels_text.splitlines():
    if "GROUND_LINK_TX_BYTES" in line:
        print(line)
PY
