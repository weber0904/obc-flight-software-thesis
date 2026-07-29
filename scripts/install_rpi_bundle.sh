#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
FORCE_INSTALL="${FORCE_INSTALL:-0}"
BUNDLE_PATH="${1:-}"

if [[ -z "${BUNDLE_PATH}" ]]; then
  BUNDLE_PATH="$(obc_latest_rpi_bundle "${ROOT_DIR}")"
fi

if [[ -z "${BUNDLE_PATH}" || ! -f "${BUNDLE_PATH}" ]]; then
  echo "Bundle tarball not found. Run scripts/package_rpi_bundle.sh first or pass a tarball path." >&2
  exit 1
fi

BUNDLE_FILENAME="$(basename "${BUNDLE_PATH}")"
RELEASE_ID="${BUNDLE_FILENAME#obc-rpi-}"
RELEASE_ID="${RELEASE_ID%.tar.gz}"
RELEASE_DIR="${RPI_INSTALL_ROOT}/releases/${RELEASE_ID}"
RUNTIME_DIR="${RPI_INSTALL_ROOT}/runtime/integ-rpi"
REMOTE_BUNDLE_PATH="${RUNTIME_DIR}/staging/${BUNDLE_FILENAME}.upload.$$"
REMOTE_STAGE_DIR="${RPI_INSTALL_ROOT}/releases/.${RELEASE_ID}.staging.$$"
LOCAL_BUNDLE_SIZE_BYTES="$(wc -c < "${BUNDLE_PATH}" | tr -d '[:space:]')"
LOCAL_BUNDLE_SHA256="$(obc_sha256_file "${BUNDLE_PATH}")"

obc_ssh "${OBC_SSH_TARGET}" /bin/bash -s -- \
  "${RPI_INSTALL_ROOT}" \
  "${RELEASE_DIR}" \
  "${RUNTIME_DIR}" \
  "${FORCE_INSTALL}" \
  "${REMOTE_BUNDLE_PATH}" \
  "${REMOTE_STAGE_DIR}" <<'EOF'
set -euo pipefail

INSTALL_ROOT="${1:?install root is required}"
RELEASE_DIR="${2:?release dir is required}"
RUNTIME_DIR="${3:?runtime dir is required}"
FORCE_INSTALL="${4:?force install flag is required}"
REMOTE_BUNDLE_PATH="${5:?remote bundle path is required}"
REMOTE_STAGE_DIR="${6:?remote stage dir is required}"

mkdir -p "${INSTALL_ROOT}/releases" "${RUNTIME_DIR}/persistent-data" "${RUNTIME_DIR}/staging" "${RUNTIME_DIR}/logs"
rm -rf "${REMOTE_STAGE_DIR}"
rm -f "${REMOTE_BUNDLE_PATH}"

if [[ -e "${RELEASE_DIR}" && "${FORCE_INSTALL}" != "1" ]]; then
  echo "Release already exists at ${RELEASE_DIR}; set FORCE_INSTALL=1 to replace it." >&2
  exit 2
fi
EOF

obc_ssh "${OBC_SSH_TARGET}" /bin/bash -c "$(printf 'cat > %q' "${REMOTE_BUNDLE_PATH}")" < "${BUNDLE_PATH}"

obc_ssh "${OBC_SSH_TARGET}" python3 - \
  "${REMOTE_BUNDLE_PATH}" \
  "${LOCAL_BUNDLE_SIZE_BYTES}" \
  "${LOCAL_BUNDLE_SHA256}" <<'PY'
import hashlib
import pathlib
import sys

bundle_path = pathlib.Path(sys.argv[1])
expected_size = int(sys.argv[2])
expected_sha = sys.argv[3]

actual_size = bundle_path.stat().st_size
digest = hashlib.sha256()
with bundle_path.open("rb") as handle:
    for chunk in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(chunk)
actual_sha = digest.hexdigest()

if actual_size != expected_size:
    raise SystemExit(
        f"Remote bundle size mismatch for {bundle_path}: "
        f"expected {expected_size}, got {actual_size}"
    )
if actual_sha != expected_sha:
    raise SystemExit(
        f"Remote bundle sha256 mismatch for {bundle_path}: "
        f"expected {expected_sha}, got {actual_sha}"
    )
PY

obc_ssh "${OBC_SSH_TARGET}" /bin/bash -s -- \
  "${RPI_INSTALL_ROOT}" \
  "${RELEASE_DIR}" \
  "${FORCE_INSTALL}" \
  "${REMOTE_BUNDLE_PATH}" \
  "${REMOTE_STAGE_DIR}" \
  "${RELEASE_ID}" <<'EOF'
set -euo pipefail

INSTALL_ROOT="${1:?install root is required}"
RELEASE_DIR="${2:?release dir is required}"
FORCE_INSTALL="${3:?force install flag is required}"
REMOTE_BUNDLE_PATH="${4:?remote bundle path is required}"
REMOTE_STAGE_DIR="${5:?remote stage dir is required}"
RELEASE_ID="${6:?release id is required}"

cleanup() {
  rm -rf "${REMOTE_STAGE_DIR}"
  rm -f "${REMOTE_BUNDLE_PATH}"
}
trap cleanup EXIT

mkdir -p "${REMOTE_STAGE_DIR}"
tar -xzf "${REMOTE_BUNDLE_PATH}" -C "${REMOTE_STAGE_DIR}"

python3 - "${REMOTE_STAGE_DIR}" <<'PY'
import hashlib
import json
import pathlib
import sys

stage_dir = pathlib.Path(sys.argv[1])
manifest_path = stage_dir / "manifest.json"
if not manifest_path.is_file():
    raise SystemExit(f"Expected manifest at {manifest_path}")

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
files = manifest.get("files", {})
if not isinstance(files, dict) or not files:
    raise SystemExit("manifest.json is missing a non-empty files mapping")

for rel_path, expected in files.items():
    candidate = stage_dir / rel_path
    if not candidate.is_file():
        raise SystemExit(f"Missing extracted file: {rel_path}")
    actual_size = candidate.stat().st_size
    expected_size = int(expected["size_bytes"])
    if actual_size != expected_size:
        raise SystemExit(
            f"Extracted size mismatch for {rel_path}: expected {expected_size}, got {actual_size}"
        )
    digest = hashlib.sha256()
    with candidate.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    actual_sha = digest.hexdigest()
    expected_sha = expected["sha256"]
    if actual_sha != expected_sha:
        raise SystemExit(
            f"Extracted sha256 mismatch for {rel_path}: expected {expected_sha}, got {actual_sha}"
        )
PY

if [[ -e "${RELEASE_DIR}" ]]; then
  if [[ "${FORCE_INSTALL}" == "1" ]]; then
    rm -rf "${RELEASE_DIR}"
  else
    echo "Release already exists at ${RELEASE_DIR}; set FORCE_INSTALL=1 to replace it." >&2
    exit 2
  fi
fi

mv "${REMOTE_STAGE_DIR}" "${RELEASE_DIR}"
ln -sfn "${RELEASE_DIR}" "${INSTALL_ROOT}/current"
printf 'Installed release %s at %s\n' "${RELEASE_ID}" "${INSTALL_ROOT}"
EOF
