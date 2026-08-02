#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="${OBC_BIN_DIR:-}"
if [[ -z "${BIN_DIR}" ]]; then
  UT_BIN_ROOT="${ROOT_DIR}/build-fprime-automatic-native-ut/bin"
  if [[ -d "${UT_BIN_ROOT}" ]]; then
    while IFS= read -r candidate; do
      if [[ -x "${candidate}/csp_zmqproxy" && -x "${candidate}/adcs_simulator" && -x "${candidate}/adcs_csp_integration_client" ]]; then
        BIN_DIR="${candidate}"
        break
      fi
    done < <(find "${UT_BIN_ROOT}" -mindepth 1 -maxdepth 1 -type d | sort)
  fi
fi
if [[ -z "${BIN_DIR}" ]]; then
  BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
fi
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build and fprime-util build --ut first." >&2
  exit 1
fi

CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56300}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57300}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"

cleanup() {
  local status=$?
  trap - EXIT INT TERM
  obc_cleanup_job_processes 5
  exit "${status}"
}
trap cleanup EXIT INT TERM

"${BIN_DIR}/csp_zmqproxy" \
  -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
  -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" &
PROXY_PID=$!

CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
CSP_TRANSPORT="${CSP_TRANSPORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &
ADCS_PID=$!

sleep 1

CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
CSP_TRANSPORT="${CSP_TRANSPORT}" \
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID}" \
"${BIN_DIR}/adcs_csp_integration_client"
