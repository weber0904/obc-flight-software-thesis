#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

GDS_DEPLOYMENT_NAME="${GDS_DEPLOYMENT_NAME:-OBC}"
DICT_PATH="${DICT_PATH:-$(obc_find_dictionary_path "${ROOT_DIR}" "${GDS_DEPLOYMENT_NAME}" || true)}"
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

GDS_BIND_HOST="${GDS_BIND_HOST:-0.0.0.0}"
GDS_PORT="${GDS_PORT:-50150}"
GDS_TTS_PORT="${GDS_TTS_PORT:-50151}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-/tmp/obc-canfd-gds-downlink}"
GDS_LOG_DIR="${GDS_LOG_DIR:-${GDS_FILE_STORAGE_DIR}/gds-logs}"
GDS_FRAMING_SELECTION="${GDS_FRAMING_SELECTION:-space-packet-space-data-link}"
GDS_SCID="${GDS_SCID:-68}"
GDS_VCID="${GDS_VCID:-1}"
GDS_FRAME_SIZE="${GDS_FRAME_SIZE:-4096}"
GDS_KEEPALIVE_INTERVAL="${GDS_KEEPALIVE_INTERVAL:-0}"
GDS_UI_MODE="${GDS_UI_MODE:-headless}"
GDS_GUI_ADDR="${GDS_GUI_ADDR:-127.0.0.1}"
GDS_GUI_PORT="${GDS_GUI_PORT:-5000}"

case "${GDS_UI_MODE}" in
  ui|headless)
    ;;
  *)
    echo "GDS_UI_MODE must be ui or headless." >&2
    exit 1
    ;;
esac

mkdir -p "${GDS_FILE_STORAGE_DIR}"
mkdir -p "${GDS_LOG_DIR}"

if [[ -x "${ROOT_DIR}/scripts/probe_port_hygiene.sh" ]]; then
  "${ROOT_DIR}/scripts/probe_port_hygiene.sh" --reap-known "${GDS_PORT}" "${GDS_TTS_PORT}" >/dev/null 2>&1 || true
fi

echo "Starting ground-side GDS only stack"
echo "  Execution host : macOS"
echo "  Active role    : ground"
echo "  GDS bind       : ${GDS_BIND_HOST}:${GDS_PORT} (tts ${GDS_TTS_PORT})"
echo "  Deployment     : ${GDS_DEPLOYMENT_NAME}"
echo "  GDS framing    : ${GDS_FRAMING_SELECTION}"
echo "  GDS UI mode    : ${GDS_UI_MODE}"
echo "  GDS GUI        : ${GDS_GUI_ADDR}:${GDS_GUI_PORT}"
echo "  Keepalive      : ${GDS_KEEPALIVE_INTERVAL}"
echo "  Log dir        : ${GDS_LOG_DIR}"

GDS_FRAMING_ARGS=(--framing-selection "${GDS_FRAMING_SELECTION}")
if [[ "${GDS_FRAMING_SELECTION}" == "space-packet-space-data-link" ]]; then
  if [[ -n "${GDS_SCID}" ]]; then
    GDS_FRAMING_ARGS+=(--scid "${GDS_SCID}")
  fi
  if [[ -n "${GDS_VCID}" ]]; then
    GDS_FRAMING_ARGS+=(--vcid "${GDS_VCID}")
  fi
  if [[ -n "${GDS_FRAME_SIZE}" ]]; then
    GDS_FRAMING_ARGS+=(--frame-size "${GDS_FRAME_SIZE}")
  fi
fi

if [[ "${GDS_UI_MODE}" == "headless" ]]; then
  exec "${FPRIME_GDS_BIN}" \
    -n \
    --gui-addr "${GDS_GUI_ADDR}" \
    --gui-port "${GDS_GUI_PORT}" \
    -g none \
    --no-zmq \
    -l "${GDS_LOG_DIR}" \
    --log-directly \
    "${GDS_FRAMING_ARGS[@]}" \
    --dictionary "${DICT_PATH}" \
    --ip-address "${GDS_BIND_HOST}" \
    --ip-port "${GDS_PORT}" \
    --tts-port "${GDS_TTS_PORT}" \
    --keepalive-interval "${GDS_KEEPALIVE_INTERVAL}" \
    --file-storage-directory "${GDS_FILE_STORAGE_DIR}" \
    --log-to-stdout
fi

exec "${FPRIME_GDS_BIN}" \
  -n \
  --gui-addr "${GDS_GUI_ADDR}" \
  --gui-port "${GDS_GUI_PORT}" \
  --no-zmq \
  -l "${GDS_LOG_DIR}" \
  --log-directly \
  "${GDS_FRAMING_ARGS[@]}" \
  --dictionary "${DICT_PATH}" \
  --ip-address "${GDS_BIND_HOST}" \
  --ip-port "${GDS_PORT}" \
  --tts-port "${GDS_TTS_PORT}" \
  --keepalive-interval "${GDS_KEEPALIVE_INTERVAL}" \
  --file-storage-directory "${GDS_FILE_STORAGE_DIR}" \
  --log-to-stdout
