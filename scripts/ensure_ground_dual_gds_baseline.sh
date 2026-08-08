#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
GROUND_BASELINE_MANAGER="${ROOT_DIR}/scripts/comm_verification/lib/ensure_ground_dual_gds_baseline.py"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

JSON_OUT="${JSON_OUT:-${GROUND_BASELINE_JSON_OUT:-}}"
process_snapshot="${GROUND_BASELINE_PROCESS_SNAPSHOT:-}"
listener_snapshot="${GROUND_BASELINE_LISTENER_SNAPSHOT:-}"
cleanup_process_snapshot=0
cleanup_listener_snapshot=0

if [[ -z "${process_snapshot}" ]]; then
  process_snapshot="$(mktemp "${TMPDIR:-/tmp}/ground-baseline-ps.XXXXXX")"
  cleanup_process_snapshot=1
fi

if [[ -z "${listener_snapshot}" ]]; then
  listener_snapshot="$(mktemp "${TMPDIR:-/tmp}/ground-baseline-lsof.XXXXXX")"
  cleanup_listener_snapshot=1
fi

cleanup_snapshots() {
  if [[ "${cleanup_process_snapshot}" -eq 1 ]]; then
    rm -f "${process_snapshot}"
  fi
  if [[ "${cleanup_listener_snapshot}" -eq 1 ]]; then
    rm -f "${listener_snapshot}"
  fi
}
trap cleanup_snapshots EXIT

if [[ ! -s "${process_snapshot}" ]]; then
  ps -ax -o pid= -o command= >"${process_snapshot}"
fi

if [[ ! -f "${listener_snapshot}" || ! -s "${listener_snapshot}" ]]; then
  lsof -nP -iTCP -sTCP:LISTEN >"${listener_snapshot}" || true
fi

if [[ -n "${JSON_OUT}" ]]; then
  env \
    GROUND_BASELINE_PROCESS_SNAPSHOT="${process_snapshot}" \
    GROUND_BASELINE_LISTENER_SNAPSHOT="${listener_snapshot}" \
    "${PYTHON_BIN}" "${GROUND_BASELINE_MANAGER}" --json-out "${JSON_OUT}"
else
  env \
    GROUND_BASELINE_PROCESS_SNAPSHOT="${process_snapshot}" \
    GROUND_BASELINE_LISTENER_SNAPSHOT="${listener_snapshot}" \
    "${PYTHON_BIN}" "${GROUND_BASELINE_MANAGER}"
fi
