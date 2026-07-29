#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
GDS_HOST="${GDS_HOST:-$(obc_default_gds_host)}"
GDS_PORT="${GDS_PORT:-0}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-uart}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"

if [[ $# -ge 1 ]]; then
  RPI_COMM_DEVICE="$1"
fi

if [[ -z "${RPI_COMM_DEVICE}" ]]; then
  echo "Usage: bash scripts/run_rpi_uart_stack.sh /dev/serial0" >&2
  echo "Set RPI_COMM_DEVICE or COMM_DEVICE to the Raspberry Pi UART device path." >&2
  exit 1
fi

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && cd $(printf '%q' "${RPI_REMOTE_DIR}")"
REMOTE_CMD+=" && if systemctl is-active --quiet obc-installed-stack.service; then echo 'obc-installed-stack.service is active; stop it before running the workspace UART stack.' >&2; exit 1; fi"
REMOTE_CMD+=" && mkdir -p $(printf '%q' "${PERSISTENT_ROOT}") $(printf '%q' "${STAGING_ROOT}") $(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && export COMM_DEVICE=$(printf '%q' "${RPI_COMM_DEVICE}")"
REMOTE_CMD+=" && export COMM_BAUDRATE=$(printf '%q' "${COMM_BAUDRATE}")"
REMOTE_CMD+=" && export RADIO_PROTOCOL=$(printf '%q' "${RADIO_PROTOCOL}")"
REMOTE_CMD+=" && export GDS_HOST=$(printf '%q' "${GDS_HOST}")"
REMOTE_CMD+=" && export GDS_PORT=$(printf '%q' "${GDS_PORT}")"
REMOTE_CMD+=" && export CSP_TRANSPORT=$(printf '%q' "${CSP_TRANSPORT}")"
REMOTE_CMD+=" && export RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")"
REMOTE_CMD+=" && export PERSISTENT_ROOT=$(printf '%q' "${PERSISTENT_ROOT}")"
REMOTE_CMD+=" && export STAGING_ROOT=$(printf '%q' "${STAGING_ROOT}")"
REMOTE_CMD+=" && export LOG_ROOT=$(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && exec bash scripts/run_uart_stack.sh"

exec ssh "${OBC_SSH_TARGET}" "${REMOTE_CMD}"
