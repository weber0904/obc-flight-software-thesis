#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
WAIT_TIMEOUT_SEC="${WAIT_TIMEOUT_SEC:-300}"
DISCONNECT_TIMEOUT_SEC="${DISCONNECT_TIMEOUT_SEC:-120}"

obc_require_systemd_service_name "${SERVICE_NAME}"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
sudo systemctl restart $(printf '%q' "${SERVICE_NAME}")
sudo systemctl is-active $(printf '%q' "${SERVICE_NAME}")
sudo reboot" || true

disconnect_deadline=$((SECONDS + DISCONNECT_TIMEOUT_SEC))
while (( SECONDS < disconnect_deadline )); do
  if ! ssh -o BatchMode=yes -o ConnectTimeout=5 "${OBC_SSH_TARGET}" "true" >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

if ssh -o BatchMode=yes -o ConnectTimeout=5 "${OBC_SSH_TARGET}" "true" >/dev/null 2>&1; then
  echo "Timed out waiting for Raspberry Pi to drop off SSH after reboot request." >&2
  exit 1
fi

reconnect_deadline=$((SECONDS + WAIT_TIMEOUT_SEC))
while (( SECONDS < reconnect_deadline )); do
  if ssh -o BatchMode=yes -o ConnectTimeout=5 "${OBC_SSH_TARGET}" "true" >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

if ! ssh -o BatchMode=yes -o ConnectTimeout=5 "${OBC_SSH_TARGET}" "true" >/dev/null 2>&1; then
  echo "Timed out waiting for Raspberry Pi to return after reboot." >&2
  exit 1
fi

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
sudo systemctl is-active $(printf '%q' "${SERVICE_NAME}")
sudo systemctl --no-pager --full status $(printf '%q' "${SERVICE_NAME}")
printf '\n=== recent journal ===\n'
sudo journalctl -u $(printf '%q' "${SERVICE_NAME}") -n 120 --no-pager
printf '\n=== installed processes ===\n'
ps -ef | grep -E 'obc-deploy/(current|releases/.+)/bin/(eps_simulator|adcs_simulator|radio_mock_server|OBC)' | grep -v grep || true"
