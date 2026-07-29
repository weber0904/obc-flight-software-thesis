#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

COMM_DEVICE="${COMM_DEVICE:-${SERIAL_DEVICE:-}}"
if [[ $# -ge 1 ]]; then
  COMM_DEVICE="$1"
fi

if [[ -z "${COMM_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_uart_stack.sh /path/to/serial-device" >&2
  echo "Set COMM_DEVICE or SERIAL_DEVICE to the target tty path." >&2
  exit 1
fi

GDS_HOST="${GDS_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-0}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-6100}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-7100}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_MANAGE_PROXY="${CSP_MANAGE_PROXY:-1}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${ROOT_DIR}/runtime/dev-serial}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"

mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}" "${LOG_ROOT}"

export CSP_HUB_HOST
export CSP_HUB_SUB_PORT
export CSP_HUB_PUB_PORT
export CSP_TRANSPORT
export EPS_CSP_NODE_ID
export ADCS_CSP_NODE_ID

cleanup() {
  local status=$?
  jobs -p | xargs -r kill >/dev/null 2>&1 || true
  wait || true
  exit "${status}"
}
trap cleanup EXIT INT TERM

if [[ "${CSP_MANAGE_PROXY}" == "1" ]]; then
  "${BIN_DIR}/csp_zmqproxy" \
    -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
    -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" &
fi
CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &
CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &

sleep 1

"${BIN_DIR}/OBC" \
  --comm serial \
  --comm-device "${COMM_DEVICE}" \
  --comm-baudrate "${COMM_BAUDRATE}" \
  --radio-protocol "${RADIO_PROTOCOL}" \
  --gds-host "${GDS_HOST}" \
  --gds-port "${GDS_PORT}" \
  --runtime-root "${RUNTIME_ROOT}" \
  --persistent-root "${PERSISTENT_ROOT}" \
  --staging-root "${STAGING_ROOT}"
