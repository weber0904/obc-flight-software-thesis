#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
COMM_MODE="${COMM_MODE:-tcp}"
COMM_HOST="${COMM_HOST:-127.0.0.1}"
COMM_PORT="${COMM_PORT:-7000}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
GDS_HOST="${GDS_HOST:-$(obc_default_gds_host)}"
GDS_PORT="${GDS_PORT:-0}"
GROUND_LINK_MODE="${GROUND_LINK_MODE:-direct-tcp}"
COMM_CSP_NODE="${COMM_CSP_NODE:-4}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-}"
COMMAND_AUTH="${COMMAND_AUTH:-}"
COMMAND_AUTH_SOURCE_ID="${COMMAND_AUTH_SOURCE_ID:-}"
COMMAND_AUTH_KEY_SLOT="${COMMAND_AUTH_KEY_SLOT:-}"
COMMAND_AUTH_KEY_HEX="${COMMAND_AUTH_KEY_HEX:-}"
CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-socketcan}")"
OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE:-can0}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
COMM_CSP_SOCKETCAN_USE_CANFD="${COMM_CSP_SOCKETCAN_USE_CANFD:-0}"
COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST:-}"
COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST="${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST:-}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-canfd-csp}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
MANAGE_AUTOSTART="${MANAGE_AUTOSTART:-0}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-0}"
REMOTE_SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
HEADLESS="${HEADLESS:-0}"
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
if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "Only CSP_TRANSPORT=socketcan is supported by this stack launcher." >&2
  exit 1
fi
if [[ "${GROUND_LINK_MODE}" != "direct-tcp" && "${GROUND_LINK_MODE}" != "comm-csp" && "${GROUND_LINK_MODE}" != "disabled" ]]; then
  echo "GROUND_LINK_MODE must be direct-tcp, comm-csp, or disabled." >&2
  exit 1
fi
if [[ ! "${COMM_CSP_NODE}" =~ ^[0-9]+$ || "${COMM_CSP_NODE}" == "0" ]]; then
  echo "COMM_CSP_NODE must be a positive integer." >&2
  exit 1
fi
if [[ "${CSP_CAN_PROMISC}" != "0" && "${CSP_CAN_PROMISC}" != "1" ]]; then
  echo "CSP_CAN_PROMISC must be 0 or 1." >&2
  exit 1
fi
if [[ "${COMM_CSP_SOCKETCAN_USE_CANFD}" != "0" && "${COMM_CSP_SOCKETCAN_USE_CANFD}" != "1" ]]; then
  echo "COMM_CSP_SOCKETCAN_USE_CANFD must be 0 or 1." >&2
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

obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
if [[ "${MANAGE_AUTOSTART}" == "1" ]]; then
  obc_require_systemd_service_name "${REMOTE_SERVICE_NAME}"
fi

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && cd $(printf '%q' "${RPI_REMOTE_DIR}")"
REMOTE_CMD+=" && source scripts/_common.sh"
REMOTE_CMD+=" && BIN_DIR=\"\$(obc_find_native_bin_dir \"\$PWD\")\""
REMOTE_CMD+=" && if [[ -z \"\${BIN_DIR}\" ]]; then echo 'Build output not found on Raspberry Pi. Run bootstrap/build first.' >&2; exit 1; fi"
REMOTE_CMD+=" && CAN_PARENTDEV=\"\$(obc_can_parentdev $(printf '%q' "${OBC_CSP_CAN_DEVICE}") || true)\""
REMOTE_CMD+=" && if [[ -z \"\${CAN_PARENTDEV}\" ]]; then echo 'Configured OBC CAN device is missing or unresolved.' >&2; exit 1; fi"
REMOTE_CMD+=" && REMOTE_SERVICE_NAME=$(printf '%q' "${REMOTE_SERVICE_NAME}")"
REMOTE_CMD+=" && MANAGE_AUTOSTART=$(printf '%q' "${MANAGE_AUTOSTART}")"
REMOTE_CMD+=" && KILL_EXISTING_PIDS=$(printf '%q' "${KILL_EXISTING_PIDS}")"
REMOTE_CMD+=" && RESTORE_AUTOSTART=0"
REMOTE_CMD+=" && if [[ \"\${MANAGE_AUTOSTART}\" == \"1\" ]] && systemctl is-active --quiet \"\${REMOTE_SERVICE_NAME}\"; then sudo systemctl stop \"\${REMOTE_SERVICE_NAME}\"; RESTORE_AUTOSTART=1; sleep 2; fi"
REMOTE_CMD+=" && restore_service() { if [[ \"\${RESTORE_AUTOSTART}\" == \"1\" ]]; then sudo systemctl start \"\${REMOTE_SERVICE_NAME}\" >/dev/null 2>&1 || true; fi; }"
REMOTE_CMD+=" && trap restore_service EXIT"
REMOTE_CMD+=" && if [[ \"\${KILL_EXISTING_PIDS}\" == \"1\" ]]; then PIDS=\$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(OBC|radio_mock_server)' || true); if [[ -n \"\${PIDS}\" ]]; then kill \${PIDS} >/dev/null 2>&1 || true; sleep 1; fi; fi"
REMOTE_CMD+=" && mkdir -p $(printf '%q' "${PERSISTENT_ROOT}") $(printf '%q' "${STAGING_ROOT}") $(printf '%q' "${LOG_ROOT}")"
REMOTE_CMD+=" && echo 'Starting OBC CAN stack'"
REMOTE_CMD+=" && echo '  Execution host : obc'"
REMOTE_CMD+=" && echo '  Active role    : OBC on shared SocketCAN bus'"
REMOTE_CMD+=" && echo '  Transport      : $(printf '%q' "${CSP_TRANSPORT}")'"
REMOTE_CMD+=" && echo '  CAN device     : $(printf '%q' "${OBC_CSP_CAN_DEVICE}") (\${CAN_PARENTDEV})'"
REMOTE_CMD+=" && echo '  Promisc        : $(printf '%q' "${CSP_CAN_PROMISC}")'"
REMOTE_CMD+=" && echo '  CAN FD         : $(printf '%q' "${COMM_CSP_SOCKETCAN_USE_CANFD}")'"
REMOTE_CMD+=" && echo '  CAN FD dst     : $(printf '%q' "${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}")'"
REMOTE_CMD+=" && echo '  CAN FD dport   : $(printf '%q' "${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}")'"
REMOTE_CMD+=" && echo '  Ground link    : $(printf '%q' "${GROUND_LINK_MODE}")'"
REMOTE_CMD+=" && echo '  COMM node      : $(printf '%q' "${COMM_CSP_NODE}")'"
REMOTE_CMD+=" && echo '  GDS target     : $(printf '%q' "${GDS_HOST}:${GDS_PORT}")'"
REMOTE_CMD+=" && export CSP_TRANSPORT=$(printf '%q' "${CSP_TRANSPORT}")"
REMOTE_CMD+=" && export CSP_CAN_DEVICE=$(printf '%q' "${OBC_CSP_CAN_DEVICE}")"
REMOTE_CMD+=" && export CSP_CAN_PROMISC=$(printf '%q' "${CSP_CAN_PROMISC}")"
REMOTE_CMD+=" && export COMM_CSP_SOCKETCAN_USE_CANFD=$(printf '%q' "${COMM_CSP_SOCKETCAN_USE_CANFD}")"
REMOTE_CMD+=" && export COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=$(printf '%q' "${COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST}")"
REMOTE_CMD+=" && export COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=$(printf '%q' "${COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST}")"
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
if [[ -n "${HEADLESS}" ]]; then
  REMOTE_CMD+=" && export HEADLESS=$(printf '%q' "${HEADLESS}")"
fi
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
REMOTE_CMD+=" --ground-link $(printf '%q' "${GROUND_LINK_MODE}")"
REMOTE_CMD+=" --comm-csp-node $(printf '%q' "${COMM_CSP_NODE}")"
REMOTE_CMD+=" --gds-host $(printf '%q' "${GDS_HOST}")"
REMOTE_CMD+=" --gds-port $(printf '%q' "${GDS_PORT}")"
REMOTE_CMD+=" --runtime-root $(printf '%q' "${RUNTIME_ROOT}")"
REMOTE_CMD+=" --persistent-root $(printf '%q' "${PERSISTENT_ROOT}")"
REMOTE_CMD+=" --staging-root $(printf '%q' "${STAGING_ROOT}")"
if [[ -n "${COMMAND_AUTHORITY_PROFILE}" ]]; then
  REMOTE_CMD+=" --command-authority-profile $(printf '%q' "${COMMAND_AUTHORITY_PROFILE}")"
fi
case "${HEADLESS}" in
  1|true|TRUE|yes|YES)
    REMOTE_CMD+=" --headless"
    ;;
esac

obc_ssh "${OBC_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "${REMOTE_CMD}")"
