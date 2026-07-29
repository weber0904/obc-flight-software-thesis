#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
OWNER_ROOT="${MANUAL_HOSTED_SURFACE_ROOT:-/tmp/manual-dual-gds/hosted}"
STATUS_JSON="${OWNER_ROOT}/status.json"
MANIFEST_JSON="${OWNER_ROOT}/manifest.json"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  PYTHON_BIN="python3"
fi

if [[ ! -f "${STATUS_JSON}" && ! -f "${MANIFEST_JSON}" ]]; then
  echo "No hosted manual surface metadata found under ${OWNER_ROOT}."
  exit 0
fi

"${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/surface_owner.py" hosted-stop "--owner-root" "${OWNER_ROOT}"
