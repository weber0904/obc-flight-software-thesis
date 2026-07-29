#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_INSTALL_ROOT}/runtime/integ-rpi}"

ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF
set -euo pipefail

RELEASE_ROOT=$(printf '%q' "${RPI_INSTALL_ROOT}")/current
RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")
PERSISTENT_ROOT="\${RUNTIME_ROOT}/persistent-data"
STAGING_ROOT="\${RUNTIME_ROOT}/staging"
LOG_ROOT="\${RUNTIME_ROOT}/logs"

pkill -f '/home/operator/obc-deploy/current/bin/(eps_simulator|adcs_simulator|radio_mock_server|OBC)' >/dev/null 2>&1 || true
sleep 1

{
  sleep 2
  printf 'status\n'
  sleep 1
  printf 'quit\n'
} | env \
  RUNTIME_ROOT="\${RUNTIME_ROOT}" \
  PERSISTENT_ROOT="\${PERSISTENT_ROOT}" \
  STAGING_ROOT="\${STAGING_ROOT}" \
  LOG_ROOT="\${LOG_ROOT}" \
  "\${RELEASE_ROOT}/launch/run_stack.sh"
EOF
