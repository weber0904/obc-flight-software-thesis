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
CAN_BITRATE="${CAN_BITRATE:-500000}"
CAN_DBITRATE="${CAN_DBITRATE:-2000000}"
CAN_RESTART_MS="${CAN_RESTART_MS:-100}"
START_NOW="${START_NOW:-1}"
UNIT_TEMPLATE="${ROOT_DIR}/packaging/lab/systemd/lab-can.service.template"

obc_require_systemd_service_name "${OBC_CAN_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}"
obc_require_distinct_remote_host_roles "${OBC_SSH_TARGET}" "${SUBSYSTEM_SIM_SSH_TARGET}"
obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"

if [[ ! "${CAN_BITRATE}" =~ ^[0-9]+$ || "${CAN_BITRATE}" == "0" ]]; then
  echo "CAN_BITRATE must be a positive integer." >&2
  exit 1
fi
if [[ ! "${CAN_DBITRATE}" =~ ^[0-9]+$ || "${CAN_DBITRATE}" == "0" ]]; then
  echo "CAN_DBITRATE must be a positive integer." >&2
  exit 1
fi
if [[ ! "${CAN_RESTART_MS}" =~ ^[0-9]+$ ]]; then
  echo "CAN_RESTART_MS must be a non-negative integer." >&2
  exit 1
fi
if [[ "${START_NOW}" != "0" && "${START_NOW}" != "1" ]]; then
  echo "START_NOW must be 0 or 1." >&2
  exit 1
fi
if [[ ! -f "${UNIT_TEMPLATE}" ]]; then
  echo "Service template not found at ${UNIT_TEMPLATE}" >&2
  exit 1
fi

mkdir -p "${ROOT_DIR}/build-artifacts"
TMP_DIR="$(mktemp -d "${ROOT_DIR}/build-artifacts/lab-can-units.XXXXXX")"
cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

render_unit() {
  local output_path="${1:?output_path is required}"
  local can_role="${2:?can_role is required}"
  local can_device="${3:?can_device is required}"

  python3 - "${UNIT_TEMPLATE}" "${output_path}" "${can_role}" "${can_device}" "${CAN_BITRATE}" "${CAN_DBITRATE}" "${CAN_RESTART_MS}" <<'PY'
import pathlib
import sys

template_path = pathlib.Path(sys.argv[1])
output_path = pathlib.Path(sys.argv[2])
can_role = sys.argv[3]
can_device = sys.argv[4]
can_bitrate = sys.argv[5]
can_dbitrate = sys.argv[6]
can_restart_ms = sys.argv[7]

content = template_path.read_text(encoding="utf-8")
content = content.replace("__CAN_ROLE__", can_role)
content = content.replace("__CAN_DEVICE__", can_device)
content = content.replace("__CAN_BITRATE__", can_bitrate)
content = content.replace("__CAN_DBITRATE__", can_dbitrate)
content = content.replace("__CAN_RESTART_MS__", can_restart_ms)
output_path.write_text(content, encoding="utf-8")
PY
}

install_unit() {
  local ssh_target="${1:?ssh_target is required}"
  local service_name="${2:?service_name is required}"
  local rendered_unit="${3:?rendered_unit is required}"

  ssh "${ssh_target}" "set -euo pipefail; sudo install -o root -g root -m 0644 /dev/stdin /etc/systemd/system/$(printf '%q' "${service_name}")" < "${rendered_unit}"
  local remote_cmd="set -euo pipefail"
  remote_cmd+=" && sudo systemctl daemon-reload"
  remote_cmd+=" && sudo systemctl enable $(printf '%q' "${service_name}")"
  if [[ "${START_NOW}" == "1" ]]; then
    remote_cmd+=" && sudo systemctl restart $(printf '%q' "${service_name}")"
    remote_cmd+=" && sudo systemctl --no-pager --full status $(printf '%q' "${service_name}")"
  else
    remote_cmd+=" && sudo systemctl is-enabled $(printf '%q' "${service_name}")"
    remote_cmd+=" && (sudo systemctl --no-pager --full status $(printf '%q' "${service_name}") || true)"
  fi
  ssh "${ssh_target}" "${remote_cmd}"
}

OBC_UNIT="${TMP_DIR}/${OBC_CAN_SERVICE_NAME}"
SUBSYSTEM_EPS_ADCS_UNIT="${TMP_DIR}/${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}"
SUBSYSTEM_COMM_UNIT="${TMP_DIR}/${SUBSYSTEM_COMM_CAN_SERVICE_NAME}"

render_unit "${OBC_UNIT}" "obc shared SocketCAN" "${OBC_CSP_CAN_DEVICE}"
render_unit "${SUBSYSTEM_EPS_ADCS_UNIT}" "subsystem EPS/ADCS SocketCAN" "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
render_unit "${SUBSYSTEM_COMM_UNIT}" "subsystem COMM SocketCAN" "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"

install_unit "${OBC_SSH_TARGET}" "${OBC_CAN_SERVICE_NAME}" "${OBC_UNIT}"
install_unit "${SUBSYSTEM_SIM_SSH_TARGET}" "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}" "${SUBSYSTEM_EPS_ADCS_UNIT}"
install_unit "${SUBSYSTEM_SIM_SSH_TARGET}" "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}" "${SUBSYSTEM_COMM_UNIT}"
