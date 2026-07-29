#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

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

FPRIME_CLI_BIN="$(find_local_tool fprime-cli || true)"
if [[ -z "${FPRIME_CLI_BIN}" ]]; then
  echo "fprime-cli not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-$(obc_default_remote_workspace_dir)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
REMOTE_HOST="${REMOTE_HOST:-$(obc_default_remote_carrier_host)}"
OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-}"
SUBSYSTEM_SIM_RESERVED_CAN_DEVICE="${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE:-}"
CSP_TRANSPORT="${CSP_TRANSPORT:-socketcan}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
COMM_MODE="${COMM_MODE:-tcp}"
COMM_HOST="${COMM_HOST:-127.0.0.1}"
COMM_PORT="${COMM_PORT:-7000}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
PATH_A_RUNTIME_ROOT="${PATH_A_RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/shared-canfd-csp-path-a}"
PATH_B_RUNTIME_ROOT="${PATH_B_RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/shared-canfd-csp-path-b}"
PATH_A_WARMUP_SEC="${PATH_A_WARMUP_SEC:-10}"
PATH_B_WARMUP_SEC="${PATH_B_WARMUP_SEC:-10}"
PATH_B_COMMAND_SETTLE_SEC="${PATH_B_COMMAND_SETTLE_SEC:-5}"
REMOTE_SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
RESERVED_CANDUMP_TIMEOUT_SEC="${RESERVED_CANDUMP_TIMEOUT_SEC:-20}"
ACTIVE_CANDUMP_TIMEOUT_SEC="${ACTIVE_CANDUMP_TIMEOUT_SEC:-20}"

if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "This probe requires CSP_TRANSPORT=socketcan." >&2
  exit 1
fi
if [[ "${CSP_CAN_PROMISC}" != "0" ]]; then
  echo "Governed acceptance must use CSP_CAN_PROMISC=0." >&2
  exit 1
fi
if [[ -z "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" || -z "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" ]]; then
  echo "SUBSYSTEM_SIM_CSP_CAN_DEVICE and SUBSYSTEM_SIM_RESERVED_CAN_DEVICE are required." >&2
  exit 1
fi
obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}"
if [[ "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" == "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" ]]; then
  echo "Subsystem primary and reserved CAN devices must differ." >&2
  exit 1
fi
obc_require_systemd_service_name "${REMOTE_SERVICE_NAME}"

port_is_available() {
  local port="${1:?port is required}"
  ! lsof -nP -iTCP:"${port}" >/dev/null 2>&1
}

find_free_port_pair() {
  local first_port="${1:?first_port is required}"
  local second_offset="${2:?second_offset is required}"
  local candidate="${first_port}"
  while ! port_is_available "${candidate}" || ! port_is_available "$((candidate + second_offset))"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

if [[ -z "${GDS_PORT:-}" ]]; then
  GDS_PORT="$(find_free_port_pair 51800 1)"
fi
if [[ -z "${GDS_TTS_PORT:-}" ]]; then
  GDS_TTS_PORT="$((GDS_PORT + 1))"
fi

PROBE_TMP_DIR="$(mktemp -d "/tmp/obc-shared-canfd-csp.XXXXXX")"
GROUND_STACK_LOG="${PROBE_TMP_DIR}/ground-gds.log"
SUBSYSTEM_STACK_LOG="${PROBE_TMP_DIR}/subsystem-can-stack.log"
PATH_A_LOG="${PROBE_TMP_DIR}/path-a.log"
PATH_B_LOG="${PROBE_TMP_DIR}/path-b.log"
CLI_LOG="${PROBE_TMP_DIR}/cli.log"
ACTIVE_CANDUMP_LOG="${PROBE_TMP_DIR}/active-bus.candump.log"
RESERVED_CANDUMP_LOG="${PROBE_TMP_DIR}/reserved-bus.candump.log"
SUBSYSTEM_LINK_DETAILS_LOG="${PROBE_TMP_DIR}/subsystem-link-details.log"
SUBSYSTEM_PRE_STATS_LOG="${PROBE_TMP_DIR}/subsystem-pre-stats.log"
SUBSYSTEM_POST_STATS_LOG="${PROBE_TMP_DIR}/subsystem-post-stats.log"
OBC_PRE_STATS_LOG="${PROBE_TMP_DIR}/obc-pre-stats.log"
OBC_POST_STATS_LOG="${PROBE_TMP_DIR}/obc-post-stats.log"
SUBSYSTEM_PARENTDEV_LOG="${PROBE_TMP_DIR}/subsystem-parentdev.log"

cleanup() {
  local status=$?
  if [[ -n "${PATH_B_PID:-}" ]]; then
    wait "${PATH_B_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${ACTIVE_CANDUMP_PID:-}" ]]; then
    wait "${ACTIVE_CANDUMP_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${RESERVED_CANDUMP_PID:-}" ]]; then
    wait "${RESERVED_CANDUMP_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${SUBSYSTEM_STACK_PID:-}" ]]; then
    kill "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
    wait "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
  fi
  if [[ -n "${GROUND_STACK_PID:-}" ]]; then
    kill "${GROUND_STACK_PID}" >/dev/null 2>&1 || true
    wait "${GROUND_STACK_PID}" >/dev/null 2>&1 || true
  fi
  if [[ ${status} -ne 0 ]]; then
    echo "Ground stack log    : ${GROUND_STACK_LOG}" >&2
    echo "Subsystem stack log : ${SUBSYSTEM_STACK_LOG}" >&2
    echo "Path A log          : ${PATH_A_LOG}" >&2
    echo "Path B log          : ${PATH_B_LOG}" >&2
    echo "CLI log             : ${CLI_LOG}" >&2
    echo "Active candump log  : ${ACTIVE_CANDUMP_LOG}" >&2
    echo "Reserved candump log: ${RESERVED_CANDUMP_LOG}" >&2
  else
    rm -rf "${PROBE_TMP_DIR}"
  fi
  exit "${status}"
}
trap cleanup EXIT INT TERM

send_cli_command() {
  local command_name="${1:?command_name is required}"
  shift

  local attempt=1
  while [[ "${attempt}" -le 5 ]]; do
    if "${FPRIME_CLI_BIN}" command-send \
      --dictionary "${DICT_PATH}" \
      --no-zmq \
      --tts-port "${GDS_TTS_PORT}" \
      "${command_name}" \
      "$@" >>"${CLI_LOG}" 2>&1; then
      return 0
    fi
    sleep 1
    attempt=$((attempt + 1))
  done

  echo "Failed to dispatch ${command_name} through fprime-cli after retries." >&2
  cat "${CLI_LOG}" >&2 || true
  return 1
}

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash <<EOF >"${SUBSYSTEM_PARENTDEV_LOG}" 2>&1
set -euo pipefail
cd $(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")
source scripts/_common.sh
PRIMARY=$(printf '%q' "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}")
RESERVED=$(printf '%q' "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}")
printf 'primary=%s parentdev=%s\n' "\${PRIMARY}" "\$(obc_can_parentdev "\${PRIMARY}")"
printf 'reserved=%s parentdev=%s\n' "\${RESERVED}" "\$(obc_can_parentdev "\${RESERVED}")"
EOF

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash <<EOF >"${SUBSYSTEM_LINK_DETAILS_LOG}" 2>&1
set -euo pipefail
ip -details link show type can
EOF

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash <<EOF >"${SUBSYSTEM_PRE_STATS_LOG}" 2>&1
set -euo pipefail
ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}")
echo "----"
ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}")
EOF

obc_ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF >"${OBC_PRE_STATS_LOG}" 2>&1
set -euo pipefail
ip -details -statistics link show dev $(printf '%q' "${OBC_CSP_CAN_DEVICE}")
EOF

env \
  GDS_PORT="${GDS_PORT}" \
  GDS_TTS_PORT="${GDS_TTS_PORT}" \
  bash "${ROOT_DIR}/scripts/run_ground_gds_only_stack.sh" >"${GROUND_STACK_LOG}" 2>&1 &
GROUND_STACK_PID=$!

env \
  SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
  SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
  SUBSYSTEM_SIM_RESERVED_CAN_DEVICE="${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" \
  CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
  EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID}" \
  ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID}" \
  KILL_EXISTING_PIDS=1 \
  bash "${ROOT_DIR}/scripts/run_subsystem_sim_remote_can_stack.sh" >"${SUBSYSTEM_STACK_LOG}" 2>&1 &
SUBSYSTEM_STACK_PID=$!

sleep 4
if ! kill -0 "${GROUND_STACK_PID}" >/dev/null 2>&1; then
  echo "Ground GDS stack failed to start." >&2
  cat "${GROUND_STACK_LOG}" >&2 || true
  exit 1
fi
if ! kill -0 "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1; then
  echo "Subsystem CAN stack failed to start." >&2
  cat "${SUBSYSTEM_STACK_LOG}" >&2 || true
  exit 1
fi

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "timeout ${ACTIVE_CANDUMP_TIMEOUT_SEC} candump ${SUBSYSTEM_SIM_CSP_CAN_DEVICE} || test \$? -eq 124")" \
  >"${ACTIVE_CANDUMP_LOG}" 2>&1 &
ACTIVE_CANDUMP_PID=$!

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "timeout ${RESERVED_CANDUMP_TIMEOUT_SEC} candump ${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE} || test \$? -eq 124")" \
  >"${RESERVED_CANDUMP_LOG}" 2>&1 &
RESERVED_CANDUMP_PID=$!

{
  sleep "${PATH_A_WARMUP_SEC}"
  printf 'status\n'
  sleep 1
  printf 'csp ping 2\n'
  sleep 1
  printf 'csp ping 3\n'
  sleep 1
  printf 'eps get\n'
  sleep 1
  printf 'adcs get\n'
  sleep 1
  printf 'quit\n'
} | env \
  OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
  RPI_REMOTE_DIR="${RPI_REMOTE_DIR}" \
  COMM_MODE="${COMM_MODE}" \
  COMM_HOST="${COMM_HOST}" \
  COMM_PORT="${COMM_PORT}" \
  RADIO_PROTOCOL="${RADIO_PROTOCOL}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE}" \
  CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
  GDS_PORT=0 \
  GDS_HOST="${REMOTE_HOST}" \
  RUNTIME_ROOT="${PATH_A_RUNTIME_ROOT}" \
  MANAGE_AUTOSTART=1 \
  KILL_EXISTING_PIDS=1 \
  SERVICE_NAME="${REMOTE_SERVICE_NAME}" \
  bash "${ROOT_DIR}/scripts/run_rpi_can_csp_stack.sh" >"${PATH_A_LOG}" 2>&1

PATH_A_PINGS="$(grep -c 'csp ping response=0 success=yes' "${PATH_A_LOG}" || true)"
if [[ "${PATH_A_PINGS}" -lt 2 ]]; then
  echo "Path A did not observe successful CSP ping to both subsystem nodes." >&2
  cat "${PATH_A_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'eps soc=' "${PATH_A_LOG}"; then
  echo "Path A did not observe EPS state." >&2
  cat "${PATH_A_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'adcs mode=' "${PATH_A_LOG}"; then
  echo "Path A did not observe ADCS state." >&2
  cat "${PATH_A_LOG}" >&2 || true
  exit 1
fi

{
  sleep "${PATH_B_WARMUP_SEC}"
  printf 'status\n'
  sleep "${PATH_B_COMMAND_SETTLE_SEC}"
  printf 'eps get\n'
  sleep 1
  printf 'adcs get\n'
  sleep 1
  printf 'quit\n'
} | env \
  OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
  RPI_REMOTE_DIR="${RPI_REMOTE_DIR}" \
  COMM_MODE="${COMM_MODE}" \
  COMM_HOST="${COMM_HOST}" \
  COMM_PORT="${COMM_PORT}" \
  RADIO_PROTOCOL="${RADIO_PROTOCOL}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE}" \
  CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
  GDS_PORT="${GDS_PORT}" \
  GDS_HOST="${REMOTE_HOST}" \
  RUNTIME_ROOT="${PATH_B_RUNTIME_ROOT}" \
  MANAGE_AUTOSTART=1 \
  KILL_EXISTING_PIDS=1 \
  SERVICE_NAME="${REMOTE_SERVICE_NAME}" \
  bash "${ROOT_DIR}/scripts/run_rpi_can_csp_stack.sh" >"${PATH_B_LOG}" 2>&1 &
PATH_B_PID=$!

sleep $((PATH_B_WARMUP_SEC + 1))
send_cli_command OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
sleep 1
send_cli_command OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING

wait "${PATH_B_PID}"
unset PATH_B_PID

if [[ -n "${ACTIVE_CANDUMP_PID:-}" ]]; then
  wait "${ACTIVE_CANDUMP_PID}" || true
  unset ACTIVE_CANDUMP_PID
fi
if [[ -n "${RESERVED_CANDUMP_PID:-}" ]]; then
  wait "${RESERVED_CANDUMP_PID}" || true
  unset RESERVED_CANDUMP_PID
fi

PATH_B_DISPATCHED="$(grep -c 'OpCodeDispatched' "${PATH_B_LOG}" || true)"
PATH_B_COMPLETED="$(grep -c 'OpCodeCompleted' "${PATH_B_LOG}" || true)"
if [[ "${PATH_B_DISPATCHED}" -lt 2 || "${PATH_B_COMPLETED}" -lt 2 ]]; then
  echo "Path B did not observe enough dispatched/completed command events." >&2
  cat "${PATH_B_LOG}" >&2 || true
  exit 1
fi
if ! grep -Eq 'eps soc=.* pdu=7' "${PATH_B_LOG}"; then
  echo "Path B did not observe the commanded EPS PDU state change." >&2
  cat "${PATH_B_LOG}" >&2 || true
  exit 1
fi
if ! grep -q 'adcs mode=POINTING' "${PATH_B_LOG}"; then
  echo "Path B did not observe the commanded ADCS mode change." >&2
  cat "${PATH_B_LOG}" >&2 || true
  exit 1
fi

if [[ ! -s "${ACTIVE_CANDUMP_LOG}" ]]; then
  echo "Active CAN bus candump captured no traffic." >&2
  exit 1
fi
if [[ -s "${RESERVED_CANDUMP_LOG}" ]]; then
  echo "Reserved CAN channel unexpectedly observed primary-bus traffic." >&2
  cat "${RESERVED_CANDUMP_LOG}" >&2 || true
  exit 1
fi

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash <<EOF >"${SUBSYSTEM_POST_STATS_LOG}" 2>&1
set -euo pipefail
ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}")
echo "----"
ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}")
EOF

obc_ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF >"${OBC_POST_STATS_LOG}" 2>&1
set -euo pipefail
ip -details -statistics link show dev $(printf '%q' "${OBC_CSP_CAN_DEVICE}")
EOF

grep -q 'ERROR-ACTIVE' "${SUBSYSTEM_POST_STATS_LOG}" || {
  echo "Subsystem post-run CAN statistics do not report ERROR-ACTIVE." >&2
  cat "${SUBSYSTEM_POST_STATS_LOG}" >&2
  exit 1
}
grep -q 'ERROR-ACTIVE' "${OBC_POST_STATS_LOG}" || {
  echo "OBC post-run CAN statistics do not report ERROR-ACTIVE." >&2
  cat "${OBC_POST_STATS_LOG}" >&2
  exit 1
}
if grep -Eq 'bus-off[^0-9]*[1-9]' "${SUBSYSTEM_POST_STATS_LOG}" || grep -Eq 'bus-off[^0-9]*[1-9]' "${OBC_POST_STATS_LOG}"; then
  echo "Observed CAN bus-off count after the governed probe." >&2
  cat "${SUBSYSTEM_POST_STATS_LOG}" >&2
  cat "${OBC_POST_STATS_LOG}" >&2
  exit 1
fi

echo "shared-canfd-csp-gds-probe: PASS"
echo "  temp dir            : ${PROBE_TMP_DIR}"
echo "  parentdev mapping   : ${SUBSYSTEM_PARENTDEV_LOG}"
echo "  active candump      : ${ACTIVE_CANDUMP_LOG}"
echo "  reserved candump    : ${RESERVED_CANDUMP_LOG}"
echo "  subsystem pre stats : ${SUBSYSTEM_PRE_STATS_LOG}"
echo "  subsystem post stats: ${SUBSYSTEM_POST_STATS_LOG}"
echo "  obc pre stats       : ${OBC_PRE_STATS_LOG}"
echo "  obc post stats      : ${OBC_POST_STATS_LOG}"
