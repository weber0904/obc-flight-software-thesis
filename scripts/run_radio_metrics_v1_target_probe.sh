#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/radio-metrics-v1-target.XXXXXX")}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" PROBE_TMP_DIR="${PROBE_TMP_DIR}" python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import re
import subprocess


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
underlying_dir = PROBE_TMP_DIR / "target-node5-command-path"
underlying_dir.mkdir(parents=True, exist_ok=True)

result = subprocess.run(
    ["bash", str(ROOT_DIR / "scripts" / "run_rpi_target_recovery_restart_probe.sh")],
    env={
        **os.environ,
        "TARGET_COMM_PROFILE": "sband",
        "PROBE_MODE": "command-path",
        "PROBE_TMP_DIR": str(underlying_dir),
    },
    capture_output=True,
    text=True,
    check=False,
)
if result.returncode != 0:
    raise RuntimeError(
        "run_rpi_target_recovery_restart_probe.sh failed\n"
        f"stdout={result.stdout}\n"
        f"stderr={result.stderr}"
    )

stdout = result.stdout
required = [
    "rpi target command-path profile probe PASS",
    "target-comm-profile=sband",
    "probe-mode=command-path",
    "command_path=authenticated SESSION_OPEN",
]
missing = [fragment for fragment in required if fragment not in stdout]
if missing:
    raise RuntimeError(f"target wrapper missing required summary fragments {missing}\n{stdout}")

service_log_match = re.search(r"^log=(.+)$", stdout, re.MULTILINE)
command_log_match = re.search(r"^command-log=(.+)$", stdout, re.MULTILINE)
events_log_match = re.search(r"^events-log=(.+)$", stdout, re.MULTILINE)
if not service_log_match or not command_log_match or not events_log_match:
    raise RuntimeError(f"target wrapper could not find underlying log paths\n{stdout}")

service_log = pathlib.Path(service_log_match.group(1).strip())
command_log = pathlib.Path(command_log_match.group(1).strip())
events_log = pathlib.Path(events_log_match.group(1).strip())
for path in (service_log, command_log, events_log):
    if not path.exists():
        raise RuntimeError(f"expected target probe artifact missing: {path}")

service_text = service_log.read_text(encoding="utf-8", errors="replace")
if "=== baseline: OBC service ===" not in service_text:
    raise RuntimeError(f"service log did not contain baseline OBC service capture: {service_log}")

print("radio-metrics-v1-target-probe: PASS")
print("target-profile=sband")
print("target-surface=service-managed-node5-command-path")
print("contract-support=active-node5-command-readback-and-service-journal")
print("non-claim=no-hosted-style-interactive-status-surface-on-target-service")
print(f"log={service_log}")
print(f"command-log={command_log}")
print(f"events-log={events_log}")
PY
