#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/chapter5-route2-hosted-link.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"
DUAL_LINK_ROOT="$(mktemp -d "/tmp/ch5r2-dual.XXXXXX")"
HK_ROOT="$(mktemp -d "/tmp/ch5r2-hk.XXXXXX")"

chapter5_init_summary "${SUMMARY_LOG}"
chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-hosted-link-recovery-and-hk: START"
chapter5_log_line "${SUMMARY_LOG}" "dual-link-root=${DUAL_LINK_ROOT}"
chapter5_log_line "${SUMMARY_LOG}" "hk-root=${HK_ROOT}"

chapter5_run_stage "${SUMMARY_LOG}" "dual_link_continuity" \
  env \
    PROBE_TMP_DIR="${DUAL_LINK_ROOT}" \
    bash "${ROOT_DIR}/scripts/run_uhf_node6_backup_probe.sh"
chapter5_run_stage "${SUMMARY_LOG}" "hk_data_product_alignment" \
  env \
    PROBE_TMP_DIR="${HK_ROOT}" \
    GDS_FILE_STORAGE_DIR="${HK_ROOT}/gds-downlink" \
    bash "${ROOT_DIR}/scripts/run_onboard_state_data_hosted_probe.sh"

chapter5_log_line "${SUMMARY_LOG}" "chapter5-route2-hosted-link-recovery-and-hk: PASS"
