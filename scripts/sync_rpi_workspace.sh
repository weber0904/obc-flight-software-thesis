#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
RPI_SYNC_EXTRA_EXCLUDE_PATH="${RPI_SYNC_EXTRA_EXCLUDE_PATH:-}"
RPI_SYNC_TRACKED_ONLY="${RPI_SYNC_TRACKED_ONLY:-0}"
RPI_SYNC_REPLACE_REMOTE="${RPI_SYNC_REPLACE_REMOTE:-0}"
TAR_CMD=(tar -C "${ROOT_DIR}")
REMOTE_TAR_CMD=(tar -xzf - -C "${RPI_REMOTE_DIR}")

for value in "${RPI_SYNC_TRACKED_ONLY}" "${RPI_SYNC_REPLACE_REMOTE}"; do
  if [[ "${value}" != "0" && "${value}" != "1" ]]; then
    echo "RPI_SYNC_TRACKED_ONLY and RPI_SYNC_REPLACE_REMOTE must be 0 or 1." >&2
    exit 2
  fi
done
if [[ "${RPI_SYNC_REPLACE_REMOTE}" == "1" && "${RPI_SYNC_TRACKED_ONLY}" != "1" ]]; then
  echo "RPI_SYNC_REPLACE_REMOTE=1 requires RPI_SYNC_TRACKED_ONLY=1." >&2
  exit 2
fi

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
if ssh "${OBC_SSH_TARGET}" 'tar --help 2>/dev/null | grep -q -- --warning=no-unknown-keyword'; then
  REMOTE_TAR_CMD=(tar --warning=no-unknown-keyword -xzf - -C "${RPI_REMOTE_DIR}")
fi

TAR_CMD+=(
  --exclude='./.git'
  --exclude='./.DS_Store'
  --exclude='./output'
  --exclude='./build-fprime-automatic-native'
  --exclude='./build-fprime-automatic-native-ut'
  --exclude='./build-artifacts'
  --exclude='./fprime-venv'
  --exclude='./logs'
  --exclude='./runtime'
)
if [[ -n "${RPI_SYNC_EXTRA_EXCLUDE_PATH}" ]]; then
  if [[ "${RPI_SYNC_EXTRA_EXCLUDE_PATH}" == /* || "${RPI_SYNC_EXTRA_EXCLUDE_PATH}" == *".."* ]]; then
    echo "RPI_SYNC_EXTRA_EXCLUDE_PATH must be a repository-relative path without '..'." >&2
    exit 2
  fi
  RPI_SYNC_EXTRA_EXCLUDE_PATH="${RPI_SYNC_EXTRA_EXCLUDE_PATH#./}"
  if [[ -z "${RPI_SYNC_EXTRA_EXCLUDE_PATH}" || "${RPI_SYNC_EXTRA_EXCLUDE_PATH}" == "." ]]; then
    echo "RPI_SYNC_EXTRA_EXCLUDE_PATH must not select the repository root." >&2
    exit 2
  fi
  TAR_CMD+=(--exclude="./${RPI_SYNC_EXTRA_EXCLUDE_PATH%/}")
fi

mkdir -p "${ROOT_DIR}/build-artifacts"

if [[ "${RPI_SYNC_TRACKED_ONLY}" == "1" ]]; then
  # Formal evidence syncs must be attributable to the reviewed Git index.
  # Recurse into initialized submodules, but never serialize untracked or
  # ignored working-tree inputs into the target build workspace.
  remote_receive_command="mkdir -p $(printf '%q' "${RPI_REMOTE_DIR}") && ${REMOTE_TAR_CMD[*]}"
  if [[ "${RPI_SYNC_REPLACE_REMOTE}" == "1" ]]; then
    case "${RPI_REMOTE_DIR}" in
      ""|"/"|"."|"/home"|"/home/"|"/home/operator"|"/home/operator/")
        echo "Refusing to replace unsafe RPI_REMOTE_DIR=${RPI_REMOTE_DIR}" >&2
        exit 2
        ;;
    esac
    remote_staging="${RPI_REMOTE_DIR}.route1-sync-new"
    remote_backup="${RPI_REMOTE_DIR}.route1-sync-previous"
    REMOTE_TAR_CMD=(tar -xzf - -C "${remote_staging}")
    if ssh "${OBC_SSH_TARGET}" 'tar --help 2>/dev/null | grep -q -- --warning=no-unknown-keyword'; then
      REMOTE_TAR_CMD=(tar --warning=no-unknown-keyword -xzf - -C "${remote_staging}")
    fi
    remote_receive_command="set -e; rm -rf $(printf '%q' "${remote_staging}") $(printf '%q' "${remote_backup}"); mkdir -p $(printf '%q' "${remote_staging}"); ${REMOTE_TAR_CMD[*]}; if [ -e $(printf '%q' "${RPI_REMOTE_DIR}") ]; then mv $(printf '%q' "${RPI_REMOTE_DIR}") $(printf '%q' "${remote_backup}"); fi; mv $(printf '%q' "${remote_staging}") $(printf '%q' "${RPI_REMOTE_DIR}")"
  fi
  git -C "${ROOT_DIR}" ls-files --recurse-submodules -z |
    COPYFILE_DISABLE=1 COPY_EXTENDED_ATTRIBUTES_DISABLE=1 "${TAR_CMD[@]}" \
      --null --files-from=- -czf - |
    ssh "${OBC_SSH_TARGET}" "${remote_receive_command}"
else
  COPYFILE_DISABLE=1 COPY_EXTENDED_ATTRIBUTES_DISABLE=1 "${TAR_CMD[@]}" \
    -czf - . |
    ssh "${OBC_SSH_TARGET}" "mkdir -p \"${RPI_REMOTE_DIR}\" && ${REMOTE_TAR_CMD[*]}"
fi

echo "Synced workspace to ${OBC_SSH_TARGET}:${RPI_REMOTE_DIR}"
