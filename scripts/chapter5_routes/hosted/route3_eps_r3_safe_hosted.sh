#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

export RECOVERY_EXECUTOR_SCOPE="eps-only"

exec bash "${ROOT_DIR}/scripts/run_recovery_executors_v1_probe.sh"
