#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-$(obc_default_remote_workspace_dir)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
REMOTE_HOST="${REMOTE_HOST:-$(obc_default_remote_carrier_host)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
PORT_SEED="${PORT_SEED:-$$}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/dual-pi-split-host-comm}"
GROUND_STACK_LOG="${GROUND_STACK_LOG:-/tmp/obc-dual-pi-comm-ground.log}"
SUBSYSTEM_STACK_LOG="${SUBSYSTEM_STACK_LOG:-/tmp/obc-dual-pi-comm-subsystem.log}"
HOST_PEER_LOG="${HOST_PEER_LOG:-/tmp/obc-dual-pi-comm-host-peer.log}"
PROBE_LOG="${PROBE_LOG:-/tmp/obc-dual-pi-comm-probe.log}"
REMOTE_SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${RPI_COMM_DEVICE}" ]]; then
  echo "Set HOST_SERIAL_DEVICE and RPI_COMM_DEVICE for the dual-Pi comm coexistence probe." >&2
  exit 1
fi

obc_require_systemd_service_name "${REMOTE_SERVICE_NAME}"

port_is_available() {
  local port="${1:?port is required}"
  ! lsof -nP -iTCP:"${port}" >/dev/null 2>&1
}

find_free_port_pair() {
  local first_port="${1:?first_port is required}"
  local second_offset="${2:?second_offset is required}"
  local candidate="${first_port}"
  while ! port_is_available "${candidate}" || ! port_is_available "$((candidate + second_offset))"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

if [[ -z "${CSP_HUB_SUB_PORT:-}" ]]; then
  CSP_HUB_SUB_PORT="$(find_free_port_pair "$((56600 + (PORT_SEED % 300)))" 1000)"
fi
if [[ -z "${CSP_HUB_PUB_PORT:-}" ]]; then
  CSP_HUB_PUB_PORT="$((CSP_HUB_SUB_PORT + 1000))"
fi
if [[ -z "${GDS_PORT:-}" ]]; then
  GDS_PORT="$(find_free_port_pair "$((51600 + (PORT_SEED % 300)))" 1)"
fi
if [[ -z "${GDS_TTS_PORT:-}" ]]; then
  GDS_TTS_PORT="$((GDS_PORT + 1))"
fi

cleanup() {
  if [[ -n "${HOST_PEER_PID:-}" ]]; then
    kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
    wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SUBSYSTEM_STACK_PID:-}" ]]; then
    kill "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
    wait "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${GROUND_STACK_PID:-}" ]]; then
    kill "${GROUND_STACK_PID}" >/dev/null 2>&1 || true
    wait "${GROUND_STACK_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT INT TERM

env \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  GDS_PORT="${GDS_PORT}" \
  GDS_TTS_PORT="${GDS_TTS_PORT}" \
  bash "${ROOT_DIR}/scripts/run_ground_csp_gds_stack.sh" >"${GROUND_STACK_LOG}" 2>&1 &
GROUND_STACK_PID=$!

env \
  SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
  SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR}" \
  REMOTE_HOST="${REMOTE_HOST}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  bash "${ROOT_DIR}/scripts/run_subsystem_sim_remote_stack.sh" >"${SUBSYSTEM_STACK_LOG}" 2>&1 &
SUBSYSTEM_STACK_PID=$!

sleep 4
if ! kill -0 "${GROUND_STACK_PID}" >/dev/null 2>&1; then
  echo "Ground stack failed to start." >&2
  cat "${GROUND_STACK_LOG}" >&2 || true
  exit 1
fi
if ! kill -0 "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1; then
  echo "Subsystem simulator stack failed to start." >&2
  cat "${SUBSYSTEM_STACK_LOG}" >&2 || true
  exit 1
fi

"${BIN_DIR}/radio_mock_server" \
  --serial-device "${HOST_SERIAL_DEVICE}" \
  --mode "${RADIO_PROTOCOL}" \
  --baudrate "${COMM_BAUDRATE}" >"${HOST_PEER_LOG}" 2>&1 &
HOST_PEER_PID=$!

sleep 1
if ! kill -0 "${HOST_PEER_PID}" >/dev/null 2>&1; then
  echo "Host serial peer failed to start." >&2
  cat "${HOST_PEER_LOG}" >&2 || true
  exit 1
fi

{
  sleep 12
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
  OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
  RPI_REMOTE_DIR="${RPI_REMOTE_DIR}" \
  REMOTE_HOST="${REMOTE_HOST}" \
  COMM_MODE=serial \
  RPI_COMM_DEVICE="${RPI_COMM_DEVICE}" \
  COMM_BAUDRATE="${COMM_BAUDRATE}" \
  RADIO_PROTOCOL="${RADIO_PROTOCOL}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  GDS_PORT=0 \
  RUNTIME_ROOT="${RUNTIME_ROOT}" \
  MANAGE_AUTOSTART=1 \
  KILL_EXISTING_PIDS=1 \
  SERVICE_NAME="${REMOTE_SERVICE_NAME}" \
  bash "${ROOT_DIR}/scripts/run_rpi_remote_csp_stack.sh" >"${PROBE_LOG}" 2>&1

PING_COUNT="$(grep -c 'csp ping response=0 success=yes' "${PROBE_LOG}" || true)"
if [[ "${PING_COUNT}" -lt 2 ]]; then
  echo "Expected successful CSP ping to both remote simulator nodes." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'Comm mode: serial' "${PROBE_LOG}"; then
  echo "Target OBC did not start in serial comm mode." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'eps soc=' "${PROBE_LOG}"; then
  echo "Target status did not include EPS state." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'adcs mode=' "${PROBE_LOG}"; then
  echo "Target status did not include ADCS state." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'radio enable response=0' "${PROBE_LOG}"; then
  echo "Radio enable command did not succeed." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'uart response: STATUS enabled=1' "${PROBE_LOG}"; then
  echo "UART raw STATUS exchange did not return enabled radio state." >&2
  cat "${PROBE_LOG}" >&2 || true
  exit 1
fi

echo "=== Host Serial Peer Highlights ==="
grep -E 'listening|accepted|STATUS|ENABLE|GET_STATUS|rx|tx' "${HOST_PEER_LOG}" || cat "${HOST_PEER_LOG}"
echo "=== Dual-Pi Split-Host Coexistence Highlights ==="
grep -E 'Comm mode:|csp ping response=|eps soc=|adcs mode=|radio enable response=|uart response:' "${PROBE_LOG}"
