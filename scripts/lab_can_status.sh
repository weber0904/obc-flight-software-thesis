#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
OBC_CAN_SERVICE_NAME="${OBC_CAN_SERVICE_NAME:-obc-lab-can.service}"
SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME="${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME:-subsystem-eps-adcs-lab-can.service}"
SUBSYSTEM_COMM_CAN_SERVICE_NAME="${SUBSYSTEM_COMM_CAN_SERVICE_NAME:-subsystem-comm-lab-can.service}"
OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_COMM_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE:-can1}"

obc_require_systemd_service_name "${OBC_CAN_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}"
obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"

remote_status() {
  local ssh_target="${1:?ssh_target is required}"
  local service_name="${2:?service_name is required}"
  local can_device="${3:?can_device is required}"

  ssh "${ssh_target}" "set -euo pipefail
printf '=== %s ===\n' $(printf '%q' "${service_name}")
sudo systemctl --no-pager --full status $(printf '%q' "${service_name}") || true
printf '\n=== %s link ===\n' $(printf '%q' "${can_device}")
ip -details -statistics link show dev $(printf '%q' "${can_device}") || true"
}

remote_status "${OBC_SSH_TARGET}" "${OBC_CAN_SERVICE_NAME}" "${OBC_CSP_CAN_DEVICE}"
remote_status "${SUBSYSTEM_SIM_SSH_TARGET}" "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}" "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
remote_status "${SUBSYSTEM_SIM_SSH_TARGET}" "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}" "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
