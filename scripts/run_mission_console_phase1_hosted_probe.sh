#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
APP_SCRIPT="${ROOT_DIR}/scripts/mission_console/app.py"
PROBE_CLIENT="${ROOT_DIR}/scripts/mission_console/probe_client.py"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
START_HOSTED_SURFACE="${ROOT_DIR}/scripts/manual_ops/hosted/start_hosted_manual_surface.sh"
STOP_HOSTED_SURFACE="${ROOT_DIR}/scripts/manual_ops/hosted/stop_hosted_manual_surface.sh"
SAMPLE_SEQUENCE="${ROOT_DIR}/scripts/manual_ops/examples/sample-sequence.bin"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

PROBE_ROOT="${PROBE_ROOT:-$(mktemp -d "${TMPDIR:-/tmp}/mission-console-hosted.XXXXXX")}"
HOSTED_ROOT="${PROBE_ROOT}/hosted-surface"
HOSTED_RUNTIME_ROOT="${PROBE_ROOT}/hosted-runtime"
MISSION_CONSOLE_ROOT="${PROBE_ROOT}/mission-console-runtime"
MISSION_CONSOLE_PORT="${MISSION_CONSOLE_PORT:-5086}"
APP_LOG="${PROBE_ROOT}/mission-console.log"
SUMMARY_LOG="${PROBE_ROOT}/summary.log"

mkdir -p "${PROBE_ROOT}"
: >"${SUMMARY_LOG}"

log_line() {
  local line="${1}"
  printf '%s\n' "${line}"
  printf '%s\n' "${line}" >>"${SUMMARY_LOG}"
}

assert_no_hosted_backpressure_events() {
  local listener_root="${MISSION_CONSOLE_ROOT}/listeners/hosted-manual-dual-gds"
  local fragments=(
    "ComCcsds.comQueue.QueueOverflow"
    "CSP_OWNER_TIMEOUT"
    "GROUND_LINK_DOWN"
  )
  local event_logs=()
  local fragment=""

  while IFS= read -r -d '' path; do
    event_logs+=("${path}")
  done < <(find "${listener_root}" -path '*/native-events/event.log' -type f -print0 2>/dev/null)

  if [[ ${#event_logs[@]} -eq 0 ]]; then
    log_line "mission-console-hosted-backpressure-check=SKIP"
    log_line "mission-console-hosted-backpressure-reason=no-listener-event-logs"
    return 0
  fi

  for fragment in "${fragments[@]}"; do
    if rg -n --fixed-strings "${fragment}" "${event_logs[@]}" >>"${SUMMARY_LOG}"; then
      log_line "mission-console-hosted-backpressure-check=FAIL"
      log_line "mission-console-hosted-backpressure-fragment=${fragment}"
      return 1
    fi
  done

  log_line "mission-console-hosted-backpressure-check=PASS"
}

cleanup() {
  set +e
  if [[ -n "${app_pid:-}" ]]; then
    kill "${app_pid}" >/dev/null 2>&1 || true
    wait "${app_pid}" >/dev/null 2>&1 || true
  fi
  MANUAL_HOSTED_SURFACE_ROOT="${HOSTED_ROOT}" bash "${STOP_HOSTED_SURFACE}" >/dev/null 2>&1 || true
  bash "${ENSURE_GROUND_BASELINE}" >/dev/null 2>&1 || true
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

log_line "mission-console-hosted: START"
log_line "probe-root=${PROBE_ROOT}"

JSON_OUT="${PROBE_ROOT}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${SUMMARY_LOG}" 2>&1

MANUAL_HOSTED_SURFACE_ROOT="${HOSTED_ROOT}" \
MANUAL_HOSTED_RUNTIME_ROOT="${HOSTED_RUNTIME_ROOT}" \
MANUAL_HOSTED_AUTO_PORTS=1 \
GDS_UI_MODE=headless \
bash "${START_HOSTED_SURFACE}" >>"${SUMMARY_LOG}" 2>&1

MISSION_CONSOLE_ROOT="${MISSION_CONSOLE_ROOT}" \
MISSION_CONSOLE_HOSTED_ROOT="${HOSTED_ROOT}" \
MISSION_CONSOLE_PORT="${MISSION_CONSOLE_PORT}" \
"${PYTHON_BIN}" "${APP_SCRIPT}" >"${APP_LOG}" 2>&1 &
app_pid=$!

wait_http "http://127.0.0.1:${MISSION_CONSOLE_PORT}/api/contexts"

"${PYTHON_BIN}" "${PROBE_CLIENT}" hosted \
  --base-url "http://127.0.0.1:${MISSION_CONSOLE_PORT}" \
  --sample-sequence "${SAMPLE_SEQUENCE}" | tee -a "${SUMMARY_LOG}"

assert_no_hosted_backpressure_events

log_line "mission-console-hosted: PASS"
