#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-}}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-lab-serial-downlink.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-lab-serial-downlink-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/comm-lab-serial-downlink-gds-downlink}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/downlink-summary.log}"

usage() {
  cat >&2 <<EOF
Usage:
  HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_comm_lab_serial_downlink_probe.sh

This is a strict wrapper around the staged physical lab serial ingress probe.
It requires Stage 1 command readback and Stage 2 fprime-cli event/channel
visibility before returning PASS for bounded physical lab serial TT&C.
EOF
}

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  usage
  exit 2
fi

mkdir -p "${PROBE_TMP_DIR}"

if ! HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE}" \
  SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE}" \
  PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
  RUNTIME_ROOT="${RUNTIME_ROOT}" \
  GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
  RUN_STAGE2_DOWNLINK=1 \
  bash "${ROOT_DIR}/scripts/run_comm_lab_serial_ingress_probe.sh" >"${SUMMARY_LOG}" 2>&1; then
  cat "${SUMMARY_LOG}" >&2
  echo "comm-lab-serial-downlink-probe: STOPPED" >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

cat "${SUMMARY_LOG}"

if ! grep -q '^formal-verdict=bounded-ttc$' "${SUMMARY_LOG}" || \
  ! grep -q '^stage1-uplink-ingress=PASS$' "${SUMMARY_LOG}" || \
  ! grep -q '^stage2-downlink=PASS$' "${SUMMARY_LOG}"; then
  echo "comm-lab-serial-downlink-probe: STOPPED" >&2
  echo "Stage 1 command readback passed, but Stage 2 did not prove bounded event/channel downlink." >&2
  echo "logs: ${PROBE_TMP_DIR}" >&2
  exit 1
fi

echo "comm-lab-serial-downlink-probe: PASS"
echo "formal-verdict=bounded-ttc"
echo "logs: ${PROBE_TMP_DIR}"
