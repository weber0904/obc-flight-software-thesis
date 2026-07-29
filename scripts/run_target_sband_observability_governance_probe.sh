#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
TARGET_PROBE="${ROOT_DIR}/scripts/comm_verification/lib/run_target_sband_observability_governance_probe.py"
ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

probe_root="${PROBE_ROOT:-$(mktemp -d "${TMPDIR:-/tmp}/target-sband-observability-governance.XXXXXX")}"
summary_log="${probe_root}/summary.log"
baseline_dir="${probe_root}/baseline"

mkdir -p "${probe_root}"
mkdir -p "${baseline_dir}"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}"
ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}"
SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target sband)}"
UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target uhf-primary)}"
SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service sband)}"
UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service uhf-primary)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-120}"
ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC:-60}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-$(obc_resolve_subsystem_sim_host)}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
QUIET_OVERRIDE_DROPIN_NAME="${QUIET_OVERRIDE_DROPIN_NAME:-50-diag-quiet-packet-egress.conf}"
PROFILE_OVERRIDE_DROPIN_NAME="${PROFILE_OVERRIDE_DROPIN_NAME:-51-target-comm-profile-override.conf}"
TARGET_SERVICE_PROFILE="${TARGET_SERVICE_PROFILE:-sband}"
TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE="${TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE:-sband-primary}"
TARGET_SERVICE_INITIAL_COMM_BAND="${TARGET_SERVICE_INITIAL_COMM_BAND:-sband}"
TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER="${TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER:-1}"
TARGET_COMMAND_AUTHORITY_PROFILE="${TARGET_COMMAND_AUTHORITY_PROFILE:-uhf-primary}"
TARGET_COMM_CSP_NODE="${TARGET_COMM_CSP_NODE:-6}"
TARGET_REQUIRES_UHF_PRIMARY_SWITCH="${TARGET_REQUIRES_UHF_PRIMARY_SWITCH:-1}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
OBC_GROUNDLINK_DIAGNOSTICS="${OBC_GROUNDLINK_DIAGNOSTICS:-1}"
UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME:-54-uhf-ingress-diagnostics.conf}"
UHF_COMM_NODE_INGRESS_DIAGNOSTICS="${UHF_COMM_NODE_INGRESS_DIAGNOSTICS:-1}"
SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME:-58-sband-ingress-diagnostics.conf}"
SBAND_COMM_NODE_INGRESS_DIAGNOSTICS="${SBAND_COMM_NODE_INGRESS_DIAGNOSTICS:-1}"

run_target_probe() {
  JSON_OUT="${baseline_dir}/target-before.json" \
    TARGET_BASELINE_REQUIRE_UHF_SERVICE=1 \
    bash "${ENSURE_TARGET_BASELINE}"
  JSON_OUT="${baseline_dir}/ground-before.json" \
    bash "${ENSURE_GROUND_BASELINE}"
  env \
    OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
    SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
    OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME}" \
    EPS_SERVICE_NAME="${EPS_SERVICE_NAME}" \
    ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME}" \
    SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME}" \
    UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME}" \
    SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME}" \
    UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME}" \
    HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
    COMM_BAUDRATE="${COMM_BAUDRATE}" \
    RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC}" \
    ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC}" \
    SBAND_TCP_HOST="${SBAND_TCP_HOST}" \
    SBAND_TCP_PORT="${SBAND_TCP_PORT}" \
    QUIET_OVERRIDE_DROPIN_NAME="${QUIET_OVERRIDE_DROPIN_NAME}" \
    PROFILE_OVERRIDE_DROPIN_NAME="${PROFILE_OVERRIDE_DROPIN_NAME}" \
    TARGET_SERVICE_PROFILE="${TARGET_SERVICE_PROFILE}" \
    TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE="${TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE}" \
    TARGET_SERVICE_INITIAL_COMM_BAND="${TARGET_SERVICE_INITIAL_COMM_BAND}" \
    TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER="${TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER}" \
    TARGET_COMMAND_AUTHORITY_PROFILE="${TARGET_COMMAND_AUTHORITY_PROFILE}" \
    TARGET_COMM_CSP_NODE="${TARGET_COMM_CSP_NODE}" \
    TARGET_REQUIRES_UHF_PRIMARY_SWITCH="${TARGET_REQUIRES_UHF_PRIMARY_SWITCH}" \
    RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT}" \
    OBC_GROUNDLINK_DIAGNOSTICS="${OBC_GROUNDLINK_DIAGNOSTICS}" \
    TARGET_BASELINE_MANAGED_EXTERNALLY="1" \
    TARGET_BASELINE_REQUIRE_UHF_SERVICE="1" \
    UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME}" \
    UHF_COMM_NODE_INGRESS_DIAGNOSTICS="${UHF_COMM_NODE_INGRESS_DIAGNOSTICS}" \
    SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME}" \
    SBAND_COMM_NODE_INGRESS_DIAGNOSTICS="${SBAND_COMM_NODE_INGRESS_DIAGNOSTICS}" \
    "${PYTHON_BIN}" "${TARGET_PROBE}" --probe-root "${probe_root}"
}

log_line() {
  local line="${1}"
  printf '%s\n' "${line}"
  printf '%s\n' "${line}" >>"${summary_log}"
}

: >"${summary_log}"
log_line "target-sband-observability-governance: START"
log_line "probe-root=${probe_root}"
probe_output="$(mktemp "${TMPDIR:-/tmp}/target-sband-observability-governance-output.XXXXXX")"
cleanup_probe_output() {
  rm -f "${probe_output}"
}
trap cleanup_probe_output EXIT
if ! run_target_probe >"${probe_output}" 2>&1; then
  cat "${probe_output}" | while IFS= read -r line; do
    log_line "${line}"
  done
  exit 1
fi
cat "${probe_output}" | while IFS= read -r line; do
  log_line "${line}"
done
log_line "target-sband-observability-governance: PASS"
