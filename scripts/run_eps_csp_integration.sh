#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="${OBC_BIN_DIR:-}"
if [[ -z "${BIN_DIR}" ]]; then
  UT_BIN_ROOT="${ROOT_DIR}/build-fprime-automatic-native-ut/bin"
  if [[ -d "${UT_BIN_ROOT}" ]]; then
    while IFS= read -r candidate; do
      if [[ -x "${candidate}/csp_zmqproxy" && -x "${candidate}/eps_simulator" && -x "${candidate}/eps_csp_integration_client" ]]; then
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

port_is_available() {
  local bind_host="${1:?bind_host is required}"
  local port="${2:?port is required}"
  python3 - "${bind_host}" "${port}" <<'PY'
import socket
import sys

host = sys.argv[1]
port = int(sys.argv[2])
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    sock.bind((host, port))
except OSError:
    raise SystemExit(1)
finally:
    sock.close()
PY
}

find_free_port_pair() {
  local bind_host="${1:?bind_host is required}"
  local first_port="${2:?first_port is required}"
  local second_offset="${3:?second_offset is required}"
  local candidate="${first_port}"
  while ! port_is_available "${bind_host}" "${candidate}" || ! port_is_available "${bind_host}" "$((candidate + second_offset))"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"

if [[ -z "${CSP_HUB_SUB_PORT:-}" ]]; then
  CSP_HUB_SUB_PORT="$(find_free_port_pair "${CSP_PROXY_BIND_HOST}" 56200 1000)"
fi
if [[ -z "${CSP_HUB_PUB_PORT:-}" ]]; then
  CSP_HUB_PUB_PORT="$((CSP_HUB_SUB_PORT + 1000))"
fi

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
"${BIN_DIR}/eps_simulator" --node-id "${EPS_CSP_NODE_ID}" &
EPS_PID=$!

sleep 1

CSP_HUB_HOST="${CSP_HUB_HOST}" \
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
CSP_TRANSPORT="${CSP_TRANSPORT}" \
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID}" \
"${BIN_DIR}/eps_csp_integration_client"
