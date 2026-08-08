#!/usr/bin/env bash
set -euo pipefail

chapter5_root_dir() {
  cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd
}

chapter5_init_summary() {
  local summary_log="${1:?summary_log is required}"
  mkdir -p "$(dirname "${summary_log}")"
  : >"${summary_log}"
}

chapter5_log_line() {
  local summary_log="${1:?summary_log is required}"
  shift
  local line="${*}"
  printf '%s\n' "${line}"
  printf '%s\n' "${line}" >>"${summary_log}"
}

chapter5_run_stage() {
  local summary_log="${1:?summary_log is required}"
  local stage_name="${2:?stage_name is required}"
  shift 2

  chapter5_log_line "${summary_log}" "${stage_name}: START"
  local output_log
  output_log="$(mktemp "${TMPDIR:-/tmp}/${stage_name//[^A-Za-z0-9_.-]/_}.XXXXXX")"
  if ! "$@" >"${output_log}" 2>&1; then
    while IFS= read -r line; do
      chapter5_log_line "${summary_log}" "${line}"
    done <"${output_log}"
    rm -f "${output_log}"
    chapter5_log_line "${summary_log}" "${stage_name}: FAIL"
    return 1
  fi
  while IFS= read -r line; do
    chapter5_log_line "${summary_log}" "${line}"
  done <"${output_log}"
  rm -f "${output_log}"
  chapter5_log_line "${summary_log}" "${stage_name}: PASS"
}

chapter5_export_target_env_defaults() {
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
  EPS_SIM_CONTROL_SOCKET="${EPS_SIM_CONTROL_SOCKET:-/tmp/subsystem-eps-sim-control.sock}"
  ADCS_SIM_CONTROL_SOCKET="${ADCS_SIM_CONTROL_SOCKET:-/tmp/subsystem-adcs-sim-control.sock}"
  QUIET_OVERRIDE_DROPIN_NAME="${QUIET_OVERRIDE_DROPIN_NAME:-50-diag-quiet-packet-egress.conf}"
  PROFILE_OVERRIDE_DROPIN_NAME="${PROFILE_OVERRIDE_DROPIN_NAME:-51-target-comm-profile-override.conf}"
  TARGET_SERVICE_PROFILE="${TARGET_SERVICE_PROFILE:-sband}"
  TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE="${TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE:-sband-primary}"
  TARGET_SERVICE_INITIAL_COMM_BAND="${TARGET_SERVICE_INITIAL_COMM_BAND:-sband}"
  TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER="${TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER:-1}"
  TARGET_COMMAND_AUTHORITY_PROFILE="${TARGET_COMMAND_AUTHORITY_PROFILE:-sband-primary}"
  TARGET_COMM_CSP_NODE="${TARGET_COMM_CSP_NODE:-5}"
  TARGET_REQUIRES_UHF_PRIMARY_SWITCH="${TARGET_REQUIRES_UHF_PRIMARY_SWITCH:-0}"
  RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
  OBC_GROUNDLINK_DIAGNOSTICS="${OBC_GROUNDLINK_DIAGNOSTICS:-1}"
  UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME:-54-uhf-ingress-diagnostics.conf}"
  UHF_COMM_NODE_INGRESS_DIAGNOSTICS="${UHF_COMM_NODE_INGRESS_DIAGNOSTICS:-0}"
  SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME="${SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME:-58-sband-ingress-diagnostics.conf}"
  SBAND_COMM_NODE_INGRESS_DIAGNOSTICS="${SBAND_COMM_NODE_INGRESS_DIAGNOSTICS:-1}"

  export \
    OBC_SSH_TARGET \
    SUBSYSTEM_SIM_SSH_TARGET \
    OBC_COMM_CSP_SERVICE_NAME \
    EPS_SERVICE_NAME \
    ADCS_SERVICE_NAME \
    SBAND_STACK_TARGET_NAME \
    UHF_STACK_TARGET_NAME \
    SBAND_COMM_SERVICE_NAME \
    UHF_COMM_SERVICE_NAME \
    HOST_SERIAL_DEVICE \
    COMM_BAUDRATE \
    RESTART_TIMEOUT_SEC \
    ADCS_RESTORE_TIMEOUT_SEC \
    SBAND_TCP_HOST \
    SBAND_TCP_PORT \
    EPS_SIM_CONTROL_SOCKET \
    ADCS_SIM_CONTROL_SOCKET \
    QUIET_OVERRIDE_DROPIN_NAME \
    PROFILE_OVERRIDE_DROPIN_NAME \
    TARGET_SERVICE_PROFILE \
    TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE \
    TARGET_SERVICE_INITIAL_COMM_BAND \
    TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER \
    TARGET_COMMAND_AUTHORITY_PROFILE \
    TARGET_COMM_CSP_NODE \
    TARGET_REQUIRES_UHF_PRIMARY_SWITCH \
    RPI_INSTALL_ROOT \
    OBC_GROUNDLINK_DIAGNOSTICS \
    UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME \
    UHF_COMM_NODE_INGRESS_DIAGNOSTICS \
    SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME \
    SBAND_COMM_NODE_INGRESS_DIAGNOSTICS
}
