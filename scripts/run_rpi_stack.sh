#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
GDS_HOST="${GDS_HOST:-$(obc_default_gds_host)}"
GDS_PORT="${GDS_PORT:-0}"
RADIO_PORT="${RADIO_PORT:-7000}"
OBC_BINARY_NAME="${OBC_BINARY_NAME:-OBC}"
GROUND_LINK_MODE="${GROUND_LINK_MODE:-direct-tcp}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-6100}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-7100}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
OBC_GPS_SOURCE_MODE="${OBC_GPS_SOURCE_MODE:-}"
OBC_GPS_REPLAY_FILE="${OBC_GPS_REPLAY_FILE:-}"
OBC_GPS_SERIAL_DEVICE="${OBC_GPS_SERIAL_DEVICE:-}"
OBC_GPS_BAUDRATE="${OBC_GPS_BAUDRATE:-}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-}"
COMMAND_AUTH="${COMMAND_AUTH:-}"
COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-}"
COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-}"
COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-}"
UHF_BEACON_CSP_NODE="${UHF_BEACON_CSP_NODE:-}"
HEADLESS="${HEADLESS:-}"

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && cd $(printf '%q' "${RPI_REMOTE_DIR}")"
REMOTE_CMD+=" && mkdir -p $(printf '%q' "${PERSISTENT_ROOT}") $(printf '%q' "${STAGING_ROOT}") $(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && export GDS_HOST=$(printf '%q' "${GDS_HOST}")"
REMOTE_CMD+=" && export GDS_PORT=$(printf '%q' "${GDS_PORT}")"
REMOTE_CMD+=" && export RADIO_PORT=$(printf '%q' "${RADIO_PORT}")"
REMOTE_CMD+=" && export OBC_BINARY_NAME=$(printf '%q' "${OBC_BINARY_NAME}")"
REMOTE_CMD+=" && export GROUND_LINK_MODE=$(printf '%q' "${GROUND_LINK_MODE}")"
REMOTE_CMD+=" && export COMM_CSP_NODE=$(printf '%q' "${COMM_CSP_NODE}")"
REMOTE_CMD+=" && export MANAGE_SBAND_COMM_NODE=0"
REMOTE_CMD+=" && export MANAGE_GROUND_TTC_GATEWAY=0"
REMOTE_CMD+=" && export CSP_TRANSPORT=$(printf '%q' "${CSP_TRANSPORT}")"
REMOTE_CMD+=" && export CSP_HUB_HOST=$(printf '%q' "${CSP_HUB_HOST}")"
REMOTE_CMD+=" && export CSP_HUB_SUB_PORT=$(printf '%q' "${CSP_HUB_SUB_PORT}")"
REMOTE_CMD+=" && export CSP_HUB_PUB_PORT=$(printf '%q' "${CSP_HUB_PUB_PORT}")"
REMOTE_CMD+=" && export EPS_CSP_NODE_ID=$(printf '%q' "${EPS_CSP_NODE_ID}")"
REMOTE_CMD+=" && export ADCS_CSP_NODE_ID=$(printf '%q' "${ADCS_CSP_NODE_ID}")"
REMOTE_CMD+=" && export RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")"
REMOTE_CMD+=" && export PERSISTENT_ROOT=$(printf '%q' "${PERSISTENT_ROOT}")"
REMOTE_CMD+=" && export STAGING_ROOT=$(printf '%q' "${STAGING_ROOT}")"
REMOTE_CMD+=" && export LOG_ROOT=$(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && export OBC_CLEANUP_RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")"
if [[ -n "${OBC_GPS_SOURCE_MODE}" ]]; then
  REMOTE_CMD+=" && export OBC_GPS_SOURCE_MODE=$(printf '%q' "${OBC_GPS_SOURCE_MODE}")"
fi
if [[ -n "${OBC_GPS_REPLAY_FILE}" ]]; then
  REMOTE_CMD+=" && export OBC_GPS_REPLAY_FILE=$(printf '%q' "${OBC_GPS_REPLAY_FILE}")"
fi
if [[ -n "${OBC_GPS_SERIAL_DEVICE}" ]]; then
  REMOTE_CMD+=" && export OBC_GPS_SERIAL_DEVICE=$(printf '%q' "${OBC_GPS_SERIAL_DEVICE}")"
fi
if [[ -n "${OBC_GPS_BAUDRATE}" ]]; then
  REMOTE_CMD+=" && export OBC_GPS_BAUDRATE=$(printf '%q' "${OBC_GPS_BAUDRATE}")"
fi
if [[ -n "${COMMAND_AUTHORITY_PROFILE}" ]]; then
  REMOTE_CMD+=" && export COMMAND_AUTHORITY_PROFILE=$(printf '%q' "${COMMAND_AUTHORITY_PROFILE}")"
fi
if [[ -n "${COMMAND_AUTH}" ]]; then
  REMOTE_CMD+=" && export COMMAND_AUTH=$(printf '%q' "${COMMAND_AUTH}")"
fi
if [[ -n "${COMMAND_AUTH_SOURCE_ID}" ]]; then
  REMOTE_CMD+=" && export COMMAND_AUTH_SOURCE_ID=$(printf '%q' "${COMMAND_AUTH_SOURCE_ID}")"
fi
if [[ -n "${COMMAND_AUTH_KEY_SLOT}" ]]; then
  REMOTE_CMD+=" && export COMMAND_AUTH_KEY_SLOT=$(printf '%q' "${COMMAND_AUTH_KEY_SLOT}")"
fi
if [[ -n "${COMMAND_AUTH_KEY_HEX}" ]]; then
  REMOTE_CMD+=" && export COMMAND_AUTH_KEY_HEX=$(printf '%q' "${COMMAND_AUTH_KEY_HEX}")"
fi
if [[ -n "${UHF_BEACON_CSP_NODE}" ]]; then
  REMOTE_CMD+=" && export UHF_BEACON_CSP_NODE=$(printf '%q' "${UHF_BEACON_CSP_NODE}")"
fi
if [[ -n "${HEADLESS}" ]]; then
  REMOTE_CMD+=" && export HEADLESS=$(printf '%q' "${HEADLESS}")"
fi
REMOTE_CMD+=" && exec bash scripts/run_dev_stack.sh"

exec ssh "${OBC_SSH_TARGET}" "${REMOTE_CMD}"
