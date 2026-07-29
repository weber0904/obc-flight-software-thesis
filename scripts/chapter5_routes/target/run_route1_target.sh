#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route1-target.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route1-target: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_ROOT}"

chapter5_run_stage "${SUMMARY_LOG}" "route1_prepare_and_capture" \
  env PROBE_ROOT="${PROBE_ROOT}/prepare-and-capture" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route1_prepare_and_capture.sh"
chapter5_run_stage "${SUMMARY_LOG}" "route1_soc_fallback_check" \
  env PROBE_ROOT="${PROBE_ROOT}/soc-fallback" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route1_soc_fallback_check.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route1-target: PASS"
