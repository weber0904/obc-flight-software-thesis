#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  BIN_ROOT="${ROOT_DIR}/build-fprime-automatic-native/bin"
  while IFS= read -r candidate; do
    if [[ -x "${candidate}/csp_zmqproxy" && -x "${candidate}/csp_service_peer" && -x "${candidate}/csp_runtime_smoke" ]]; then
      BIN_DIR="${candidate}"
      break
    fi
  done < <(find "${BIN_ROOT}" -mindepth 1 -maxdepth 1 -type d | sort)
fi
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56100}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57100}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
PEER_NODE_ID="${PEER_NODE_ID:-2}"
LOCAL_NODE_ID="${LOCAL_NODE_ID:-1}"

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

"${BIN_DIR}/csp_service_peer" \
  --node-id "${PEER_NODE_ID}" \
  --transport "${CSP_TRANSPORT}" \
  --hub-host "${CSP_HUB_HOST}" \
  --hub-sub-port "${CSP_HUB_SUB_PORT}" \
  --hub-pub-port "${CSP_HUB_PUB_PORT}" &
PEER_PID=$!

sleep 1

"${BIN_DIR}/csp_runtime_smoke" \
  --node-id "${LOCAL_NODE_ID}" \
  --peer-node "${PEER_NODE_ID}" \
  --transport "${CSP_TRANSPORT}" \
  --hub-host "${CSP_HUB_HOST}" \
  --hub-sub-port "${CSP_HUB_SUB_PORT}" \
  --hub-pub-port "${CSP_HUB_PUB_PORT}"
