#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
PACKAGE_ROOT="${PACKAGE_ROOT:-${ROOT_DIR}/build-artifacts/packages/rpi}"
RELEASE_ID_OVERRIDE="${RELEASE_ID:-}"
OBC_PACKAGE_KEYSTORE_PATH="${OBC_PACKAGE_KEYSTORE_PATH:-}"
PUBLIC_EXAMPLE_KEYSTORE="${ROOT_DIR}/config/security/command-auth.example.ini"

if [[ -z "${OBC_PACKAGE_KEYSTORE_PATH}" ]]; then
  echo "OBC_PACKAGE_KEYSTORE_PATH is required for target packaging." >&2
  echo "Provide a private keystore outside the repository; the public example is refused." >&2
  exit 1
fi
if [[ ! -f "${OBC_PACKAGE_KEYSTORE_PATH}" ]]; then
  echo "Target packaging keystore not found: ${OBC_PACKAGE_KEYSTORE_PATH}" >&2
  exit 1
fi
if cmp -s "${OBC_PACKAGE_KEYSTORE_PATH}" "${PUBLIC_EXAMPLE_KEYSTORE}"; then
  echo "Refusing to package the public example command-auth keystore." >&2
  exit 1
fi

TMP_DIR="$(mktemp -d "${ROOT_DIR}/build-artifacts/rpi-package.XXXXXX")"
cleanup() {
  rm -rf "${TMP_DIR}"
}
trap cleanup EXIT

mkdir -p "${PACKAGE_ROOT}"

VERSION_OUTPUT="$(ssh "${OBC_SSH_TARGET}" "python3 - <<'PY'
import json
from pathlib import Path

path = Path('${RPI_REMOTE_DIR}/build-fprime-automatic-native/versions/version.json')
data = json.loads(path.read_text())
print(data['project_version'])
print(data['framework_version'])
PY")"

PROJECT_VERSION="$(printf '%s\n' "${VERSION_OUTPUT}" | sed -n '1p')"
FRAMEWORK_VERSION="$(printf '%s\n' "${VERSION_OUTPUT}" | sed -n '2p')"

if [[ -z "${PROJECT_VERSION}" || -z "${FRAMEWORK_VERSION}" ]]; then
  echo "Failed to read remote version metadata from ${OBC_SSH_TARGET}:${RPI_REMOTE_DIR}" >&2
  exit 1
fi

RELEASE_ID="${RELEASE_ID_OVERRIDE:-$(obc_sanitize_name "${PROJECT_VERSION}")}"
BUNDLE_NAME="obc-rpi-${RELEASE_ID}"
STAGE_DIR="${TMP_DIR}/${BUNDLE_NAME}"
PACKAGE_DIR="${PACKAGE_ROOT}/${RELEASE_ID}"

mkdir -p "${STAGE_DIR}/bin" "${STAGE_DIR}/config/security" "${STAGE_DIR}/dict" "${STAGE_DIR}/launch" "${STAGE_DIR}/meta" "${STAGE_DIR}/systemd" "${STAGE_DIR}/udev" "${PACKAGE_DIR}"

ssh "${OBC_SSH_TARGET}" "cd $(printf '%q' "${RPI_REMOTE_DIR}") && tar -czf - \
  build-fprime-automatic-native/bin/Linux/payload_camera_backend_helper \
  build-fprime-automatic-native/bin/Linux/eps_simulator \
  build-fprime-automatic-native/bin/Linux/csp_zmqproxy \
  build-fprime-automatic-native/bin/Linux/adcs_simulator \
  build-fprime-automatic-native/bin/Linux/radio_mock_server \
  build-artifacts/Linux/OBC/bin/OBC \
  build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json \
  build-fprime-automatic-native/versions/version.json" | tar -xzf - -C "${TMP_DIR}"

cp "${TMP_DIR}/build-fprime-automatic-native/bin/Linux/payload_camera_backend_helper" "${STAGE_DIR}/bin/payload_camera_backend_helper"
cp "${TMP_DIR}/build-artifacts/Linux/OBC/bin/OBC" "${STAGE_DIR}/bin/OBC"
cp "${TMP_DIR}/build-fprime-automatic-native/bin/Linux/eps_simulator" "${STAGE_DIR}/bin/eps_simulator"
cp "${TMP_DIR}/build-fprime-automatic-native/bin/Linux/csp_zmqproxy" "${STAGE_DIR}/bin/csp_zmqproxy"
cp "${TMP_DIR}/build-fprime-automatic-native/bin/Linux/adcs_simulator" "${STAGE_DIR}/bin/adcs_simulator"
cp "${TMP_DIR}/build-fprime-automatic-native/bin/Linux/radio_mock_server" "${STAGE_DIR}/bin/radio_mock_server"
install -m 0600 "${OBC_PACKAGE_KEYSTORE_PATH}" "${STAGE_DIR}/config/security/command-auth.ini"
cp "${TMP_DIR}/build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json" "${STAGE_DIR}/dict/AppTopologyDictionary.json"
cp "${TMP_DIR}/build-fprime-automatic-native/versions/version.json" "${STAGE_DIR}/meta/version.json"
cp "${ROOT_DIR}/packaging/rpi/launch/run_stack.sh" "${STAGE_DIR}/launch/run_stack.sh"
cp "${ROOT_DIR}/packaging/rpi/launch/run_obc_comm_csp_stack.sh" "${STAGE_DIR}/launch/run_obc_comm_csp_stack.sh"
cp "${ROOT_DIR}/scripts/_common.sh" "${STAGE_DIR}/launch/_common.sh"
cp "${ROOT_DIR}/packaging/rpi/systemd/obc-comm-csp-stack.service.template" "${STAGE_DIR}/systemd/obc-comm-csp-stack.service.template"
cp "${ROOT_DIR}/packaging/rpi/udev/90-obc-watchdog.rules" "${STAGE_DIR}/udev/90-obc-watchdog.rules"
chmod +x "${STAGE_DIR}/launch/run_stack.sh"
chmod +x "${STAGE_DIR}/launch/run_obc_comm_csp_stack.sh"

MANIFEST_PATH="${STAGE_DIR}/manifest.json"
python3 - "${STAGE_DIR}" "${MANIFEST_PATH}" "${BUNDLE_NAME}" "${RELEASE_ID}" "${PROJECT_VERSION}" "${FRAMEWORK_VERSION}" "${RPI_REMOTE_DIR}" <<'PY'
import datetime as dt
import hashlib
import json
import os
import pathlib
import sys

stage_dir = pathlib.Path(sys.argv[1])
manifest_path = pathlib.Path(sys.argv[2])
bundle_name = sys.argv[3]
release_id = sys.argv[4]
project_version = sys.argv[5]
framework_version = sys.argv[6]
source_remote_dir = sys.argv[7]

tracked = [
    "bin/OBC",
    "bin/payload_camera_backend_helper",
    "bin/csp_zmqproxy",
    "bin/eps_simulator",
    "bin/adcs_simulator",
    "bin/radio_mock_server",
    "config/security/command-auth.ini",
    "dict/AppTopologyDictionary.json",
    "launch/_common.sh",
    "launch/run_stack.sh",
    "launch/run_obc_comm_csp_stack.sh",
    "systemd/obc-comm-csp-stack.service.template",
    "udev/90-obc-watchdog.rules",
    "meta/version.json",
]

files = {}
for rel_path in tracked:
    full_path = stage_dir / rel_path
    digest = hashlib.sha256()
    with full_path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    files[rel_path] = {
        "size_bytes": full_path.stat().st_size,
        "sha256": digest.hexdigest(),
    }

manifest = {
    "bundle_name": bundle_name,
    "release_id": release_id,
    "project_version": project_version,
    "framework_version": framework_version,
    "created_at_utc": dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat(),
    "source_remote_dir": source_remote_dir,
    "files": files,
}

manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
PY

TARBALL_PATH="${PACKAGE_DIR}/${BUNDLE_NAME}.tar.gz"
COPYFILE_DISABLE=1 tar --format ustar -C "${STAGE_DIR}" -czf "${TARBALL_PATH}" .
cp "${MANIFEST_PATH}" "${PACKAGE_DIR}/manifest.json"
cp "${STAGE_DIR}/meta/version.json" "${PACKAGE_DIR}/version.json"

printf 'Created Raspberry Pi bundle:\n'
printf '  tarball: %s\n' "${TARBALL_PATH}"
printf '  manifest: %s\n' "${PACKAGE_DIR}/manifest.json"
