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

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/dual-link-orchestration-hosted.XXXXXX")}"
SUMMARY_LOG="${SUMMARY_LOG:-${PROBE_TMP_DIR}/summary.log}"
mkdir -p "${PROBE_TMP_DIR}"

choose_free_port() {
  "${PYTHON_BIN}" - <<'PY'
import socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.bind(("127.0.0.1", 0))
    print(sock.getsockname()[1])
PY
}

run_happy_path() {
  local run_root="${PROBE_TMP_DIR}/happy-path"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  mkdir -p "${run_root}"

  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=3 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import socket
import sys

owner_root = pathlib.Path(sys.argv[1])
manifest_path = owner_root / "manifest.json"
status_path = owner_root / "status.json"
if not manifest_path.exists() or not status_path.exists():
    raise RuntimeError("orchestration owner artifacts are missing")

manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
status = json.loads(status_path.read_text(encoding="utf-8"))

if manifest.get("ownerType") != "thin-lifecycle-owner":
    raise RuntimeError("expected thin lifecycle owner manifest")
if manifest.get("layer1Baseline", {}).get("registryEntry") != "43B":
    raise RuntimeError("expected layer-1 baseline registry entry 43B")
if status.get("lifecyclePhase") != "stopped":
    raise RuntimeError(f"expected final stopped lifecycle, got {status.get('lifecyclePhase')!r}")

phases = [entry["phase"] for entry in status.get("phaseHistory", [])]
expected = ["preflight", "starting", "ready", "stopping", "stopped"]
if phases != expected:
    raise RuntimeError(f"unexpected phase history: {phases!r}")

layer1_manifest = pathlib.Path(status["layer1Baseline"]["manifestPath"])
if not layer1_manifest.exists():
    raise RuntimeError(f"delegated layer-1 manifest missing: {layer1_manifest}")

listener_checks = status.get("cleanup", {}).get("listenerChecks", [])
if not listener_checks:
    raise RuntimeError("cleanup listener checks missing")
for item in listener_checks:
    if not item.get("closed"):
        raise RuntimeError(f"listener should be closed after stop: {item}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(0.2)
        if sock.connect_ex(("127.0.0.1", int(item["port"]))) == 0:
            raise RuntimeError(f"listener still open after stop: {item}")

if manifest.get("sharedRuntimeInteraction", {}).get("managedBy") != "delegated layer-1 combined baseline helper":
    raise RuntimeError("shared runtime interaction wording drifted")

print(f"owner-happy-path=PASS manifest={manifest_path}")
print(f"owner-phase-sequence=PASS phases={'|'.join(phases)}")
print(f"owner-layer1-reference=PASS layer1Manifest={layer1_manifest}")
PY
}

run_failure_path() {
  local run_root="${PROBE_TMP_DIR}/startup-failure"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  local collision_port
  local collision_server="${run_root}/port-collision.py"
  mkdir -p "${run_root}"
  collision_port="$(choose_free_port)"

  cat >"${collision_server}" <<'PY'
import socket
import sys
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", int(sys.argv[1])))
sock.listen(1)
try:
    while True:
        conn, _ = sock.accept()
        conn.close()
finally:
    sock.close()
PY

  "${PYTHON_BIN}" "${collision_server}" "${collision_port}" >"${run_root}/collision.log" 2>&1 &
  local collision_pid="$!"
  trap 'kill "${collision_pid}" 2>/dev/null || true; wait "${collision_pid}" 2>/dev/null || true' RETURN
  sleep 1

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  SBAND_GDS_PORT="${collision_port}" \
  SBAND_GDS_TTS_PORT="$(choose_free_port)" \
  UHF_GDS_PORT="$(choose_free_port)" \
  UHF_GDS_TTS_PORT="$(choose_free_port)" \
  CSP_HUB_SUB_PORT="$(choose_free_port)" \
  CSP_HUB_PUB_PORT="$(choose_free_port)" \
  RADIO_PORT="$(choose_free_port)" \
  SBAND_TCP_PORT="$(choose_free_port)" \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected startup-failure path to fail" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${collision_port}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
collision_port = int(sys.argv[2])
manifest = json.loads((owner_root / "manifest.json").read_text(encoding="utf-8"))
status = json.loads((owner_root / "status.json").read_text(encoding="utf-8"))

if manifest.get("ownerType") != "thin-lifecycle-owner":
    raise RuntimeError("expected thin lifecycle owner manifest")
if status.get("lifecyclePhase") != "startup_failed":
    raise RuntimeError(f"expected startup_failed lifecycle, got {status.get('lifecyclePhase')!r}")

phases = [entry["phase"] for entry in status.get("phaseHistory", [])]
if phases != ["preflight", "startup_failed"]:
    raise RuntimeError(f"unexpected startup failure phase history: {phases!r}")

failure = status.get("failure") or {}
if "already in use" not in failure.get("message", ""):
    raise RuntimeError(f"expected port collision message, got {failure!r}")

listener_checks = status.get("cleanup", {}).get("listenerChecks", [])
if not listener_checks:
    raise RuntimeError("expected cleanup listener checks after startup failure")
if not any(item.get("port") == collision_port for item in listener_checks):
    raise RuntimeError("collision port missing from cleanup summary")

print(f"owner-startup-failure=PASS collidedPort={collision_port}")
print(f"owner-startup-failure-oracle=PASS phases={'|'.join(phases)}")
PY

  kill "${collision_pid}" 2>/dev/null || true
  wait "${collision_pid}" 2>/dev/null || true
  trap - RETURN
}

run_active_owner_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/active-owner-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local first_log="${run_root}/first-owner.log"
  local second_log="${run_root}/second-owner.log"
  mkdir -p "${run_root}"

  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=30 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${first_log}" 2>&1 &
  local first_pid="$!"
  trap 'kill "${first_pid}" 2>/dev/null || true; wait "${first_pid}" 2>/dev/null || true' RETURN

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys
import time

owner_root = pathlib.Path(sys.argv[1])
status_path = owner_root / "status.json"
deadline = time.time() + 30
while time.time() < deadline:
    if status_path.exists():
        status = json.loads(status_path.read_text(encoding="utf-8"))
        if status.get("lifecyclePhase") == "ready":
            raise SystemExit(0)
    time.sleep(0.2)
raise RuntimeError("owner never reached ready before rerun conflict check")
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${second_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected same-root rerun to fail while first owner is active" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${second_log}" <<'PY'
from __future__ import annotations

from collections import Counter
import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
second_log = pathlib.Path(sys.argv[2])
manifest_path = owner_root / "manifest.json"
status_path = owner_root / "status.json"
if not manifest_path.exists() or not status_path.exists():
    raise RuntimeError("active owner artifacts were removed by same-root rerun")
status = json.loads(status_path.read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "ready":
    raise RuntimeError(f"expected live owner to remain ready during rerun rejection, got {status.get('lifecyclePhase')!r}")
owned_names = Counter(item.get("name") for item in status.get("ownedProcessSet", []))
if owned_names["ground_gds"] < 2 or owned_names["events"] < 2 or owned_names["channels"] < 2:
    raise RuntimeError(f"expected delegated ground processes in ownedProcessSet, got {owned_names}")
log_text = second_log.read_text(encoding="utf-8")
if "already belongs to an active dual-link orchestration owner" not in log_text:
    raise RuntimeError("rerun failure log did not report active-owner conflict")
print(f"owner-active-conflict=PASS ownerRoot={owner_root}")
print(
    "owner-owned-process-inventory=PASS "
    f"ground_gds={owned_names['ground_gds']} "
    f"events={owned_names['events']} "
    f"channels={owned_names['channels']}"
)
PY

  kill "${first_pid}" 2>/dev/null || true
  wait "${first_pid}" 2>/dev/null || true
  trap - RETURN
}

run_startup_claim_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/startup-claim-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local first_log="${run_root}/first-owner.log"
  local second_log="${run_root}/second-owner.log"
  local claim_path="${run_root}/.owner.startup-claim.json"
  mkdir -p "${run_root}"

  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  DUAL_LINK_ORCHESTRATION_STARTUP_CLAIM_DELAY_SECS=5 \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${first_log}" 2>&1 &
  local first_pid="$!"
  trap 'kill "${first_pid}" 2>/dev/null || true; wait "${first_pid}" 2>/dev/null || true' RETURN

  "${PYTHON_BIN}" - "${claim_path}" <<'PY'
from __future__ import annotations

import pathlib
import sys
import time

claim_path = pathlib.Path(sys.argv[1])
deadline = time.time() + 10
while time.time() < deadline:
    if claim_path.exists():
        raise SystemExit(0)
    time.sleep(0.1)
raise RuntimeError(f"startup claim never appeared: {claim_path}")
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${second_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected fresh same-root startup claim conflict to fail" >&2
    return 1
  fi

  wait "${first_pid}"
  trap - RETURN

  "${PYTHON_BIN}" - "${owner_root}" "${claim_path}" "${second_log}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
claim_path = pathlib.Path(sys.argv[2])
second_log = pathlib.Path(sys.argv[3])
manifest_path = owner_root / "manifest.json"
status_path = owner_root / "status.json"
if not manifest_path.exists() or not status_path.exists():
    raise RuntimeError("first owner artifacts are missing after startup-claim conflict")
status = json.loads(status_path.read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "stopped":
    raise RuntimeError(f"expected first owner to finish cleanly, got {status.get('lifecyclePhase')!r}")
if claim_path.exists():
    raise RuntimeError(f"startup claim should be released after owner exit: {claim_path}")
log_text = second_log.read_text(encoding="utf-8")
if "active startup claim" not in log_text:
    raise RuntimeError("second launch did not report startup-claim conflict")
print(f"owner-startup-claim-conflict=PASS ownerRoot={owner_root}")
PY
}

run_stale_nonterminal_owner_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/stale-nonterminal-owner-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  mkdir -p "${owner_root}" "${runtime_root}"

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
status_path = owner_root / "status.json"
manifest_path = owner_root / "manifest.json"

manifest_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "ownerRoot": str(owner_root),
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
status_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "lifecyclePhase": "starting",
            "orchestratorPid": 999999,
            "ownedProcessSet": [],
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected stale non-terminal same-root rerun to fail" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${launcher_log}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
launcher_log = pathlib.Path(sys.argv[2])
status = json.loads((owner_root / "status.json").read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "starting":
    raise RuntimeError(f"expected stale status to remain untouched, got {status.get('lifecyclePhase')!r}")
log_text = launcher_log.read_text(encoding="utf-8")
if "stale non-terminal dual-link orchestration status" not in log_text:
    raise RuntimeError("launcher did not report stale non-terminal owner rejection")
print(f"owner-stale-nonterminal-conflict=PASS ownerRoot={owner_root}")
PY
}

run_stale_cleanup_failed_owner_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/stale-cleanup-failed-owner-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  mkdir -p "${owner_root}" "${runtime_root}"

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
status_path = owner_root / "status.json"
manifest_path = owner_root / "manifest.json"

manifest_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "ownerRoot": str(owner_root),
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
status_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "lifecyclePhase": "cleanup_failed",
            "orchestratorPid": 999999,
            "ownedProcessSet": [],
            "cleanup": {
                "listenerChecks": [
                    {
                        "label": "sbandGdsPort",
                        "host": "127.0.0.1",
                        "port": 59999,
                        "closed": False,
                    }
                ]
            },
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected stale cleanup_failed same-root rerun to fail" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${launcher_log}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
launcher_log = pathlib.Path(sys.argv[2])
status = json.loads((owner_root / "status.json").read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "cleanup_failed":
    raise RuntimeError(f"expected cleanup_failed status to remain untouched, got {status.get('lifecyclePhase')!r}")
listener_checks = (status.get("cleanup") or {}).get("listenerChecks", [])
if not listener_checks or listener_checks[0].get("closed") is not False:
    raise RuntimeError(f"expected stale open-listener cleanup oracle, got {listener_checks!r}")
log_text = launcher_log.read_text(encoding="utf-8")
if "stale terminal dual-link orchestration status (cleanup_failed) with an open listener" not in log_text:
    raise RuntimeError("launcher did not report stale cleanup_failed owner rejection")
print(f"owner-stale-cleanup-failed-conflict=PASS ownerRoot={owner_root}")
PY
}

run_stale_startup_failed_owner_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/stale-startup-failed-owner-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  mkdir -p "${owner_root}" "${runtime_root}"

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
status_path = owner_root / "status.json"
manifest_path = owner_root / "manifest.json"

manifest_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "ownerRoot": str(owner_root),
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
status_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "lifecyclePhase": "startup_failed",
            "orchestratorPid": 999999,
            "ownedProcessSet": [],
            "cleanup": {
                "listenerChecks": [
                    {
                        "label": "uhfGdsPort",
                        "host": "127.0.0.1",
                        "port": 59998,
                        "closed": False,
                    }
                ]
            },
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected stale startup_failed same-root rerun to fail" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${launcher_log}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
launcher_log = pathlib.Path(sys.argv[2])
status = json.loads((owner_root / "status.json").read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "startup_failed":
    raise RuntimeError(f"expected startup_failed status to remain untouched, got {status.get('lifecyclePhase')!r}")
listener_checks = (status.get("cleanup") or {}).get("listenerChecks", [])
if not listener_checks or listener_checks[0].get("closed") is not False:
    raise RuntimeError(f"expected stale open-listener startup_failed oracle, got {listener_checks!r}")
log_text = launcher_log.read_text(encoding="utf-8")
if "stale terminal dual-link orchestration status (startup_failed) with an open listener" not in log_text:
    raise RuntimeError("launcher did not report stale startup_failed owner rejection")
print(f"owner-stale-startup-failed-conflict=PASS ownerRoot={owner_root}")
PY
}

run_stale_stopped_owner_conflict_path() {
  local run_root="${PROBE_TMP_DIR}/stale-stopped-owner-conflict"
  local owner_root="${run_root}/owner"
  local runtime_root="${run_root}/runtime"
  local launcher_log="${run_root}/launcher.log"
  mkdir -p "${owner_root}" "${runtime_root}"

  "${PYTHON_BIN}" - "${owner_root}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
status_path = owner_root / "status.json"
manifest_path = owner_root / "manifest.json"

manifest_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "ownerRoot": str(owner_root),
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
status_path.write_text(
    json.dumps(
        {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "lifecyclePhase": "stopped",
            "orchestratorPid": 999999,
            "ownedProcessSet": [],
            "cleanup": {
                "listenerChecks": [
                    {
                        "label": "cspHubPubPort",
                        "host": "0.0.0.0",
                        "port": 59997,
                        "closed": False,
                    }
                ]
            },
        },
        indent=2,
        sort_keys=True,
    )
    + "\n",
    encoding="utf-8",
)
PY

  set +e
  DUAL_LINK_ORCHESTRATION_ROOT="${owner_root}" \
  DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT="${runtime_root}" \
  PER_BAND_AUTO_PORTS=1 \
  ORCHESTRATION_HOLD_SECS=1 \
  bash "${ROOT_DIR}/scripts/run_hosted_dual_link_orchestration.sh" >"${launcher_log}" 2>&1
  local rc=$?
  set -e
  if [[ "${rc}" -eq 0 ]]; then
    echo "expected stale stopped same-root rerun to fail" >&2
    return 1
  fi

  "${PYTHON_BIN}" - "${owner_root}" "${launcher_log}" <<'PY'
from __future__ import annotations

import json
import pathlib
import sys

owner_root = pathlib.Path(sys.argv[1])
launcher_log = pathlib.Path(sys.argv[2])
status = json.loads((owner_root / "status.json").read_text(encoding="utf-8"))
if status.get("lifecyclePhase") != "stopped":
    raise RuntimeError(f"expected stopped status to remain untouched, got {status.get('lifecyclePhase')!r}")
listener_checks = (status.get("cleanup") or {}).get("listenerChecks", [])
if not listener_checks or listener_checks[0].get("closed") is not False:
    raise RuntimeError(f"expected stale open-listener stopped oracle, got {listener_checks!r}")
log_text = launcher_log.read_text(encoding="utf-8")
if "stale terminal dual-link orchestration status (stopped) with an open listener" not in log_text:
    raise RuntimeError("launcher did not report stale stopped owner rejection")
print(f"owner-stale-stopped-conflict=PASS ownerRoot={owner_root}")
PY
}

{
  run_happy_path
  run_failure_path
  run_active_owner_conflict_path
  run_startup_claim_conflict_path
  run_stale_nonterminal_owner_conflict_path
  run_stale_cleanup_failed_owner_conflict_path
  run_stale_startup_failed_owner_conflict_path
  run_stale_stopped_owner_conflict_path
  echo "dual-link-orchestration-hosted-probe: PASS"
  echo "formal-verdict=dual-link-orchestration-hosted-owner"
  echo "oracle=orchestration-owned manifest/status/cleanup only"
  echo "reused-proof=43B maintained per-band baseline remains separate"
  echo "non-claim=no command authority ownership"
  echo "non-claim=no gateway multiplexer behavior"
  echo "non-claim=no one-GDS heterogeneous multi-upstream behavior"
  echo "non-claim=no target-bearing simultaneous dual-link proof"
  echo "non-claim=no RF closure"
} | tee "${SUMMARY_LOG}"
