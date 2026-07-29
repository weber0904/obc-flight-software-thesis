#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"

probe_root="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route3-target-r6.XXXXXX")}"
baseline_dir="${probe_root}/baseline"
summary_log="${SUMMARY_LOG:-${probe_root}/summary.log}"

mkdir -p "${baseline_dir}"
chapter5_export_target_env_defaults
TARGET_BASELINE_REQUIRE_UHF_SERVICE=1

chapter5_init_summary "${summary_log}"
chapter5_log_line "${summary_log}" "chapter5-route3-target-r6: START"
JSON_OUT="${baseline_dir}/target-before.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1
JSON_OUT="${baseline_dir}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1

stage_status=0
set +e
env TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
  bash "${ROOT_DIR}/scripts/run_rpi_target_hardware_watchdog_reset_probe.sh" | tee -a "${summary_log}"
stage_status=${PIPESTATUS[0]}
set -e

JSON_OUT="${baseline_dir}/target-after.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1 || true
JSON_OUT="${baseline_dir}/ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1 || true

if [[ "${stage_status}" -ne 0 ]]; then
  exit "${stage_status}"
fi

chapter5_log_line "${summary_log}" "chapter5-route3-target-r6: PASS"
