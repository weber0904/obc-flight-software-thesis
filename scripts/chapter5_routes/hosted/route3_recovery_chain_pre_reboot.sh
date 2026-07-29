#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route3-hosted.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-hosted-pre-reboot: START"

chapter5_run_stage "${SUMMARY_LOG}" "adcs_r3_reset" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route3_adcs_r3_hosted.sh"
chapter5_run_stage "${SUMMARY_LOG}" "eps_timeout_fdir" \
  bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route3_eps_r3_safe_hosted.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route3-hosted-pre-reboot: PASS"
