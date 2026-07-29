#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_BINARY_NAME="${OBC_BINARY_NAME:-OBC}"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="${DICT_PATH:-$(obc_find_dictionary_path "${ROOT_DIR}" "${OBC_BINARY_NAME}" || true)}"
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

COMM_TTC_FILE_LINK_MODE="${COMM_TTC_FILE_LINK_MODE:-hosted-pty}"
case "${COMM_TTC_FILE_LINK_MODE}" in
  hosted-pty|physical-serial|hosted-sband-tcp) ;;
  *)
    echo "COMM_TTC_FILE_LINK_MODE must be hosted-pty, physical-serial, or hosted-sband-tcp." >&2
    exit 2
    ;;
esac

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
COMMAND_PREFIX="${COMMAND_PREFIX:-OBCApp}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-sband-primary}"
COMMAND_AUTH="${COMMAND_AUTH:-hmac-sha256}"
COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-}"
COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-}"
COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-}"
COMMAND_AUTH_SESSION_ID="${COMMAND_AUTH_SESSION_ID:-9701}"
GDS_FRAMING_SELECTION="${GDS_FRAMING_SELECTION:-fprime}"
GDS_SCID="${GDS_SCID:-}"
GDS_VCID="${GDS_VCID:-}"
GDS_FRAME_SIZE="${GDS_FRAME_SIZE:-}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-127.0.0.1}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
REMOTE_CSP_HUB_HOST="${REMOTE_CSP_HUB_HOST:-$(obc_default_remote_carrier_host)}"
PREPARE_SUBSYSTEM_WORKSPACE="${PREPARE_SUBSYSTEM_WORKSPACE:-0}"
RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO="${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO:-0}"
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES:-20}"
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS:-500}"
LAB_SERIAL_STARTUP_DELAY_SEC="${LAB_SERIAL_STARTUP_DELAY_SEC:-}"
LAB_SERIAL_COMMAND_ATTEMPTS="${LAB_SERIAL_COMMAND_ATTEMPTS:-6}"
HK_TARGET_OCCUPIED_SLOTS="${HK_TARGET_OCCUPIED_SLOTS:-2}"
HK_CAPTURE_MAX_ATTEMPTS="${HK_CAPTURE_MAX_ATTEMPTS:-32}"
HK_CAPTURE_DELAY_SEC="${HK_CAPTURE_DELAY_SEC:-0.8}"
FILE_DOWNLINK_TIMEOUT_SEC="${FILE_DOWNLINK_TIMEOUT_SEC:-60}"
FILE_DOWNLINK_COMMAND_ATTEMPTS="${FILE_DOWNLINK_COMMAND_ATTEMPTS:-3}"
FILE_DOWNLINK_START_TIMEOUT_SEC="${FILE_DOWNLINK_START_TIMEOUT_SEC:-20}"
PTY_BRIDGE_PATH_TIMEOUT_SEC="${PTY_BRIDGE_PATH_TIMEOUT_SEC:-30}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-ttc-file-downlink-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-ttc-file-downlink-gds-downlink}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-ttc-file-downlink.XXXXXX")}"
PTY_BRIDGE_LOG="${PTY_BRIDGE_LOG:-${PROBE_TMP_DIR}/pty-bridge.log}"
GROUND_GDS_LOG="${GROUND_GDS_LOG:-${PROBE_TMP_DIR}/ground-gds.log}"
COMM_NODE_LOG="${COMM_NODE_LOG:-${PROBE_TMP_DIR}/comm-node.log}"
GATEWAY_LOG="${GATEWAY_LOG:-${PROBE_TMP_DIR}/gateway.log}"
GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND="${GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND:-}"
GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS="${GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS:-}"
REMOTE_COMM_NODE_LOG="${REMOTE_COMM_NODE_LOG:-${PROBE_TMP_DIR}/remote-comm-node.log}"
OBC_LOG="${OBC_LOG:-${PROBE_TMP_DIR}/obc.log}"
CLI_COMMAND_LOG="${CLI_COMMAND_LOG:-${PROBE_TMP_DIR}/command-send.log}"
CLI_EVENTS_LOG="${CLI_EVENTS_LOG:-${PROBE_TMP_DIR}/events.log}"
CLI_CHANNELS_LOG="${CLI_CHANNELS_LOG:-${PROBE_TMP_DIR}/channels.log}"
REMOTE_COMM_NODE_PID_FILE="${REMOTE_COMM_NODE_PID_FILE:-/tmp/obc-comm-ttc-file-downlink-${RANDOM}-$$.pid}"

usage() {
  cat >&2 <<EOF
Usage:
  COMM_TTC_FILE_LINK_MODE=hosted-pty bash scripts/run_comm_ttc_file_downlink_probe.sh
  COMM_TTC_FILE_LINK_MODE=physical-serial HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_comm_ttc_file_downlink_probe.sh

Environment:
  COMM_TTC_FILE_LINK_MODE         hosted-pty or physical-serial
  OBC_BINARY_NAME                 hosted OBC executable name, default OBC
  COMMAND_PREFIX                  F' command namespace, default OBCApp
  COMMAND_AUTHORITY_PROFILE       runtime authority profile, default sband-primary
  COMMAND_AUTH                    authenticated envelope mode for this probe, must be hmac-sha256
  COMMAND_AUTH_SOURCE_ID          authenticated envelope source id, default follows profile
  COMMAND_AUTH_KEY_SLOT           authenticated envelope key slot, default follows profile
  COMMAND_AUTH_KEY_HEX            authenticated envelope HMAC key, default follows profile
  COMMAND_AUTH_SESSION_ID         authenticated envelope session id, default 9701
  GDS_FRAMING_SELECTION           fprime or space-packet-space-data-link, default fprime
  SBAND_TCP_HOST                  hosted-sband-tcp listen/connect host, default 127.0.0.1
  SBAND_TCP_PORT                  hosted-sband-tcp listen/connect port, default auto-selected
  HOST_SERIAL_DEVICE              macOS serial endpoint for physical-serial mode, default /dev/cu.usbserial-CHANGE_ME
  SUBSYSTEM_SIM_COMM_DEVICE       subsystem.local serial endpoint for physical-serial mode, default /dev/serial0
  HK_TARGET_OCCUPIED_SLOTS        max source .fdp files to snapshot/compare, default 2
  HK_CAPTURE_MAX_ATTEMPTS         bounded paced HK_CAPTURE_NOW attempts, default 32
  HK_CAPTURE_DELAY_SEC            delay after each HK_CAPTURE_NOW, default 0.8
  FILE_DOWNLINK_TIMEOUT_SEC       bounded wait for received files, default 60
  FILE_DOWNLINK_COMMAND_ATTEMPTS  bounded downlink command retries when no file START is seen, default 3
  FILE_DOWNLINK_START_TIMEOUT_SEC bounded wait for GDS file START metadata before retry, default 20
  PTY_BRIDGE_PATH_TIMEOUT_SEC     hosted PTY path read timeout, default 30
  PREPARE_SUBSYSTEM_WORKSPACE     physical mode sync/bootstrap before probe when set to 1
EOF
}

require_command() {
  local command_name="${1:?command_name is required}"
  if ! command -v "${command_name}" >/dev/null 2>&1; then
    echo "Required command not found: ${command_name}" >&2
    exit 2
  fi
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
  python3 - "${port}" <<'PY'
import socket
import sys

port = int(sys.argv[1])
for host in ("0.0.0.0", "127.0.0.1"):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind((host, port))
    except OSError:
        sys.exit(1)
    finally:
        sock.close()
sys.exit(0)
PY
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

require_bool PREPARE_SUBSYSTEM_WORKSPACE "${PREPARE_SUBSYSTEM_WORKSPACE}"
require_bool RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO "${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO}"

if [[ "${COMMAND_AUTH}" != "hmac-sha256" ]]; then
  echo "run_comm_ttc_file_downlink_probe.sh requires COMMAND_AUTH=hmac-sha256." >&2
  echo "This probe proves the authenticated restricted-command path, not raw command-send fallback." >&2
  exit 2
fi

if [[ -z "${COMMAND_AUTH_SOURCE_ID}" || -z "${COMMAND_AUTH_KEY_SLOT}" || -z "${COMMAND_AUTH_KEY_HEX}" ]]; then
  case "${COMMAND_AUTHORITY_PROFILE}" in
    sband-primary)
      COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-1}"
      COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-1}"
      COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F}"
      ;;
    uhf-primary|uhf-backup)
      COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-2}"
      COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-2}"
      COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F}"
      ;;
    dev-direct)
      COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-3}"
      COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-3}"
      COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-505152535455565758595A5B5C5D5E5F}"
      ;;
    internal)
      COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-4}"
      COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-4}"
      COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-606162636465666768696A6B6C6D6E6F}"
      ;;
    *)
      echo "Unsupported COMMAND_AUTHORITY_PROFILE: ${COMMAND_AUTHORITY_PROFILE}" >&2
      exit 2
      ;;
  esac
fi

if ! [[ "${HK_TARGET_OCCUPIED_SLOTS}" =~ ^[1-9][0-9]*$ && \
  "${HK_CAPTURE_MAX_ATTEMPTS}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_COMMAND_ATTEMPTS}" =~ ^[1-9][0-9]*$ && \
  "${FILE_DOWNLINK_START_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${PTY_BRIDGE_PATH_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ && \
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" =~ ^[0-9]+$ && \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" =~ ^[0-9]+$ && \
  "${LAB_SERIAL_COMMAND_ATTEMPTS}" =~ ^[1-9][0-9]*$ ]]; then
  echo "Numeric probe settings are invalid." >&2
  usage
  exit 2
fi

if [[ -z "${LAB_SERIAL_STARTUP_DELAY_SEC}" ]]; then
  LAB_SERIAL_STARTUP_DELAY_SEC="$(( (LAB_SERIAL_TX_PREAMBLE_LINES * LAB_SERIAL_TX_PREAMBLE_DELAY_MS + 999) / 1000 + 2 ))"
  if [[ "${LAB_SERIAL_STARTUP_DELAY_SEC}" -lt 5 ]]; then
    LAB_SERIAL_STARTUP_DELAY_SEC=5
  fi
fi

if [[ "${COMM_TTC_FILE_LINK_MODE}" == "physical-serial" && ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial device not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi

require_command python3

if [[ -z "${GDS_PORT:-}" ]]; then
  GDS_PORT="$(find_free_port_pair 50520 1)"
fi
if [[ -z "${GDS_TTS_PORT:-}" ]]; then
  GDS_TTS_PORT="$((GDS_PORT + 1))"
fi
if [[ -z "${CSP_HUB_SUB_PORT:-}" ]]; then
  CSP_HUB_SUB_PORT="$(find_free_port_pair 56520 1000)"
fi
if [[ -z "${CSP_HUB_PUB_PORT:-}" ]]; then
  CSP_HUB_PUB_PORT="$((CSP_HUB_SUB_PORT + 1000))"
fi
if [[ -z "${RADIO_PORT:-}" ]]; then
  RADIO_PORT="$(find_free_port 17520)"
fi
if [[ "${COMM_TTC_FILE_LINK_MODE}" == "hosted-sband-tcp" && -z "${SBAND_TCP_PORT:-}" ]]; then
  SBAND_TCP_PORT="$(find_free_port 18520)"
fi

mkdir -p "${PROBE_TMP_DIR}"
rm -rf "${RUNTIME_ROOT}" "${GDS_FILE_STORAGE_DIR}"
mkdir -p "${RUNTIME_ROOT}" "${GDS_FILE_STORAGE_DIR}"

if [[ "${COMM_TTC_FILE_LINK_MODE}" == "physical-serial" && "${PREPARE_SUBSYSTEM_WORKSPACE}" == "1" ]]; then
  bash "${ROOT_DIR}/scripts/sync_subsystem_sim_workspace.sh"
  bash "${ROOT_DIR}/scripts/bootstrap_subsystem_sim_workspace.sh"
fi

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
COMM_TTC_FILE_LINK_MODE="${COMM_TTC_FILE_LINK_MODE}" \
OBC_BINARY_NAME="${OBC_BINARY_NAME}" \
COMMAND_PREFIX="${COMMAND_PREFIX}" \
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE}" \
COMMAND_AUTH="${COMMAND_AUTH}" \
COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID}" \
COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT}" \
COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX}" \
COMMAND_AUTH_SESSION_ID="${COMMAND_AUTH_SESSION_ID}" \
GDS_FRAMING_SELECTION="${GDS_FRAMING_SELECTION}" \
GDS_SCID="${GDS_SCID}" \
GDS_VCID="${GDS_VCID}" \
GDS_FRAME_SIZE="${GDS_FRAME_SIZE}" \
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR}" \
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE}" \
COMM_BAUDRATE="${COMM_BAUDRATE}" \
COMM_CSP_NODE="${COMM_CSP_NODE}" \
SBAND_TCP_HOST="${SBAND_TCP_HOST}" \
SBAND_TCP_PORT="${SBAND_TCP_PORT:-0}" \
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID}" \
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
REMOTE_CSP_HUB_HOST="${REMOTE_CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
GDS_PORT="${GDS_PORT}" \
GDS_TTS_PORT="${GDS_TTS_PORT}" \
RADIO_PORT="${RADIO_PORT}" \
RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO="${RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO}" \
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES}" \
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" \
LAB_SERIAL_STARTUP_DELAY_SEC="${LAB_SERIAL_STARTUP_DELAY_SEC}" \
LAB_SERIAL_COMMAND_ATTEMPTS="${LAB_SERIAL_COMMAND_ATTEMPTS}" \
HK_TARGET_OCCUPIED_SLOTS="${HK_TARGET_OCCUPIED_SLOTS}" \
HK_CAPTURE_MAX_ATTEMPTS="${HK_CAPTURE_MAX_ATTEMPTS}" \
HK_CAPTURE_DELAY_SEC="${HK_CAPTURE_DELAY_SEC}" \
FILE_DOWNLINK_TIMEOUT_SEC="${FILE_DOWNLINK_TIMEOUT_SEC}" \
FILE_DOWNLINK_COMMAND_ATTEMPTS="${FILE_DOWNLINK_COMMAND_ATTEMPTS}" \
FILE_DOWNLINK_START_TIMEOUT_SEC="${FILE_DOWNLINK_START_TIMEOUT_SEC}" \
PTY_BRIDGE_PATH_TIMEOUT_SEC="${PTY_BRIDGE_PATH_TIMEOUT_SEC}" \
REMOTE_COMM_NODE_PID_FILE="${REMOTE_COMM_NODE_PID_FILE}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
PTY_BRIDGE_LOG="${PTY_BRIDGE_LOG}" \
GROUND_GDS_LOG="${GROUND_GDS_LOG}" \
COMM_NODE_LOG="${COMM_NODE_LOG}" \
GATEWAY_LOG="${GATEWAY_LOG}" \
GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND="${GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND}" \
GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS="${GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS}" \
REMOTE_COMM_NODE_LOG="${REMOTE_COMM_NODE_LOG}" \
OBC_LOG="${OBC_LOG}" \
CLI_COMMAND_LOG="${CLI_COMMAND_LOG}" \
CLI_EVENTS_LOG="${CLI_EVENTS_LOG}" \
CLI_CHANNELS_LOG="${CLI_CHANNELS_LOG}" \
python3 - <<'PY'
import csv
import filecmp
import hashlib
import hmac
import json
import os
import pathlib
import re
import selectors
import shlex
import shutil
import socket
import struct
import subprocess
import sys
import time

MAX_SANITIZED_LOG_BYTES = 2 * 1024 * 1024
COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32
FW_SERIALIZE_TRUE_VALUE = 0xFF
FW_WAIT_NO_WAIT = 1
ADCS_MODE_POINTING = 2

def require_env(name):
    value = os.environ.get(name)
    if value is None or value == "":
        raise RuntimeError(f"Required probe environment variable is missing: {name}")
    return value


sys.path.insert(0, os.path.join(require_env("ROOT_DIR"), "scripts"))
from probe_process_utils import (
    GDS_PROCESS_MARKERS,
    ManagedProcess,
    build_gds_stale_match_groups,
    cleanup_managed_processes,
    install_signal_cleanup,
    start_managed_process_with_handle,
    stop_managed_process,
)


root_dir = require_env("ROOT_DIR")
bin_dir = require_env("BIN_DIR")
dict_path = require_env("DICT_PATH")
fprime_cli_bin = require_env("FPRIME_CLI_BIN")
link_mode = require_env("COMM_TTC_FILE_LINK_MODE")
obc_binary_name = require_env("OBC_BINARY_NAME")
command_prefix = require_env("COMMAND_PREFIX")
command_authority_profile = require_env("COMMAND_AUTHORITY_PROFILE")
command_auth = require_env("COMMAND_AUTH")
command_auth_source_id = int(require_env("COMMAND_AUTH_SOURCE_ID"))
command_auth_key_slot = int(require_env("COMMAND_AUTH_KEY_SLOT"))
command_auth_key_bytes = bytes.fromhex(require_env("COMMAND_AUTH_KEY_HEX"))
command_auth_session_id = int(require_env("COMMAND_AUTH_SESSION_ID"))


def authority_identity_role(profile):
    table = {
        "sband-primary": (1, 1),
        "uhf-backup": (2, 2),
        "uhf-primary": (2, 3),
        "dev-direct": (3, 4),
        "internal": (4, 5),
    }
    try:
        return table[profile]
    except KeyError as exc:
        raise RuntimeError(f"Unsupported COMMAND_AUTHORITY_PROFILE: {profile}") from exc


command_auth_identity, command_auth_role = authority_identity_role(command_authority_profile)
gds_framing_selection = require_env("GDS_FRAMING_SELECTION")
gds_scid = os.environ.get("GDS_SCID", "")
gds_vcid = os.environ.get("GDS_VCID", "")
gds_frame_size = os.environ.get("GDS_FRAME_SIZE", "")
subsystem_ssh_target = require_env("SUBSYSTEM_SIM_SSH_TARGET")
subsystem_remote_dir = require_env("SUBSYSTEM_SIM_REMOTE_DIR")
host_serial_device = require_env("HOST_SERIAL_DEVICE")
subsystem_serial_device = require_env("SUBSYSTEM_SIM_COMM_DEVICE")
baudrate = require_env("COMM_BAUDRATE")
comm_csp_node = require_env("COMM_CSP_NODE")
sband_tcp_host = require_env("SBAND_TCP_HOST")
sband_tcp_port = require_env("SBAND_TCP_PORT")
eps_csp_node_id = require_env("EPS_CSP_NODE_ID")
adcs_csp_node_id = require_env("ADCS_CSP_NODE_ID")
local_csp_hub_host = require_env("CSP_HUB_HOST")
remote_csp_hub_host = require_env("REMOTE_CSP_HUB_HOST")
csp_hub_sub_port = require_env("CSP_HUB_SUB_PORT")
csp_hub_pub_port = require_env("CSP_HUB_PUB_PORT")
gds_port = require_env("GDS_PORT")
gds_tts_port = require_env("GDS_TTS_PORT")
radio_port = require_env("RADIO_PORT")
run_subsystem_comm_node_with_sudo = require_env("RUN_SUBSYSTEM_COMM_NODE_WITH_SUDO")
lab_serial_tx_preamble_lines = require_env("LAB_SERIAL_TX_PREAMBLE_LINES")
lab_serial_tx_preamble_delay_ms = require_env("LAB_SERIAL_TX_PREAMBLE_DELAY_MS")
lab_serial_startup_delay_sec = require_env("LAB_SERIAL_STARTUP_DELAY_SEC")
lab_serial_command_attempts = require_env("LAB_SERIAL_COMMAND_ATTEMPTS")
hk_target_occupied_slots = require_env("HK_TARGET_OCCUPIED_SLOTS")
hk_capture_max_attempts = require_env("HK_CAPTURE_MAX_ATTEMPTS")
hk_capture_delay_sec = require_env("HK_CAPTURE_DELAY_SEC")
file_downlink_timeout_sec = require_env("FILE_DOWNLINK_TIMEOUT_SEC")
file_downlink_command_attempts = require_env("FILE_DOWNLINK_COMMAND_ATTEMPTS")
file_downlink_start_timeout_sec = require_env("FILE_DOWNLINK_START_TIMEOUT_SEC")
pty_bridge_path_timeout_sec = require_env("PTY_BRIDGE_PATH_TIMEOUT_SEC")
remote_comm_node_pid_file = require_env("REMOTE_COMM_NODE_PID_FILE")
runtime_root = require_env("RUNTIME_ROOT")
gds_file_storage_dir = require_env("GDS_FILE_STORAGE_DIR")
probe_tmp_dir = require_env("PROBE_TMP_DIR")
pty_bridge_log = require_env("PTY_BRIDGE_LOG")
ground_gds_log = require_env("GROUND_GDS_LOG")
comm_node_log = require_env("COMM_NODE_LOG")
gateway_log = require_env("GATEWAY_LOG")
gateway_capture_gds_to_southbound = os.environ.get("GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND", "")
gateway_capture_southbound_to_gds = os.environ.get("GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS", "")
remote_comm_node_log = require_env("REMOTE_COMM_NODE_LOG")
obc_log = require_env("OBC_LOG")
cli_command_log = require_env("CLI_COMMAND_LOG")
cli_events_log = require_env("CLI_EVENTS_LOG")
cli_channels_log = require_env("CLI_CHANNELS_LOG")

command_attempts = int(lab_serial_command_attempts)
target_occupied_slots = int(hk_target_occupied_slots)
capture_max_attempts = int(hk_capture_max_attempts)
capture_delay_sec = float(hk_capture_delay_sec)
file_timeout_sec = int(file_downlink_timeout_sec)
file_command_attempts = int(file_downlink_command_attempts)
file_start_timeout_sec = int(file_downlink_start_timeout_sec)
pty_path_timeout_sec = int(pty_bridge_path_timeout_sec)
sanitized_log_cache = {}
dictionary = json.loads(pathlib.Path(dict_path).read_text(encoding="utf-8"))
authenticated_session_open = False
next_authenticated_sequence = 1


def shq(value):
    return shlex.quote(str(value))


def start_process(
    name,
    args,
    *,
    env=None,
    handle=None,
    stdin=None,
    stale_match_groups=(),
    stale_match_markers=(),
):
    if handle is None:
        raise RuntimeError(f"Managed probe process {name} requires a log handle")
    managed = start_managed_process_with_handle(
        name,
        args,
        handle,
        env=env,
        stdin=stdin,
        stale_match_groups=stale_match_groups,
        stale_match_markers=stale_match_markers,
    )
    processes.append(managed)
    return managed.process


def read_pty_bridge_lines(process, handle, timeout_sec):
    if process.stdout is None:
        raise RuntimeError("pty_pair_bridge stdout pipe is unavailable")

    deadline = time.monotonic() + timeout_sec
    buffer = b""
    lines = []
    selector = selectors.DefaultSelector()
    try:
        selector.register(process.stdout, selectors.EVENT_READ)
        while len(lines) < 2:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                if process.poll() is not None:
                    raise RuntimeError("pty_pair_bridge exited before reporting both PTY paths")
                raise RuntimeError(f"Timed out after {timeout_sec}s waiting for pty_pair_bridge PTY paths")
            if not selector.select(remaining):
                continue
            chunk = os.read(process.stdout.fileno(), 4096)
            if not chunk:
                raise RuntimeError("pty_pair_bridge did not report both PTY paths")
            buffer += chunk
            while b"\n" in buffer and len(lines) < 2:
                raw_line, buffer = buffer.split(b"\n", 1)
                line = raw_line.decode("utf-8", "replace") + "\n"
                handle.write(line)
                lines.append(line)
    finally:
        selector.close()
    return lines


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


def require_running(process, name):
    if process.poll() is not None:
        raise RuntimeError(f"{name} exited early with code {process.returncode}")


def wait_for_log_fragment(path, fragment, timeout_sec):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if fragment in sanitize_file(path):
            return True
        time.sleep(0.2)
    return False


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


def fq_command(relative_name):
    return f"{command_prefix}.{relative_name}"


def dictionary_command_opcode(name):
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"Command not found in dictionary: {name}")


def inner_command(opcode, args=b""):
    return struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args


def gds_command_packet(opcode, args=b""):
    packet = inner_command(opcode, args)
    return struct.pack(">II", COMMAND_DESCRIPTOR, len(packet)) + packet


def authenticated_envelope(inner, sequence_number):
    if command_auth != "hmac-sha256":
        raise RuntimeError(
            "run_comm_ttc_file_downlink_probe.sh requires COMMAND_AUTH=hmac-sha256 "
            f"for authenticated restricted-command proof, got: {command_auth}"
        )
    header = struct.pack(
        ">IBBHIIIHHHH",
        COMMAND_ENVELOPE_V1_MAGIC,
        COMMAND_ENVELOPE_V1_VERSION,
        0,
        COMMAND_ENVELOPE_V1_HEADER_LENGTH,
        command_auth_source_id,
        command_auth_session_id,
        sequence_number,
        command_auth_key_slot,
        len(inner),
        COMMAND_ENVELOPE_V1_MAC_LENGTH,
        0,
    )
    auth_tag = hmac.new(command_auth_key_bytes, header + inner, hashlib.sha256).digest()
    return gds_command_packet(OBC_COMMAND_ENVELOPE_V1_OPCODE, header + inner + auth_tag)


def send_authenticated_envelope(label, opcode, sequence_number, args=b""):
    payload = authenticated_envelope(inner_command(opcode, args), sequence_number)
    with open(cli_command_log, "a", encoding="utf-8") as handle:
        handle.write(
            f"auth-envelope label={label} tts=127.0.0.1:{gds_tts_port} "
            f"opcode=0x{opcode:x} session={command_auth_session_id} seq={sequence_number} "
            f"source={command_auth_source_id} keySlot={command_auth_key_slot} outer={payload.hex()}\n"
        )
    with socket.create_connection(("127.0.0.1", int(gds_tts_port)), timeout=5.0) as sock:
        sock.sendall(b"Register GUI\n")
        time.sleep(0.1)
        sock.sendall(b"A5A5 FSW " + payload)
        time.sleep(0.3)


def ensure_authenticated_session():
    global authenticated_session_open
    if authenticated_session_open:
        return
    opcode = dictionary_command_opcode(fq_command("commandIngressAuthority.SESSION_OPEN"))
    expected = (
        f"Command session opened ingress 0 identity {command_auth_identity} "
        f"role {command_auth_role} session {command_auth_session_id}"
    )
    for _ in range(command_attempts):
        send_authenticated_envelope("SESSION_OPEN", opcode, 0)
        if wait_for_log_fragment(obc_log, expected, 8.0):
            authenticated_session_open = True
            return
        time.sleep(0.8)
    raise RuntimeError("Authenticated SESSION_OPEN did not complete according to OBC log")


def run_authenticated_command(relative_name, args=b"", expected_fragment=None, timeout_sec=8.0):
    global next_authenticated_sequence
    ensure_authenticated_session()
    opcode = dictionary_command_opcode(fq_command(relative_name))
    label = fq_command(relative_name)
    for attempt in range(command_attempts):
        sequence = next_authenticated_sequence
        next_authenticated_sequence += 1
        send_authenticated_envelope(label, opcode, sequence, args)
        if expected_fragment is None or wait_for_log_fragment(obc_log, expected_fragment, timeout_sec):
            return
        if attempt + 1 < command_attempts:
            time.sleep(0.8)
    raise RuntimeError(f"Authenticated command did not produce expected evidence: {label}")


def start_cli_listener(handle, subcommand, *extra_args):
    return start_process(
        f"cli-{subcommand}",
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


def gateway_capture_args():
    args = []
    if gateway_capture_gds_to_southbound:
        args.extend(["--capture-gds-to-southbound", gateway_capture_gds_to_southbound])
    if gateway_capture_southbound_to_gds:
        args.extend(["--capture-southbound-to-gds", gateway_capture_southbound_to_gds])
    return args


def send_obc_command(obc_process, command_text, delay_sec=0.5):
    assert obc_process.stdin is not None
    obc_process.stdin.write(command_text + "\n")
    obc_process.stdin.flush()
    time.sleep(delay_sec)


def start_remote_comm_node(handle):
    runner_prefix = "sudo -n " if run_subsystem_comm_node_with_sudo == "1" else ""
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
CSP_TRANSPORT=zmqhub \\
CSP_HUB_HOST={shq(remote_csp_hub_host)} \\
CSP_HUB_SUB_PORT={shq(csp_hub_sub_port)} \\
CSP_HUB_PUB_PORT={shq(csp_hub_pub_port)} \\
{runner_prefix}"${{BIN_DIR}}/comm_csp_node" \\
  --serial-device {shq(subsystem_serial_device)} \\
  --baudrate {shq(baudrate)} \\
  --node-id {shq(comm_csp_node)} &
REMOTE_COMM_PID=$!
echo "${{REMOTE_COMM_PID}}" >{shq(remote_comm_node_pid_file)}
wait "${{REMOTE_COMM_PID}}"
"""
    return start_process(
        "remote_comm_csp_node",
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
    if link_mode != "physical-serial":
        return
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


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def received_downlink_path(dest_name):
    return pathlib.Path(gds_file_storage_dir) / "fprime-downlink" / dest_name


def received_downlink_dir():
    return pathlib.Path(gds_file_storage_dir) / "fprime-downlink"


def source_snapshot_path(dest_name):
    snapshot_dir = pathlib.Path(probe_tmp_dir) / "source-snapshots"
    snapshot_dir.mkdir(parents=True, exist_ok=True)
    return snapshot_dir / dest_name


def snapshot_source_file(source_path, dest_name):
    source_path = pathlib.Path(source_path)
    if not source_path.is_file():
        raise RuntimeError(f"Source file is not available for snapshot: {source_path}")
    snapshot_path = source_snapshot_path(dest_name)
    shutil.copyfile(source_path, snapshot_path)
    return snapshot_path


def list_data_product_files():
    return sorted((pathlib.Path(runtime_root) / "data-products").glob("Dp_*.fdp"))


def select_data_product_files():
    files = list_data_product_files()
    attempts = 0
    while len(files) < 1 and attempts < capture_max_attempts:
        attempts += 1
        time.sleep(capture_delay_sec)
        files = list_data_product_files()
    if len(files) < 1:
        raise RuntimeError(
            f"Only {len(files)} official .fdp files appeared after {attempts} waits; "
            "needed at least one file for file downlink"
        )
    return files[:target_occupied_slots], attempts


def wait_for_matching_file(source_path, received_path, timeout_sec):
    deadline = time.time() + timeout_sec
    source_path = pathlib.Path(source_path)
    received_path = pathlib.Path(received_path)
    while time.time() < deadline:
        if source_path.is_file() and received_path.is_file() and filecmp.cmp(source_path, received_path, shallow=False):
            return sha256_file(received_path), received_path.stat().st_size
        time.sleep(0.5)
    raise RuntimeError(f"Timed out waiting for byte-matching downlink: source={source_path} received={received_path}")


def wait_for_downlink_start(dest_name, timeout_sec):
    return wait_for_log_fragment(ground_gds_log, f"Destination: {dest_name}", timeout_sec)


def remove_received_file(received_path):
    try:
        pathlib.Path(received_path).unlink()
    except FileNotFoundError:
        pass


def remove_received_fdp_files():
    for path in received_downlink_dir().glob("*.fdp"):
        remove_received_file(path)


def snapshot_source_files(source_paths, label_prefix):
    snapshots = []
    for index, source_path in enumerate(source_paths, start=1):
        snapshot_name = f"{label_prefix}-{index}-{pathlib.Path(source_path).name}"
        snapshots.append(
            {
                "source_path": pathlib.Path(source_path),
                "snapshot_path": snapshot_source_file(source_path, snapshot_name),
            }
        )
    return snapshots


def wait_for_any_matching_file(expected_snapshots, timeout_sec):
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
    expected_names = ", ".join(entry["source_path"].name for entry in expected_snapshots)
    raise RuntimeError(f"Timed out waiting for any byte-matching .fdp downlink among [{expected_names}]")


def run_start_xmit_catalog():
    run_authenticated_command(
        "dpCatalog.START_XMIT_CATALOG",
        args=struct.pack(">B", FW_WAIT_NO_WAIT),
        expected_fragment="SendingProduct",
        timeout_sec=file_start_timeout_sec,
    )


def downlink_data_product(source_paths, snapshot_label):
    remove_received_fdp_files()
    last_error = None
    for attempt in range(1, file_command_attempts + 1):
        expected_snapshots = snapshot_source_files(source_paths, f"{snapshot_label}-attempt-{attempt}")
        try:
            run_authenticated_command(
                "dpCatalog.BUILD_CATALOG",
                expected_fragment="CatalogBuildComplete",
                timeout_sec=file_start_timeout_sec,
            )
            if not wait_for_log_fragment(obc_log, "CatalogBuildComplete", file_start_timeout_sec):
                raise RuntimeError("Timed out waiting for CatalogBuildComplete after DpCatalog.BUILD_CATALOG")
            run_start_xmit_catalog()
            if not wait_for_log_fragment(obc_log, "SendingProduct", file_start_timeout_sec):
                raise RuntimeError("Timed out waiting for SendingProduct after DpCatalog.START_XMIT_CATALOG")
            return wait_for_any_matching_file(expected_snapshots, file_timeout_sec)
        except RuntimeError as exc:
            last_error = exc
            if attempt < file_command_attempts:
                remove_received_fdp_files()
                time.sleep(2.0)
    assert last_error is not None
    raise last_error


def ensure_ttc_prerequisite(obc_process):
    stage1_pass = False
    eps_readback_pass = False
    adcs_readback_pass = False
    for attempt in range(command_attempts):
        current_obc_text = sanitize_file(obc_log)
        eps_readback_pass = eps_readback_pass or re.search(r"eps soc=.* pdu=7", current_obc_text) is not None
        adcs_readback_pass = adcs_readback_pass or "adcs mode=POINTING" in current_obc_text
        if not eps_readback_pass:
            run_authenticated_command(
                "epsBridge.EPS_SET_PDU",
                args=struct.pack(">BB", 2, FW_SERIALIZE_TRUE_VALUE),
                expected_fragment="EPS_PDU_CHANGE",
            )
            time.sleep(1.0)
        if not adcs_readback_pass:
            run_authenticated_command(
                "adcsBridge.ADCS_SET_MODE",
                args=struct.pack(">B", ADCS_MODE_POINTING),
                expected_fragment="ADCS_MODE_CHANGE",
            )
            time.sleep(1.0)
        for _ in range(6):
            send_obc_command(obc_process, "eps get", 0.4)
            send_obc_command(obc_process, "adcs get", 0.4)
            current_obc_text = sanitize_file(obc_log)
            eps_readback_pass = eps_readback_pass or re.search(r"eps soc=.* pdu=7", current_obc_text) is not None
            adcs_readback_pass = adcs_readback_pass or "adcs mode=POINTING" in current_obc_text
            if eps_readback_pass and adcs_readback_pass:
                stage1_pass = True
                break
            time.sleep(0.8)
        if stage1_pass:
            break
    if not stage1_pass:
        missing = []
        if not eps_readback_pass:
            missing.append("EPS pdu=7")
        if not adcs_readback_pass:
            missing.append("ADCS mode=POINTING")
        raise RuntimeError("TT&C prerequisite did not produce required OBC readback: " + ", ".join(missing))

    time.sleep(2.0)
    for _ in range(command_attempts):
        events_text = sanitize_file(cli_events_log)
        channels_text = sanitize_file(cli_channels_log)
        if (
            events_text.count("OpCodeDispatched") >= 2
            and events_text.count("OpCodeCompleted") >= 2
            and "GROUND_LINK_TX_BYTES" in channels_text
        ):
            break
        run_authenticated_command(
            "epsBridge.EPS_SET_PDU",
            args=struct.pack(">BB", 2, FW_SERIALIZE_TRUE_VALUE),
        )
        time.sleep(1.0)
        run_authenticated_command(
            "adcsBridge.ADCS_SET_MODE",
            args=struct.pack(">B", ADCS_MODE_POINTING),
        )
        time.sleep(2.0)

    events_text = sanitize_file(cli_events_log)
    channels_text = sanitize_file(cli_channels_log)
    if events_text.count("OpCodeDispatched") < 2 or events_text.count("OpCodeCompleted") < 2:
        raise RuntimeError("TT&C prerequisite did not observe command events through fprime-cli events")
    if "GROUND_LINK_TX_BYTES" not in channels_text:
        raise RuntimeError("TT&C prerequisite did not observe GROUND_LINK_TX_BYTES through fprime-cli channels")


processes: list[ManagedProcess] = []
selected_slots = []
matched_source_path = None
matched_source_snapshot = None
matched_received_path = None
matched_received_hash = ""
matched_received_size = 0
capture_attempts_used = 0
gateway_serial = ""
comm_serial = ""


def cleanup_processes() -> None:
    cleanup_managed_processes(processes, timeout_sec=5.0)
    cleanup_remote_comm_node()


install_signal_cleanup(cleanup_processes)

try:
    with open(ground_gds_log, "w", encoding="utf-8", buffering=1) as gds_handle, \
        open(comm_node_log, "w", encoding="utf-8", buffering=1) as comm_handle, \
        open(gateway_log, "w", encoding="utf-8", buffering=1) as gateway_handle, \
        open(remote_comm_node_log, "w", encoding="utf-8", buffering=1) as remote_comm_handle, \
        open(obc_log, "w", encoding="utf-8", buffering=1) as obc_handle, \
        open(cli_events_log, "w", encoding="utf-8", buffering=1) as events_handle, \
        open(cli_channels_log, "w", encoding="utf-8", buffering=1) as channels_handle:

        if link_mode == "hosted-pty":
            with open(pty_bridge_log, "w", encoding="utf-8", buffering=1) as pty_handle:
                pty_bridge = subprocess.Popen(
                    [os.path.join(bin_dir, "pty_pair_bridge")],
                    stdin=subprocess.DEVNULL,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    start_new_session=True,
                )
                processes.append(ManagedProcess("pty_pair_bridge", pty_bridge, pty_handle, (), ()))
                serial_paths = {}
                for line in read_pty_bridge_lines(pty_bridge, pty_handle, pty_path_timeout_sec):
                    key, value = line.strip().split("=", 1)
                    serial_paths[key] = value
            gateway_serial = serial_paths["PTY_A"]
            comm_serial = serial_paths["PTY_B"]
        elif link_mode == "physical-serial":
            gateway_serial = host_serial_device
            comm_serial = subsystem_serial_device

        ground_env = os.environ.copy()
        ground_env["GDS_PORT"] = gds_port
        ground_env["GDS_TTS_PORT"] = gds_tts_port
        ground_env["GDS_FILE_STORAGE_DIR"] = gds_file_storage_dir
        ground_env["DICT_PATH"] = dict_path
        ground_env["GDS_FRAMING_SELECTION"] = gds_framing_selection
        if gds_scid:
            ground_env["GDS_SCID"] = gds_scid
        if gds_vcid:
            ground_env["GDS_VCID"] = gds_vcid
        if gds_frame_size:
            ground_env["GDS_FRAME_SIZE"] = gds_frame_size
        ground_gds = start_process(
            "ground_gds",
            ["bash", os.path.join(root_dir, "scripts/run_ground_gds_only_stack.sh")],
            env=ground_env,
            handle=gds_handle,
            stale_match_groups=build_gds_stale_match_groups(
                ip_port=gds_port,
                tts_port=gds_tts_port,
                file_storage_dir=gds_file_storage_dir,
            ),
            stale_match_markers=GDS_PROCESS_MARKERS,
        )

        runtime_env = os.environ.copy()
        runtime_env["CSP_HUB_HOST"] = local_csp_hub_host
        runtime_env["CSP_HUB_SUB_PORT"] = csp_hub_sub_port
        runtime_env["CSP_HUB_PUB_PORT"] = csp_hub_pub_port
        runtime_env["EPS_CSP_NODE_ID"] = eps_csp_node_id
        runtime_env["ADCS_CSP_NODE_ID"] = adcs_csp_node_id
        runtime_env["COMM_CSP_NODE"] = comm_csp_node

        if link_mode == "hosted-pty":
            comm_node = start_process(
                "comm_csp_node",
                [
                    os.path.join(bin_dir, "comm_csp_node"),
                    "--serial-device",
                    comm_serial,
                    "--baudrate",
                    baudrate,
                    "--node-id",
                    comm_csp_node,
                ],
                env=runtime_env,
                handle=comm_handle,
            )
            gateway = start_process(
                "ground_ttc_gateway",
                [
                    os.path.join(bin_dir, "ground_ttc_gateway"),
                    "--serial-device",
                    gateway_serial,
                    "--baudrate",
                    baudrate,
                    "--gds-host",
                    "127.0.0.1",
                    "--gds-port",
                    gds_port,
                    *gateway_capture_args(),
                ],
                env=runtime_env,
                handle=gateway_handle,
            )
        elif link_mode == "hosted-sband-tcp":
            comm_node = start_process(
                "sband_comm_csp_node",
                [
                    os.path.join(bin_dir, "sband_comm_csp_node"),
                    "--tcp-listen-host",
                    sband_tcp_host,
                    "--tcp-listen-port",
                    sband_tcp_port,
                    "--node-id",
                    comm_csp_node,
                ],
                env=runtime_env,
                handle=comm_handle,
            )
            gateway = start_process(
                "ground_ttc_gateway",
                [
                    os.path.join(bin_dir, "ground_ttc_gateway"),
                    "--rf-tcp-host",
                    sband_tcp_host,
                    "--rf-tcp-port",
                    sband_tcp_port,
                    "--link-identity",
                    "sband",
                    "--gds-host",
                    "127.0.0.1",
                    "--gds-port",
                    gds_port,
                    *gateway_capture_args(),
                ],
                env=runtime_env,
                handle=gateway_handle,
            )
        else:
            # Physical mode needs the hosted OBC proxy reachable from subsystem.local.
            runtime_env["CSP_PROXY_BIND_HOST"] = "0.0.0.0"

        obc_env = runtime_env.copy()
        obc_env["RADIO_PORT"] = radio_port
        obc_env["GDS_PORT"] = "0"
        obc_env["RUNTIME_ROOT"] = runtime_root
        obc_env["GROUND_LINK_MODE"] = "comm-csp"
        obc_env["COMM_CSP_NODE"] = comm_csp_node
        obc_env["CSP_MANAGE_PROXY"] = "1"
        obc_env["OBC_BINARY_NAME"] = obc_binary_name
        obc_env["MANAGE_SBAND_COMM_NODE"] = "0"
        obc_env["MANAGE_GROUND_TTC_GATEWAY"] = "0"
        obc = start_process(
            "run_dev_stack",
            ["bash", os.path.join(root_dir, "scripts/run_dev_stack.sh")],
            env=obc_env,
            handle=obc_handle,
            stdin=subprocess.PIPE,
        )

        if link_mode == "physical-serial":
            time.sleep(2.0)
            remote_comm = start_remote_comm_node(remote_comm_handle)

            gateway_args = [
                os.path.join(bin_dir, "ground_ttc_gateway"),
                "--serial-device",
                gateway_serial,
                "--baudrate",
                baudrate,
                "--gds-host",
                "127.0.0.1",
                "--gds-port",
                gds_port,
            ]
            gateway_args.extend(
                [
                    "--serial-tx-preamble-lines",
                    lab_serial_tx_preamble_lines,
                    "--serial-tx-preamble-delay-ms",
                    lab_serial_tx_preamble_delay_ms,
                ]
            )
            gateway_args.extend(gateway_capture_args())
            gateway = start_process("ground_ttc_gateway", gateway_args, env=runtime_env, handle=gateway_handle)

        startup_delay = float(lab_serial_startup_delay_sec) if link_mode == "physical-serial" else 5.0
        time.sleep(startup_delay)
        for managed in processes:
            require_running(managed.process, managed.name)

        events_listener = start_cli_listener(events_handle, "events", "--search", "OpCode")
        channels_listener = start_cli_listener(channels_handle, "channels", "--search", "GROUND_LINK_TX_BYTES")
        time.sleep(1.0)
        wait_for_log_fragment(cli_channels_log, "GROUND_LINK_TX_BYTES", 8.0)

        send_obc_command(obc, "status", 0.5)
        ensure_ttc_prerequisite(obc)
        run_authenticated_command(
            "hkTrendProductProducer.HK_TREND_FLUSH",
            expected_fragment="HK_TREND_PRODUCT_WRITTEN",
            timeout_sec=max(file_start_timeout_sec, 8.0),
        )

        selected_slots, capture_attempts_used = select_data_product_files()
        matched_entry, matched_received_path, matched_received_hash, matched_received_size = downlink_data_product(
            selected_slots,
            "source-dp",
        )
        matched_source_path = str(matched_entry["source_path"])
        matched_source_snapshot = str(matched_entry["snapshot_path"])

        send_obc_command(obc, "quit", 0.0)
        try:
            obc.wait(timeout=20.0)
        except subprocess.TimeoutExpired:
            pass
finally:
    cleanup_processes()

obc_text = sanitize_file(obc_log)
events_text = sanitize_file(cli_events_log)
channels_text = sanitize_file(cli_channels_log)
gateway_text = sanitize_file(gateway_log)

if (gateway_capture_gds_to_southbound or gateway_capture_southbound_to_gds) and "capture-write-error" in gateway_text:
    print("COMM TT&C file/downlink probe observed a gateway capture write error.", file=sys.stderr)
    print(f"gateway-log={gateway_log}", file=sys.stderr)
    raise SystemExit(1)

print("comm-ttc-file-downlink-probe: PASS")
print("formal-verdict=file-downlink")
print(f"link-mode={link_mode}")
print(f"obc-binary={obc_binary_name}")
print(f"command-prefix={command_prefix}")
print(f"framing={gds_framing_selection}")
if gds_scid:
    print(f"scid={gds_scid}")
if gds_vcid:
    print(f"vcid={gds_vcid}")
if gds_frame_size:
    print(f"frame-size={gds_frame_size}")
print("stage0-ttc-prerequisite=PASS")
print(f"source-product-sample-limit={target_occupied_slots}")
print(f"source-product-wait-attempts={capture_attempts_used}")
print(f"source-product-observed-count={len(selected_slots)}")
print(
    f"downlinked-product=PASS source={matched_source_path} "
    f"source-snapshot={matched_source_snapshot} "
    f"received={matched_received_path} bytes={matched_received_size} sha256={matched_received_hash}"
)
if link_mode == "hosted-sband-tcp":
    print(f"sband-tcp-endpoint={sband_tcp_host}:{sband_tcp_port}")
else:
    print(f"host-serial-device={gateway_serial}")
    print(f"subsystem-serial-device={comm_serial}")
print(f"baudrate={baudrate}")
print(f"comm-node={comm_csp_node}")
print(f"local-csp-hub-host={local_csp_hub_host}")
print(f"remote-csp-hub-host={remote_csp_hub_host if link_mode == 'physical-serial' else local_csp_hub_host}")
print(f"csp-hub-sub-port={csp_hub_sub_port}")
print(f"csp-hub-pub-port={csp_hub_pub_port}")
print(f"gds-port={gds_port}")
print(f"gds-tts-port={gds_tts_port}")
print(f"gds-file-storage-dir={gds_file_storage_dir}")
print(f"runtime-root={runtime_root}")
print(f"lab-serial-tx-preamble-lines={lab_serial_tx_preamble_lines if link_mode == 'physical-serial' else 0}")
print(f"lab-serial-tx-preamble-delay-ms={lab_serial_tx_preamble_delay_ms if link_mode == 'physical-serial' else 0}")
print(f"lab-serial-startup-delay-sec={lab_serial_startup_delay_sec if link_mode == 'physical-serial' else 5}")
print(f"lab-serial-command-attempts={lab_serial_command_attempts}")
print(f"file-downlink-command-attempts={file_downlink_command_attempts}")
print(f"file-downlink-start-timeout-sec={file_downlink_start_timeout_sec}")
print(f"file-downlink-timeout-sec={file_downlink_timeout_sec}")
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
