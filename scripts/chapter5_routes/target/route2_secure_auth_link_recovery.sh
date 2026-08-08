#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

probe_root="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route2-target-secure-auth.XXXXXX")}"
summary_log="${SUMMARY_LOG:-${probe_root}/summary.log}"
mkdir -p "${probe_root}"
: >"${summary_log}"

env \
  TARGET_BASELINE_MANAGED_EXTERNALLY="1" \
  TARGET_BASELINE_REQUIRE_UHF_SERVICE="1" \
  AUTONOMOUS_FAILOVER_INCLUDE_HK_FILE="1" \
  OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}" \
  SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}" \
  OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}" \
  SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service sband)}" \
  UHF_COMM_SERVICE_NAME="${UHF_COMM_SERVICE_NAME:-$(obc_target_comm_profile_subsystem_comm_service uhf-primary)}" \
  EPS_SERVICE_NAME="${EPS_SERVICE_NAME:-subsystem-eps-csp.service}" \
  ADCS_SERVICE_NAME="${ADCS_SERVICE_NAME:-subsystem-adcs-csp.service}" \
  SBAND_STACK_TARGET_NAME="${SBAND_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target sband)}" \
  UHF_STACK_TARGET_NAME="${UHF_STACK_TARGET_NAME:-$(obc_target_comm_profile_subsystem_stack_target uhf-primary)}" \
  HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}" \
  COMM_BAUDRATE="${COMM_BAUDRATE:-115200}" \
  RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-120}" \
  ADCS_RESTORE_TIMEOUT_SEC="${ADCS_RESTORE_TIMEOUT_SEC:-60}" \
  SBAND_TCP_HOST="${SBAND_TCP_HOST:-$(obc_resolve_subsystem_sim_host)}" \
  SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}" \
  OBC_GROUNDLINK_DIAGNOSTICS="${OBC_GROUNDLINK_DIAGNOSTICS:-1}" \
  SBAND_COMM_NODE_INGRESS_DIAGNOSTICS="${SBAND_COMM_NODE_INGRESS_DIAGNOSTICS:-1}" \
  UHF_COMM_NODE_INGRESS_DIAGNOSTICS="${UHF_COMM_NODE_INGRESS_DIAGNOSTICS:-1}" \
  PROBE_ROOT="${probe_root}/autonomous-failover-proof" \
  bash "${ROOT_DIR}/scripts/run_target_autonomous_uhf_failover_probe.sh" | tee -a "${summary_log}"
