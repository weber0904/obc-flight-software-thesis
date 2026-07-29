#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-230400}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-endurosat-transparent}"
HOST_PEER_LOG="${HOST_PEER_LOG:-/tmp/obc-host-transparent-peer.log}"

if [[ $# -ge 1 ]]; then
  HOST_SERIAL_DEVICE="$1"
fi
if [[ $# -ge 2 ]]; then
  RPI_COMM_DEVICE="$2"
fi

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${RPI_COMM_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_rpi_endurosat_transparent_probe.sh /path/to/host-serial-device /dev/serial0" >&2
  echo "Set HOST_SERIAL_DEVICE and RPI_COMM_DEVICE to the host and Raspberry Pi UART device paths." >&2
  exit 1
fi

obc_require_systemd_service_name "${SERVICE_NAME:-obc-installed-stack.service}"

"${BIN_DIR}/radio_mock_server" \
  --serial-device "${HOST_SERIAL_DEVICE}" \
  --mode transparent-echo \
  --baudrate "${COMM_BAUDRATE}" >"${HOST_PEER_LOG}" 2>&1 &
HOST_PEER_PID=$!

cleanup() {
  kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
  wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

sleep 1

ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF
set -euo pipefail

cd $(printf '%q' "${RPI_REMOTE_DIR}")
RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")
PERSISTENT_ROOT="\${RUNTIME_ROOT}/persistent-data"
STAGING_ROOT="\${RUNTIME_ROOT}/staging"
LOG_ROOT="\${RUNTIME_ROOT}/logs"
SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
RESTORE_AUTOSTART=0

if systemctl is-active --quiet "\${SERVICE_NAME}"; then
  sudo systemctl stop "\${SERVICE_NAME}"
  RESTORE_AUTOSTART=1
  sleep 2
fi

restore_service() {
  if [[ "\${RESTORE_AUTOSTART}" == "1" ]]; then
    sudo systemctl start "\${SERVICE_NAME}" >/dev/null 2>&1 || true
  fi
}
trap restore_service EXIT

pids=\$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\\/bin\\/Linux\\/(eps_simulator|adcs_simulator|OBC)/ {print \$1}')
if [[ -n "\${pids}" ]]; then
  kill \${pids} >/dev/null 2>&1 || true
  sleep 1
fi

{
  sleep 2
  printf 'uart raw ENDUROSAT_PING\n'
  sleep 1
  printf 'uart raw S_BAND_FRAME_001\n'
  sleep 1
  printf 'uart raw PAYLOAD-ALPHA-123\n'
  sleep 1
  printf 'quit\n'
} | env \
  COMM_DEVICE=$(printf '%q' "${RPI_COMM_DEVICE}") \
  COMM_BAUDRATE=$(printf '%q' "${COMM_BAUDRATE}") \
  RADIO_PROTOCOL=transparent-passive \
  RUNTIME_ROOT="\${RUNTIME_ROOT}" \
  PERSISTENT_ROOT="\${PERSISTENT_ROOT}" \
  STAGING_ROOT="\${STAGING_ROOT}" \
  LOG_ROOT="\${LOG_ROOT}" \
  bash scripts/run_uart_stack.sh
EOF
