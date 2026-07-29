#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route2-target.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_ROOT}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-target: START"
chapter5_log_line "${SUMMARY_LOG}" "probe-root=${PROBE_ROOT}"

chapter5_run_stage "${SUMMARY_LOG}" "route2_mode_ttc_entry" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route2_mode_ttc_entry.sh"
chapter5_run_stage "${SUMMARY_LOG}" "route2_link_recovery_and_hk" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/target/route2_link_recovery_and_hk.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-target: PASS"
