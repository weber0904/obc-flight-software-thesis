#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
TARGET_PROBE="${ROOT_DIR}/scripts/comm_verification/lib/run_node5_comm_csp_downlink_v3_target_adaptive_search.py"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

probe_root="${PROBE_ROOT:-$(mktemp -d "${TMPDIR:-/tmp}/node5-comm-csp-downlink-v3-target-search.XXXXXX")}"
summary_log="${probe_root}/summary.log"

mkdir -p "${probe_root}"

log_line() {
  local line="${1}"
  printf '%s\n' "${line}"
  printf '%s\n' "${line}" >>"${summary_log}"
}

: >"${summary_log}"
log_line "node5-comm-csp-downlink-v3-target-adaptive-search: START"
log_line "probe-root=${probe_root}"
probe_output="$(mktemp "${TMPDIR:-/tmp}/node5-comm-csp-downlink-v3-target-search-output.XXXXXX")"
cleanup_probe_output() {
  rm -f "${probe_output}"
}
trap cleanup_probe_output EXIT
if ! "${PYTHON_BIN}" "${TARGET_PROBE}" --probe-root "${probe_root}" >"${probe_output}" 2>&1; then
  cat "${probe_output}" | while IFS= read -r line; do
    log_line "${line}"
  done
  exit 1
fi
cat "${probe_output}" | while IFS= read -r line; do
  log_line "${line}"
done
log_line "node5-comm-csp-downlink-v3-target-adaptive-search: PASS"
