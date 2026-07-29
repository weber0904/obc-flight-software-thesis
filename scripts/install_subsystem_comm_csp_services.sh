#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_TARGET_USER="${SUBSYSTEM_TARGET_USER:-operator}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
ACTIVE_STACK_TARGET_NAME="${STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target "${TARGET_COMM_PROFILE}")}"
SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME:-subsystem-sband-csp-stack.target}"
UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME:-subsystem-uhf-csp-stack.target}"
EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}"
ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}"
SBAND_SERVICE_NAME="${SBAND_SERVICE_NAME:-subsystem-sband-csp.service}"
UHF_SERVICE_NAME="${UHF_SERVICE_NAME:-subsystem-uhf-csp.service}"
SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME="${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME:-subsystem-eps-adcs-lab-can.service}"
SUBSYSTEM_COMM_CAN_SERVICE_NAME="${SUBSYSTEM_COMM_CAN_SERVICE_NAME:-subsystem-comm-lab-can.service}"
LEGACY_STACK_TARGET_NAME="${LEGACY_STACK_TARGET_NAME:-subsystem-comm-csp-stack.target}"
LEGACY_COMM_SERVICE_NAME="${LEGACY_COMM_SERVICE_NAME:-subsystem-comm-csp.service}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_COMM_CAN_DEVICE="${SUBSYSTEM_SIM_COMM_CAN_DEVICE:-can1}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-0.0.0.0}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
EPS_SIM_CONTROL_SOCKET="${EPS_SIM_CONTROL_SOCKET:-/tmp/subsystem-eps-sim-control.sock}"
ADCS_SIM_CONTROL_SOCKET="${ADCS_SIM_CONTROL_SOCKET:-/tmp/subsystem-adcs-sim-control.sock}"
SBAND_COMM_CSP_NODE="$(obc_target_comm_profile_node sband)"
UHF_COMM_CSP_NODE="$(obc_target_comm_profile_node uhf-primary)"
ALL_STACK_TARGET_NAMES="${SBAND_STACK_TARGET_NAME} ${UHF_STACK_TARGET_NAME}"
START_NOW="${START_NOW:-1}"
SUBSYSTEM_SERVICE_RENDER_ONLY_DIR="${SUBSYSTEM_SERVICE_RENDER_ONLY_DIR:-}"
TEMPLATE_DIR="${ROOT_DIR}/packaging/lab/systemd"

obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"
obc_require_distinct_remote_host_roles "${OBC_SSH_TARGET}" "${SUBSYSTEM_SIM_SSH_TARGET}"
obc_require_systemd_service_name "${EPS_SERVICE_NAME}"
obc_require_systemd_service_name "${ADCS_SERVICE_NAME}"
obc_require_systemd_service_name "${SBAND_SERVICE_NAME}"
obc_require_systemd_service_name "${UHF_SERVICE_NAME}"
obc_require_systemd_service_name "${LEGACY_COMM_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}"
obc_require_systemd_service_name "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}"
for target_name in "${ACTIVE_STACK_TARGET_NAME}" "${SBAND_STACK_TARGET_NAME}" "${UHF_STACK_TARGET_NAME}" "${LEGACY_STACK_TARGET_NAME}"; do
  if [[ ! "${target_name}" =~ ^[A-Za-z0-9_.@:-]+\.target$ ]]; then
    echo "Stack target names must be safe systemd target names ending in .target." >&2
    exit 1
  fi
done
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}"
if [[ "${CSP_CAN_PROMISC}" != "0" && "${CSP_CAN_PROMISC}" != "1" ]]; then
  echo "CSP_CAN_PROMISC must be 0 or 1." >&2
  exit 1
fi
if [[ "${START_NOW}" != "0" && "${START_NOW}" != "1" ]]; then
  echo "START_NOW must be 0 or 1." >&2
  exit 1
fi
if [[ -z "${SUBSYSTEM_TARGET_USER}" ]]; then
  echo "SUBSYSTEM_TARGET_USER is required." >&2
  exit 1
fi
if [[ -z "${SUBSYSTEM_SIM_REMOTE_DIR}" || "${SUBSYSTEM_SIM_REMOTE_DIR}" != /* ]]; then
  echo "SUBSYSTEM_SIM_REMOTE_DIR must be an absolute path." >&2
  exit 1
fi
if [[ -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_COMM_DEVICE is required." >&2
  exit 1
fi
if ! [[ "${COMM_BAUDRATE}" =~ ^[1-9][0-9]*$ && \
  "${SBAND_TCP_PORT}" =~ ^[1-9][0-9]*$ && \
  "${EPS_CSP_NODE_ID}" =~ ^[1-9][0-9]*$ && \
  "${ADCS_CSP_NODE_ID}" =~ ^[1-9][0-9]*$ ]]; then
  echo "COMM_BAUDRATE, SBAND_TCP_PORT, and subsystem node ids must be positive integers." >&2
  exit 1
fi

mkdir -p "${ROOT_DIR}/build-artifacts"
TMP_DIR="$(mktemp -d "${ROOT_DIR}/build-artifacts/subsystem-services.XXXXXX")"
cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

render_template() {
  local template_name="${1:?template_name is required}"
  local output_path="${2:?output_path is required}"
  local stack_target_name="${3:?stack_target_name is required}"
  local comm_service_name="${4:?comm_service_name is required}"
  local comm_exec_role="${5:?comm_exec_role is required}"
  local comm_node="${6:?comm_node is required}"

  python3 - \
    "${TEMPLATE_DIR}/${template_name}" \
    "${output_path}" \
    "${SUBSYSTEM_TARGET_USER}" \
    "${SUBSYSTEM_SIM_REMOTE_DIR}" \
    "${stack_target_name}" \
    "${EPS_SERVICE_NAME}" \
    "${ADCS_SERVICE_NAME}" \
    "${comm_service_name}" \
    "${SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME}" \
    "${SUBSYSTEM_COMM_CAN_SERVICE_NAME}" \
    "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
    "${SUBSYSTEM_SIM_COMM_CAN_DEVICE}" \
    "${SUBSYSTEM_SIM_COMM_DEVICE}" \
    "${COMM_BAUDRATE}" \
    "${SBAND_TCP_HOST}" \
    "${SBAND_TCP_PORT}" \
    "${CSP_CAN_PROMISC}" \
    "${EPS_CSP_NODE_ID}" \
    "${ADCS_CSP_NODE_ID}" \
    "${EPS_SIM_CONTROL_SOCKET}" \
    "${ADCS_SIM_CONTROL_SOCKET}" \
    "${comm_node}" \
    "${comm_exec_role}" <<'PY'
import pathlib
import sys

template_path = pathlib.Path(sys.argv[1])
output_path = pathlib.Path(sys.argv[2])
content = template_path.read_text(encoding="utf-8")
replacements = {
    "__TARGET_USER__": sys.argv[3],
    "__REMOTE_DIR__": sys.argv[4],
    "__STACK_TARGET_NAME__": sys.argv[5],
    "__EPS_SERVICE_NAME__": sys.argv[6],
    "__ADCS_SERVICE_NAME__": sys.argv[7],
    "__COMM_SERVICE_NAME__": sys.argv[8],
    "__EPS_ADCS_CAN_SERVICE_NAME__": sys.argv[9],
    "__COMM_CAN_SERVICE_NAME__": sys.argv[10],
    "__EPS_ADCS_CAN_DEVICE__": sys.argv[11],
    "__COMM_CAN_DEVICE__": sys.argv[12],
    "__COMM_SERIAL_DEVICE__": sys.argv[13],
    "__COMM_BAUDRATE__": sys.argv[14],
    "__SBAND_TCP_HOST__": sys.argv[15],
    "__SBAND_TCP_PORT__": sys.argv[16],
    "__CSP_CAN_PROMISC__": sys.argv[17],
    "__EPS_CSP_NODE_ID__": sys.argv[18],
    "__ADCS_CSP_NODE_ID__": sys.argv[19],
    "__EPS_SIM_CONTROL_SOCKET__": sys.argv[20],
    "__ADCS_SIM_CONTROL_SOCKET__": sys.argv[21],
    "__COMM_CSP_NODE__": sys.argv[22],
    "__COMM_EXEC_ROLE__": sys.argv[23],
}
for key, value in replacements.items():
    content = content.replace(key, value)
output_path.write_text(content, encoding="utf-8")
PY
}

render_template "subsystem-comm-csp-stack.target.template" "${TMP_DIR}/${SBAND_STACK_TARGET_NAME}" "${SBAND_STACK_TARGET_NAME}" "${SBAND_SERVICE_NAME}" "sband" "${SBAND_COMM_CSP_NODE}"
render_template "subsystem-comm-csp-stack.target.template" "${TMP_DIR}/${UHF_STACK_TARGET_NAME}" "${UHF_STACK_TARGET_NAME}" "${UHF_SERVICE_NAME}" "uhf" "${UHF_COMM_CSP_NODE}"
render_template "subsystem-eps-csp.service.template" "${TMP_DIR}/${EPS_SERVICE_NAME}" "${ALL_STACK_TARGET_NAMES}" "${SBAND_SERVICE_NAME}" "eps" "${SBAND_COMM_CSP_NODE}"
render_template "subsystem-adcs-csp.service.template" "${TMP_DIR}/${ADCS_SERVICE_NAME}" "${ALL_STACK_TARGET_NAMES}" "${SBAND_SERVICE_NAME}" "adcs" "${SBAND_COMM_CSP_NODE}"
render_template "subsystem-sband-csp.service.template" "${TMP_DIR}/${SBAND_SERVICE_NAME}" "${SBAND_STACK_TARGET_NAME}" "${SBAND_SERVICE_NAME}" "sband" "${SBAND_COMM_CSP_NODE}"
render_template "subsystem-uhf-csp.service.template" "${TMP_DIR}/${UHF_SERVICE_NAME}" "${UHF_STACK_TARGET_NAME}" "${UHF_SERVICE_NAME}" "uhf" "${UHF_COMM_CSP_NODE}"

if [[ -n "${SUBSYSTEM_SERVICE_RENDER_ONLY_DIR}" ]]; then
  mkdir -p "${SUBSYSTEM_SERVICE_RENDER_ONLY_DIR}"
  for unit_name in "${SBAND_STACK_TARGET_NAME}" "${UHF_STACK_TARGET_NAME}" "${EPS_SERVICE_NAME}" "${ADCS_SERVICE_NAME}" "${SBAND_SERVICE_NAME}" "${UHF_SERVICE_NAME}"; do
    install -m 0644 "${TMP_DIR}/${unit_name}" "${SUBSYSTEM_SERVICE_RENDER_ONLY_DIR}/${unit_name}"
    printf 'Rendered %s\n' "${SUBSYSTEM_SERVICE_RENDER_ONLY_DIR}/${unit_name}"
  done
  exit 0
fi

for unit_name in "${SBAND_STACK_TARGET_NAME}" "${UHF_STACK_TARGET_NAME}" "${EPS_SERVICE_NAME}" "${ADCS_SERVICE_NAME}" "${SBAND_SERVICE_NAME}" "${UHF_SERVICE_NAME}"; do
  ssh "${SUBSYSTEM_SIM_SSH_TARGET}" "set -euo pipefail; sudo install -o root -g root -m 0644 /dev/stdin /etc/systemd/system/$(printf '%q' "${unit_name}")" < "${TMP_DIR}/${unit_name}"
done

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && test -x $(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}/scripts/run_subsystem_csp_service.sh")"
REMOTE_CMD+=" && sudo systemctl daemon-reload"
REMOTE_CMD+=" && sudo systemctl enable $(printf '%q' "${SBAND_STACK_TARGET_NAME}") $(printf '%q' "${UHF_STACK_TARGET_NAME}")"
REMOTE_CMD+=" && (sudo systemctl disable --now $(printf '%q' "${LEGACY_STACK_TARGET_NAME}") >/dev/null 2>&1 || true)"
REMOTE_CMD+=" && (sudo systemctl disable --now $(printf '%q' "${LEGACY_COMM_SERVICE_NAME}") >/dev/null 2>&1 || true)"
if [[ "${START_NOW}" == "1" ]]; then
  REMOTE_CMD+=" && sudo systemctl restart $(printf '%q' "${SBAND_STACK_TARGET_NAME}") $(printf '%q' "${UHF_STACK_TARGET_NAME}")"
fi

append_unit_status_check() {
  local unit_name="${1:?unit_name is required}"
  REMOTE_CMD+=" && (sudo systemctl --no-pager --full status $(printf '%q' "${unit_name}") || true)"
}

append_unit_status_check "${ACTIVE_STACK_TARGET_NAME}"
append_unit_status_check "${EPS_SERVICE_NAME}"
append_unit_status_check "${ADCS_SERVICE_NAME}"
if [[ "${ACTIVE_STACK_TARGET_NAME}" == "${SBAND_STACK_TARGET_NAME}" ]]; then
  append_unit_status_check "${SBAND_SERVICE_NAME}"
else
  append_unit_status_check "${UHF_SERVICE_NAME}"
fi

ssh "${SUBSYSTEM_SIM_SSH_TARGET}" "${REMOTE_CMD}"
