#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found on subsystem host. Run bootstrap/build first." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-zmqhub}")"
if [[ "${CSP_TRANSPORT}" != "zmqhub" ]]; then
  echo "Target TCP parity subsystem stack requires CSP_TRANSPORT=zmqhub." >&2
  exit 1
fi

CSP_HUB_HOST="${CSP_HUB_HOST:-${REMOTE_HOST:-}}"
if [[ -z "${CSP_HUB_HOST}" ]]; then
  echo "CSP_HUB_HOST or REMOTE_HOST is required." >&2
  exit 1
fi

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56630}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57630}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
SBAND_COMM_NODE_ID="${SBAND_COMM_NODE_ID:-5}"
UHF_COMM_NODE_ID="${UHF_COMM_NODE_ID:-6}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-0.0.0.0}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
UHF_TCP_HOST="${UHF_TCP_HOST:-0.0.0.0}"
UHF_TCP_PORT="${UHF_TCP_PORT:-18620}"
UHF_SOUTHBOUND_MODE="${UHF_SOUTHBOUND_MODE:-tcp-listen}"
UHF_BAUDRATE="${UHF_BAUDRATE:-115200}"
COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS:-0}"
COMM_NODE_STRIP_TC_FILL_PATTERN="${COMM_NODE_STRIP_TC_FILL_PATTERN:-0}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"

if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi
if [[ "${UHF_SOUTHBOUND_MODE}" != "tcp-listen" && "${UHF_SOUTHBOUND_MODE}" != "serial-pty" ]]; then
  echo "UHF_SOUTHBOUND_MODE must be tcp-listen or serial-pty." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" == "1" ]]; then
  PIDS="$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\/bin\/Linux\/(eps_simulator|adcs_simulator|sband_comm_csp_node|uhf_comm_csp_node|pty_pair_bridge)/ {print $1}')"
  if [[ -n "${PIDS}" ]]; then
    kill ${PIDS} >/dev/null 2>&1 || true
    sleep 1
  fi
fi

cleanup() {
  local status=$?
  jobs -p | xargs -r kill >/dev/null 2>&1 || true
  wait || true
  exit "${status}"
}
trap cleanup EXIT HUP INT TERM

COMMON_ENV=(
  CSP_TRANSPORT="${CSP_TRANSPORT}"
  CSP_HUB_HOST="${CSP_HUB_HOST}"
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}"
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}"
  COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS}"
  COMM_NODE_STRIP_TC_FILL_PATTERN="${COMM_NODE_STRIP_TC_FILL_PATTERN}"
)

echo "Starting target TCP parity subsystem stack"
echo "  Execution host : subsystem.local"
echo "  CSP hub        : ${CSP_HUB_HOST}:${CSP_HUB_SUB_PORT}/${CSP_HUB_PUB_PORT}"
echo "  EPS node       : ${EPS_CSP_NODE_ID}"
echo "  ADCS node      : ${ADCS_CSP_NODE_ID}"
echo "  S-band node    : ${SBAND_COMM_NODE_ID} tcp=${SBAND_TCP_HOST}:${SBAND_TCP_PORT}"
if [[ "${UHF_SOUTHBOUND_MODE}" == "tcp-listen" ]]; then
  echo "  UHF node       : ${UHF_COMM_NODE_ID} tcp=${UHF_TCP_HOST}:${UHF_TCP_PORT}"
else
  echo "  UHF node       : ${UHF_COMM_NODE_ID} serial-pty @ ${UHF_BAUDRATE}"
fi

env "${COMMON_ENV[@]}" "${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &
env "${COMMON_ENV[@]}" "${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &
env "${COMMON_ENV[@]}" "${BIN_DIR}/sband_comm_csp_node" \
  --tcp-listen-host "${SBAND_TCP_HOST}" \
  --tcp-listen-port "${SBAND_TCP_PORT}" \
  --node-id "${SBAND_COMM_NODE_ID}" &
if [[ "${UHF_SOUTHBOUND_MODE}" == "tcp-listen" ]]; then
  env "${COMMON_ENV[@]}" "${BIN_DIR}/uhf_comm_csp_node" \
    --tcp-listen-host "${UHF_TCP_HOST}" \
    --tcp-listen-port "${UHF_TCP_PORT}" \
    --node-id "${UHF_COMM_NODE_ID}" &
else
  if [[ ! -x "${BIN_DIR}/pty_pair_bridge" ]]; then
    echo "pty_pair_bridge not found on subsystem host. Run bootstrap/build first." >&2
    exit 1
  fi
  UHF_PTY_LOG="/tmp/target-tcp-uhf-pty-${UHF_COMM_NODE_ID}.log"
  : > "${UHF_PTY_LOG}"
  "${BIN_DIR}/pty_pair_bridge" >>"${UHF_PTY_LOG}" 2>&1 &
  for _ in $(seq 1 50); do
    if [[ "$(wc -l < "${UHF_PTY_LOG}")" -ge 2 ]]; then
      break
    fi
    sleep 0.1
  done
  UHF_BRIDGE_LINE="$(sed -n '1p' "${UHF_PTY_LOG}")"
  UHF_NODE_LINE="$(sed -n '2p' "${UHF_PTY_LOG}")"
  UHF_BRIDGE_SERIAL="$(printf '%s\n' "${UHF_BRIDGE_LINE}" | awk -F= '/^PTY_A=/{print $2}')"
  UHF_NODE_SERIAL="$(printf '%s\n' "${UHF_NODE_LINE}" | awk -F= '/^PTY_B=/{print $2}')"
  if [[ -z "${UHF_BRIDGE_SERIAL}" || -z "${UHF_NODE_SERIAL}" ]]; then
    echo "pty_pair_bridge did not report PTY_A/PTY_B paths." >&2
    exit 1
  fi
  echo "UHF_BRIDGE_SERIAL=${UHF_BRIDGE_SERIAL}"
  echo "UHF_NODE_SERIAL=${UHF_NODE_SERIAL}"
  env "${COMMON_ENV[@]}" "${BIN_DIR}/uhf_comm_csp_node" \
    --serial-device "${UHF_NODE_SERIAL}" \
    --baudrate "${UHF_BAUDRATE}" \
    --node-id "${UHF_COMM_NODE_ID}" &
fi

wait
