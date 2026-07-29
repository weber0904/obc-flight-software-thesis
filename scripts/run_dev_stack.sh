#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

RADIO_PORT="${RADIO_PORT:-7000}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
GDS_HOST="${GDS_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-50000}"
OBC_BINARY_NAME="${OBC_BINARY_NAME:-OBC}"
GROUND_LINK_MODE="${GROUND_LINK_MODE:-comm-csp}"
COMM_CSP_NODE="${COMM_CSP_NODE:-5}"
UHF_BEACON_CSP_NODE="${UHF_BEACON_CSP_NODE:-0}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-}"
BOOT_TRUST="${BOOT_TRUST:-hmac-sha256}"
BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID:-repo-dev-boot-signer}"
BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT:-1}"
BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX:-424f4f545f54525553545f434841494e5f56315f4445565f4b4559}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-6100}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-7100}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_MANAGE_PROXY="${CSP_MANAGE_PROXY:-1}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-127.0.0.1}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18520}"
MANAGE_SBAND_COMM_NODE="${MANAGE_SBAND_COMM_NODE:-}"
MANAGE_GROUND_TTC_GATEWAY="${MANAGE_GROUND_TTC_GATEWAY:-}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
EPS_SIM_CONTROL_SOCKET="${EPS_SIM_CONTROL_SOCKET:-}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${ROOT_DIR}/runtime/dev-macos}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
HEADLESS="${HEADLESS:-0}"
OBC_CLEANUP_RUNTIME_ROOT="${OBC_CLEANUP_RUNTIME_ROOT:-${RUNTIME_ROOT}}"

mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}" "${LOG_ROOT}"

if [[ -z "${COMMAND_AUTHORITY_PROFILE}" ]]; then
  COMMAND_AUTHORITY_PROFILE="sband-primary"
fi

if [[ ! -x "${BIN_DIR}/${OBC_BINARY_NAME}" ]]; then
  echo "OBC binary not found or not executable: ${BIN_DIR}/${OBC_BINARY_NAME}" >&2
  exit 1
fi

if [[ -z "${MANAGE_SBAND_COMM_NODE}" ]]; then
  if [[ "${GROUND_LINK_MODE}" == "comm-csp" && "${COMM_CSP_NODE}" == "5" ]]; then
    MANAGE_SBAND_COMM_NODE=1
  else
    MANAGE_SBAND_COMM_NODE=0
  fi
fi
if [[ -z "${MANAGE_GROUND_TTC_GATEWAY}" ]]; then
  if [[ "${MANAGE_SBAND_COMM_NODE}" == "1" && "${GDS_PORT}" != "0" ]]; then
    MANAGE_GROUND_TTC_GATEWAY=1
  else
    MANAGE_GROUND_TTC_GATEWAY=0
  fi
fi

export CSP_HUB_HOST
export CSP_HUB_SUB_PORT
export CSP_HUB_PUB_PORT
export CSP_TRANSPORT
export EPS_CSP_NODE_ID
export ADCS_CSP_NODE_ID

reap_owned_stack_processes() {
  obc_reap_matching_processes "${BIN_DIR}/${OBC_BINARY_NAME}" "--runtime-root ${OBC_CLEANUP_RUNTIME_ROOT}"
  obc_reap_matching_processes "${BIN_DIR}/radio_mock_server" "--port ${RADIO_PORT}"

  if [[ "${CSP_MANAGE_PROXY}" == "1" ]]; then
    obc_reap_matching_processes \
      "${BIN_DIR}/csp_zmqproxy" \
      "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
      "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
  fi

  if [[ "${MANAGE_SBAND_COMM_NODE}" == "1" ]]; then
    obc_reap_matching_processes \
      "${BIN_DIR}/sband_comm_csp_node" \
      "--tcp-listen-port ${SBAND_TCP_PORT}" \
      "--node-id ${COMM_CSP_NODE}"
  fi

  if [[ "${MANAGE_GROUND_TTC_GATEWAY}" == "1" ]]; then
    obc_reap_matching_processes \
      "${BIN_DIR}/ground_ttc_gateway" \
      "--rf-tcp-port ${SBAND_TCP_PORT}" \
      "--gds-port ${GDS_PORT}" \
      "--link-identity sband"
  fi
}

cleanup() {
  local status=$?
  obc_cleanup_job_processes 5
  reap_owned_stack_processes
  exit "${status}"
}
trap cleanup EXIT INT TERM

reap_owned_stack_processes

if [[ "${CSP_MANAGE_PROXY}" == "1" ]]; then
  "${BIN_DIR}/csp_zmqproxy" \
    -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
    -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" &
fi
EPS_ARGS=(--node-id "${EPS_CSP_NODE_ID}")
if [[ -n "${EPS_SIM_CONTROL_SOCKET}" ]]; then
  EPS_ARGS+=(--control-socket "${EPS_SIM_CONTROL_SOCKET}")
fi

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/eps_simulator" "${EPS_ARGS[@]}" &
CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &
"${BIN_DIR}/radio_mock_server" --port "${RADIO_PORT}" &

if [[ "${MANAGE_SBAND_COMM_NODE}" == "1" ]]; then
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  CSP_HUB_HOST="${CSP_HUB_HOST}" \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  "${BIN_DIR}/sband_comm_csp_node" \
    --tcp-listen-host "${SBAND_TCP_HOST}" \
    --tcp-listen-port "${SBAND_TCP_PORT}" \
    --node-id "${COMM_CSP_NODE}" &
fi

if [[ "${MANAGE_GROUND_TTC_GATEWAY}" == "1" ]]; then
  "${BIN_DIR}/ground_ttc_gateway" \
    --rf-tcp-host "${SBAND_TCP_HOST}" \
    --rf-tcp-port "${SBAND_TCP_PORT}" \
    --link-identity sband \
    --gds-host "${GDS_HOST}" \
    --gds-port "${GDS_PORT}" &
fi

sleep 1

OBC_ARGS=(
  "${BIN_DIR}/${OBC_BINARY_NAME}"
  --comm tcp
  --comm-host 127.0.0.1
  --comm-port "${RADIO_PORT}"
  --radio-protocol "${RADIO_PROTOCOL}"
  --ground-link "${GROUND_LINK_MODE}"
  --comm-csp-node "${COMM_CSP_NODE}"
  --gds-host "${GDS_HOST}"
  --gds-port "${GDS_PORT}"
  --runtime-root "${RUNTIME_ROOT}"
  --persistent-root "${PERSISTENT_ROOT}"
  --staging-root "${STAGING_ROOT}"
  --command-authority-profile "${COMMAND_AUTHORITY_PROFILE}"
  --boot-trust "${BOOT_TRUST}"
  --boot-trust-signer-id "${BOOT_TRUST_SIGNER_ID}"
  --boot-trust-key-slot "${BOOT_TRUST_KEY_SLOT}"
  --boot-trust-key-hex "${BOOT_TRUST_KEY_HEX}"
)

if [[ "${UHF_BEACON_CSP_NODE}" != "0" ]]; then
  OBC_ARGS+=(--uhf-beacon-csp-node "${UHF_BEACON_CSP_NODE}")
fi

case "${HEADLESS}" in
  1|true|TRUE|yes|YES)
    OBC_ARGS+=(--headless)
    ;;
esac

"${OBC_ARGS[@]}"
