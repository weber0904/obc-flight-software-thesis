#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
STACK_TARGET_NAME="${STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target "${TARGET_COMM_PROFILE}")}"
EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}"
ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}"
COMM_SERVICE_NAME="${COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service "${TARGET_COMM_PROFILE}")}"
JOURNAL_LINES="${JOURNAL_LINES:-80}"

obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"
for service_name in "${EPS_SERVICE_NAME}" "${ADCS_SERVICE_NAME}" "${COMM_SERVICE_NAME}"; do
  obc_require_systemd_service_name "${service_name}"
done
if [[ ! "${STACK_TARGET_NAME}" =~ ^[A-Za-z0-9_.@:-]+\.target$ ]]; then
  echo "STACK_TARGET_NAME must be a safe systemd target name ending in .target." >&2
  exit 1
fi
if ! [[ "${JOURNAL_LINES}" =~ ^[1-9][0-9]*$ ]]; then
  echo "JOURNAL_LINES must be a positive integer." >&2
  exit 1
fi

ssh "${SUBSYSTEM_SIM_SSH_TARGET}" "set -euo pipefail
for unit in $(printf '%q' "${STACK_TARGET_NAME}") $(printf '%q' "${EPS_SERVICE_NAME}") $(printf '%q' "${ADCS_SERVICE_NAME}") $(printf '%q' "${COMM_SERVICE_NAME}"); do
  printf '=== %s ===\n' \"\${unit}\"
  sudo systemctl --no-pager --full status \"\${unit}\" || true
  printf '\n=== recent journal: %s ===\n' \"\${unit}\"
  sudo journalctl -u \"\${unit}\" -n $(printf '%q' "${JOURNAL_LINES}") --no-pager || true
  printf '\n'
done"
