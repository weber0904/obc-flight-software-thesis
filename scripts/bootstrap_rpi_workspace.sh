#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
HOST_PROJECT_VERSION="$(git -C "${ROOT_DIR}" describe --tags --always --dirty --broken)"
HOST_FRAMEWORK_VERSION="$(git -C "${ROOT_DIR}/lib/fprime" describe --tags --always --dirty --broken)"

if [[ "${SKIP_SYNC:-0}" != "1" ]]; then
  "${ROOT_DIR}/scripts/sync_rpi_workspace.sh"
fi

ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF
set -euo pipefail
cd $(printf '%q' "${RPI_REMOTE_DIR}")

if ! pkg-config --exists libzmq; then
  echo "error: libzmq development package is missing on the Raspberry Pi target. Install libzmq3-dev before building." >&2
  exit 2
fi

if ! pkg-config --exists libsocketcan; then
  echo "error: libsocketcan development package is missing on the Raspberry Pi target. Install libsocketcan-dev before building." >&2
  exit 2
fi

python3 -m venv fprime-venv
./fprime-venv/bin/pip install --upgrade pip setuptools wheel cmake
./fprime-venv/bin/pip install -r requirements.txt

export PATH="$(printf '%q' "${RPI_REMOTE_DIR}")/fprime-venv/bin:\$PATH"
export FPRIME_PROJECT_VERSION_OVERRIDE=$(printf '%q' "${HOST_PROJECT_VERSION}")
export FPRIME_FRAMEWORK_VERSION_OVERRIDE=$(printf '%q' "${HOST_FRAMEWORK_VERSION}")
fprime-util generate -f
fprime-util build

mkdir -p runtime/integ-rpi/staging runtime/integ-rpi/persistent-data runtime/integ-rpi/logs
EOF

echo "Raspberry Pi workspace bootstrapped at ${OBC_SSH_TARGET}:${RPI_REMOTE_DIR}"
