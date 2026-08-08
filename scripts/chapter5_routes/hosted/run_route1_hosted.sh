#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route1-hosted.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route1-hosted: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_TMP_DIR}"

chapter5_run_stage "${SUMMARY_LOG}" "route1_prepare_and_capture" \
  env PROBE_TMP_DIR="${PROBE_TMP_DIR}/prepare-and-capture" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route1_prepare_and_capture.sh"
chapter5_run_stage "${SUMMARY_LOG}" "route1_soc_fallback_check" \
  env PROBE_TMP_DIR="${PROBE_TMP_DIR}/soc-fallback" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route1_soc_fallback_check.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route1-hosted: PASS"
