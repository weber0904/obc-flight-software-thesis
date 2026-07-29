#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-/dev/cu.usbserial-CHANGE_ME}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-/dev/serial0}}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-lab-serial-file-downlink.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-lab-serial-file-downlink-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-lab-serial-file-downlink-gds-downlink}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/file-downlink-summary.log}"
EXPECTED_DOWNLINK_SLOTS="${HK_TARGET_OCCUPIED_SLOTS:-2}"

usage() {
  cat >&2 <<EOF
Usage:
  HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_comm_lab_serial_file_downlink_probe.sh

Defaults:
  HOST_SERIAL_DEVICE=${HOST_SERIAL_DEVICE}
  SUBSYSTEM_SIM_COMM_DEVICE=${SUBSYSTEM_SIM_COMM_DEVICE}
EOF
}

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  usage
  exit 2
fi

mkdir -p "${PROBE_TMP_DIR}"

if ! COMM_TTC_FILE_LINK_MODE=physical-serial \
  HK_TARGET_OCCUPIED_SLOTS="${EXPECTED_DOWNLINK_SLOTS}" \
  HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
  SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE}" \
  PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
  RUNTIME_ROOT="${RUNTIME_ROOT}" \
  GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
  bash "${ROOT_DIR}/scripts/run_comm_ttc_file_downlink_probe.sh" >"${SUMMARY_LOG}" 2>&1; then
  cat "${SUMMARY_LOG}" >&2
  echo "comm-lab-serial-file-downlink-probe: STOPPED" >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

cat "${SUMMARY_LOG}"

if ! grep -q '^formal-verdict=file-downlink$' "${SUMMARY_LOG}" || \
  ! grep -q '^link-mode=physical-serial$' "${SUMMARY_LOG}" || \
  ! grep -q '^stage0-ttc-prerequisite=PASS$' "${SUMMARY_LOG}" || \
  ! grep -q '^downlinked-product=PASS ' "${SUMMARY_LOG}" || \
  ! grep -Fxq "source-product-sample-limit=${EXPECTED_DOWNLINK_SLOTS}" "${SUMMARY_LOG}"; then
  echo "comm-lab-serial-file-downlink-probe: STOPPED" >&2
  echo "Physical COMM file/downlink summary did not contain the required PASS markers." >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

echo "comm-lab-serial-file-downlink-probe: PASS"
echo "formal-verdict=file-downlink"
echo "logs: ${PROBE_TMP_DIR}"
