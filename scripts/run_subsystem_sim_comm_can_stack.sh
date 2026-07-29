#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-socketcan}")"
if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "Only CSP_TRANSPORT=socketcan is supported by this stack launcher." >&2
  exit 1
fi

SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-}"
SUBSYSTEM_SIM_COMM_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE:-}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"

if [[ -z "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_CSP_CAN_DEVICE and SUBSYSTEM_SIM_COMM_CAN_DEVICE are required." >&2
  exit 1
fi
if [[ -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_COMM_DEVICE is required." >&2
  exit 1
fi
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
if [[ "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" == "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" ]]; then
  echo "EPS/ADCS and COMM CAN devices must differ." >&2
  exit 1
fi
if [[ "${CSP_CAN_PROMISC}" != "0" && "${CSP_CAN_PROMISC}" != "1" ]]; then
  echo "CSP_CAN_PROMISC must be 0 or 1." >&2
  exit 1
fi
if [[ ! "${COMM_CSP_NODE}" =~ ^[0-9]+$ || "${COMM_CSP_NODE}" == "0" ]]; then
  echo "COMM_CSP_NODE must be a positive integer." >&2
  exit 1
fi
if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi

EPS_ADCS_PARENTDEV="$(obc_can_parentdev "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" || true)"
COMM_PARENTDEV="$(obc_can_parentdev "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" || true)"
if [[ -z "${EPS_ADCS_PARENTDEV}" || -z "${COMM_PARENTDEV}" ]]; then
  echo "Unable to resolve CAN parentdev mapping. Ensure both subsystem CAN devices exist." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" == "1" ]]; then
  PIDS="$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(eps_simulator|adcs_simulator|comm_csp_node)' || true)"
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
trap cleanup EXIT INT TERM

echo "Starting subsystem CAN simulator stack with COMM"
echo "  Execution host : subsystem-sim"
echo "  EPS/ADCS role  : shared SocketCAN bus"
echo "  COMM role      : SocketCAN + lab serial ingress"
echo "  Transport      : ${CSP_TRANSPORT}"
echo "  EPS/ADCS CAN   : ${SUBSYSTEM_SIM_CSP_CAN_DEVICE} (${EPS_ADCS_PARENTDEV})"
echo "  COMM CAN       : ${SUBSYSTEM_SIM_COMM_CAN_DEVICE} (${COMM_PARENTDEV})"
echo "  COMM serial    : ${SUBSYSTEM_SIM_COMM_DEVICE} @ ${COMM_BAUDRATE}"
echo "  Promisc        : ${CSP_CAN_PROMISC}"
echo "  EPS node       : ${EPS_CSP_NODE_ID}"
echo "  ADCS node      : ${ADCS_CSP_NODE_ID}"
echo "  COMM node      : ${COMM_CSP_NODE}"

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" \
CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
"${BIN_DIR}/comm_csp_node" \
  --serial-device "${SUBSYSTEM_SIM_COMM_DEVICE}" \
  --baudrate "${COMM_BAUDRATE}" \
  --node-id "${COMM_CSP_NODE}" &

wait
