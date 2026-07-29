#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
RPI_TARGET_USER="${RPI_TARGET_USER:-operator}"
SERVICE_NAME="${SERVICE_NAME:-obc-installed-stack.service}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_INSTALL_ROOT}/runtime/integ-rpi}"
START_NOW="${START_NOW:-1}"
UNIT_TEMPLATE="${ROOT_DIR}/packaging/rpi/systemd/obc-installed-stack.service.template"

obc_require_systemd_service_name "${SERVICE_NAME}"

if [[ ! -f "${UNIT_TEMPLATE}" ]]; then
  echo "Service template not found at ${UNIT_TEMPLATE}" >&2
  exit 1
fi

RENDERED_UNIT="$(mktemp "${ROOT_DIR}/build-artifacts/rpi-autostart-unit.XXXXXX")"
cleanup() {
  rm -f "${RENDERED_UNIT}"
}
trap cleanup EXIT

python3 - "${UNIT_TEMPLATE}" "${RENDERED_UNIT}" "${RPI_TARGET_USER}" "${RPI_INSTALL_ROOT}" "${RUNTIME_ROOT}" <<'PY'
import pathlib
import sys

template_path = pathlib.Path(sys.argv[1])
output_path = pathlib.Path(sys.argv[2])
target_user = sys.argv[3]
install_root = sys.argv[4]
runtime_root = sys.argv[5]

content = template_path.read_text(encoding="utf-8")
content = content.replace("__TARGET_USER__", target_user)
content = content.replace("__INSTALL_ROOT__", install_root)
content = content.replace("__RUNTIME_ROOT__", runtime_root)
output_path.write_text(content, encoding="utf-8")
PY

SERVICE_UNIT_PATH="/etc/systemd/system/${SERVICE_NAME}"

ssh "${OBC_SSH_TARGET}" "set -euo pipefail
test -x $(printf '%q' "${RPI_INSTALL_ROOT}/current/launch/run_stack.sh")
sudo install -o root -g root -m 0644 /dev/stdin $(printf '%q' "${SERVICE_UNIT_PATH}")" < "${RENDERED_UNIT}"

REMOTE_CMD="set -euo pipefail"
REMOTE_CMD+=" && sudo systemctl daemon-reload"
REMOTE_CMD+=" && sudo systemctl enable $(printf '%q' "${SERVICE_NAME}")"
if [[ "${START_NOW}" == "1" ]]; then
  REMOTE_CMD+=" && sudo systemctl restart $(printf '%q' "${SERVICE_NAME}")"
fi
REMOTE_CMD+=" && sudo systemctl --no-pager --full status $(printf '%q' "${SERVICE_NAME}")"

ssh "${OBC_SSH_TARGET}" "${REMOTE_CMD}"
