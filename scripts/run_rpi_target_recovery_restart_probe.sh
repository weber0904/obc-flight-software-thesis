#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
RECOVERY_PROBE_PY="${ROOT_DIR}/scripts/comm_verification/lib/run_rpi_target_recovery_restart_probe.py"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
RUNTIME_ROOT="${RUNTIME_ROOT:-}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}"
ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}"
SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target sband)}"
UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target uhf-primary)}"
SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service sband)}"
UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service uhf-primary)}"
RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-120}"
ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC:-60}"
PROBE_MODE="${PROBE_MODE:-command-path}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/rpi-target-recovery-restart.XXXXXX")}"

obc_require_systemd_service_name "${OBC_COMM_CSP_SERVICE_NAME}"
obc_require_systemd_service_name "${EPS_SERVICE_NAME}"
obc_require_systemd_service_name "${ADCS_SERVICE_NAME}"
obc_require_systemd_service_name "${SBAND_COMM_SERVICE_NAME}"
obc_require_systemd_service_name "${UHF_COMM_SERVICE_NAME}"
obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"
if [[ ! "${SBAND_STACK_TARGET_NAME}" =~ ^[A-Za-z0-9_.@:-]+\.target$ || \
  ! "${UHF_STACK_TARGET_NAME}" =~ ^[A-Za-z0-9_.@:-]+\.target$ ]]; then
  echo "SBAND_STACK_TARGET_NAME and UHF_STACK_TARGET_NAME must be safe systemd target names ending in .target." >&2
  exit 1
fi
if [[ "${PROBE_MODE}" != "r2-recovery" && "${PROBE_MODE}" != "command-path" ]]; then
  echo "PROBE_MODE must be command-path." >&2
  exit 1
fi

if [[ "${PROBE_MODE}" == "command-path" ]]; then
  exec env \
    PROBE_ROOT="${PROBE_TMP_DIR}" \
    OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
    SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
    OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME}" \
    EPS_SERVICE_NAME="${EPS_SERVICE_NAME}" \
    ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME}" \
    SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME}" \
    UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME}" \
    SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME}" \
    UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME}" \
    HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}" \
    COMM_BAUDRATE="${COMM_BAUDRATE:-115200}" \
    RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC}" \
    ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC}" \
    SBAND_TCP_HOST="${SBAND_TCP_HOST:-$(obc_resolve_subsystem_sim_host)}" \
    SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}" \
    QUIET_OVERRIDE_DROPIN_NAME="${QUIET_OVERRIDE_DROPIN_NAME:-50-diag-quiet-packet-egress.conf}" \
    PROFILE_OVERRIDE_DROPIN_NAME="${PROFILE_OVERRIDE_DROPIN_NAME:-51-target-comm-profile-override.conf}" \
    bash "${ROOT_DIR}/scripts/run_target_secure_auth_command_path_probe.sh"
fi

echo "PROBE_MODE=r2-recovery is no longer supported on the current ADCS R3 baseline. Use PROBE_MODE=command-path." >&2
exit 2
