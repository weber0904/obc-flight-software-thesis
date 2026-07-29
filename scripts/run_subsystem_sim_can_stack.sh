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
SUBSYSTEM_SIM_RESERVED_CAN_DEVICE="${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE:-}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"

if [[ -z "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" || -z "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_CSP_CAN_DEVICE and SUBSYSTEM_SIM_RESERVED_CAN_DEVICE are required." >&2
  exit 1
fi
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}"
if [[ "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" == "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" ]]; then
  echo "Primary and reserved subsystem CAN devices must differ." >&2
  exit 1
fi
if [[ "${CSP_CAN_PROMISC}" != "0" && "${CSP_CAN_PROMISC}" != "1" ]]; then
  echo "CSP_CAN_PROMISC must be 0 or 1." >&2
  exit 1
fi
if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi

PRIMARY_PARENTDEV="$(obc_can_parentdev "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" || true)"
RESERVED_PARENTDEV="$(obc_can_parentdev "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" || true)"
if [[ -z "${PRIMARY_PARENTDEV}" || -z "${RESERVED_PARENTDEV}" ]]; then
  echo "Unable to resolve CAN parentdev mapping. Ensure both subsystem CAN devices exist and are up." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" == "1" ]]; then
  PIDS="$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(eps_simulator|adcs_simulator)' || true)"
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

echo "Starting subsystem CAN simulator stack"
echo "  Execution host : subsystem-sim"
echo "  Active role    : EPS/ADCS on shared SocketCAN bus"
echo "  Transport      : ${CSP_TRANSPORT}"
echo "  Primary CAN    : ${SUBSYSTEM_SIM_CSP_CAN_DEVICE} (${PRIMARY_PARENTDEV})"
echo "  Reserved CAN   : ${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE} (${RESERVED_PARENTDEV})"
echo "  Promisc        : ${CSP_CAN_PROMISC}"
echo "  EPS node       : ${EPS_CSP_NODE_ID}"
echo "  ADCS node      : ${ADCS_CSP_NODE_ID}"

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &

wait
