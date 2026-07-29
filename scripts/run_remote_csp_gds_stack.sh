#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" || true)"
if [[ -z "${DICT_PATH}" ]]; then
  echo "Dictionary not found. Run fprime-util build first." >&2
  exit 1
fi

find_local_tool() {
  local tool_name="${1:?tool_name is required}"
  if [[ -x "${ROOT_DIR}/fprime-venv/bin/${tool_name}" ]]; then
    printf '%s\n' "${ROOT_DIR}/fprime-venv/bin/${tool_name}"
    return 0
  fi
  command -v "${tool_name}"
}

FPRIME_GDS_BIN="$(find_local_tool fprime-gds || true)"
if [[ -z "${FPRIME_GDS_BIN}" ]]; then
  echo "fprime-gds not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi

CSP_TRANSPORT="$(obc_lowercase "${CSP_TRANSPORT:-zmqhub}")"
if [[ "${CSP_TRANSPORT}" != "zmqhub" ]]; then
  echo "Only CSP_TRANSPORT=zmqhub is currently supported by this stack launcher." >&2
  exit 1
fi

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56620}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57620}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
SIM_CSP_HUB_HOST="${SIM_CSP_HUB_HOST:-127.0.0.1}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
GDS_BIND_HOST="${GDS_BIND_HOST:-0.0.0.0}"
GDS_PORT="${GDS_PORT:-50140}"
GDS_TTS_PORT="${GDS_TTS_PORT:-50141}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/obc-remote-gds-downlink}"

mkdir -p "${GDS_FILE_STORAGE_DIR}"

reap_owned_remote_stack_processes() {
  obc_reap_matching_processes "fprime-gds" "--ip-port ${GDS_PORT}"
  obc_reap_matching_processes "fprime_gds.executables.comm" "--ip-port ${GDS_PORT}"
  obc_reap_matching_processes "fprime_gds.executables.tcpserver" "--tts-port ${GDS_TTS_PORT}"
  obc_reap_matching_processes "CustomDataHandlers" "--file-storage-directory ${GDS_FILE_STORAGE_DIR}"
  obc_reap_matching_processes \
    "${BIN_DIR}/csp_zmqproxy" \
    "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
    "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
}

cleanup() {
  local status=$?
  obc_cleanup_job_processes 5
  reap_owned_remote_stack_processes
  exit "${status}"
}
trap cleanup EXIT INT TERM

reap_owned_remote_stack_processes

echo "Starting remote CSP + GDS host stack"
echo "  CSP transport : ${CSP_TRANSPORT}"
echo "  CSP bind      : tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT} / tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
echo "  GDS bind      : ${GDS_BIND_HOST}:${GDS_PORT} (tts ${GDS_TTS_PORT})"
echo "  EPS node      : ${EPS_CSP_NODE_ID}"
echo "  ADCS node     : ${ADCS_CSP_NODE_ID}"

"${BIN_DIR}/csp_zmqproxy" \
  -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
  -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${SIM_CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &

CSP_TRANSPORT="${CSP_TRANSPORT}" \
CSP_HUB_HOST="${SIM_CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
"${BIN_DIR}/adcs_simulator" --node-id "${ADCS_CSP_NODE_ID}" &

"${FPRIME_GDS_BIN}" \
  -n \
  -g none \
  --no-zmq \
  --framing-selection fprime \
  --dictionary "${DICT_PATH}" \
  --ip-address "${GDS_BIND_HOST}" \
  --ip-port "${GDS_PORT}" \
  --tts-port "${GDS_TTS_PORT}" \
  --file-storage-directory "${GDS_FILE_STORAGE_DIR}" \
  --log-to-stdout &

wait
