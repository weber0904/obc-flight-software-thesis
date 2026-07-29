#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
PROBE_IMPL="${ROOT_DIR}/scripts/chapter5_routes/target/route1_soc_fallback_check.py"
ENSURE_TARGET_BASELINE="${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
ENSURE_GROUND_BASELINE="${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

probe_root="${PROBE_ROOT:-$(mktemp -d "/private/tmp/chapter5-route1-target-fallback.XXXXXX")}"
summary_log="${SUMMARY_LOG:-${probe_root}/summary.log}"
baseline_dir="${probe_root}/baseline"

mkdir -p "${baseline_dir}"
: >"${summary_log}"

chapter5_export_target_env_defaults
if [[ "${OBC_COMM_CSP_SERVICE_NAME}" != "obc-comm-csp-stack.service" ]]; then
  echo "Route 1 requires OBC_COMM_CSP_SERVICE_NAME=obc-comm-csp-stack.service" >&2
  exit 2
fi
# A owns the Route 1 service profile. Do not let caller-provided manual-auth
# timeout knobs make A install a post-deployment timeout drop-in before C.
unset \
  MANUAL_TARGET_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS \
  MANUAL_TARGET_COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS \
  MANUAL_TARGET_COMM_GROUNDLINK_HEALTH_TIMEOUT_MS \
  MANUAL_TARGET_OBC_GROUNDLINK_TIMEOUTS_DROPIN_NAME \
  MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS_DROPIN_NAME \
  MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS_DROPIN_NAME
export MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS=0
export MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS=1

TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1 \
  JSON_OUT="${baseline_dir}/target-before.json" \
  bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1
JSON_OUT="${baseline_dir}/ground-before.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1

probe_status=0
set +e
# A owns the shared target service profile. Explicitly remove every inherited
# timeout override and pin C's service-profile expectations to the Route 1
# node-5 baseline so this second functional stage cannot install a drop-in or
# restart the OBC service.
env \
  -u COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS \
  -u COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS \
  -u COMM_GROUNDLINK_HEALTH_TIMEOUT_MS \
  -u COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES \
  -u COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE \
  -u COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS \
  -u COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS \
  -u COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC \
  -u COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC \
  TARGET_SERVICE_PROFILE=sband \
  TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE=sband-primary \
  TARGET_SERVICE_INITIAL_COMM_BAND=sband \
  TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER=1 \
  TARGET_BASELINE_MANAGED_EXTERNALLY=1 \
  OBC_GROUNDLINK_DIAGNOSTICS=0 \
  SBAND_COMM_NODE_INGRESS_DIAGNOSTICS=0 \
  "${PYTHON_BIN}" "${PROBE_IMPL}" --probe-root "${probe_root}" | tee -a "${summary_log}"
probe_status=${PIPESTATUS[0]}

set +e
JSON_OUT="${baseline_dir}/target-after.json" bash "${ENSURE_TARGET_BASELINE}" >>"${summary_log}" 2>&1
target_after_status=$?
JSON_OUT="${baseline_dir}/ground-after.json" bash "${ENSURE_GROUND_BASELINE}" >>"${summary_log}" 2>&1
ground_after_status=$?
set -e

if [[ "${probe_status}" -ne 0 ]]; then
  exit "${probe_status}"
fi
if [[ "${target_after_status}" -ne 0 || "${ground_after_status}" -ne 0 ]]; then
  echo "Route 1 target postflight baseline check failed; see ${baseline_dir}." >&2
  exit 1
fi
