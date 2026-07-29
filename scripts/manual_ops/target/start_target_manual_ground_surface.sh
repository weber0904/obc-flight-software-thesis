#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
GROUND_BASELINE_MANAGER="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
OWNER_ROOT="${MANUAL_TARGET_GROUND_ROOT:-/tmp/manual-dual-gds/target-ground}"
GDS_UI_MODE="${GDS_UI_MODE:-ui}"
AUTO_PORTS="${MANUAL_TARGET_GROUND_AUTO_PORTS:-1}"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

bash "${GROUND_BASELINE_MANAGER}"

mkdir -p "${OWNER_ROOT}"
if [[ -f "${OWNER_ROOT}/status.json" ]]; then
  existing_pid="$("${PYTHON_BIN}" - "${OWNER_ROOT}/status.json" <<'PY'
import json
import pathlib
import sys
payload = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
print(payload.get("ownerPid", ""))
PY
)"
  if [[ -n "${existing_pid}" ]] && kill -0 "${existing_pid}" >/dev/null 2>&1; then
    echo "Target manual ground surface already active with owner pid ${existing_pid}." >&2
    exit 1
  fi
fi
rm -f "${OWNER_ROOT}/status.json" "${OWNER_ROOT}/manifest.json" "${OWNER_ROOT}/owner.pid" "${OWNER_ROOT}/launcher.log"
owner_args=("${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/surface_owner.py" target-ground-owner "--owner-root" "${OWNER_ROOT}" "--gds-ui-mode" "${GDS_UI_MODE}" "--auto-ports" "${AUTO_PORTS}")
owner_pid="$(
  "${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/detached_owner_launcher.py" \
    --log-path "${OWNER_ROOT}/launcher.log" -- "${owner_args[@]}"
)"

deadline=$((SECONDS + 40))
while [[ ${SECONDS} -lt ${deadline} ]]; do
  if [[ -f "${OWNER_ROOT}/status.json" ]]; then
    lifecycle_state="$("${PYTHON_BIN}" - "${OWNER_ROOT}/status.json" <<'PY'
import json
import pathlib
import sys
payload = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
print(payload.get("lifecycleState", ""))
PY
)"
    status_owner_pid="$("${PYTHON_BIN}" - "${OWNER_ROOT}/status.json" <<'PY'
import json
import pathlib
import sys
payload = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
print(payload.get("ownerPid", ""))
PY
)"
    if [[ "${lifecycle_state}" == "running" && "${status_owner_pid}" == "${owner_pid}" ]]; then
      "${PYTHON_BIN}" - "${OWNER_ROOT}/status.json" <<'PY'
import json
import pathlib
import sys
payload = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
surfaces = payload.get("operatorSurfaces", {})
print("Target manual ground surface ready.")
print(f"  owner root : {payload.get('surfaceRoot')}")
print(f"  manifest   : {pathlib.Path(payload.get('surfaceRoot')).resolve() / 'manifest.json'}")
print(f"  owner pid  : {payload.get('ownerPid')}")
for band_name in ("sband", "uhf"):
    surface = surfaces.get(band_name, {})
    gui_url = surface.get("guiUrl")
    if gui_url:
        print(f"  {band_name} gui : {gui_url}")
PY
      exit 0
    fi
    if [[ "${lifecycle_state}" == "startup_failed" ]]; then
      echo "Target manual ground surface failed to start. Check ${OWNER_ROOT}/launcher.log" >&2
      exit 1
    fi
  fi
  sleep 1
done

echo "Timed out waiting for target manual ground surface manifest. Check ${OWNER_ROOT}/launcher.log" >&2
exit 1
