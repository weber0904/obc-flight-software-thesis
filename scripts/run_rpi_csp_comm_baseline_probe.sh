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
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56510}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57510}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-csp-comm-baseline}"
HOST_PEER_LOG="${HOST_PEER_LOG:-/tmp/obc-rpi-csp-comm-host-peer.log}"
RPI_PROBE_LOG="${RPI_PROBE_LOG:-/tmp/obc-rpi-csp-comm-baseline.log}"

if [[ $# -ge 1 ]]; then
  HOST_SERIAL_DEVICE="$1"
fi
if [[ $# -ge 2 ]]; then
  RPI_COMM_DEVICE="$2"
fi

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${RPI_COMM_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_rpi_csp_comm_baseline_probe.sh /path/to/host-serial-device /dev/serial0" >&2
  echo "Set HOST_SERIAL_DEVICE and RPI_COMM_DEVICE to the host and Raspberry Pi UART device paths." >&2
  exit 1
fi

obc_require_systemd_service_name "${SERVICE_NAME:-obc-installed-stack.service}"

"${BIN_DIR}/radio_mock_server" \
  --serial-device "${HOST_SERIAL_DEVICE}" \
  --mode "${RADIO_PROTOCOL}" \
  --baudrate "${COMM_BAUDRATE}" >"${HOST_PEER_LOG}" 2>&1 &
HOST_PEER_PID=$!

cleanup() {
  if [[ -n "${HOST_PEER_PID:-}" ]]; then
    kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
    wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

sleep 1

if ! kill -0 "${HOST_PEER_PID}" >/dev/null 2>&1; then
  echo "Host serial peer failed to start" >&2
  cat "${HOST_PEER_LOG}" >&2 || true
  exit 1
fi

ssh "${OBC_SSH_TARGET}" /bin/bash >"${RPI_PROBE_LOG}" 2>&1 <<EOF
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

pids=\$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\\/bin\\/Linux\\/(csp_zmqproxy|eps_simulator|adcs_simulator|OBC)/ {print \$1}')
if [[ -n "\${pids}" ]]; then
  kill \${pids} >/dev/null 2>&1 || true
  sleep 1
fi

{
  sleep 2
  printf 'status\n'
  sleep 1
  printf 'csp ping 2\n'
  sleep 1
  printf 'csp ping 3\n'
  sleep 1
  printf 'eps get\n'
  sleep 1
  printf 'adcs get\n'
  sleep 1
  printf 'radio status\n'
  sleep 1
  printf 'radio enable on\n'
  sleep 1
  printf 'uart raw STATUS\n'
  sleep 1
  printf 'quit\n'
} | env \
  COMM_DEVICE=$(printf '%q' "${RPI_COMM_DEVICE}") \
  COMM_BAUDRATE=$(printf '%q' "${COMM_BAUDRATE}") \
  RADIO_PROTOCOL=$(printf '%q' "${RADIO_PROTOCOL}") \
  CSP_HUB_SUB_PORT=$(printf '%q' "${CSP_HUB_SUB_PORT}") \
  CSP_HUB_PUB_PORT=$(printf '%q' "${CSP_HUB_PUB_PORT}") \
  RUNTIME_ROOT="\${RUNTIME_ROOT}" \
  PERSISTENT_ROOT="\${PERSISTENT_ROOT}" \
  STAGING_ROOT="\${STAGING_ROOT}" \
  LOG_ROOT="\${LOG_ROOT}" \
  bash scripts/run_uart_stack.sh
EOF

ping_count="$(grep -c 'csp ping response=0 success=yes' "${RPI_PROBE_LOG}" || true)"
if [[ "${ping_count}" -lt 2 ]]; then
  echo "Expected successful CSP ping to both EPS and ADCS nodes" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'Comm mode: serial' "${RPI_PROBE_LOG}"; then
  echo "Target OBC did not start in serial comm mode" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'eps soc=' "${RPI_PROBE_LOG}"; then
  echo "Target status did not include EPS state" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'adcs mode=' "${RPI_PROBE_LOG}"; then
  echo "Target status did not include ADCS state" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'radio enable response=0' "${RPI_PROBE_LOG}"; then
  echo "Radio enable command did not succeed" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'uart response: STATUS enabled=1' "${RPI_PROBE_LOG}"; then
  echo "UART raw STATUS exchange did not return enabled radio state" >&2
  cat "${RPI_PROBE_LOG}" >&2 || true
  exit 1
fi

echo "=== Host Serial Peer Highlights ==="
grep -E 'listening|accepted|STATUS|ENABLE|GET_STATUS|rx|tx' "${HOST_PEER_LOG}" || cat "${HOST_PEER_LOG}"
echo "=== Raspberry Pi CSP + Comm Highlights ==="
grep -E 'Comm mode:|csp initialized=|csp ping response=|eps soc=|adcs mode=|radio enable response=|uart response:' "${RPI_PROBE_LOG}"
