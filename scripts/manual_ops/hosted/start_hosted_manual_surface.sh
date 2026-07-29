#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
OWNER_ROOT="${MANUAL_HOSTED_SURFACE_ROOT:-/tmp/manual-dual-gds/hosted}"
RUNTIME_ROOT="${MANUAL_HOSTED_RUNTIME_ROOT:-}"
GDS_UI_MODE="${GDS_UI_MODE:-ui}"
AUTO_PORTS="${MANUAL_HOSTED_AUTO_PORTS:-1}"
HOSTED_UPLINK_POLL_TIMEOUT_MS="${MANUAL_HOSTED_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS:-${COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS:-1000}}"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

mkdir -p "${OWNER_ROOT}"
if [[ -f "${OWNER_ROOT}/status.json" || -f "${OWNER_ROOT}/manifest.json" ]]; then
  state_json="${OWNER_ROOT}/status.json"
  if [[ ! -f "${state_json}" ]]; then
    state_json="${OWNER_ROOT}/manifest.json"
  fi
  existing_pid="$("${PYTHON_BIN}" - "${state_json}" <<'PY'
import json
import pathlib
import sys
payload = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
print(payload.get("ownerPid", ""))
PY
)"
  if [[ -n "${existing_pid}" ]] && kill -0 "${existing_pid}" >/dev/null 2>&1; then
    echo "Hosted manual surface already active with owner pid ${existing_pid}." >&2
    exit 1
  fi
  "${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/surface_owner.py" hosted-stop "--owner-root" "${OWNER_ROOT}" >/dev/null
fi
rm -f "${OWNER_ROOT}/status.json" "${OWNER_ROOT}/manifest.json" "${OWNER_ROOT}/owner.pid" "${OWNER_ROOT}/launcher.log"

owner_args=("${PYTHON_BIN}" "${ROOT_DIR}/scripts/manual_ops/lib/surface_owner.py" hosted-owner "--owner-root" "${OWNER_ROOT}" "--gds-ui-mode" "${GDS_UI_MODE}" "--auto-ports" "${AUTO_PORTS}")
if [[ -n "${RUNTIME_ROOT}" ]]; then
  owner_args+=("--runtime-root" "${RUNTIME_ROOT}")
fi

owner_pid="$(
  COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS="${HOSTED_UPLINK_POLL_TIMEOUT_MS}" \
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
      echo "Hosted manual surface ready."
      echo "  owner root : ${OWNER_ROOT}"
      echo "  manifest   : ${OWNER_ROOT}/manifest.json"
      echo "  owner pid  : ${owner_pid}"
      exit 0
    fi
    if [[ "${lifecycle_state}" == "startup_failed" ]]; then
      echo "Hosted manual surface failed to start. Check ${OWNER_ROOT}/launcher.log" >&2
      exit 1
    fi
  fi
  sleep 1
done

echo "Timed out waiting for hosted manual surface manifest. Check ${OWNER_ROOT}/launcher.log" >&2
exit 1
