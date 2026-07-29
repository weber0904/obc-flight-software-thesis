#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
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

FPRIME_GDS_BIN="$(find_local_tool fprime-gds || true)"
if [[ -z "${FPRIME_GDS_BIN}" ]]; then
  echo "fprime-gds not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi
if [[ ! -x "${BIN_DIR}/ground_ttc_gateway" ]]; then
  echo "ground_ttc_gateway not found at ${BIN_DIR}/ground_ttc_gateway. Run fprime-util build first." >&2
  exit 1
fi

GDS_BIND_HOST="${GDS_BIND_HOST:-0.0.0.0}"
GDS_GATEWAY_HOST="${GDS_GATEWAY_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-51900}"
GDS_TTS_PORT="${GDS_TTS_PORT:-51901}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/target-obc-comm-csp-lab-gds-downlink}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-$(obc_resolve_subsystem_sim_host)}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
CCSDS_SCID="${CCSDS_SCID:-68}"
CCSDS_VCID="${CCSDS_VCID:-}"
CCSDS_FRAME_SIZE="${CCSDS_FRAME_SIZE:-4096}"
LAB_SERIAL_TX_PREAMBLE_LINES="${LAB_SERIAL_TX_PREAMBLE_LINES:-0}"
LAB_SERIAL_TX_PREAMBLE_DELAY_MS="${LAB_SERIAL_TX_PREAMBLE_DELAY_MS:-0}"
GDS_KEEPALIVE_INTERVAL="${GDS_KEEPALIVE_INTERVAL:-0}"
GDS_STARTUP_TIMEOUT_SEC="${GDS_STARTUP_TIMEOUT_SEC:-20}"
TARGET_COMM_CSP_GROUND_LOG_DIR="${TARGET_COMM_CSP_GROUND_LOG_DIR:-${ROOT_DIR}/build-artifacts/target-obc-comm-csp-ground-stack}"
GDS_UI_MODE="${GDS_UI_MODE:-headless}"
GDS_GUI_ADDR="${GDS_GUI_ADDR:-127.0.0.1}"
GDS_GUI_PORT="${GDS_GUI_PORT:-5000}"
GROUND_TTC_GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND="${GROUND_TTC_GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND:-}"
GROUND_TTC_GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS="${GROUND_TTC_GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS:-}"

obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"
if [[ -z "${CCSDS_VCID}" ]]; then
  CCSDS_VCID="$(obc_target_comm_profile_ccsds_vcid "${TARGET_COMM_PROFILE}")"
fi

if ! [[ "${GDS_PORT}" =~ ^[1-9][0-9]*$ && \
  "${GDS_TTS_PORT}" =~ ^[1-9][0-9]*$ && \
  "${COMM_BAUDRATE}" =~ ^[1-9][0-9]*$ && \
  "${SBAND_TCP_PORT}" =~ ^[1-9][0-9]*$ && \
  "${CCSDS_SCID}" =~ ^[0-9]+$ && \
  "${CCSDS_VCID}" =~ ^[0-9]+$ && \
  "${CCSDS_FRAME_SIZE}" =~ ^[1-9][0-9]*$ && \
  "${GDS_KEEPALIVE_INTERVAL}" =~ ^[0-9]+([.][0-9]+)?$ && \
  "${LAB_SERIAL_TX_PREAMBLE_LINES}" =~ ^[0-9]+$ && \
  "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}" =~ ^[0-9]+$ && \
  "${GDS_STARTUP_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ ]]; then
  echo "GDS ports, baudrate, CCSDS settings, keepalive interval, preamble settings, and startup timeout must be numeric." >&2
  exit 1
fi
case "${GDS_UI_MODE}" in
  ui|headless)
    ;;
  *)
    echo "GDS_UI_MODE must be ui or headless." >&2
    exit 1
    ;;
esac
GROUND_PATH_KIND="$(obc_target_comm_profile_ground_path_kind "${TARGET_COMM_PROFILE}")"
if [[ "${GROUND_PATH_KIND}" == "serial" && ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial endpoint not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi

mkdir -p "${GDS_FILE_STORAGE_DIR}" "${TARGET_COMM_CSP_GROUND_LOG_DIR}"

GDS_LOG="${TARGET_COMM_CSP_GROUND_LOG_DIR}/fprime-gds.log"
GATEWAY_LOG="${TARGET_COMM_CSP_GROUND_LOG_DIR}/ground-ttc-gateway.log"

GDS_PID=""
GATEWAY_PID=""

cleanup() {
  if [[ -n "${GATEWAY_PID}" ]]; then
    kill "${GATEWAY_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${GDS_PID}" ]]; then
    kill "${GDS_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

wait_for_tcp_port() {
  local host="${1:?host is required}"
  local port="${2:?port is required}"
  local timeout_sec="${3:?timeout_sec is required}"

  python3 - "${host}" "${port}" "${timeout_sec}" <<'PY'
import socket
import sys
import time

host = sys.argv[1]
port = int(sys.argv[2])
deadline = time.monotonic() + float(sys.argv[3])

while time.monotonic() < deadline:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.5)
        try:
            sock.connect((host, port))
            sys.exit(0)
        except OSError:
            time.sleep(0.25)

sys.exit(1)
PY
}

printf 'Starting target COMM CSP lab ground stack\n'
printf '  GDS bind            : %s:%s\n' "${GDS_BIND_HOST}" "${GDS_PORT}"
printf '  GDS TTS port        : %s\n' "${GDS_TTS_PORT}"
printf '  GDS file storage    : %s\n' "${GDS_FILE_STORAGE_DIR}"
printf '  Target COMM profile : %s (%s)\n' "${TARGET_COMM_PROFILE}" "${GROUND_PATH_KIND}"
printf '  Gateway GDS target  : %s:%s\n' "${GDS_GATEWAY_HOST}" "${GDS_PORT}"
if [[ "${GROUND_PATH_KIND}" == "serial" ]]; then
  printf '  Host serial endpoint: %s @ %s\n' "${HOST_SERIAL_DEVICE}" "${COMM_BAUDRATE}"
else
  printf '  S-band TCP target   : %s:%s\n' "${SBAND_TCP_HOST}" "${SBAND_TCP_PORT}"
fi
printf '  CCSDS framing       : scid=%s vcid=%s frame-size=%s\n' "${CCSDS_SCID}" "${CCSDS_VCID}" "${CCSDS_FRAME_SIZE}"
printf '  GDS UI mode         : %s\n' "${GDS_UI_MODE}"
printf '  GDS GUI             : %s:%s\n' "${GDS_GUI_ADDR}" "${GDS_GUI_PORT}"
printf '  GDS keepalive       : %s\n' "${GDS_KEEPALIVE_INTERVAL}"
printf '  Serial preamble     : lines=%s delay-ms=%s\n' "${LAB_SERIAL_TX_PREAMBLE_LINES}" "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}"
printf '  Dictionary          : %s\n' "${DICT_PATH}"
printf '  Logs                : %s\n' "${TARGET_COMM_CSP_GROUND_LOG_DIR}"

if [[ "${GDS_UI_MODE}" == "headless" ]]; then
  "${FPRIME_GDS_BIN}" \
    -n \
    --gui-addr "${GDS_GUI_ADDR}" \
    --gui-port "${GDS_GUI_PORT}" \
    -g none \
    --no-zmq \
    --framing-selection space-packet-space-data-link \
    --scid "${CCSDS_SCID}" \
    --vcid "${CCSDS_VCID}" \
    --frame-size "${CCSDS_FRAME_SIZE}" \
    --keepalive-interval "${GDS_KEEPALIVE_INTERVAL}" \
    --dictionary "${DICT_PATH}" \
    --ip-address "${GDS_BIND_HOST}" \
    --ip-port "${GDS_PORT}" \
    --tts-port "${GDS_TTS_PORT}" \
    --file-storage-directory "${GDS_FILE_STORAGE_DIR}" \
    --log-to-stdout >"${GDS_LOG}" 2>&1 &
else
  "${FPRIME_GDS_BIN}" \
    -n \
    --gui-addr "${GDS_GUI_ADDR}" \
    --gui-port "${GDS_GUI_PORT}" \
    --no-zmq \
    --framing-selection space-packet-space-data-link \
    --scid "${CCSDS_SCID}" \
    --vcid "${CCSDS_VCID}" \
    --frame-size "${CCSDS_FRAME_SIZE}" \
    --keepalive-interval "${GDS_KEEPALIVE_INTERVAL}" \
    --dictionary "${DICT_PATH}" \
    --ip-address "${GDS_BIND_HOST}" \
    --ip-port "${GDS_PORT}" \
    --tts-port "${GDS_TTS_PORT}" \
    --file-storage-directory "${GDS_FILE_STORAGE_DIR}" \
    --log-to-stdout >"${GDS_LOG}" 2>&1 &
fi
GDS_PID="$!"

if ! wait_for_tcp_port "${GDS_GATEWAY_HOST}" "${GDS_PORT}" "${GDS_STARTUP_TIMEOUT_SEC}"; then
  echo "fprime-gds did not open ${GDS_GATEWAY_HOST}:${GDS_PORT} within ${GDS_STARTUP_TIMEOUT_SEC}s." >&2
  tail -n 80 "${GDS_LOG}" >&2 || true
  exit 1
fi

GATEWAY_ARGS=(
  --gds-host "${GDS_GATEWAY_HOST}"
  --gds-port "${GDS_PORT}"
)
if [[ -n "${GROUND_TTC_GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND}" ]]; then
  GATEWAY_ARGS+=(
    --capture-gds-to-southbound "${GROUND_TTC_GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND}"
  )
fi
if [[ -n "${GROUND_TTC_GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS}" ]]; then
  GATEWAY_ARGS+=(
    --capture-southbound-to-gds "${GROUND_TTC_GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS}"
  )
fi
if [[ "${GROUND_PATH_KIND}" == "serial" ]]; then
  GATEWAY_ARGS+=(
    --serial-device "${HOST_SERIAL_DEVICE}"
    --baudrate "${COMM_BAUDRATE}"
    --link-identity "uhf"
    --serial-tx-preamble-lines "${LAB_SERIAL_TX_PREAMBLE_LINES}"
    --serial-tx-preamble-delay-ms "${LAB_SERIAL_TX_PREAMBLE_DELAY_MS}"
  )
else
  GATEWAY_ARGS+=(
    --rf-tcp-host "${SBAND_TCP_HOST}"
    --rf-tcp-port "${SBAND_TCP_PORT}"
    --link-identity "sband"
  )
fi

"${BIN_DIR}/ground_ttc_gateway" "${GATEWAY_ARGS[@]}" >"${GATEWAY_LOG}" 2>&1 &
GATEWAY_PID="$!"

printf 'Ground stack ready. Press Ctrl-C to stop both processes.\n'

while true; do
  if ! kill -0 "${GDS_PID}" >/dev/null 2>&1; then
    echo "fprime-gds exited. Last log lines:" >&2
    tail -n 80 "${GDS_LOG}" >&2 || true
    exit 1
  fi
  if ! kill -0 "${GATEWAY_PID}" >/dev/null 2>&1; then
    echo "ground_ttc_gateway exited. Last log lines:" >&2
    tail -n 80 "${GATEWAY_LOG}" >&2 || true
    exit 1
  fi
  sleep 1
done
