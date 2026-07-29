#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-$(obc_default_remote_workspace_dir)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE:-can0}"
SUBSYSTEM_SIM_RESERVED_CAN_DEVICE="${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE:-can1}"
CSP_TRANSPORT="${CSP_TRANSPORT:-socketcan}"
CSP_CAN_PROMISC="${CSP_CAN_PROMISC:-0}"
CAN_BITRATE="${CAN_BITRATE:-500000}"
CAN_DBITRATE="${CAN_DBITRATE:-2000000}"
CAN_RESTART_MS="${CAN_RESTART_MS:-100}"
PREPARE_CAN_INTERFACES="${PREPARE_CAN_INTERFACES:-1}"
PATH_A_WARMUP_SEC="${PATH_A_WARMUP_SEC:-10}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-shared-canfd-health.XXXXXX")}"
BRINGUP_LOG="${BRINGUP_LOG:-${PROBE_TMP_DIR}/bringup.log}"
SUBSYSTEM_STACK_LOG="${SUBSYSTEM_STACK_LOG:-${PROBE_TMP_DIR}/subsystem-can-stack.log}"
OBC_LOG="${OBC_LOG:-${PROBE_TMP_DIR}/obc-can-stack.log}"
SUBSYSTEM_STATS_LOG="${SUBSYSTEM_STATS_LOG:-${PROBE_TMP_DIR}/subsystem-can-stats.log}"
OBC_STATS_LOG="${OBC_STATS_LOG:-${PROBE_TMP_DIR}/obc-can-stats.log}"

if [[ "${CSP_TRANSPORT}" != "socketcan" ]]; then
  echo "This probe requires CSP_TRANSPORT=socketcan." >&2
  exit 2
fi
if [[ "${CSP_CAN_PROMISC}" != "0" ]]; then
  echo "Governed acceptance must use CSP_CAN_PROMISC=0." >&2
  exit 2
fi
if [[ "${PREPARE_CAN_INTERFACES}" != "0" && "${PREPARE_CAN_INTERFACES}" != "1" ]]; then
  echo "PREPARE_CAN_INTERFACES must be 0 or 1." >&2
  exit 2
fi
obc_require_can_device_name "${OBC_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
obc_require_can_device_name "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}"

mkdir -p "${PROBE_TMP_DIR}"

cleanup() {
  local rc=$?
  if [[ -n "${SUBSYSTEM_STACK_PID:-}" ]]; then
    kill "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
    wait "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1 || true
  fi
  if [[ ${rc} -ne 0 ]]; then
    echo "shared-canfd-eps-adcs-health-probe: STOPPED" >&2
    echo "log-dir=${PROBE_TMP_DIR}" >&2
    echo "bringup-log=${BRINGUP_LOG}" >&2
    echo "subsystem-stack-log=${SUBSYSTEM_STACK_LOG}" >&2
    echo "obc-log=${OBC_LOG}" >&2
    echo "subsystem-stats-log=${SUBSYSTEM_STATS_LOG}" >&2
    echo "obc-stats-log=${OBC_STATS_LOG}" >&2
  fi
  exit "${rc}"
}
trap cleanup EXIT INT TERM

remote_can_bring_up() {
  local target="${1:?target is required}"
  local remote_dir="${2:?remote_dir is required}"
  local can_device="${3:?can_device is required}"
  local remote_cd="cd $(printf '%q' "${remote_dir}")"
  local quoted_device
  local remote_script

  # Stage 0 must work before this branch is synced to the remote workspaces.
  # Keep bring-up independent of newly added remote helper functions.
  quoted_device="$(printf '%q' "${can_device}")"
  remote_script="set -euo pipefail; ${remote_cd}; sudo -n ip link set ${quoted_device} down >/dev/null 2>&1 || true; sudo -n ip link set ${quoted_device} up type can bitrate $(printf '%q' "${CAN_BITRATE}") dbitrate $(printf '%q' "${CAN_DBITRATE}") restart-ms $(printf '%q' "${CAN_RESTART_MS}") fd on"
  obc_ssh "${target}" /bin/bash -lc "$(printf '%q' "${remote_script}")"
}

if [[ "${PREPARE_CAN_INTERFACES}" == "1" ]]; then
  {
    echo "Preparing ${SUBSYSTEM_SIM_SSH_TARGET}:${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    remote_can_bring_up "${SUBSYSTEM_SIM_SSH_TARGET}" "${SUBSYSTEM_SIM_REMOTE_DIR}" "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"
    echo "Preparing ${OBC_SSH_TARGET}:${OBC_CSP_CAN_DEVICE}"
    remote_can_bring_up "${OBC_SSH_TARGET}" "${RPI_REMOTE_DIR}" "${OBC_CSP_CAN_DEVICE}"
  } >"${BRINGUP_LOG}" 2>&1
else
  echo "PREPARE_CAN_INTERFACES=0; using existing CAN link state." >"${BRINGUP_LOG}"
fi

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"); echo ----; ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}")")" \
  >"${SUBSYSTEM_STATS_LOG}" 2>&1 || true
obc_ssh "${OBC_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "ip -details -statistics link show dev $(printf '%q' "${OBC_CSP_CAN_DEVICE}")")" \
  >"${OBC_STATS_LOG}" 2>&1 || true

env \
  SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET}" \
  SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  SUBSYSTEM_SIM_CSP_CAN_DEVICE="${SUBSYSTEM_SIM_CSP_CAN_DEVICE}" \
  SUBSYSTEM_SIM_RESERVED_CAN_DEVICE="${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}" \
  CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
  KILL_EXISTING_PIDS=1 \
  bash "${ROOT_DIR}/scripts/run_subsystem_sim_remote_can_stack.sh" >"${SUBSYSTEM_STACK_LOG}" 2>&1 &
SUBSYSTEM_STACK_PID=$!

sleep 5
if ! kill -0 "${SUBSYSTEM_STACK_PID}" >/dev/null 2>&1; then
  echo "Subsystem CAN stack failed to start." >&2
  cat "${SUBSYSTEM_STACK_LOG}" >&2 || true
  exit 1
fi

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
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  OBC_CSP_CAN_DEVICE="${OBC_CSP_CAN_DEVICE}" \
  CSP_CAN_PROMISC="${CSP_CAN_PROMISC}" \
  GDS_PORT=0 \
  MANAGE_AUTOSTART=1 \
  KILL_EXISTING_PIDS=1 \
  RUNTIME_ROOT="${RPI_REMOTE_DIR}/runtime/comm-csp-socketcan-stage0" \
  bash "${ROOT_DIR}/scripts/run_rpi_can_csp_stack.sh" >"${OBC_LOG}" 2>&1

if [[ "$(grep -c 'csp ping response=0 success=yes' "${OBC_LOG}" || true)" -lt 2 ]]; then
  echo "Did not observe successful CSP pings to both EPS and ADCS." >&2
  cat "${OBC_LOG}" >&2 || true
  exit 1
fi
grep -q 'eps soc=' "${OBC_LOG}" || {
  echo "Did not observe EPS readback." >&2
  cat "${OBC_LOG}" >&2 || true
  exit 1
}
grep -q 'adcs mode=' "${OBC_LOG}" || {
  echo "Did not observe ADCS readback." >&2
  cat "${OBC_LOG}" >&2 || true
  exit 1
}

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_CSP_CAN_DEVICE}"); echo ----; ip -details -statistics link show dev $(printf '%q' "${SUBSYSTEM_SIM_RESERVED_CAN_DEVICE}")")" \
  >>"${SUBSYSTEM_STATS_LOG}" 2>&1 || true
obc_ssh "${OBC_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "ip -details -statistics link show dev $(printf '%q' "${OBC_CSP_CAN_DEVICE}")")" \
  >>"${OBC_STATS_LOG}" 2>&1 || true

echo "shared-canfd-eps-adcs-health-probe: PASS"
echo "formal-verdict=eps-adcs-health"
echo "log-dir=${PROBE_TMP_DIR}"
grep -E 'csp ping response=0 success=yes|eps soc=|adcs mode=' "${OBC_LOG}" || true
