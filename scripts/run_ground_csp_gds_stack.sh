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

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56630}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57630}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
GDS_BIND_HOST="${GDS_BIND_HOST:-0.0.0.0}"
GDS_PORT="${GDS_PORT:-50150}"
GDS_TTS_PORT="${GDS_TTS_PORT:-50151}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/obc-dual-pi-gds-downlink}"

mkdir -p "${GDS_FILE_STORAGE_DIR}"

cleanup() {
  local status=$?
  jobs -p | xargs -r kill >/dev/null 2>&1 || true
  wait || true
  exit "${status}"
}
trap cleanup EXIT INT TERM

echo "Starting ground-side CSP + GDS stack"
echo "  CSP transport : ${CSP_TRANSPORT}"
echo "  CSP bind      : tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT} / tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
echo "  GDS bind      : ${GDS_BIND_HOST}:${GDS_PORT} (tts ${GDS_TTS_PORT})"

"${BIN_DIR}/csp_zmqproxy" \
  -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
  -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}" &

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
