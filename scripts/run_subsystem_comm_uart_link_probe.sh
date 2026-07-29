#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
PROBE_TIMEOUT_MS="${PROBE_TIMEOUT_MS:-1000}"
PROBE_SETTLE_MS="${PROBE_SETTLE_MS:-200}"
PEER_STARTUP_DELAY="${PEER_STARTUP_DELAY:-5}"
PREPARE_SUBSYSTEM_WORKSPACE="${PREPARE_SUBSYSTEM_WORKSPACE:-0}"
STOP_SUBSYSTEM_SERIAL_GETTY="${STOP_SUBSYSTEM_SERIAL_GETTY:-0}"
RUN_SUBSYSTEM_PROBE_WITH_SUDO="${RUN_SUBSYSTEM_PROBE_WITH_SUDO:-0}"
PROBE_DIRECTION="${PROBE_DIRECTION:-mac-to-subsystem}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-subsystem-comm-uart.XXXXXX")}"
HOST_LOG="${HOST_LOG:-${PROBE_TMP_DIR}/host.log}"
SUBSYSTEM_LOG="${SUBSYSTEM_LOG:-${PROBE_TMP_DIR}/subsystem.log}"
REMOTE_HELPER_PID_FILE="${REMOTE_HELPER_PID_FILE:-/tmp/obc-subsystem-comm-uart-helper-${RANDOM}-$$.pid}"

usage() {
  cat >&2 <<EOF
Usage:
  HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_subsystem_comm_uart_link_probe.sh

Environment:
  HOST_SERIAL_DEVICE             macOS serial endpoint connected to subsystem.local
  SUBSYSTEM_SIM_COMM_DEVICE      subsystem.local serial endpoint connected to macOS
  COMM_BAUDRATE                  serial baudrate, default 115200
  PROBE_TIMEOUT_MS               serial exchange timeout, default 1000
  PROBE_SETTLE_MS                settle interval after opening the requester serial port, default 200
  PEER_STARTUP_DELAY             seconds to wait for the passive serial peer, default 5
  PROBE_DIRECTION                mac-to-subsystem (default) or subsystem-to-mac
  PREPARE_SUBSYSTEM_WORKSPACE    run sync/bootstrap before the probe when set to 1
  STOP_SUBSYSTEM_SERIAL_GETTY    stop and restore active serial-getty on the subsystem endpoint when set to 1
  RUN_SUBSYSTEM_PROBE_WITH_SUDO  run the subsystem serial helper through sudo -n when set to 1
EOF
}

require_bool() {
  local name="$1"
  local value="$2"
  case "${value}" in
    0|1) ;;
    *)
      echo "${name} must be 0 or 1." >&2
      exit 2
      ;;
  esac
}

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  usage
  exit 2
fi

require_bool PREPARE_SUBSYSTEM_WORKSPACE "${PREPARE_SUBSYSTEM_WORKSPACE}"
require_bool STOP_SUBSYSTEM_SERIAL_GETTY "${STOP_SUBSYSTEM_SERIAL_GETTY}"
require_bool RUN_SUBSYSTEM_PROBE_WITH_SUDO "${RUN_SUBSYSTEM_PROBE_WITH_SUDO}"

case "${PROBE_DIRECTION}" in
  mac-to-subsystem|subsystem-to-mac) ;;
  *)
    echo "PROBE_DIRECTION must be mac-to-subsystem or subsystem-to-mac." >&2
    exit 2
    ;;
esac

mkdir -p "${PROBE_TMP_DIR}"

if [[ "${PREPARE_SUBSYSTEM_WORKSPACE}" == "1" ]]; then
  bash "${ROOT_DIR}/scripts/sync_subsystem_sim_workspace.sh"
  bash "${ROOT_DIR}/scripts/bootstrap_subsystem_sim_workspace.sh"
fi

if [[ ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial device not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi

HOST_PEER_PID=""
SUBSYSTEM_HELPER_PID=""

cleanup_subsystem_serial_helpers() {
  obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash >/dev/null 2>&1 <<EOF || true
set -euo pipefail
REMOTE_HELPER_PID_FILE=$(printf '%q' "${REMOTE_HELPER_PID_FILE}")
SUBSYSTEM_SERIAL_DEVICE=$(printf '%q' "${SUBSYSTEM_SIM_COMM_DEVICE}")
if [[ -f "\${REMOTE_HELPER_PID_FILE}" ]]; then
  pid="\$(cat "\${REMOTE_HELPER_PID_FILE}")"
  kill "\${pid}" 2>/dev/null || true
  rm -f "\${REMOTE_HELPER_PID_FILE}"
fi
pkill -f "[r]adio_mock_server .*--serial-device \${SUBSYSTEM_SERIAL_DEVICE}" 2>/dev/null || true
pkill -f "[s]erial_link_probe .*--serial-device \${SUBSYSTEM_SERIAL_DEVICE}" 2>/dev/null || true
EOF
}

cleanup() {
  if [[ -n "${HOST_PEER_PID}" ]]; then
    kill "${HOST_PEER_PID}" >/dev/null 2>&1 || true
    wait "${HOST_PEER_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SUBSYSTEM_HELPER_PID}" ]]; then
    kill "${SUBSYSTEM_HELPER_PID}" >/dev/null 2>&1 || true
    wait "${SUBSYSTEM_HELPER_PID}" >/dev/null 2>&1 || true
    sleep 0.2
    cleanup_subsystem_serial_helpers
  fi
}
trap cleanup EXIT INT TERM

run_subsystem_helper() {
  local helper="$1"
  shift
  local remote_helper
  local remote_helper_args=""
  local arg
  remote_helper="$(printf '%q' "${helper}")"
  for arg in "$@"; do
    remote_helper_args+=" $(printf '%q' "${arg}")"
  done

  obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash >"${SUBSYSTEM_LOG}" 2>&1 <<EOF
set -euo pipefail
cd $(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")
source scripts/_common.sh
SUBSYSTEM_SERIAL_DEVICE=$(printf '%q' "${SUBSYSTEM_SIM_COMM_DEVICE}")
STOP_SERIAL_GETTY=$(printf '%q' "${STOP_SUBSYSTEM_SERIAL_GETTY}")
RUN_WITH_SUDO=$(printf '%q' "${RUN_SUBSYSTEM_PROBE_WITH_SUDO}")
REMOTE_HELPER_PID_FILE=$(printf '%q' "${REMOTE_HELPER_PID_FILE}")
BIN_DIR="\$(obc_find_native_bin_dir "\$PWD" || true)"
HELPER_PID=""
RESTORE_GETTY_SERVICE=""
remote_cleanup() {
  if [[ -n "\${HELPER_PID}" ]]; then
    kill "\${HELPER_PID}" >/dev/null 2>&1 || true
    wait "\${HELPER_PID}" >/dev/null 2>&1 || true
    rm -f "\${REMOTE_HELPER_PID_FILE}" >/dev/null 2>&1 || true
  fi
  if [[ -n "\${RESTORE_GETTY_SERVICE}" ]]; then
    sudo -n systemctl restart "\${RESTORE_GETTY_SERVICE}" >/dev/null 2>&1 || true
  fi
}
trap remote_cleanup EXIT INT TERM HUP
if [[ -z "\${BIN_DIR}" ]]; then
  echo "Build output not found on subsystem host. Run scripts/bootstrap_subsystem_sim_workspace.sh first." >&2
  exit 1
fi
if [[ ! -e "\${SUBSYSTEM_SERIAL_DEVICE}" ]]; then
  echo "Subsystem serial device not found: \${SUBSYSTEM_SERIAL_DEVICE}" >&2
  exit 1
fi
SERIAL_REALPATH="\$(readlink -f "\${SUBSYSTEM_SERIAL_DEVICE}" || true)"
if [[ -n "\${SERIAL_REALPATH}" ]] && command -v systemctl >/dev/null 2>&1; then
  SERIAL_BASENAME="\$(basename "\${SERIAL_REALPATH}")"
  SERIAL_GETTY_SERVICE="serial-getty@\${SERIAL_BASENAME}.service"
  if systemctl is-active --quiet "\${SERIAL_GETTY_SERVICE}"; then
    if [[ "\${STOP_SERIAL_GETTY}" != "1" ]]; then
      echo "Subsystem serial device is owned by active \${SERIAL_GETTY_SERVICE}; set STOP_SUBSYSTEM_SERIAL_GETTY=1 for this probe." >&2
      exit 1
    fi
    echo "stopping-active-serial-getty: \${SERIAL_GETTY_SERVICE}"
    sudo -n systemctl stop "\${SERIAL_GETTY_SERVICE}"
    RESTORE_GETTY_SERVICE="\${SERIAL_GETTY_SERVICE}"
  fi
fi
if [[ "\${RUN_WITH_SUDO}" != "1" ]] && { [[ ! -r "\${SUBSYSTEM_SERIAL_DEVICE}" ]] || [[ ! -w "\${SUBSYSTEM_SERIAL_DEVICE}" ]]; }; then
  echo "Subsystem serial device is not readable/writable by \$(id -un); set RUN_SUBSYSTEM_PROBE_WITH_SUDO=1 or fix device permissions." >&2
  ls -l "\${SUBSYSTEM_SERIAL_DEVICE}" "\${SERIAL_REALPATH:-\${SUBSYSTEM_SERIAL_DEVICE}}" 2>&1 || true
  exit 1
fi
RUNNER=()
if [[ "\${RUN_WITH_SUDO}" == "1" ]]; then
  RUNNER=(sudo -n)
fi
"\${RUNNER[@]}" "\${BIN_DIR}/${remote_helper}"${remote_helper_args} --serial-device "\${SUBSYSTEM_SERIAL_DEVICE}" --baudrate $(printf '%q' "${COMM_BAUDRATE}") &
HELPER_PID=\$!
echo "\${HELPER_PID}" >"\${REMOTE_HELPER_PID_FILE}"
wait "\${HELPER_PID}"
EOF
}

assert_probe_passed() {
  local log_path="$1"
  if ! grep -q 'serial-link-probe: PASS' "${log_path}"; then
    echo "Subsystem COMM UART link probe did not pass." >&2
    cat "${log_path}" >&2 || true
    exit 1
  fi

  if ! grep -q 'final-status-response: STATUS enabled=1' "${log_path}"; then
    echo "Subsystem COMM UART link probe did not observe final enabled status." >&2
    cat "${log_path}" >&2 || true
    exit 1
  fi
}

if [[ "${PROBE_DIRECTION}" == "mac-to-subsystem" ]]; then
  run_subsystem_helper radio_mock_server --mode mock-text &
  SUBSYSTEM_HELPER_PID=$!

  sleep "${PEER_STARTUP_DELAY}"
  if ! kill -0 "${SUBSYSTEM_HELPER_PID}" >/dev/null 2>&1; then
    echo "Subsystem serial peer failed to start." >&2
    cat "${SUBSYSTEM_LOG}" >&2 || true
    exit 1
  fi

  if ! "${BIN_DIR}/serial_link_probe" \
    --serial-device "${HOST_SERIAL_DEVICE}" \
    --baudrate "${COMM_BAUDRATE}" \
    --timeout-ms "${PROBE_TIMEOUT_MS}" \
    --settle-ms "${PROBE_SETTLE_MS}" >"${HOST_LOG}" 2>&1; then
    echo "Host-side serial link probe failed." >&2
    cat "${HOST_LOG}" >&2 || true
    exit 1
  fi
  assert_probe_passed "${HOST_LOG}"
  cleanup_subsystem_serial_helpers
  SUBSYSTEM_HELPER_PID=""
else
  "${BIN_DIR}/radio_mock_server" \
    --serial-device "${HOST_SERIAL_DEVICE}" \
    --mode mock-text \
    --baudrate "${COMM_BAUDRATE}" >"${HOST_LOG}" 2>&1 &
  HOST_PEER_PID=$!

  sleep "${PEER_STARTUP_DELAY}"
  if ! kill -0 "${HOST_PEER_PID}" >/dev/null 2>&1; then
    echo "Host serial peer failed to start." >&2
    cat "${HOST_LOG}" >&2 || true
    exit 1
  fi

  if ! run_subsystem_helper serial_link_probe --timeout-ms "${PROBE_TIMEOUT_MS}" --settle-ms "${PROBE_SETTLE_MS}"; then
    echo "Subsystem-side serial link probe failed." >&2
    cat "${SUBSYSTEM_LOG}" >&2 || true
    exit 1
  fi
  assert_probe_passed "${SUBSYSTEM_LOG}"
fi

echo "subsystem-comm-uart-link-probe: PASS"
echo "direction=${PROBE_DIRECTION}"
echo "host-serial-device=${HOST_SERIAL_DEVICE}"
echo "subsystem-serial-device=${SUBSYSTEM_SIM_COMM_DEVICE}"
echo "baudrate=${COMM_BAUDRATE}"
echo "log-dir=${PROBE_TMP_DIR}"
echo "=== Host Log ==="
cat "${HOST_LOG}" || true
echo "=== Subsystem Log ==="
cat "${SUBSYSTEM_LOG}" || true
