#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
GPS_SERIAL_DEVICE="${OBC_GPS_SERIAL_DEVICE:-/dev/serial0}"
GPS_BAUDRATE="${OBC_GPS_BAUDRATE:-9600}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/integ-rpi-gps-live}"
GPS_WARMUP_SEC="${GPS_WARMUP_SEC:-10}"
GPS_ATTEMPTS="${GPS_ATTEMPTS:-10}"
GPS_POLL_INTERVAL_SEC="${GPS_POLL_INTERVAL_SEC:-1}"
GPS_PROBE_TIMEOUT_SEC="${GPS_PROBE_TIMEOUT_SEC:-30}"
REMOTE_SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
PROBE_TMP_DIR="$(mktemp -d "/tmp/obc-rpi-gps-live.XXXXXX")"
PROBE_LOG="${PROBE_LOG:-${PROBE_TMP_DIR}/gps-live.log}"

cleanup_local_probe_tmp() {
  rm -rf "${PROBE_TMP_DIR}"
}
trap cleanup_local_probe_tmp EXIT

obc_require_systemd_service_name "${REMOTE_SERVICE_NAME}"

ssh "${OBC_SSH_TARGET}" /bin/bash >"${PROBE_LOG}" 2>&1 <<EOF
set -euo pipefail

cd $(printf '%q' "${RPI_REMOTE_DIR}")
source scripts/_common.sh
BIN_DIR="\$(obc_find_native_bin_dir "\$PWD")"
if [[ -z "\${BIN_DIR}" ]]; then
  echo "Build output not found on Raspberry Pi. Run bootstrap/build first." >&2
  exit 1
fi
RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")
PERSISTENT_ROOT="\${RUNTIME_ROOT}/persistent-data"
STAGING_ROOT="\${RUNTIME_ROOT}/staging"
LOG_ROOT="\${RUNTIME_ROOT}/logs"
STACK_LOG="\${LOG_ROOT}/gps-live-stack.log"
SERVICE_NAME=$(printf '%q' "${REMOTE_SERVICE_NAME}")
RESTORE_AUTOSTART=0

if systemctl is-active --quiet "\${SERVICE_NAME}"; then
  sudo systemctl stop "\${SERVICE_NAME}"
  RESTORE_AUTOSTART=1
  sleep 2
fi

restore_service() {
  if [[ "\${RESTORE_AUTOSTART}" == "1" ]]; then
    sudo systemctl start "\${SERVICE_NAME}" >/dev/null 2>&1 || true
  fi
}
trap restore_service EXIT

mkdir -p "\${PERSISTENT_ROOT}" "\${STAGING_ROOT}" "\${LOG_ROOT}"

pids=\$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\\/bin\\/Linux\\/(csp_zmqproxy|eps_simulator|adcs_simulator|radio_mock_server|OBC)/ {print \$1}')
if [[ -n "\${pids}" ]]; then
  kill \${pids} >/dev/null 2>&1 || true
  sleep 1
fi

"\${BIN_DIR}/csp_zmqproxy" -s tcp://0.0.0.0:6100 -p tcp://0.0.0.0:7100 >"\${STACK_LOG}" 2>&1 &
PROXY_PID=\$!
CSP_TRANSPORT=zmqhub CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=6100 CSP_HUB_PUB_PORT=7100 "\${BIN_DIR}/eps_simulator" --node-id 2 >>"\${STACK_LOG}" 2>&1 &
EPS_PID=\$!
CSP_TRANSPORT=zmqhub CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=6100 CSP_HUB_PUB_PORT=7100 "\${BIN_DIR}/adcs_simulator" --node-id 3 >>"\${STACK_LOG}" 2>&1 &
ADCS_PID=\$!
"\${BIN_DIR}/radio_mock_server" --port 7000 >>"\${STACK_LOG}" 2>&1 &
RADIO_PID=\$!

cleanup_runtime() {
  kill "\${RADIO_PID}" "\${ADCS_PID}" "\${EPS_PID}" "\${PROXY_PID}" >/dev/null 2>&1 || true
}
trap 'cleanup_runtime; restore_service' EXIT

sleep 2

probe_status=0
{
  sleep $(printf '%q' "${GPS_WARMUP_SEC}")
  printf 'status\n'
  sleep $(printf '%q' "${GPS_POLL_INTERVAL_SEC}")
$(for idx in $(seq 1 "${GPS_ATTEMPTS}"); do
  printf "  printf 'gps get\\n'\n"
  if [[ "${idx}" -lt "${GPS_ATTEMPTS}" ]]; then
    printf "  sleep %q\n" "${GPS_POLL_INTERVAL_SEC}"
  fi
done)
  printf 'quit\n'
} | timeout $(printf '%q' "${GPS_PROBE_TIMEOUT_SEC}") env \
  OBC_GPS_SOURCE_MODE=live-uart \
  OBC_GPS_SERIAL_DEVICE=$(printf '%q' "${GPS_SERIAL_DEVICE}") \
  OBC_GPS_BAUDRATE=$(printf '%q' "${GPS_BAUDRATE}") \
  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT=6100 \
  CSP_HUB_PUB_PORT=7100 \
  "\${BIN_DIR}/OBC" \
  --comm tcp \
  --comm-host 127.0.0.1 \
  --comm-port 7000 \
  --radio-protocol mock-text \
  --gds-host 127.0.0.1 \
  --gds-port 0 \
  --runtime-root "\${RUNTIME_ROOT}" \
  --persistent-root "\${PERSISTENT_ROOT}" \
  --staging-root "\${STAGING_ROOT}" || probe_status=\$?

if [[ "\${probe_status}" != "0" && "\${probe_status}" != "124" ]]; then
  exit "\${probe_status}"
fi
EOF

if ! grep -q 'gps source=LIVE_UART' "${PROBE_LOG}"; then
  echo "GPS live probe did not report LIVE_UART mode." >&2
  cat "${PROBE_LOG}" >&2
  exit 1
fi

if ! grep -q 'gps get success=yes' "${PROBE_LOG}"; then
  echo "GPS live probe did not observe a successful live UART GPS poll." >&2
  cat "${PROBE_LOG}" >&2
  exit 1
fi

if ! grep -Eq 'accepted=[1-9][0-9]*' "${PROBE_LOG}"; then
  echo "GPS live probe did not observe accepted GPS sentences." >&2
  cat "${PROBE_LOG}" >&2
  exit 1
fi

if ! grep -q 'sample=yes' "${PROBE_LOG}"; then
  echo "GPS live probe did not observe cached GPS sample state." >&2
  cat "${PROBE_LOG}" >&2
  exit 1
fi

grep -E 'OBC runtime started|Ground link disabled|GPS_FIX_|gps get success=|gps source=' "${PROBE_LOG}" || true
