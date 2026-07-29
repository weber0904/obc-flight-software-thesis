#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
PROBE_PY="${ROOT_DIR}/scripts/comm_verification/lib/run_target_autonomous_uhf_failover_probe.py"
ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

probe_root="${PROBE_ROOT:-$(mktemp -d "/private/tmp/target-autonomous-uhf-failover.XXXXXX")}"
summary_log="${SUMMARY_LOG:-${probe_root}/summary.log}"
baseline_managed_externally="${TARGET_BASELINE_MANAGED_EXTERNALLY:-0}"

mkdir -p "${probe_root}"

run_probe() {
  env \
    TARGET_BASELINE_MANAGED_EXTERNALLY="1" \
    TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}" \
    AUTONOMOUS_FAILOVER_INCLUDE_HK_FILE="${AUTONOMOUS_FAILOVER_INCLUDE_HK_FILE:-0}" \
    COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC="${COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC:-60}" \
    OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}" \
    SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}" \
    OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}" \
    EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}" \
    ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}" \
    SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target sband)}" \
    UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target uhf-primary)}" \
    SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service sband)}" \
    UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service uhf-primary)}" \
    QUIET_OVERRIDE_DROPIN_NAME="${QUIET_OVERRIDE_DROPIN_NAME:-50-diag-quiet-packet-egress.conf}" \
    PROFILE_OVERRIDE_DROPIN_NAME="${PROFILE_OVERRIDE_DROPIN_NAME:-51-target-comm-profile-override.conf}" \
    TARGET_SERVICE_PROFILE="${TARGET_SERVICE_PROFILE:-sband}" \
    TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE="${TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE:-sband-primary}" \
    TARGET_SERVICE_INITIAL_COMM_BAND="${TARGET_SERVICE_INITIAL_COMM_BAND:-sband}" \
    TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER="${TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER:-1}" \
    TARGET_COMMAND_AUTHORITY_PROFILE="${TARGET_COMMAND_AUTHORITY_PROFILE:-uhf-primary}" \
    TARGET_COMM_CSP_NODE="${TARGET_COMM_CSP_NODE:-6}" \
    TARGET_REQUIRES_UHF_PRIMARY_SWITCH="${TARGET_REQUIRES_UHF_PRIMARY_SWITCH:-1}" \
    UHF_BEACON_OVERRIDE_DROPIN_NAME="${UHF_BEACON_OVERRIDE_DROPIN_NAME:-52-uhf-beacon-csp-node.conf}" \
    UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME:-54-uhf-ingress-diagnostics.conf}" \
    RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-120}" \
    ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC:-60}" \
    HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}" \
    COMM_BAUDRATE="${COMM_BAUDRATE:-115200}" \
    SBAND_TCP_HOST="${SBAND_TCP_HOST:-$(obc_resolve_subsystem_sim_host)}" \
    SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}" \
    OBC_GROUNDLINK_DIAGNOSTICS="${OBC_GROUNDLINK_DIAGNOSTICS:-1}" \
    SBAND_COMM_NODE_INGRESS_DIAGNOSTICS="${SBAND_COMM_NODE_INGRESS_DIAGNOSTICS:-1}" \
    UHF_COMM_NODE_INGRESS_DIAGNOSTICS="${UHF_COMM_NODE_INGRESS_DIAGNOSTICS:-1}" \
    PROBE_ROOT="${probe_root}" \
    "${PYTHON_BIN}" "${PROBE_PY}" --probe-root "${probe_root}"
}

if [[ "${baseline_managed_externally}" != "1" ]]; then
  JSON_OUT="${probe_root}/baseline-target-before.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}" bash "${ENSURE_TARGET_BASELINE}" >"${probe_root}/baseline-target-before.log" 2>&1
  JSON_OUT="${probe_root}/baseline-ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >"${probe_root}/baseline-ground-before.log" 2>&1
fi

probe_status=0
set +e
{
  echo "target-autonomous-uhf-failover: START"
  echo "probe-root=${probe_root}"
  run_probe
} | tee "${summary_log}"
probe_status=${PIPESTATUS[0]}
set -e

if [[ "${baseline_managed_externally}" != "1" ]]; then
  JSON_OUT="${probe_root}/baseline-target-after.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}" bash "${ENSURE_TARGET_BASELINE}" >"${probe_root}/baseline-target-after.log" 2>&1 || true
  JSON_OUT="${probe_root}/baseline-ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >"${probe_root}/baseline-ground-after.log" 2>&1 || true
fi

exit "${probe_status}"
