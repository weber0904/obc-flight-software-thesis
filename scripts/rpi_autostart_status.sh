#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
JOURNAL_LINES="${JOURNAL_LINES:-80}"

obc_require_systemd_service_name "${SERVICE_NAME}"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
sudo systemctl --no-pager --full status $(printf '%q' "${SERVICE_NAME}")
printf '\n=== is-enabled ===\n'
sudo systemctl is-enabled $(printf '%q' "${SERVICE_NAME}")
printf '\n=== is-active ===\n'
sudo systemctl is-active $(printf '%q' "${SERVICE_NAME}")
printf '\n=== recent journal ===\n'
sudo journalctl -u $(printf '%q' "${SERVICE_NAME}") -n $(printf '%q' "${JOURNAL_LINES}") --no-pager"
