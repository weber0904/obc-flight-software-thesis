#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RELEASE_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
INSTALL_ROOT="$(cd "${RELEASE_ROOT}/../.." && pwd)"
BIN_DIR="${RELEASE_ROOT}/bin"
cd "${RELEASE_ROOT}"

RADIO_PORT="${RADIO_PORT:-7000}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
GDS_HOST="${GDS_HOST:-127.0.0.1}"
GDS_PORT="${GDS_PORT:-0}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-6100}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-7100}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_MANAGE_PROXY="${CSP_MANAGE_PROXY:-1}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${INSTALL_ROOT}/runtime/integ-rpi}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
OBC_HEADLESS="${OBC_HEADLESS:-0}"
OBC_CLEANUP_RUNTIME_ROOT="${OBC_CLEANUP_RUNTIME_ROOT:-${RUNTIME_ROOT}}"
COMMAND_AUTHORITY_PROFILE="${COMMAND_AUTHORITY_PROFILE:-sband-primary}"
BOOT_TRUST="${BOOT_TRUST:-hmac-sha256}"
BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID:-repo-dev-boot-signer}"
BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT:-1}"
BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX:-424f4f545f54525553545f434841494e5f56315f4445565f4b4559}"

mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}" "${LOG_ROOT}"

export CSP_HUB_HOST
export CSP_HUB_SUB_PORT
export CSP_HUB_PUB_PORT
export CSP_TRANSPORT
export EPS_CSP_NODE_ID
export ADCS_CSP_NODE_ID

iter_process_rows() {
  ps -ax -o pid= -o command= | awk '{
    pid=$1
    $1=""
    sub(/^ /, "", $0)
    printf "%s\t%s\n", pid, $0
  }'
}

command_matches_all_fragments() {
  local command="${1:?command is required}"
  shift
  local fragment
  for fragment in "$@"; do
    if [[ "${command}" != *"${fragment}"* ]]; then
      return 1
    fi
  done
  return 0
}

signal_matching_processes() {
  local signal_name="${1:?signal name is required}"
  shift
  local pid
  local command

  while IFS=$'\t' read -r pid command; do
    [[ -n "${pid}" && -n "${command}" ]] || continue
    if command_matches_all_fragments "${command}" "$@"; then
      kill "-${signal_name}" "${pid}" >/dev/null 2>&1 || true
    fi
  done < <(iter_process_rows)
}

reap_matching_processes() {
  if [[ "$#" -lt 1 ]]; then
    return 0
  fi
  signal_matching_processes TERM "$@"
  sleep 1
  signal_matching_processes KILL "$@"
}

reap_owned_stack_processes() {
  reap_matching_processes "${BIN_DIR}/OBC" "--runtime-root ${OBC_CLEANUP_RUNTIME_ROOT}"
  reap_matching_processes "${BIN_DIR}/radio_mock_server" "--port ${RADIO_PORT}"

  if [[ "${CSP_MANAGE_PROXY}" == "1" ]]; then
    reap_matching_processes \
      "${BIN_DIR}/csp_zmqproxy" \
      "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
      "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
  fi
}

cleanup() {
  local status=$?
  pids=($(jobs -p))
  if [[ "${#pids[@]}" -gt 0 ]]; then
    kill "${pids[@]}" >/dev/null 2>&1 || true
    deadline=$((SECONDS + 5))
    while :; do
      survivors=()
      for pid in "${pids[@]}"; do
        if kill -0 "${pid}" >/dev/null 2>&1; then
          survivors+=("${pid}")
        fi
      done
      if [[ "${#survivors[@]}" -eq 0 ]]; then
        break
      fi
      if [[ "${SECONDS}" -ge "${deadline}" ]]; then
        kill -9 "${survivors[@]}" >/dev/null 2>&1 || true
        break
      fi
      sleep 1
    done
    wait >/dev/null 2>&1 || true
  fi
  reap_owned_stack_processes
  exit "${status}"
}
trap cleanup EXIT INT TERM

reap_owned_stack_processes

if [[ "${CSP_MANAGE_PROXY}" == "1" ]]; then
  "${BIN_DIR}/csp_zmqproxy" \
    -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
    -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" >"${LOG_ROOT}/csp_zmqproxy.log" 2>&1 &
fi
CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" >"${LOG_ROOT}/eps_simulator.log" 2>&1 &
CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" >"${LOG_ROOT}/adcs_simulator.log" 2>&1 &
"${BIN_DIR}/radio_mock_server" --port "${RADIO_PORT}" >"${LOG_ROOT}/radio_mock_server.log" 2>&1 &

sleep 1

OBC_ARGS=(
  --comm tcp
  --comm-host 127.0.0.1
  --comm-port "${RADIO_PORT}"
  --radio-protocol "${RADIO_PROTOCOL}"
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

if [[ "${OBC_HEADLESS}" == "1" ]]; then
  OBC_ARGS+=(--headless)
fi

"${BIN_DIR}/OBC" "${OBC_ARGS[@]}"
