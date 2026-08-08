#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route2-target-link.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"
baseline_dir="${PROBE_ROOT}/baseline"

mkdir -p "${baseline_dir}"
chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-target-link-recovery-and-hk: START"
chapter5_export_target_env_defaults
TARGET_BASELINE_REQUIRE_UHF_SERVICE=1

JSON_OUT="${baseline_dir}/target-before.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
JSON_OUT="${baseline_dir}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1

stage_status=0
set +e
chapter5_run_stage "${SUMMARY_LOG}" "target_secure_auth_link_recovery" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route2_secure_auth_link_recovery.sh"
stage_status=$?
set -e

JSON_OUT="${baseline_dir}/target-after.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1 || true
JSON_OUT="${baseline_dir}/ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1 || true

if [[ "${stage_status}" -ne 0 ]]; then
  exit "${stage_status}"
fi

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-target-link-recovery-and-hk: PASS"
