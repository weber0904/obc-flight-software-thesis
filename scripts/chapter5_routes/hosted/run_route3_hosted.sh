#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

exec bash "${ROOT_DIR}/scripts/chapter5_routes/hosted/route3_recovery_chain_pre_reboot.sh"
