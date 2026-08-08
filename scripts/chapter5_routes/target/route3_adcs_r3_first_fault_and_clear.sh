#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route3-target-adcs.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"
baseline_dir="${PROBE_ROOT}/baseline"

mkdir -p "${baseline_dir}"
chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target-adcs-r3: START"
chapter5_export_target_env_defaults
TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}"

JSON_OUT="${baseline_dir}/target-before.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
JSON_OUT="${baseline_dir}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1

stage_status=0
set +e
env TARGET_BASELINE_MANAGED_EXTERNALLY=1 TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
  "${ROOT_DIR}/fprime-venv/bin/python" "${ROOT_DIR}/scripts/comm_verification/lib/run_target_route3_pre_reboot_recovery_probe.py" \
  --probe-root "${PROBE_ROOT}/adcs-r3" \
  --stage adcs-r3 | tee -a "${SUMMARY_LOG}"
stage_status=${PIPESTATUS[0]}
set -e

JSON_OUT="${baseline_dir}/target-after.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1 || true
JSON_OUT="${baseline_dir}/ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1 || true

if [[ "${stage_status}" -ne 0 ]]; then
  exit "${stage_status}"
fi

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-target-adcs-r3: PASS"
