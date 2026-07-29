#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
REMOTE_HOST="${REMOTE_HOST:-$(obc_default_remote_carrier_host)}"
COMM_MODE="${COMM_MODE:-tcp}"
COMM_HOST="${COMM_HOST:-127.0.0.1}"
COMM_PORT="${COMM_PORT:-7000}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-${REMOTE_HOST}}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56620}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57620}"
GDS_HOST="${GDS_HOST:-${REMOTE_HOST}}"
GDS_PORT="${GDS_PORT:-0}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-remote-csp}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
MANAGE_AUTOSTART="${MANAGE_AUTOSTART:-0}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-0}"
REMOTE_SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
OBC_GPS_SOURCE_MODE="${OBC_GPS_SOURCE_MODE:-}"
OBC_GPS_REPLAY_FILE="${OBC_GPS_REPLAY_FILE:-}"
OBC_GPS_SERIAL_DEVICE="${OBC_GPS_SERIAL_DEVICE:-}"
OBC_GPS_BAUDRATE="${OBC_GPS_BAUDRATE:-}"

if [[ "${COMM_MODE}" != "tcp" && "${COMM_MODE}" != "serial" ]]; then
  echo "COMM_MODE must be tcp or serial." >&2
  exit 1
fi

if [[ "${COMM_MODE}" == "serial" && -z "${RPI_COMM_DEVICE}" ]]; then
  echo "RPI_COMM_DEVICE is required when COMM_MODE=serial." >&2
  exit 1
fi

if [[ "${MANAGE_AUTOSTART}" != "0" && "${MANAGE_AUTOSTART}" != "1" ]]; then
  echo "MANAGE_AUTOSTART must be 0 or 1." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi

if [[ "${MANAGE_AUTOSTART}" == "1" ]]; then
  obc_require_systemd_service_name "${REMOTE_SERVICE_NAME}"
fi

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && cd $(printf '%q' "${RPI_REMOTE_DIR}")"
REMOTE_CMD+=" && source scripts/_common.sh"
REMOTE_CMD+=" && BIN_DIR=\"\$(obc_find_native_bin_dir \"\$PWD\")\""
REMOTE_CMD+=" && if [[ -z \"\${BIN_DIR}\" ]]; then echo 'Build output not found on Raspberry Pi. Run bootstrap/build first.' >&2; exit 1; fi"
REMOTE_CMD+=" && REMOTE_SERVICE_NAME=$(printf '%q' "${REMOTE_SERVICE_NAME}")"
REMOTE_CMD+=" && MANAGE_AUTOSTART=$(printf '%q' "${MANAGE_AUTOSTART}")"
REMOTE_CMD+=" && KILL_EXISTING_PIDS=$(printf '%q' "${KILL_EXISTING_PIDS}")"
REMOTE_CMD+=" && RESTORE_AUTOSTART=0"
REMOTE_CMD+=" && if [[ \"\${MANAGE_AUTOSTART}\" == \"1\" ]] && systemctl is-active --quiet \"\${REMOTE_SERVICE_NAME}\"; then sudo systemctl stop \"\${REMOTE_SERVICE_NAME}\"; RESTORE_AUTOSTART=1; sleep 2; fi"
REMOTE_CMD+=" && restore_service() { if [[ \"\${RESTORE_AUTOSTART}\" == \"1\" ]]; then sudo systemctl start \"\${REMOTE_SERVICE_NAME}\" >/dev/null 2>&1 || true; fi; }"
REMOTE_CMD+=" && trap restore_service EXIT"
REMOTE_CMD+=" && if [[ \"\${KILL_EXISTING_PIDS}\" == \"1\" ]]; then PIDS=\$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\\/bin\\/Linux\\/(csp_zmqproxy|eps_simulator|adcs_simulator|radio_mock_server|OBC)/ {print \$1}'); if [[ -n \"\${PIDS}\" ]]; then kill \${PIDS} >/dev/null 2>&1 || true; sleep 1; fi; fi"
REMOTE_CMD+=" && mkdir -p $(printf '%q' "${PERSISTENT_ROOT}") $(printf '%q' "${STAGING_ROOT}") $(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && export CSP_TRANSPORT=$(printf '%q' "${CSP_TRANSPORT}")"
REMOTE_CMD+=" && export CSP_HUB_HOST=$(printf '%q' "${CSP_HUB_HOST}")"
REMOTE_CMD+=" && export CSP_HUB_SUB_PORT=$(printf '%q' "${CSP_HUB_SUB_PORT}")"
REMOTE_CMD+=" && export CSP_HUB_PUB_PORT=$(printf '%q' "${CSP_HUB_PUB_PORT}")"
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
REMOTE_CMD+=" && \"\${BIN_DIR}/OBC\""
REMOTE_CMD+=" --comm $(printf '%q' "${COMM_MODE}")"
if [[ "${COMM_MODE}" == "tcp" ]]; then
  REMOTE_CMD+=" --comm-host $(printf '%q' "${COMM_HOST}")"
  REMOTE_CMD+=" --comm-port $(printf '%q' "${COMM_PORT}")"
else
  REMOTE_CMD+=" --comm-device $(printf '%q' "${RPI_COMM_DEVICE}")"
  REMOTE_CMD+=" --comm-baudrate $(printf '%q' "${COMM_BAUDRATE}")"
fi
REMOTE_CMD+=" --radio-protocol $(printf '%q' "${RADIO_PROTOCOL}")"
REMOTE_CMD+=" --gds-host $(printf '%q' "${GDS_HOST}")"
REMOTE_CMD+=" --gds-port $(printf '%q' "${GDS_PORT}")"
REMOTE_CMD+=" --runtime-root $(printf '%q' "${RUNTIME_ROOT}")"
REMOTE_CMD+=" --persistent-root $(printf '%q' "${PERSISTENT_ROOT}")"
REMOTE_CMD+=" --staging-root $(printf '%q' "${STAGING_ROOT}")"

exec ssh "${OBC_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "${REMOTE_CMD}")"
