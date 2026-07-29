#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route2-hosted.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-hosted: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_TMP_DIR}"

chapter5_run_stage "${SUMMARY_LOG}" "route2_mode_ttc_entry" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route2_mode_ttc_entry.sh"
chapter5_run_stage "${SUMMARY_LOG}" "route2_link_recovery_and_hk" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route2_link_recovery_and_hk.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-hosted: PASS"
