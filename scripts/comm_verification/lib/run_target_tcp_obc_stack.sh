#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found on OBC host. Run bootstrap/build first." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-zmqhub}")"
if [[ "${CSP_TRANSPORT}" != "zmqhub" ]]; then
  echo "Target TCP parity OBC stack requires CSP_TRANSPORT=zmqhub." >&2
  exit 1
fi

CSP_HUB_HOST="${CSP_HUB_HOST:-${REMOTE_HOST:-}}"
if [[ -z "${CSP_HUB_HOST}" ]]; then
  echo "CSP_HUB_HOST or REMOTE_HOST is required." >&2
  exit 1
fi

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56630}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57630}"
COMM_MODE="${COMM_MODE:-tcp}"
COMM_HOST="${COMM_HOST:-127.0.0.1}"
COMM_PORT="${COMM_PORT:-7000}"
RPI_COMM_DEVICE="${RPI_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
GROUND_LINK_MODE="${GROUND_LINK_MODE:-comm-csp}"
COMM_CSP_NODE="${COMM_CSP_NODE:-5}"
GDS_HOST="${GDS_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-0}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${ROOT_DIR}/runtime/target-tcp-parity}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-sband-primary}"
INITIAL_COMM_BAND="${INITIAL_COMM_BAND:-sband}"
ENABLE_PRIMARY_GROUND_LINK_DRIVER="${ENABLE_PRIMARY_GROUND_LINK_DRIVER:-1}"
ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR="${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR:-1}"
COMM_SUBSYSTEM_PING_TIMEOUT_MS="${COMM_SUBSYSTEM_PING_TIMEOUT_MS:-1000}"
COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD="${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD:-10}"
DIAGNOSTIC_QUIET_PACKET_EGRESS="${DIAGNOSTIC_QUIET_PACKET_EGRESS:-0}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"
HEADLESS="${HEADLESS:-0}"
MANAGE_RADIO_MOCK_SERVER="${MANAGE_RADIO_MOCK_SERVER:-0}"
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
if [[ "${GROUND_LINK_MODE}" != "direct-tcp" && "${GROUND_LINK_MODE}" != "comm-csp" && "${GROUND_LINK_MODE}" != "disabled" ]]; then
  echo "GROUND_LINK_MODE must be direct-tcp, comm-csp, or disabled." >&2
  exit 1
fi
if [[ "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" != "0" && "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" != "1" ]]; then
  echo "ENABLE_PRIMARY_GROUND_LINK_DRIVER must be 0 or 1." >&2
  exit 1
fi
if [[ "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" != "0" && "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" != "1" ]]; then
  echo "ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR must be 0 or 1." >&2
  exit 1
fi
if ! [[ "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}" =~ ^[0-9]+$ ]] || [[ "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}" == "0" ]]; then
  echo "COMM_SUBSYSTEM_PING_TIMEOUT_MS must be a non-zero integer." >&2
  exit 1
fi
if ! [[ "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}" =~ ^[0-9]+$ ]] || [[ "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}" == "0" ]]; then
  echo "COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD must be a non-zero integer." >&2
  exit 1
fi
if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" != "0" && "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" != "1" ]]; then
  echo "DIAGNOSTIC_QUIET_PACKET_EGRESS must be 0 or 1." >&2
  exit 1
fi
if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi
if [[ "${MANAGE_RADIO_MOCK_SERVER}" != "0" && "${MANAGE_RADIO_MOCK_SERVER}" != "1" ]]; then
  echo "MANAGE_RADIO_MOCK_SERVER must be 0 or 1." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" == "1" ]]; then
  PIDS="$(pgrep -f 'build-fprime-automatic-native/bin/Linux/(OBC|radio_mock_server)' || true)"
  if [[ -n "${PIDS}" ]]; then
    kill ${PIDS} >/dev/null 2>&1 || true
    sleep 1
  fi
fi

mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}"

echo "Starting target TCP parity OBC stack"
echo "  Execution host : obc.local"
echo "  CSP hub        : ${CSP_HUB_HOST}:${CSP_HUB_SUB_PORT}/${CSP_HUB_PUB_PORT}"
echo "  Ground link    : ${GROUND_LINK_MODE} node=${COMM_CSP_NODE}"
echo "  Comm mode      : ${COMM_MODE}"
echo "  Authority      : ${COMMAND_AUTHORITY_PROFILE}"
echo "  Initial band   : ${INITIAL_COMM_BAND}"
echo "  Subsystem ping : ${COMM_SUBSYSTEM_PING_TIMEOUT_MS}ms threshold=${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}"
if [[ "${COMM_MODE}" == "tcp" && "${MANAGE_RADIO_MOCK_SERVER}" == "1" ]]; then
  echo "  Radio peer     : radio_mock_server tcp=127.0.0.1:${COMM_PORT}"
fi
if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" == "1" ]]; then
  echo "  Diagnostic quiet packet egress: enabled"
fi

export CSP_TRANSPORT
export CSP_HUB_HOST
export CSP_HUB_SUB_PORT
export CSP_HUB_PUB_PORT
if [[ -n "${OBC_GPS_SOURCE_MODE}" ]]; then
  export OBC_GPS_SOURCE_MODE
fi
if [[ -n "${OBC_GPS_REPLAY_FILE}" ]]; then
  export OBC_GPS_REPLAY_FILE
fi
if [[ -n "${OBC_GPS_SERIAL_DEVICE}" ]]; then
  export OBC_GPS_SERIAL_DEVICE
fi
if [[ -n "${OBC_GPS_BAUDRATE}" ]]; then
  export OBC_GPS_BAUDRATE
fi

cleanup() {
  local status=$?
  jobs -p | xargs -r kill >/dev/null 2>&1 || true
  wait || true
  exit "${status}"
}
trap cleanup EXIT INT TERM

if [[ "${COMM_MODE}" == "tcp" && "${MANAGE_RADIO_MOCK_SERVER}" == "1" ]]; then
  "${BIN_DIR}/radio_mock_server" --port "${COMM_PORT}" &
  sleep 1
fi

ARGS=(
  "--comm" "${COMM_MODE}"
  "--radio-protocol" "${RADIO_PROTOCOL}"
  "--ground-link" "${GROUND_LINK_MODE}"
  "--comm-csp-node" "${COMM_CSP_NODE}"
  "--gds-host" "${GDS_HOST}"
  "--gds-port" "${GDS_PORT}"
  "--runtime-root" "${RUNTIME_ROOT}"
  "--persistent-root" "${PERSISTENT_ROOT}"
  "--staging-root" "${STAGING_ROOT}"
  "--command-authority-profile" "${COMMAND_AUTHORITY_PROFILE}"
  "--initial-comm-band" "${INITIAL_COMM_BAND}"
)
case "${HEADLESS}" in
  1|true|TRUE|yes|YES)
    ARGS+=("--headless")
    ;;
esac
if [[ "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" == "0" ]]; then
  ARGS+=("--disable-primary-ground-link-driver")
fi
if [[ "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" == "1" ]]; then
  ARGS+=(
    "--enable-comm-subsystem-health-detector"
    "--comm-subsystem-ping-timeout-ms" "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}"
    "--comm-primary-unavailable-failure-threshold" "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}"
  )
fi
if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" == "1" ]]; then
  ARGS+=("--diagnostic-quiet-packet-egress")
fi
if [[ "${COMM_MODE}" == "tcp" ]]; then
  ARGS+=("--comm-host" "${COMM_HOST}" "--comm-port" "${COMM_PORT}")
else
  ARGS+=("--comm-device" "${RPI_COMM_DEVICE}" "--comm-baudrate" "${COMM_BAUDRATE}")
fi

exec "${BIN_DIR}/OBC" "${ARGS[@]}"
