#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

ROLE="${1:-${SUBSYSTEM_SERVICE_ROLE:-}}"
BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found on subsystem host. Run scripts/bootstrap_subsystem_sim_workspace.sh first." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-socketcan}")"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD:-0}"
COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST:-}"
COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST:-}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_COMM_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE:-can1}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS:-0}"
COMM_NODE_STRIP_TC_FILL_PATTERN="${COMM_NODE_STRIP_TC_FILL_PATTERN:-0}"
SUBSYSTEM_SIM_COMM_BEACON_DEVICE="${SUBSYSTEM_SIM_COMM_BEACON_DEVICE:-}"
COMM_BEACON_BAUDRATE="${COMM_BEACON_BAUDRATE:-${COMM_BAUDRATE}}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-127.0.0.1}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
EPS_SIM_CONTROL_SOCKET="${EPS_SIM_CONTROL_SOCKET:-}"
ADCS_SIM_CONTROL_SOCKET="${ADCS_SIM_CONTROL_SOCKET:-}"

if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "Subsystem lab services require CSP_TRANSPORT=socketcan." >&2
  exit 1
fi
if [[ "${CSP_CAN_PROMISC}" != "0" && "${CSP_CAN_PROMISC}" != "1" ]]; then
  echo "CSP_CAN_PROMISC must be 0 or 1." >&2
  exit 1
fi
if [[ "${COMM_CSP_SOCKETCAN_USE_CANFD}" != "0" && "${COMM_CSP_SOCKETCAN_USE_CANFD}" != "1" ]]; then
  echo "COMM_CSP_SOCKETCAN_USE_CANFD must be 0 or 1." >&2
  exit 1
fi

require_executable() {
  local executable_path="${1:?executable_path is required}"
  local label="${2:?label is required}"

  if [[ ! -x "${executable_path}" ]]; then
    echo "${label} executable not found at ${executable_path}. Run scripts/bootstrap_subsystem_sim_workspace.sh first." >&2
    exit 1
  fi
}

case "${ROLE}" in
  eps)
    obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    require_executable "${BIN_DIR}/eps_simulator" "EPS simulator"
    echo "Starting subsystem EPS node ${EPS_CSP_NODE_ID} on ${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    EPS_ARGS=(--node-id "${EPS_CSP_NODE_ID}")
    if [[ -n "${EPS_SIM_CONTROL_SOCKET}" ]]; then
      echo "  Control socket : ${EPS_SIM_CONTROL_SOCKET}"
      EPS_ARGS+=(--control-socket "${EPS_SIM_CONTROL_SOCKET}")
    fi
    exec env \
      CSP_TRANSPORT="${CSP_TRANSPORT}" \
      CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
      CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
      COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD}" \
      COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}" \
      COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}" \
      "${BIN_DIR}/eps_simulator" "${EPS_ARGS[@]}"
    ;;
  adcs)
    obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    require_executable "${BIN_DIR}/adcs_simulator" "ADCS simulator"
    echo "Starting subsystem ADCS node ${ADCS_CSP_NODE_ID} on ${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    ADCS_ARGS=(--node-id "${ADCS_CSP_NODE_ID}")
    if [[ -n "${ADCS_SIM_CONTROL_SOCKET}" ]]; then
      echo "  Control socket : ${ADCS_SIM_CONTROL_SOCKET}"
      ADCS_ARGS+=(--control-socket "${ADCS_SIM_CONTROL_SOCKET}")
    fi
    exec env \
      CSP_TRANSPORT="${CSP_TRANSPORT}" \
      CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
      CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
      COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD}" \
      COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}" \
      COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}" \
      "${BIN_DIR}/adcs_simulator" "${ADCS_ARGS[@]}"
    ;;
  comm|uhf)
    obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
    if [[ -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
      echo "SUBSYSTEM_SIM_COMM_DEVICE is required for COMM." >&2
      exit 1
    fi
    comm_label="COMM"
    comm_exec="comm_csp_node"
    if [[ "${ROLE}" == "uhf" ]]; then
      comm_label="UHF COMM"
      comm_exec="uhf_comm_csp_node"
    fi
    require_executable "${BIN_DIR}/${comm_exec}" "${comm_label} CSP node"
    echo "Starting subsystem ${comm_label} node ${COMM_CSP_NODE} on ${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
    echo "  Serial ingress: ${SUBSYSTEM_SIM_COMM_DEVICE} @ ${COMM_BAUDRATE}"
    COMM_ARGS=(
      --serial-device "${SUBSYSTEM_SIM_COMM_DEVICE}"
      --baudrate "${COMM_BAUDRATE}"
      --node-id "${COMM_CSP_NODE}"
    )
    if [[ "${ROLE}" == "uhf" && -n "${SUBSYSTEM_SIM_COMM_BEACON_DEVICE}" ]]; then
      echo "  Beacon egress : ${SUBSYSTEM_SIM_COMM_BEACON_DEVICE} @ ${COMM_BEACON_BAUDRATE}"
      COMM_ARGS+=(
        --beacon-serial-device "${SUBSYSTEM_SIM_COMM_BEACON_DEVICE}"
        --beacon-baudrate "${COMM_BEACON_BAUDRATE}"
      )
    fi
    exec env \
      CSP_TRANSPORT="${CSP_TRANSPORT}" \
      CSP_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" \
      CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
      COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD}" \
      COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}" \
      COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}" \
      COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS}" \
      COMM_NODE_STRIP_TC_FILL_PATTERN="${COMM_NODE_STRIP_TC_FILL_PATTERN}" \
      "${BIN_DIR}/${comm_exec}" \
      "${COMM_ARGS[@]}"
    ;;
  sband)
    obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
    require_executable "${BIN_DIR}/sband_comm_csp_node" "S-band COMM CSP node"
    echo "Starting subsystem S-band COMM node ${COMM_CSP_NODE} on ${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
    echo "  TCP listener  : ${SBAND_TCP_HOST}:${SBAND_TCP_PORT}"
    exec env \
      CSP_TRANSPORT="${CSP_TRANSPORT}" \
      CSP_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" \
      CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
      COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD}" \
      COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}" \
      COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}" \
      COMM_NODE_INGRESS_DIAGNOSTICS="${COMM_NODE_INGRESS_DIAGNOSTICS}" \
      COMM_NODE_STRIP_TC_FILL_PATTERN="${COMM_NODE_STRIP_TC_FILL_PATTERN}" \
      "${BIN_DIR}/sband_comm_csp_node" \
        --tcp-listen-host "${SBAND_TCP_HOST}" \
        --tcp-listen-port "${SBAND_TCP_PORT}" \
        --node-id "${COMM_CSP_NODE}"
    ;;
  *)
    echo "Usage: $0 eps|adcs|comm|sband|uhf" >&2
    exit 1
    ;;
esac
