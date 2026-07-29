#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

export PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE="${PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE:-preview-only}"

exec bash "${ROOT_DIR}/scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh"
