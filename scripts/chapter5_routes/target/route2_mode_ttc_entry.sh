#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
PROBE_IMPL="${ROOT_DIR}/scripts/chapter5_routes/target/route2_mode_ttc_entry.py"
ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

probe_root="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route2-target-ttc.XXXXXX")}"
summary_log="${SUMMARY_LOG:-${probe_root}/summary.log}"
baseline_dir="${probe_root}/baseline"

mkdir -p "${baseline_dir}"
: >"${summary_log}"

chapter5_export_target_env_defaults
TARGET_BASELINE_REQUIRE_UHF_SERVICE=1

JSON_OUT="${baseline_dir}/target-before.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1
JSON_OUT="${baseline_dir}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1

probe_status=0
set +e
env TARGET_BASELINE_MANAGED_EXTERNALLY=1 TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
  "${PYTHON_BIN}" "${PROBE_IMPL}" --probe-root "${probe_root}" | tee -a "${summary_log}"
probe_status=${PIPESTATUS[0]}
set -e

JSON_OUT="${baseline_dir}/target-after.json" TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1 || true
JSON_OUT="${baseline_dir}/ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1 || true

exit "${probe_status}"
