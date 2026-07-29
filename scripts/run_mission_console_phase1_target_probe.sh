#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
APP_SCRIPT="${ROOT_DIR}/scripts/mission_console/app.py"
PROBE_CLIENT="${ROOT_DIR}/scripts/mission_console/probe_client.py"
MANUAL_SECURE_OPS="${ROOT_DIR}/scripts/manual_ops/manual_secure_ops.py"
ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
START_TARGET_BASELINE="${ROOT_DIR}/scripts/manual_ops/target/start_target_manual_baseline.sh"
STOP_TARGET_BASELINE="${ROOT_DIR}/scripts/manual_ops/target/stop_target_manual_baseline.sh"
START_TARGET_GROUND="${ROOT_DIR}/scripts/manual_ops/target/start_target_manual_ground_surface.sh"
STOP_TARGET_GROUND="${ROOT_DIR}/scripts/manual_ops/target/stop_target_manual_ground_surface.sh"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "${TMPDIR:-/tmp}/mission-console-target.XXXXXX")}"
TARGET_BASELINE_ROOT="${PROBE_ROOT}/target-baseline"
TARGET_GROUND_ROOT="${PROBE_ROOT}/target-ground"
MISSION_CONSOLE_ROOT="${PROBE_ROOT}/mission-console-runtime"
MISSION_CONSOLE_PORT="${MISSION_CONSOLE_PORT:-5087}"
APP_LOG="${PROBE_ROOT}/mission-console.log"
SUMMARY_LOG="${PROBE_ROOT}/summary.log"
TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE:-1}"
OBC_SSH_TARGET="${OBC_SSH_TARGET:-operator@obc.local}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
TARGET_SBAND_TCP_REACHABILITY_TIMEOUT_SEC="${TARGET_SBAND_TCP_REACHABILITY_TIMEOUT_SEC:-15}"
TARGET_SBAND_GROUND_READY_TIMEOUT_SEC="${TARGET_SBAND_GROUND_READY_TIMEOUT_SEC:-20}"
cleanup_finalized=0

mkdir -p "${PROBE_ROOT}"
: >"${SUMMARY_LOG}"

log_line() {
  local line="${1}"
  printf '%s\n' "${line}"
  printf '%s\n' "${line}" >>"${SUMMARY_LOG}"
}

clear_local_auth_state() {
  local manifest_path="${TARGET_GROUND_ROOT}/manifest.json"
  if [[ ! -f "${manifest_path}" ]]; then
    return 0
  fi
  for band in sband uhf-backup uhf-primary-after-failover; do
    "${PYTHON_BIN}" "${MANUAL_SECURE_OPS}" \
      --env target \
      --band "${band}" \
      --manifest "${manifest_path}" \
      auth clear >>"${SUMMARY_LOG}" 2>&1 || true
  done
}

target_journal_now() {
  PYTHONPATH="${ROOT_DIR}/scripts${PYTHONPATH:+:${PYTHONPATH}}" \
  OBC_SSH_TARGET="${OBC_SSH_TARGET}" "${PYTHON_BIN}" - <<'PY'
from manual_ops.lib.common import remote_journal_since_now
import os
print(remote_journal_since_now(os.environ["OBC_SSH_TARGET"]))
PY
}

wait_for_target_sband_tcp_reachability() {
  TARGET_GROUND_MANIFEST="${TARGET_GROUND_ROOT}/manifest.json" \
  TARGET_SBAND_TCP_REACHABILITY_TIMEOUT_SEC="${TARGET_SBAND_TCP_REACHABILITY_TIMEOUT_SEC}" \
  "${PYTHON_BIN}" - <<'PY'
import json
import os
import pathlib
import socket
import sys
import time

manifest = json.loads(pathlib.Path(os.environ["TARGET_GROUND_MANIFEST"]).read_text(encoding="utf-8"))
endpoint = manifest["operatorSurfaces"]["sband"]["southbound"]["endpoint"]
host, port_text = endpoint.rsplit(":", 1)
port = int(port_text)
deadline = time.time() + float(os.environ["TARGET_SBAND_TCP_REACHABILITY_TIMEOUT_SEC"])
last_error = "unreached"
while time.time() < deadline:
    try:
        with socket.create_connection((host, port), timeout=2.0):
            print(f"target-sband-tcp-reachability=PASS host={host} port={port}")
            sys.exit(0)
    except OSError as exc:
        last_error = str(exc)
        time.sleep(0.5)
raise SystemExit(
    f"target-sband-tcp-reachability=FAIL host={host} port={port} error={last_error}"
)
PY
}

wait_for_target_sband_ground_readiness() {
  TARGET_GROUND_MANIFEST="${TARGET_GROUND_ROOT}/manifest.json" \
  TARGET_SBAND_GROUND_READY_TIMEOUT_SEC="${TARGET_SBAND_GROUND_READY_TIMEOUT_SEC}" \
  "${PYTHON_BIN}" - <<'PY'
import json
import os
import pathlib
import sys
import time

manifest = json.loads(pathlib.Path(os.environ["TARGET_GROUND_MANIFEST"]).read_text(encoding="utf-8"))
sband = manifest["operatorSurfaces"]["sband"]
gateway_log = pathlib.Path(sband["logs"]["gateway"])
downlink_capture = pathlib.Path(sband["captures"]["southboundToGds"])
deadline = time.time() + float(os.environ["TARGET_SBAND_GROUND_READY_TIMEOUT_SEC"])
gateway_opened = False
gateway_opened_since = None
while time.time() < deadline:
    if gateway_log.exists():
        gateway_text = gateway_log.read_text(encoding="utf-8", errors="replace")
        if "southbound-opened mode=tcp-client" in gateway_text:
            gateway_opened = True
            if gateway_opened_since is None:
                gateway_opened_since = time.time()
    if downlink_capture.exists() and downlink_capture.stat().st_size > 0:
        print("target-sband-ground-readiness=PASS readiness=downlink-bytes")
        sys.exit(0)
    if gateway_opened_since is not None and (time.time() - gateway_opened_since) >= 2.0:
        print("target-sband-ground-readiness=PASS readiness=southbound-opened-stable")
        sys.exit(0)
    time.sleep(0.25)
raise SystemExit(
    "target-sband-ground-readiness=FAIL reason=no-downlink-bytes-or-stable-southbound-open "
    f"gateway_opened={gateway_opened}"
)
PY
}

wait_for_target_sband_ground_link_up() {
  local since="${1}"
  PYTHONPATH="${ROOT_DIR}/scripts${PYTHONPATH:+:${PYTHONPATH}}" \
  OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
  OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME}" \
  TARGET_SBAND_GROUND_READY_TIMEOUT_SEC="${TARGET_SBAND_GROUND_READY_TIMEOUT_SEC}" \
  TARGET_SBAND_GROUND_LINK_SINCE="${since}" \
  "${PYTHON_BIN}" - <<'PY'
from manual_ops.lib.common import wait_remote_journal_fragments
import os

wait_remote_journal_fragments(
    os.environ["OBC_SSH_TARGET"],
    os.environ["OBC_COMM_CSP_SERVICE_NAME"],
    ("groundLinkDriver) GROUND_LINK_UP",),
    float(os.environ["TARGET_SBAND_GROUND_READY_TIMEOUT_SEC"]),
    since=os.environ["TARGET_SBAND_GROUND_LINK_SINCE"],
)
print("target-sband-ground-link-up=PASS")
PY
}

perform_cleanup() {
  local strict="${1}"
  if [[ -n "${app_pid:-}" ]]; then
    kill "${app_pid}" >/dev/null 2>&1 || true
    wait "${app_pid}" >/dev/null 2>&1 || true
  fi
  if [[ "${strict}" == "strict" ]]; then
    clear_local_auth_state
    MANUAL_TARGET_GROUND_ROOT="${TARGET_GROUND_ROOT}" bash "${STOP_TARGET_GROUND}" >/dev/null 2>&1
  else
    clear_local_auth_state
    MANUAL_TARGET_GROUND_ROOT="${TARGET_GROUND_ROOT}" bash "${STOP_TARGET_GROUND}" >/dev/null 2>&1 || true
  fi
  if [[ "${strict}" == "strict" ]]; then
    TARGET_BASELINE_MANAGED_EXTERNALLY=1 \
      MANUAL_TARGET_BASELINE_ROOT="${TARGET_BASELINE_ROOT}" \
      bash "${STOP_TARGET_BASELINE}" >/dev/null 2>&1
    JSON_OUT="${PROBE_ROOT}/target-after.json" \
      TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1 \
      TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1 \
      TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
      bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
    log_line "target-baseline-after=PASS source=ensure-target-comm-lab-baseline"
    JSON_OUT="${PROBE_ROOT}/ground-after.json" \
      bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
  else
    TARGET_BASELINE_MANAGED_EXTERNALLY=1 \
      MANUAL_TARGET_BASELINE_ROOT="${TARGET_BASELINE_ROOT}" \
      bash "${STOP_TARGET_BASELINE}" >/dev/null 2>&1 || true
    if JSON_OUT="${PROBE_ROOT}/target-after.json" \
      TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1 \
      TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1 \
      TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
      bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1; then
      log_line "target-baseline-after=PASS source=ensure-target-comm-lab-baseline"
    fi
    bash "${ENSURE_GROUND_BASELINE}" >/dev/null 2>&1 || true
  fi
}

finalize_success_cleanup() {
  perform_cleanup strict
  cleanup_finalized=1
}

cleanup() {
  if [[ "${cleanup_finalized}" == "1" ]]; then
    return 0
  fi
  set +e
  perform_cleanup best-effort
}
trap cleanup EXIT

wait_http() {
  local url="${1}"
  local deadline=$((SECONDS + 40))
  while [[ ${SECONDS} -lt ${deadline} ]]; do
    if curl -fsS "${url}" >/dev/null 2>&1; then
      return 0
    fi
    sleep 1
  done
  return 1
}

log_line "mission-console-target: START"
log_line "probe-root=${PROBE_ROOT}"

JSON_OUT="${PROBE_ROOT}/target-before.json" \
  TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1 \
  TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1 \
  TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
  bash "${ENSURE_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1
JSON_OUT="${PROBE_ROOT}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1

TARGET_BASELINE_MANAGED_EXTERNALLY=1 \
TARGET_BASELINE_READY_JSON="${PROBE_ROOT}/target-before.json" \
TARGET_BASELINE_REQUIRE_UHF_SERVICE="${TARGET_BASELINE_REQUIRE_UHF_SERVICE}" \
MANUAL_TARGET_BASELINE_ROOT="${TARGET_BASELINE_ROOT}" \
bash "${START_TARGET_BASELINE}" >>"${SUMMARY_LOG}" 2>&1

ground_link_attach_since="$(target_journal_now)"
MANUAL_TARGET_GROUND_ROOT="${TARGET_GROUND_ROOT}" \
MANUAL_TARGET_BASELINE_ROOT="${TARGET_BASELINE_ROOT}" \
MANUAL_TARGET_GROUND_AUTO_PORTS=1 \
GDS_UI_MODE=headless \
bash "${START_TARGET_GROUND}" >>"${SUMMARY_LOG}" 2>&1

wait_for_target_sband_ground_link_up "${ground_link_attach_since}" >>"${SUMMARY_LOG}" 2>&1
wait_for_target_sband_tcp_reachability >>"${SUMMARY_LOG}" 2>&1
wait_for_target_sband_ground_readiness >>"${SUMMARY_LOG}" 2>&1

MISSION_CONSOLE_ROOT="${MISSION_CONSOLE_ROOT}" \
MISSION_CONSOLE_TARGET_BASELINE_ROOT="${TARGET_BASELINE_ROOT}" \
MISSION_CONSOLE_TARGET_GROUND_ROOT="${TARGET_GROUND_ROOT}" \
MISSION_CONSOLE_PORT="${MISSION_CONSOLE_PORT}" \
MANUAL_SECURE_AUTH_ESTABLISH_TIMEOUT_SEC=120 \
"${PYTHON_BIN}" "${APP_SCRIPT}" >"${APP_LOG}" 2>&1 &
app_pid=$!

wait_http "http://127.0.0.1:${MISSION_CONSOLE_PORT}/api/contexts"
sleep 5

MISSION_CONSOLE_PORT="${MISSION_CONSOLE_PORT}" "${PYTHON_BIN}" - <<'PY' >>"${SUMMARY_LOG}" 2>&1
import json
import os
import time
from urllib.request import urlopen

url = (
    f"http://127.0.0.1:{os.environ['MISSION_CONSOLE_PORT']}"
    "/api/beacon/latest?contextId=target-manual-ground-dual-gds&band=uhf-backup"
)
deadline = time.monotonic() + 45.0
last = None
while time.monotonic() < deadline:
    try:
        with urlopen(url, timeout=5.0) as response:
            last = json.load(response)
    except Exception as exc:
        last = {"error": str(exc)}
        time.sleep(1.0)
        continue
    if (
        last.get("supported")
        and last.get("available")
        and last.get("sourceKind") == "target-remote-sidecar"
        and last.get("capture", {}).get("frameSize") == 108
        and last.get("decode", {}).get("status") == "ok"
    ):
        print("target-beacon-viewer=PASS source=target-remote-sidecar")
        break
    time.sleep(1.0)
else:
    raise SystemExit(f"target-beacon-viewer=FAIL payload={last}")
PY

"${PYTHON_BIN}" "${PROBE_CLIENT}" target \
  --base-url "http://127.0.0.1:${MISSION_CONSOLE_PORT}" | tee -a "${SUMMARY_LOG}"

finalize_success_cleanup
log_line "mission-console-target: PASS"
