#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${PYTHON_BIN}" ]]; then
  echo "Required build outputs or fprime-venv python are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/per-band-stock-ground-stacks-hosted.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"
mkdir -p "${PROBE_TMP_DIR}"

ALT_SIM_PROXY_PID=""
ALT_SIM_EPS_PID=""
ALT_SIM_ADCS_PID=""

cleanup_alt_simulators() {
  local pid
  for pid in "${ALT_SIM_ADCS_PID}" "${ALT_SIM_EPS_PID}" "${ALT_SIM_PROXY_PID}"; do
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
      kill "${pid}" 2>/dev/null || true
      wait "${pid}" 2>/dev/null || true
    fi
  done
}

trap cleanup_alt_simulators EXIT

pid_is_live_non_zombie() {
  local pid="$1"
  local stat
  if ! kill -0 "${pid}" 2>/dev/null; then
    return 1
  fi
  stat="$(/bin/ps -o stat= -p "${pid}" 2>/dev/null | tr -d '[:space:]')"
  [[ -n "${stat}" && "${stat}" != *Z* ]]
}

choose_free_port() {
  "${PYTHON_BIN}" - <<'PY'
import socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.bind(("127.0.0.1", 0))
    print(sock.getsockname()[1])
PY
}

start_alt_simulators() {
  local run_root="${PROBE_TMP_DIR}/alternate-simulator-stack"
  local sub_port
  local pub_port
  mkdir -p "${run_root}"
  sub_port="$(choose_free_port)"
  pub_port="$(choose_free_port)"

  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${sub_port}" \
  CSP_HUB_PUB_PORT="${pub_port}" \
  "${BIN_DIR}/csp_zmqproxy" \
    -s "tcp://0.0.0.0:${sub_port}" \
    -p "tcp://0.0.0.0:${pub_port}" \
    >"${run_root}/csp-zmqproxy.log" 2>&1 &
  ALT_SIM_PROXY_PID="$!"

  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${sub_port}" \
  CSP_HUB_PUB_PORT="${pub_port}" \
  "${BIN_DIR}/eps_simulator" --node-id 2 >"${run_root}/eps.log" 2>&1 &
  ALT_SIM_EPS_PID="$!"

  CSP_TRANSPORT=zmqhub \
  CSP_HUB_HOST=127.0.0.1 \
  CSP_HUB_SUB_PORT="${sub_port}" \
  CSP_HUB_PUB_PORT="${pub_port}" \
  "${BIN_DIR}/adcs_simulator" --node-id 3 >"${run_root}/adcs.log" 2>&1 &
  ALT_SIM_ADCS_PID="$!"

  sleep 1
  for pid in "${ALT_SIM_PROXY_PID}" "${ALT_SIM_EPS_PID}" "${ALT_SIM_ADCS_PID}"; do
    if ! pid_is_live_non_zombie "${pid}"; then
      echo "failed to start alternate simulator-backed hosted stack; logs: ${run_root}" >&2
      exit 1
    fi
  done
}

assert_alt_simulators_alive() {
  local mode="$1"
  local pid
  for pid in "${ALT_SIM_PROXY_PID}" "${ALT_SIM_EPS_PID}" "${ALT_SIM_ADCS_PID}"; do
    if ! pid_is_live_non_zombie "${pid}"; then
      echo "alternate simulator-backed hosted stack died during ${mode} launcher probe" >&2
      echo "logs: ${PROBE_TMP_DIR}/alternate-simulator-stack" >&2
      exit 1
    fi
  done
  echo "unrelated-active-simulator-stack-${mode}=PASS"
}

run_launcher_probe() {
  local mode="$1"
  local run_root="${PROBE_TMP_DIR}/${mode}"
  local stack_root="${run_root}/stack"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  local preexisting_aliases="${run_root}/preexisting-aliases.txt"
  mkdir -p "${run_root}"

  {
    find "${ROOT_DIR}" -maxdepth 1 \( -name '.adm-*' -o -name '.stg-*' -o -name '.sequence-staging' -o -name '.sequence-admitted' \) -print
  } | sort >"${preexisting_aliases}"

  if ! PER_BAND_STACK_ROOT="${stack_root}" \
    PER_BAND_RUNTIME_ROOT="${runtime_root}" \
    PER_BAND_AUTO_PORTS=1 \
    STACK_HOLD_SECS=3 \
    bash "${ROOT_DIR}/scripts/run_hosted_stock_ground_stack.sh" "${mode}" >"${launcher_log}" 2>&1; then
    cat "${launcher_log}" >&2
    echo "per-band-stock-ground-stacks-hosted-probe: STOPPED" >&2
    echo "mode=${mode}" >&2
    echo "logs: ${run_root}" >&2
    exit 1
  fi

  "${PYTHON_BIN}" - "${mode}" "${stack_root}/manifest.json" "${stack_root}" "${runtime_root}" "${ROOT_DIR}" "${preexisting_aliases}" <<'PY'
from __future__ import annotations

import json
import os
import pathlib
import socket
import subprocess
import sys


def check_port_closed(port: int) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.2)
        if sock.connect_ex(("127.0.0.1", port)) == 0:
            raise RuntimeError(f"expected port {port} to be closed after launcher exit")


mode = sys.argv[1]
manifest_path = pathlib.Path(sys.argv[2])
stack_root = pathlib.Path(sys.argv[3])
runtime_root = pathlib.Path(sys.argv[4])
root_dir = pathlib.Path(sys.argv[5])
preexisting_aliases_path = pathlib.Path(sys.argv[6])
if not manifest_path.exists():
    raise RuntimeError(f"manifest not found: {manifest_path}")

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if manifest.get("mode") != mode:
    raise RuntimeError(f"unexpected manifest mode {manifest.get('mode')!r} for {mode}")
if not manifest.get("hostedOnly"):
    raise RuntimeError("launcher manifest must stay hosted-only")

requested_runtime_root = manifest.get("requestedRuntimeRoot")
shared_runtime = manifest.get("sharedHostedRuntime")
requested_runtime_root_path = runtime_root.resolve()
expected_runtime_root = (runtime_root / mode).resolve()
if requested_runtime_root is not None:
    if pathlib.Path(requested_runtime_root).resolve() != requested_runtime_root_path:
        raise RuntimeError(
            f"{mode}: expected requestedRuntimeRoot {requested_runtime_root_path}, got {requested_runtime_root}"
        )
    if pathlib.Path(shared_runtime).resolve() != expected_runtime_root:
        raise RuntimeError(
            f"{mode}: expected owned sharedHostedRuntime {expected_runtime_root}, got {shared_runtime}"
        )

expected_surfaces = {
    "sband": {"sband"},
    "uhf": {"uhf"},
    "combined": {"sband", "uhf"},
}[mode]
surfaces = manifest.get("operatorSurfaces", {})
if set(surfaces.keys()) != expected_surfaces:
    raise RuntimeError(f"{mode}: expected operator surfaces {expected_surfaces}, got {set(surfaces.keys())}")

if mode == "combined":
    sband = surfaces["sband"]
    uhf = surfaces["uhf"]
    if sband["gdsPort"] == uhf["gdsPort"] or sband["gdsTtsPort"] == uhf["gdsTtsPort"]:
        raise RuntimeError("combined: expected distinct GDS/TTS ports")
    if sband["fileStorageDir"] == uhf["fileStorageDir"]:
        raise RuntimeError("combined: expected distinct per-band file stores")
    if sband["southbound"]["endpoint"] == uhf["southbound"]["gatewaySerialDevice"]:
        raise RuntimeError("combined: expected distinct southbound endpoints")

ports = []
for surface in surfaces.values():
    ports.append(int(surface["gdsPort"]))
    ports.append(int(surface["gdsTtsPort"]))
    southbound = surface["southbound"]
    if southbound.get("kind") == "tcp":
        ports.append(int(str(southbound["endpoint"]).rsplit(":", 1)[1]))

internal_runtime = manifest.get("internalOnlyRuntime", {})
for key in ("cspHubSubPort", "cspHubPubPort", "radioMockPort", "sbandCommTcpPort"):
    value = internal_runtime.get(key)
    if value is not None:
        ports.append(int(value))

for port in ports:
    check_port_closed(port)

preexisting_aliases = {
    pathlib.Path(line.strip())
    for line in preexisting_aliases_path.read_text(encoding="utf-8").splitlines()
    if line.strip()
}
for alias in list(root_dir.glob(".adm-*")) + list(root_dir.glob(".stg-*")) + [
    root_dir / ".sequence-staging",
    root_dir / ".sequence-admitted",
]:
    if not (alias.exists() or alias.is_symlink()):
        continue
    if alias in preexisting_aliases:
        continue
    raise RuntimeError(f"repo root alias residue should not be created: {alias}")

command_output = subprocess.run(
    ["ps", "-ax", "-o", "pid=", "-o", "ppid=", "-o", "command="],
    check=True,
    capture_output=True,
    text=True,
).stdout
current_pid = os.getpid()
parent_pid = os.getppid()
for fragment in (str(stack_root), str(runtime_root)):
    for line in command_output.splitlines():
        parts = line.strip().split(None, 2)
        if len(parts) < 3:
            continue
        pid = int(parts[0])
        ppid = int(parts[1])
        command = parts[2]
        if pid in {current_pid, parent_pid} or ppid in {current_pid, parent_pid}:
            continue
        if fragment in command:
            raise RuntimeError(f"found stale owned process fragment after exit: {fragment}")

print(f"launcher-{mode}=PASS manifest={manifest_path}")
if mode == "combined":
    print(
        "combined-distinct-surfaces=PASS "
        f"sbandGds={surfaces['sband']['gdsBind']} "
        f"uhfGds={surfaces['uhf']['gdsBind']} "
        f"runtime={manifest['sharedHostedRuntime']}"
    )
PY

  assert_alt_simulators_alive "${mode}"
}

start_alt_simulators

{
  run_launcher_probe "sband"
  run_launcher_probe "uhf"
  run_launcher_probe "combined"
  echo "per-band-stock-ground-stacks-hosted-probe: PASS"
  echo "bounded-noninterference=unrelated active EPS/ADCS simulator stack survived all launcher reruns"
  echo "formal-verdict=per-band-stock-ground-stacks-hosted-baseline"
  echo "proof-scope=hosted maintained launcher start-stop-manifest review only"
  echo "non-claim=no simultaneous dual-link runtime arbitration"
  echo "non-claim=no one stock GDS heterogeneous multi-upstream surface"
  echo "non-claim=no one gateway simultaneous multiplexer behavior"
  echo "non-claim=no target-bearing simultaneous dual-link proof"
  echo "non-claim=no RF closure"
  echo "reused-proof=sband hosted CCSDS adoption remains separate"
  echo "reused-proof=uhf hosted CCSDS adoption remains separate"
  echo "reused-proof=comm-session-and-downlink-qos remains separate"
  echo "reused-proof=uhf beacon suppression remains separate"
  echo "reused-proof=uhf primary packet quiet remains separate"
  echo "logs: ${PROBE_TMP_DIR}"
} | tee "${SUMMARY_LOG}"
