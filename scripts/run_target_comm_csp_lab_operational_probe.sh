#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

RUN_NODE5_PROOF="${RUN_NODE5_PROOF:-1}"
RUN_UHF_PRIMARY_PROOF="${RUN_UHF_PRIMARY_PROOF:-1}"
RUN_UHF_BACKUP_PROOF="${RUN_UHF_BACKUP_PROOF:-1}"

if [[ "${RUN_NODE5_PROOF}" != "0" && "${RUN_NODE5_PROOF}" != "1" ]]; then
  echo "RUN_NODE5_PROOF must be 0 or 1." >&2
  exit 1
fi
if [[ "${RUN_UHF_PRIMARY_PROOF}" != "0" && "${RUN_UHF_PRIMARY_PROOF}" != "1" ]]; then
  echo "RUN_UHF_PRIMARY_PROOF must be 0 or 1." >&2
  exit 1
fi
if [[ "${RUN_UHF_BACKUP_PROOF}" != "0" && "${RUN_UHF_BACKUP_PROOF}" != "1" ]]; then
  echo "RUN_UHF_BACKUP_PROOF must be 0 or 1." >&2
  exit 1
fi

run_profile_probe() {
  local profile="${1:?profile is required}"
  printf '\n=== target COMM profile proof: %s ===\n' "${profile}"
  TARGET_COMM_PROFILE="${profile}" \
    bash "${ROOT_DIR}/scripts/run_rpi_target_comm_profile_probe.sh"
}

if [[ "${RUN_NODE5_PROOF}" == "1" ]]; then
  run_profile_probe sband
fi
if [[ "${RUN_UHF_PRIMARY_PROOF}" == "1" ]]; then
  run_profile_probe uhf-primary
fi
if [[ "${RUN_UHF_BACKUP_PROOF}" == "1" ]]; then
  run_profile_probe uhf-backup
fi

printf '\nTarget COMM CSP lab operational baseline PASS\n'
