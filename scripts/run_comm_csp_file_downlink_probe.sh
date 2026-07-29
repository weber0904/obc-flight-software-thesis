#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-csp-file-downlink.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-csp-file-downlink-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-csp-file-downlink-gds-downlink}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/file-downlink-summary.log}"
EXPECTED_DOWNLINK_SLOTS="${HK_TARGET_OCCUPIED_SLOTS:-2}"

mkdir -p "${PROBE_TMP_DIR}"

if ! COMM_TTC_FILE_LINK_MODE=hosted-pty \
  HK_TARGET_OCCUPIED_SLOTS="${EXPECTED_DOWNLINK_SLOTS}" \
  PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
  RUNTIME_ROOT="${RUNTIME_ROOT}" \
  GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
  bash "${ROOT_DIR}/scripts/run_comm_ttc_file_downlink_probe.sh" >"${SUMMARY_LOG}" 2>&1; then
  cat "${SUMMARY_LOG}" >&2
  echo "comm-csp-file-downlink-probe: STOPPED" >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

cat "${SUMMARY_LOG}"

if ! grep -q '^formal-verdict=file-downlink$' "${SUMMARY_LOG}" || \
  ! grep -q '^link-mode=hosted-pty$' "${SUMMARY_LOG}" || \
  ! grep -q '^stage0-ttc-prerequisite=PASS$' "${SUMMARY_LOG}" || \
  ! grep -q '^downlinked-product=PASS ' "${SUMMARY_LOG}" || \
  ! grep -Fxq "source-product-sample-limit=${EXPECTED_DOWNLINK_SLOTS}" "${SUMMARY_LOG}"; then
  echo "comm-csp-file-downlink-probe: STOPPED" >&2
  echo "Hosted COMM file/downlink summary did not contain the required PASS markers." >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

echo "comm-csp-file-downlink-probe: PASS"
echo "formal-verdict=file-downlink"
echo "logs: ${PROBE_TMP_DIR}"
