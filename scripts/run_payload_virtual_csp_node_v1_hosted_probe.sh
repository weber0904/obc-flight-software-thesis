#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" || ! -x "${BIN_DIR}/payload_csp_probe_main" || ! -x "${BIN_DIR}/payload_camera_backend_helper" ]]; then
  echo "Required native build outputs are missing. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/payload-virtual-csp-node-v1-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${PROBE_TMP_DIR}/runtime}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
STACK_LOG="${PROBE_TMP_DIR}/stack.log"
PROBE_LOG="${PROBE_TMP_DIR}/probe.log"

CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56140}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57140}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
RADIO_PORT="${RADIO_PORT:-17040}"

obc_force_cleanup_process_pattern 'payload-virtual-csp-node-v1-hosted\.'
obc_force_cleanup_process_pattern "${BIN_DIR}/OBC --comm tcp --comm-host 127.0.0.1 --comm-port ${RADIO_PORT} --radio-protocol mock-text --ground-link disabled"
obc_force_cleanup_process_pattern "${BIN_DIR}/csp_zmqproxy -s tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT} -p tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
obc_force_cleanup_process_pattern "${BIN_DIR}/eps_simulator --node-id 2"
obc_force_cleanup_process_pattern "${BIN_DIR}/adcs_simulator --node-id 3"
obc_force_cleanup_process_pattern "${BIN_DIR}/radio_mock_server --port ${RADIO_PORT}"

cleanup() {
  local status=$?
  obc_force_cleanup_process_pattern 'payload-virtual-csp-node-v1-hosted\.'
  obc_force_cleanup_process_pattern "${BIN_DIR}/OBC --comm tcp --comm-host 127.0.0.1 --comm-port ${RADIO_PORT} --radio-protocol mock-text --ground-link disabled"
  obc_force_cleanup_process_pattern "${BIN_DIR}/csp_zmqproxy -s tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT} -p tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
  obc_force_cleanup_process_pattern "${BIN_DIR}/eps_simulator --node-id 2"
  obc_force_cleanup_process_pattern "${BIN_DIR}/adcs_simulator --node-id 3"
  obc_force_cleanup_process_pattern "${BIN_DIR}/radio_mock_server --port ${RADIO_PORT}"
  exit "${status}"
}
trap cleanup EXIT INT TERM

mkdir -p "${PROBE_TMP_DIR}" "${PERSISTENT_ROOT}" "${STAGING_ROOT}"

(
  cd "${ROOT_DIR}"
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  "${BIN_DIR}/csp_zmqproxy" \
    -s "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_SUB_PORT}" \
    -p "tcp://${CSP_PROXY_BIND_HOST}:${CSP_HUB_PUB_PORT}"
) >"${PROBE_TMP_DIR}/csp_zmqproxy.log" 2>&1 &

(
  cd "${PROBE_TMP_DIR}"
  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  "${BIN_DIR}/eps_simulator" --node-id 2
) >"${PROBE_TMP_DIR}/eps.log" 2>&1 &

(
  cd "${PROBE_TMP_DIR}"
  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  "${BIN_DIR}/adcs_simulator" --node-id 3
) >"${PROBE_TMP_DIR}/adcs.log" 2>&1 &

(
  cd "${ROOT_DIR}"
  "${BIN_DIR}/radio_mock_server" --port "${RADIO_PORT}"
) >"${PROBE_TMP_DIR}/radio.log" 2>&1 &

(
  cd "${PROBE_TMP_DIR}"
  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  "${BIN_DIR}/OBC" \
    --headless \
    --comm tcp \
    --comm-host 127.0.0.1 \
    --comm-port "${RADIO_PORT}" \
    --radio-protocol mock-text \
    --ground-link disabled \
    --disable-primary-ground-link-driver \
    --gds-port 0 \
    --runtime-root "${RUNTIME_ROOT}" \
    --persistent-root "${PERSISTENT_ROOT}" \
    --staging-root "${STAGING_ROOT}"
) >"${STACK_LOG}" 2>&1 &

echo "Probe root: ${PROBE_TMP_DIR}"

for _ in $(seq 1 30); do
  if "${BIN_DIR}/payload_csp_probe_main" \
      --local-node 8 \
      --target-node 1 \
      --transport zmqhub \
      --hub-host 127.0.0.1 \
      --hub-sub-port "${CSP_HUB_SUB_PORT}" \
      --hub-pub-port "${CSP_HUB_PUB_PORT}" \
      --timeout-ms 500 \
      --expect-capture-id-min 0 >"${PROBE_LOG}" 2>&1; then
    cat "${PROBE_LOG}"
    echo "PASS ${PROBE_TMP_DIR}"
    exit 0
  fi
  sleep 1
done

echo "payload virtual CSP probe failed; stack log follows:" >&2
cat "${STACK_LOG}" >&2
echo "probe log follows:" >&2
cat "${PROBE_LOG}" >&2
exit 1
