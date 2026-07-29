#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
HELPER="${ROOT_DIR}/scripts/comm_verification/lib/managed_sband_unavailable_window.py"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
SBAND_COMM_SERVICE_NAME="${SBAND_COMM_SERVICE_NAME:-subsystem-sband-csp.service}"
RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-120}"
COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC="${COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC:-60}"
ARTIFACT_PATH="${ARTIFACT_PATH:-/tmp/managed-sband-unavailable-window.json}"

obc_require_systemd_service_name "${OBC_COMM_CSP_SERVICE_NAME}"
obc_require_systemd_service_name "${SBAND_COMM_SERVICE_NAME}"

exec "${PYTHON_BIN}" "${HELPER}" \
  --artifact-path "${ARTIFACT_PATH}" \
  --obc-target "${OBC_SSH_TARGET}" \
  --obc-service "${OBC_COMM_CSP_SERVICE_NAME}" \
  --subsystem-target "${SUBSYSTEM_SIM_SSH_TARGET}" \
  --sband-comm-service "${SBAND_COMM_SERVICE_NAME}" \
  --restart-timeout "${RESTART_TIMEOUT_SEC}" \
  --failover-timeout "${COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC}"
