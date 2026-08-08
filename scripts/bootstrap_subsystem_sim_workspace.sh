#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
HOST_PROJECT_VERSION="$(git -C "${ROOT_DIR}" describe --tags --always --dirty --broken)"
HOST_FRAMEWORK_VERSION="$(git -C "${ROOT_DIR}/lib/fprime" describe --tags --always --dirty --broken)"

if [[ "${SKIP_SYNC:-0}" != "1" ]]; then
  bash "${ROOT_DIR}/scripts/sync_subsystem_sim_workspace.sh"
fi

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash <<EOF
set -euo pipefail
cd $(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")

if ! pkg-config --exists libzmq; then
  echo "error: libzmq development package is missing on the subsystem simulator host. Install libzmq3-dev before building." >&2
  exit 2
fi

if ! pkg-config --exists libsocketcan; then
  echo "error: libsocketcan development package is missing on the subsystem simulator host. Install libsocketcan-dev before building." >&2
  exit 2
fi

python3 -m venv fprime-venv
./fprime-venv/bin/pip install --upgrade pip setuptools wheel cmake
./fprime-venv/bin/pip install -r requirements.txt

export PATH=$(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")/fprime-venv/bin:\$PATH
export FPRIME_PROJECT_VERSION_OVERRIDE=$(printf '%q' "${HOST_PROJECT_VERSION}")
export FPRIME_FRAMEWORK_VERSION_OVERRIDE=$(printf '%q' "${HOST_FRAMEWORK_VERSION}")
fprime-util generate -f
fprime-util build

mkdir -p runtime/subsystem-sim/logs
EOF

echo "Subsystem simulator workspace bootstrapped at ${SUBSYSTEM_SIM_SSH_TARGET}:${SUBSYSTEM_SIM_REMOTE_DIR}"
