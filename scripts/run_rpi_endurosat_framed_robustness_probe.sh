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
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-endurosat-framed-robustness}"
HOST_PEER_PHASE1_LOG="${HOST_PEER_PHASE1_LOG:-/tmp/obc-host-framed-peer-phase1.log}"
HOST_PEER_PHASE2_LOG="${HOST_PEER_PHASE2_LOG:-/tmp/obc-host-framed-peer-phase2.log}"
RPI_PROBE_LOG="${RPI_PROBE_LOG:-/tmp/obc-rpi-framed-robustness.log}"

if [[ $# -ge 1 ]]; then
  HOST_SERIAL_DEVICE="$1"
fi
if [[ $# -ge 2 ]]; then
  RPI_COMM_DEVICE="$2"
fi

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${RPI_COMM_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_rpi_endurosat_framed_robustness_probe.sh /path/to/host-serial-device /dev/serial0" >&2
  echo "Set HOST_SERIAL_DEVICE and RPI_COMM_DEVICE to the host and Raspberry Pi UART device paths." >&2
  exit 1
fi

obc_require_systemd_service_name "${SERVICE_NAME:-obc-installed-stack.service}"

start_host_peer() {
  local log_path="$1"
  local max_frames="$2"
  : >"${log_path}"
  local args=(
    --serial-device "${HOST_SERIAL_DEVICE}"
    --mode transparent-framed-echo
    --baudrate "${COMM_BAUDRATE}"
  )
  if [[ "${max_frames}" != "0" ]]; then
    args+=(--max-framed-exchanges "${max_frames}")
  fi
  "${BIN_DIR}/radio_mock_server" "${args[@]}" >"${log_path}" 2>&1 &
  HOST_PEER_PID=$!
  sleep 1
  if ! kill -0 "${HOST_PEER_PID}" >/dev/null 2>&1; then
    echo "Host framed peer failed to start" >&2
    cat "${log_path}" >&2 || true
    exit 1
  fi
}

cleanup() {
  if [[ -n "${HOST_PEER_PID:-}" ]]; then
    kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
    wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SSH_PID:-}" ]]; then
    wait "${SSH_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

start_host_peer "${HOST_PEER_PHASE1_LOG}" 3

ssh "${OBC_SSH_TARGET}" /bin/bash >"${RPI_PROBE_LOG}" 2>&1 <<EOF &
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
  sleep 3
  printf 'uart frame-hex 454E4455524F5341545F50494E47\n'
  sleep 1
  printf 'uart frame-hex 007E7D414243FF10\n'
  sleep 1
  printf 'uart frame-hex 0102030405060708090A0B0C0D0E0F\n'
  sleep 2
  printf 'uart frame-hex DEADBEEF0011\n'
  sleep 4
  printf 'uart frame-hex A1B2C3D4E5F60708\n'
  sleep 1
  printf 'uart frame-hex 5566778899AABBCC\n'
  sleep 2
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
SSH_PID=$!

wait "${HOST_PEER_PID}"
HOST_PEER_PID=""

sleep 3
start_host_peer "${HOST_PEER_PHASE2_LOG}" 0

wait "${SSH_PID}"
SSH_PID=""

kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
HOST_PEER_PID=""
trap - EXIT INT TERM

phase1_count="$(grep -c 'transparent-framed-echo frame=' "${HOST_PEER_PHASE1_LOG}" || true)"
phase2_count="$(grep -c 'transparent-framed-echo frame=' "${HOST_PEER_PHASE2_LOG}" || true)"

if [[ "${phase1_count}" -lt 3 ]]; then
  echo "Phase 1 peer did not observe three framed payloads" >&2
  cat "${HOST_PEER_PHASE1_LOG}" >&2 || true
  exit 1
fi
if [[ "${phase2_count}" -lt 1 ]]; then
  echo "Phase 2 peer did not observe any post-restart framed payloads" >&2
  cat "${HOST_PEER_PHASE2_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'uart framed exchange failed' "${RPI_PROBE_LOG}"; then
  echo "Reconnect probe did not observe an exchange failure during peer downtime" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'uart frame response hex: A1B2C3D4E5F60708' "${RPI_PROBE_LOG}"; then
  echo "Reconnect probe did not observe the post-restart framed response" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi

echo "=== Phase 1 Host Peer ==="
grep 'transparent-framed-echo' "${HOST_PEER_PHASE1_LOG}"
echo "=== Phase 2 Host Peer ==="
grep 'transparent-framed-echo' "${HOST_PEER_PHASE2_LOG}"
echo "=== Raspberry Pi Probe Highlights ==="
grep -E 'uart frame response hex:|uart framed exchange failed' "${RPI_PROBE_LOG}"
