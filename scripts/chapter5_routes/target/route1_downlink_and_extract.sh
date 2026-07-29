#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"
source "${ROOT_DIR}/scripts/chapter5_routes/lib/common.sh"

chapter5_export_target_env_defaults
export PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE="${PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE:-preview-only}"

eps_soc_response="$(
  ssh "${SUBSYSTEM_SIM_SSH_TARGET}" python3 - "${EPS_SIM_CONTROL_SOCKET}" 80.0 0.0 <<'PY'
import socket
import sys

socket_path = sys.argv[1]
value = float(sys.argv[2])
transition_sec = float(sys.argv[3])
command = f"set-soc {value:.2f} {transition_sec:.2f}\n".encode("utf-8")
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(socket_path)
sock.sendall(command)
response = sock.recv(4096).decode("utf-8", errors="replace").strip()
sock.close()
print(response)
if not response.startswith("OK "):
    raise SystemExit(2)
PY
)"
printf 'route1-downlink-precondition=%s\n' "${eps_soc_response}"

exec bash "${ROOT_DIR}/scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh"
