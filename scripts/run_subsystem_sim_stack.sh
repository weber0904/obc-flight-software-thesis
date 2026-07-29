#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-zmqhub}")"
if [[ "${CSP_TRANSPORT}" != "zmqhub" ]]; then
  echo "Only CSP_TRANSPORT=zmqhub is currently supported by this stack launcher." >&2
  exit 1
fi

CSP_HUB_HOST="${CSP_HUB_HOST:-${REMOTE_HOST:-}}"
if [[ -z "${CSP_HUB_HOST}" ]]; then
  echo "CSP_HUB_HOST or REMOTE_HOST is required." >&2
  exit 1
fi

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56630}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57630}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"

if [[ "${KILL_EXISTING_PIDS}" != "0" && "${KILL_EXISTING_PIDS}" != "1" ]]; then
  echo "KILL_EXISTING_PIDS must be 0 or 1." >&2
  exit 1
fi

if [[ "${KILL_EXISTING_PIDS}" == "1" ]]; then
  PIDS="$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\/bin\/Linux\/(eps_simulator|adcs_simulator)/ {print $1}')"
  if [[ -n "${PIDS}" ]]; then
    kill ${PIDS} >/dev/null 2>&1 || true
    sleep 1
  fi
fi

cleanup() {
  local status=$?
  jobs -p | xargs -r kill >/dev/null 2>&1 || true
  wait || true
  exit "${status}"
}
trap cleanup EXIT INT TERM

echo "Starting subsystem simulator stack"
echo "  CSP transport : ${CSP_TRANSPORT}"
echo "  CSP host      : ${CSP_HUB_HOST}:${CSP_HUB_SUB_PORT}/${CSP_HUB_PUB_PORT}"
echo "  EPS node      : ${EPS_CSP_NODE_ID}"
echo "  ADCS node     : ${ADCS_CSP_NODE_ID}"

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &

wait
