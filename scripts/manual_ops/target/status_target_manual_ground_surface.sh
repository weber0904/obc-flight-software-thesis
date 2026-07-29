#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
OWNER_ROOT="${MANUAL_TARGET_GROUND_ROOT:-/tmp/manual-dual-gds/target-ground}"
STATUS_JSON="${OWNER_ROOT}/status.json"

if [[ ! -f "${STATUS_JSON}" ]]; then
  echo "No target manual ground surface status found at ${STATUS_JSON}."
  exit 1
fi

"${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/surface_owner.py" render-status "${STATUS_JSON}"
