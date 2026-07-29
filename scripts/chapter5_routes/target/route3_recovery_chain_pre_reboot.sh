#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route3-target-pre-reboot.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"
baseline_dir="${PROBE_ROOT}/baseline-boundaries"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target-pre-reboot: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_ROOT}"
mkdir -p "${baseline_dir}"

chapter5_run_stage "${SUMMARY_LOG}" "route3_adcs_r3_first_fault_and_clear" \
  env PROBE_ROOT="${PROBE_ROOT}/adcs" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh"

chapter5_log_line "${SUMMARY_LOG}" "route3-stage-boundary-resync: START"
JSON_OUT="${baseline_dir}/target-between-adcs-eps.json" bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
JSON_OUT="${baseline_dir}/ground-between-adcs-eps.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
sleep 3
chapter5_log_line "${SUMMARY_LOG}" "route3-stage-boundary-resync: PASS"

chapter5_run_stage "${SUMMARY_LOG}" "route3_eps_r3_safe_fallback" \
  env PROBE_ROOT="${PROBE_ROOT}/eps" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target-pre-reboot: PASS"
