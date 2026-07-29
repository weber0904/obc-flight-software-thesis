#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
REMOTE_HOST="${REMOTE_HOST:-$(obc_default_remote_carrier_host)}"
CSP_TRANSPORT="${CSP_TRANSPORT:-zmqhub}"
CSP_HUB_HOST="${CSP_HUB_HOST:-${REMOTE_HOST}}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56630}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57630}"
EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID:-2}"
ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID:-3}"
KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS:-1}"

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && cd $(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")"
REMOTE_CMD+=" && bash scripts/run_subsystem_sim_stack.sh"

exec ssh "${SUBSYSTEM_SIM_SSH_TARGET}" \
  env \
  REMOTE_HOST="${REMOTE_HOST}" \
  CSP_TRANSPORT="${CSP_TRANSPORT}" \
  CSP_HUB_HOST="${CSP_HUB_HOST}" \
  CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
  CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
  EPS_CSP_NODE_ID="${EPS_CSP_NODE_ID}" \
  ADCS_CSP_NODE_ID="${ADCS_CSP_NODE_ID}" \
  KILL_EXISTING_PIDS="${KILL_EXISTING_PIDS}" \
  /bin/bash -lc "$(printf '%q' "${REMOTE_CMD}")"
