#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

SERIAL_DEVICE="${SERIAL_DEVICE:-${HOST_SERIAL_DEVICE:-}}"
HOST_PEER_MODE="${HOST_PEER_MODE:-mock-text}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
HOST_PEER_MAX_FRAMED_EXCHANGES="${HOST_PEER_MAX_FRAMED_EXCHANGES:-0}"
if [[ $# -ge 1 ]]; then
  SERIAL_DEVICE="$1"
fi

if [[ -z "${SERIAL_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_host_serial_peer.sh /path/to/serial-device" >&2
  echo "Set SERIAL_DEVICE or HOST_SERIAL_DEVICE to the host tty path." >&2
  exit 1
fi

ARGS=(
  --serial-device "${SERIAL_DEVICE}"
  --mode "${HOST_PEER_MODE}"
  --baudrate "${COMM_BAUDRATE}"
)

if [[ "${HOST_PEER_MAX_FRAMED_EXCHANGES}" != "0" ]]; then
  ARGS+=(--max-framed-exchanges "${HOST_PEER_MAX_FRAMED_EXCHANGES}")
fi

exec "${BIN_DIR}/radio_mock_server" "${ARGS[@]}"
