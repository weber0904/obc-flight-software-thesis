#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
LEGACY_SERVICE_NAME="${LEGACY_SERVICE_NAME:-obc-installed-stack.service}"
JOURNAL_LINES="${JOURNAL_LINES:-100}"

obc_require_systemd_service_name "${OBC_COMM_CSP_SERVICE_NAME}"
obc_require_systemd_service_name "${LEGACY_SERVICE_NAME}"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
printf '=== %s ===\n' $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}")
sudo systemctl --no-pager --full status $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}") || true
printf '\n=== is-enabled ===\n'
sudo systemctl is-enabled $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}") || true
printf '\n=== is-active ===\n'
sudo systemctl is-active $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}") || true
printf '\n=== legacy service ===\n'
sudo systemctl is-enabled $(printf '%q' "${LEGACY_SERVICE_NAME}") || true
sudo systemctl is-active $(printf '%q' "${LEGACY_SERVICE_NAME}") || true
printf '\n=== recent journal ===\n'
sudo journalctl -u $(printf '%q' "${OBC_COMM_CSP_SERVICE_NAME}") -n $(printf '%q' "${JOURNAL_LINES}") --no-pager"
