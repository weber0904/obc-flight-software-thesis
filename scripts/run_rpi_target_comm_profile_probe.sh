#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export PROBE_MODE="command-path"
exec "${ROOT_DIR}/scripts/run_rpi_target_recovery_restart_probe.sh"
