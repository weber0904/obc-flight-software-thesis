#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
RPI_TARGET_USER="${RPI_TARGET_USER:-operator}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
LEGACY_SERVICE_NAME="${LEGACY_SERVICE_NAME:-obc-installed-stack.service}"
OBC_CAN_SERVICE_NAME="${OBC_CAN_SERVICE_NAME:-obc-lab-can.service}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_INSTALL_ROOT}/runtime/comm-csp-lab-obc}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-}"
HARDWARE_WATCHDOG_MODE="${HARDWARE_WATCHDOG_MODE:-linux-device}"
HARDWARE_WATCHDOG_DEVICE="${HARDWARE_WATCHDOG_DEVICE:-/dev/watchdog0}"
HARDWARE_WATCHDOG_TIMEOUT_SEC="${HARDWARE_WATCHDOG_TIMEOUT_SEC:-15}"
COMM_SUBSYSTEM_PING_TIMEOUT_MS="${COMM_SUBSYSTEM_PING_TIMEOUT_MS:-500}"
COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD="${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD:-10}"
START_NOW="${START_NOW:-1}"
RENDERED_UNIT_RECEIPT_PATH="${RENDERED_UNIT_RECEIPT_PATH:-}"
UNIT_TEMPLATE="${ROOT_DIR}/packaging/rpi/systemd/obc-comm-csp-stack.service.template"
WATCHDOG_RULE_TEMPLATE="${ROOT_DIR}/packaging/rpi/udev/90-obc-watchdog.rules"

obc_require_systemd_service_name "${OBC_COMM_CSP_SERVICE_NAME}"
obc_require_systemd_service_name "${LEGACY_SERVICE_NAME}"
obc_require_systemd_service_name "${OBC_CAN_SERVICE_NAME}"
obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"

COMM_CSP_NODE="$(obc_target_comm_profile_node "${TARGET_COMM_PROFILE}")"
PROFILE_COMMAND_AUTHORITY_PROFILE="$(obc_target_comm_profile_service_authority "${TARGET_COMM_PROFILE}")"
INITIAL_COMM_BAND="$(obc_target_comm_profile_initial_band "${TARGET_COMM_PROFILE}")"
ENABLE_PRIMARY_GROUND_LINK_DRIVER="$(obc_target_comm_profile_enable_primary_ground_link_driver "${TARGET_COMM_PROFILE}")"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-${PROFILE_COMMAND_AUTHORITY_PROFILE}}"
if [[ "${COMMAND_AUTHORITY_PROFILE}" != "${PROFILE_COMMAND_AUTHORITY_PROFILE}" ]]; then
  echo "COMMAND_AUTHORITY_PROFILE=${COMMAND_AUTHORITY_PROFILE} does not match TARGET_COMM_PROFILE=${TARGET_COMM_PROFILE} (expected ${PROFILE_COMMAND_AUTHORITY_PROFILE})." >&2
  exit 1
fi

if [[ ! -f "${UNIT_TEMPLATE}" ]]; then
  echo "Service template not found at ${UNIT_TEMPLATE}" >&2
  exit 1
fi
if [[ ! -f "${WATCHDOG_RULE_TEMPLATE}" ]]; then
  echo "Watchdog udev rule not found at ${WATCHDOG_RULE_TEMPLATE}" >&2
  exit 1
fi

mkdir -p "${ROOT_DIR}/build-artifacts"
RENDERED_UNIT="$(mktemp "${ROOT_DIR}/build-artifacts/rpi-comm-csp-unit.XXXXXX")"
cleanup() {
  rm -f "${RENDERED_UNIT}"
}
trap cleanup EXIT

python3 - \
  "${UNIT_TEMPLATE}" \
  "${RENDERED_UNIT}" \
  "${RPI_TARGET_USER}" \
  "${RPI_INSTALL_ROOT}" \
  "${RUNTIME_ROOT}" \
  "${TARGET_COMM_PROFILE}" \
  "${COMM_CSP_NODE}" \
  "${INITIAL_COMM_BAND}" \
  "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" \
  "${OBC_CAN_SERVICE_NAME}" \
  "${COMMAND_AUTHORITY_PROFILE}" \
  "${HARDWARE_WATCHDOG_MODE}" \
  "${HARDWARE_WATCHDOG_DEVICE}" \
  "${HARDWARE_WATCHDOG_TIMEOUT_SEC}" \
  "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}" \
  "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}" <<'PY'
import pathlib
import sys

template_path = pathlib.Path(sys.argv[1])
output_path = pathlib.Path(sys.argv[2])
target_user = sys.argv[3]
install_root = sys.argv[4]
runtime_root = sys.argv[5]
target_comm_profile = sys.argv[6]
comm_csp_node = sys.argv[7]
initial_comm_band = sys.argv[8]
enable_primary_ground_link_driver = sys.argv[9]
obc_can_service_name = sys.argv[10]
command_authority_profile = sys.argv[11]
hardware_watchdog_mode = sys.argv[12]
hardware_watchdog_device = sys.argv[13]
hardware_watchdog_timeout_sec = sys.argv[14]
comm_subsystem_ping_timeout_ms = sys.argv[15]
comm_primary_unavailable_failure_threshold = sys.argv[16]

content = template_path.read_text(encoding="utf-8")
content = content.replace("__TARGET_USER__", target_user)
content = content.replace("__INSTALL_ROOT__", install_root)
content = content.replace("__RUNTIME_ROOT__", runtime_root)
content = content.replace("__TARGET_COMM_PROFILE__", target_comm_profile)
content = content.replace("__COMM_CSP_NODE__", comm_csp_node)
content = content.replace("__INITIAL_COMM_BAND__", initial_comm_band)
content = content.replace("__ENABLE_PRIMARY_GROUND_LINK_DRIVER__", enable_primary_ground_link_driver)
content = content.replace("__OBC_CAN_SERVICE_NAME__", obc_can_service_name)
content = content.replace("__COMMAND_AUTHORITY_PROFILE__", command_authority_profile)
content = content.replace("__HARDWARE_WATCHDOG_MODE__", hardware_watchdog_mode)
content = content.replace("__HARDWARE_WATCHDOG_DEVICE__", hardware_watchdog_device)
content = content.replace("__HARDWARE_WATCHDOG_TIMEOUT_SEC__", hardware_watchdog_timeout_sec)
content = content.replace("__COMM_SUBSYSTEM_PING_TIMEOUT_MS__", comm_subsystem_ping_timeout_ms)
content = content.replace("__COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD__", comm_primary_unavailable_failure_threshold)
output_path.write_text(content, encoding="utf-8")
PY

if [[ -n "${RENDERED_UNIT_RECEIPT_PATH}" ]]; then
  mkdir -p "$(dirname "${RENDERED_UNIT_RECEIPT_PATH}")"
  install -m 0644 "${RENDERED_UNIT}" "${RENDERED_UNIT_RECEIPT_PATH}"
fi

SERVICE_UNIT_PATH="/etc/systemd/system/${OBC_COMM_CSP_SERVICE_NAME}"
WATCHDOG_RULE_PATH="/etc/udev/rules.d/90-obc-watchdog.rules"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
test -x $(printf '%q' "${RPI_INSTALL_ROOT}/current/launch/run_obc_comm_csp_stack.sh")
sudo install -o root -g root -m 0644 /dev/stdin $(printf '%q' "${SERVICE_UNIT_PATH}")" < "${RENDERED_UNIT}"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
sudo install -o root -g root -m 0644 /dev/stdin $(printf '%q' "${WATCHDOG_RULE_PATH}")" < "${WATCHDOG_RULE_TEMPLATE}"

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && sudo getent group watchdog >/dev/null || sudo groupadd --system watchdog"
REMOTE_CMD+=" && sudo usermod -a -G watchdog $(printf '%q' "${RPI_TARGET_USER}")"
REMOTE_CMD+=" && sudo udevadm control --reload-rules"
REMOTE_CMD+=" && sudo udevadm trigger --subsystem-match=watchdog"
REMOTE_CMD+=" && sudo systemctl daemon-reload"
REMOTE_CMD+=" && (sudo systemctl disable --now $(printf '%q' "${LEGACY_SERVICE_NAME}") >/dev/null 2>&1 || true)"
REMOTE_CMD+=" && sudo systemctl enable $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}")"
if [[ "${START_NOW}" == "1" ]]; then
  REMOTE_CMD+=" && sudo systemctl restart $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}")"
  REMOTE_CMD+=" && sudo systemctl --no-pager --full status $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}")"
else
  REMOTE_CMD+=" && sudo systemctl is-enabled $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}")"
  REMOTE_CMD+=" && (sudo systemctl --no-pager --full status $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}") || true)"
fi

ssh "${OBC_SSH_TARGET}" "${REMOTE_CMD}"
