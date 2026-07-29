#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
SUBSYSTEM_SIM_REMOTE_DIR="${SUBSYSTEM_SIM_REMOTE_DIR:-$(obc_resolve_subsystem_sim_remote_dir)}"
TAR_CMD=(tar -C "${ROOT_DIR}")
REMOTE_DIR_QUOTED="$(printf '%q' "${SUBSYSTEM_SIM_REMOTE_DIR}")"
REMOTE_TAR_CMD="tar -xzf - -C ${REMOTE_DIR_QUOTED}"

if tar --help 2>/dev/null | grep -q -- '--no-mac-metadata'; then
  TAR_CMD+=(--no-mac-metadata)
fi
if tar --help 2>/dev/null | grep -q -- '--no-xattrs'; then
  TAR_CMD+=(--no-xattrs)
fi
if tar --help 2>/dev/null | grep -q -- '--no-acls'; then
  TAR_CMD+=(--no-acls)
fi
if tar --help 2>/dev/null | grep -q -- '--no-fflags'; then
  TAR_CMD+=(--no-fflags)
fi
if obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" 'tar --help 2>/dev/null | grep -q -- --warning=no-unknown-keyword'; then
  REMOTE_TAR_CMD="tar --warning=no-unknown-keyword -xzf - -C ${REMOTE_DIR_QUOTED}"
fi

mkdir -p "${ROOT_DIR}/build-artifacts"

obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "mkdir -p ${REMOTE_DIR_QUOTED}")"

COPYFILE_DISABLE=1 COPY_EXTENDED_ATTRIBUTES_DISABLE=1 "${TAR_CMD[@]}" \
  --exclude='./.git' \
  --exclude='./.DS_Store' \
  --exclude='./build-fprime-automatic-native' \
  --exclude='./build-fprime-automatic-native-ut' \
  --exclude='./build-artifacts' \
  --exclude='./fprime-venv' \
  --exclude='./logs' \
  --exclude='./runtime' \
  -czf - . | obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash -lc "$(printf '%q' "${REMOTE_TAR_CMD}")"

echo "Synced workspace to ${SUBSYSTEM_SIM_SSH_TARGET}:${SUBSYSTEM_SIM_REMOTE_DIR}"
