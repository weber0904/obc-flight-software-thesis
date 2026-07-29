#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route3-target.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_ROOT}"

chapter5_run_stage "${SUMMARY_LOG}" "route3_adcs_r3_first_fault_and_clear" \
  env PROBE_ROOT="${PROBE_ROOT}/adcs" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh"
if ! chapter5_run_stage "${SUMMARY_LOG}" "route3_eps_r3_safe_fallback" \
  env PROBE_ROOT="${PROBE_ROOT}/eps" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh"; then
  chapter5_log_line "${SUMMARY_LOG}" "route3_eps_r3_safe_fallback: RETRY_AFTER_STAGE_FAIL"
  chapter5_run_stage "${SUMMARY_LOG}" "route3_eps_r3_safe_fallback_retry1" \
    env PROBE_ROOT="${PROBE_ROOT}/eps-retry1" \
    bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh"
fi
chapter5_run_stage "${SUMMARY_LOG}" "route3_watchdog_reboot_and_postcheck" \
  env PROBE_ROOT="${PROBE_ROOT}/watchdog-r6" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_watchdog_reboot_and_postcheck.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target: PASS"
