#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PAYLOAD_DUAL_ARTIFACT_ACTIVE_ENTRYPOINT=1 exec bash "${ROOT_DIR}/scripts/run_payload_e2e_downlink_closure_v1_target_probe.sh" "$@"
