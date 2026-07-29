#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RELEASE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
INSTALL_ROOT="$(cd "${RELEASE_ROOT}/../.." && pwd)"
BIN_DIR="${RELEASE_ROOT}/bin"
cd "${RELEASE_ROOT}"

source "${SCRIPT_DIR}/_common.sh"

COMM_MODE="${COMM_MODE:-tcp}"
COMM_HOST="${COMM_HOST:-127.0.0.1}"
COMM_PORT="${COMM_PORT:-7000}"
COMM_DEVICE="${COMM_DEVICE:-}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
GROUND_LINK_MODE="${GROUND_LINK_MODE:-comm-csp}"
TARGET_COMM_PROFILE="${TARGET_COMM_PROFILE:-sband}"
COMM_CSP_NODE="${COMM_CSP_NODE:-}"
CSP_TRANSPORT="${CSP_TRANSPORT:-socketcan}"
CSP_CAN_DEVICE="${CSP_CAN_DEVICE:-can0}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
GDS_HOST="${GDS_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-0}"
OBC_GPS_SOURCE_MODE="${OBC_GPS_SOURCE_MODE:-live-uart}"
OBC_GPS_SERIAL_DEVICE="${OBC_GPS_SERIAL_DEVICE:-/dev/serial0}"
OBC_GPS_BAUDRATE="${OBC_GPS_BAUDRATE:-9600}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${INSTALL_ROOT}/runtime/comm-csp-lab-obc}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
OBC_HEADLESS="${OBC_HEADLESS:-1}"
MANAGE_RADIO_PEER="${MANAGE_RADIO_PEER:-1}"
HARDWARE_WATCHDOG="${HARDWARE_WATCHDOG:-linux-device}"
HARDWARE_WATCHDOG_DEVICE="${HARDWARE_WATCHDOG_DEVICE:-/dev/watchdog0}"
HARDWARE_WATCHDOG_TIMEOUT_SEC="${HARDWARE_WATCHDOG_TIMEOUT_SEC:-15}"
COMM_SUBSYSTEM_PING_TIMEOUT_MS="${COMM_SUBSYSTEM_PING_TIMEOUT_MS:-500}"
COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD="${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD:-10}"
DIAGNOSTIC_QUIET_PACKET_EGRESS="${DIAGNOSTIC_QUIET_PACKET_EGRESS:-0}"
ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR="${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR:-1}"
INITIAL_COMM_BAND="${INITIAL_COMM_BAND:-}"
ENABLE_PRIMARY_GROUND_LINK_DRIVER="${ENABLE_PRIMARY_GROUND_LINK_DRIVER:-}"
UHF_BEACON_CSP_NODE="${UHF_BEACON_CSP_NODE:-0}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-}"
BOOT_TRUST="${BOOT_TRUST:-hmac-sha256}"
BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID:-repo-dev-boot-signer}"
BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT:-1}"
BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX:-424f4f545f54525553545f434841494e5f56315f4445565f4b4559}"

if [[ "${COMM_MODE}" != "tcp" && "${COMM_MODE}" != "serial" ]]; then
  echo "COMM_MODE must be tcp or serial." >&2
  exit 1
fi
if [[ "${COMM_MODE}" == "serial" && -z "${COMM_DEVICE}" ]]; then
  echo "COMM_DEVICE is required when COMM_MODE=serial." >&2
  exit 1
fi
if [[ "${MANAGE_RADIO_PEER}" != "0" && "${MANAGE_RADIO_PEER}" != "1" ]]; then
  echo "MANAGE_RADIO_PEER must be 0 or 1." >&2
  exit 1
fi
if [[ "${GROUND_LINK_MODE}" != "comm-csp" ]]; then
  echo "This installed lab profile requires GROUND_LINK_MODE=comm-csp." >&2
  exit 1
fi
obc_require_target_comm_profile "${TARGET_COMM_PROFILE}"
if [[ "${HARDWARE_WATCHDOG}" != "disabled" && "${HARDWARE_WATCHDOG}" != "linux-device" ]]; then
  echo "HARDWARE_WATCHDOG must be disabled or linux-device." >&2
  exit 1
fi
if [[ "${HARDWARE_WATCHDOG}" == "linux-device" ]]; then
  if [[ -z "${HARDWARE_WATCHDOG_DEVICE}" ]]; then
    echo "HARDWARE_WATCHDOG_DEVICE is required when HARDWARE_WATCHDOG=linux-device." >&2
    exit 1
  fi
  if [[ ! "${HARDWARE_WATCHDOG_TIMEOUT_SEC}" =~ ^[0-9]+$ || "${HARDWARE_WATCHDOG_TIMEOUT_SEC}" == "0" ]]; then
    echo "HARDWARE_WATCHDOG_TIMEOUT_SEC must be a positive integer when HARDWARE_WATCHDOG=linux-device." >&2
    exit 1
  fi
fi
if [[ ! "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}" =~ ^[0-9]+$ || "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}" == "0" ]]; then
  echo "COMM_SUBSYSTEM_PING_TIMEOUT_MS must be a positive integer." >&2
  exit 1
fi
if [[ ! "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}" =~ ^[0-9]+$ || "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}" == "0" ]]; then
  echo "COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD must be a positive integer." >&2
  exit 1
fi
if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" != "0" && "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" != "1" ]]; then
  echo "DIAGNOSTIC_QUIET_PACKET_EGRESS must be 0 or 1." >&2
  exit 1
fi
if [[ "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" != "0" && "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" != "1" ]]; then
  echo "ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR must be 0 or 1." >&2
  exit 1
fi
if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "This installed lab profile requires CSP_TRANSPORT=socketcan." >&2
  exit 1
fi
PROFILE_COMM_CSP_NODE="$(obc_target_comm_profile_node "${TARGET_COMM_PROFILE}")"
PROFILE_COMMAND_AUTHORITY_PROFILE="$(obc_target_comm_profile_service_authority "${TARGET_COMM_PROFILE}")"
COMM_CSP_NODE="${COMM_CSP_NODE:-${PROFILE_COMM_CSP_NODE}}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-${PROFILE_COMMAND_AUTHORITY_PROFILE}}"
if [[ "${COMM_CSP_NODE}" != "${PROFILE_COMM_CSP_NODE}" ]]; then
  echo "COMM_CSP_NODE=${COMM_CSP_NODE} does not match TARGET_COMM_PROFILE=${TARGET_COMM_PROFILE} (expected ${PROFILE_COMM_CSP_NODE})." >&2
  exit 1
fi
if [[ "${COMMAND_AUTHORITY_PROFILE}" != "${PROFILE_COMMAND_AUTHORITY_PROFILE}" ]]; then
  echo "COMMAND_AUTHORITY_PROFILE=${COMMAND_AUTHORITY_PROFILE} does not match TARGET_COMM_PROFILE=${TARGET_COMM_PROFILE} (expected ${PROFILE_COMMAND_AUTHORITY_PROFILE})." >&2
  exit 1
fi
if [[ -z "${INITIAL_COMM_BAND}" ]]; then
  INITIAL_COMM_BAND="$(obc_target_comm_profile_initial_band "${TARGET_COMM_PROFILE}")"
fi
if [[ "${INITIAL_COMM_BAND}" != "sband" && "${INITIAL_COMM_BAND}" != "uhf" ]]; then
  echo "INITIAL_COMM_BAND must be sband or uhf." >&2
  exit 1
fi
if [[ -z "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" ]]; then
  ENABLE_PRIMARY_GROUND_LINK_DRIVER="$(obc_target_comm_profile_enable_primary_ground_link_driver "${TARGET_COMM_PROFILE}")"
fi
if [[ "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" != "0" && "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" != "1" ]]; then
  echo "ENABLE_PRIMARY_GROUND_LINK_DRIVER must be 0 or 1." >&2
  exit 1
fi
if [[ ! "${UHF_BEACON_CSP_NODE}" =~ ^[0-9]+$ ]]; then
  echo "UHF_BEACON_CSP_NODE must be a non-negative integer." >&2
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
mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}" "${LOG_ROOT}"

export CSP_TRANSPORT
export CSP_CAN_DEVICE
export CSP_CAN_PROMISC
export OBC_GPS_SOURCE_MODE
export OBC_GPS_SERIAL_DEVICE
export OBC_GPS_BAUDRATE

echo "Starting installed OBC COMM CSP lab profile"
echo "  Release root : ${RELEASE_ROOT}"
echo "  Runtime root : ${RUNTIME_ROOT}"
echo "  CSP          : ${CSP_TRANSPORT} on ${CSP_CAN_DEVICE} promisc=${CSP_CAN_PROMISC}"
echo "  Target COMM  : profile=${TARGET_COMM_PROFILE} commNode=${COMM_CSP_NODE} authority=${COMMAND_AUTHORITY_PROFILE}"
echo "  Initial band : ${INITIAL_COMM_BAND}"
if [[ "${UHF_BEACON_CSP_NODE}" != "0" ]]; then
  echo "  UHF beacon   : node=${UHF_BEACON_CSP_NODE}"
fi
echo "  Ground link  : ${GROUND_LINK_MODE}"
echo "  GPS          : ${OBC_GPS_SOURCE_MODE} ${OBC_GPS_SERIAL_DEVICE} @ ${OBC_GPS_BAUDRATE}"
echo "  Radio peer   : manage=${MANAGE_RADIO_PEER} protocol=${RADIO_PROTOCOL}"
echo "  HW watchdog  : ${HARDWARE_WATCHDOG} ${HARDWARE_WATCHDOG_DEVICE} timeout=${HARDWARE_WATCHDOG_TIMEOUT_SEC}"
echo "  COMM FDIR    : pingTimeoutMs=${COMM_SUBSYSTEM_PING_TIMEOUT_MS} unavailableThreshold=${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}"
if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" == "1" ]]; then
  echo "  Diagnostic quiet packet egress: enabled"
fi

OBC_ARGS=(
  --comm "${COMM_MODE}"
  --radio-protocol "${RADIO_PROTOCOL}"
  --ground-link "${GROUND_LINK_MODE}"
  --comm-csp-node "${COMM_CSP_NODE}"
  --initial-comm-band "${INITIAL_COMM_BAND}"
  --gds-host "${GDS_HOST}"
  --gds-port "${GDS_PORT}"
  --runtime-root "${RUNTIME_ROOT}"
  --persistent-root "${PERSISTENT_ROOT}"
  --staging-root "${STAGING_ROOT}"
  --hardware-watchdog "${HARDWARE_WATCHDOG}"
  --hardware-watchdog-device "${HARDWARE_WATCHDOG_DEVICE}"
  --hardware-watchdog-timeout-sec "${HARDWARE_WATCHDOG_TIMEOUT_SEC}"
  --comm-subsystem-ping-timeout-ms "${COMM_SUBSYSTEM_PING_TIMEOUT_MS}"
  --comm-primary-unavailable-failure-threshold "${COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD}"
  --command-authority-profile "${COMMAND_AUTHORITY_PROFILE}"
  --boot-trust "${BOOT_TRUST}"
  --boot-trust-signer-id "${BOOT_TRUST_SIGNER_ID}"
  --boot-trust-key-slot "${BOOT_TRUST_KEY_SLOT}"
  --boot-trust-key-hex "${BOOT_TRUST_KEY_HEX}"
)

if [[ "${UHF_BEACON_CSP_NODE}" != "0" ]]; then
  OBC_ARGS+=(--uhf-beacon-csp-node "${UHF_BEACON_CSP_NODE}")
fi

if [[ "${COMM_MODE}" == "tcp" ]]; then
  OBC_ARGS+=(--comm-host "${COMM_HOST}" --comm-port "${COMM_PORT}")
else
  OBC_ARGS+=(--comm-device "${COMM_DEVICE}" --comm-baudrate "${COMM_BAUDRATE}")
fi

if [[ "${OBC_HEADLESS}" == "1" ]]; then
  OBC_ARGS+=(--headless)
fi

if [[ "${DIAGNOSTIC_QUIET_PACKET_EGRESS}" == "1" ]]; then
  OBC_ARGS+=(--diagnostic-quiet-packet-egress)
fi
if [[ "${ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR}" == "1" ]]; then
  OBC_ARGS+=(--enable-comm-subsystem-health-detector)
fi
if [[ "${ENABLE_PRIMARY_GROUND_LINK_DRIVER}" == "0" ]]; then
  OBC_ARGS+=(--disable-primary-ground-link-driver)
fi

RADIO_PEER_PID=""

cleanup() {
  local status=$?
  if [[ -n "${RADIO_PEER_PID}" ]]; then
    kill "${RADIO_PEER_PID}" >/dev/null 2>&1 || true
    wait "${RADIO_PEER_PID}" >/dev/null 2>&1 || true
  fi
  exit "${status}"
}
trap cleanup EXIT INT TERM

if [[ "${COMM_MODE}" == "tcp" && "${MANAGE_RADIO_PEER}" == "1" ]]; then
  "${BIN_DIR}/radio_mock_server" --port "${COMM_PORT}" >"${LOG_ROOT}/radio_mock_server.log" 2>&1 &
  RADIO_PEER_PID="$!"
  sleep 1
fi

"${BIN_DIR}/OBC" "${OBC_ARGS[@]}"
