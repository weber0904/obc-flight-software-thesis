#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/mode-model-v2-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${PROBE_TMP_DIR}/runtime}"
OBC_LOG="${PROBE_TMP_DIR}/obc-mode-smoke.log"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
OBC_LOG="${OBC_LOG}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


bin_dir = pathlib.Path(require_env("BIN_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
obc_log = pathlib.Path(require_env("OBC_LOG"))

if runtime_root.exists():
    shutil.rmtree(runtime_root)
runtime_root.mkdir(parents=True, exist_ok=True)

commands = "\n".join(
    [
        "status",
        "mode payload",
        "mode ttc",
        "mode idle",
        "mode payload",
        "mode ttc",
        "mode safe",
        "mode idle",
        "mode ttc",
        "mode payload",
        "mode idle",
        "mode hell",
        "mode nominal",
        "mode low-power",
        "mode debug",
        "mode update",
        "mode cruise",
        "mode Payload",
        "quit",
        "",
    ]
)

cmd = [
    str(bin_dir / "OBC"),
    "--ground-link",
    "disabled",
        "--runtime-root",
        str(runtime_root),
        "--tick-ms",
        "60000",
    ]

result = subprocess.run(
    cmd,
    input=commands,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    timeout=120,
    check=False,
)
obc_log.write_text(result.stdout, encoding="utf-8")

if result.returncode != 0:
    sys.stderr.write(result.stdout)
    raise SystemExit(result.returncode)

output = result.stdout

mode_lines = [line for line in output.splitlines() if line.startswith("mode=")]

expected_modes = [
    "mode=SAFE ",
    "mode=SAFE ",
    "mode=SAFE ",
    "mode=IDLE ",
    "mode=PAYLOAD ",
    "mode=PAYLOAD ",
    "mode=SAFE ",
    "mode=IDLE ",
    "mode=TTC ",
    "mode=TTC ",
    "mode=IDLE ",
    "mode=IDLE ",
]

missing = []
if len(mode_lines) < len(expected_modes):
    missing.append(f"{len(expected_modes)} mode status lines")
else:
    for index, expected in enumerate(expected_modes):
        if expected not in mode_lines[index]:
            missing.append(f"mode line {index}: expected {expected!r}, got {mode_lines[index]!r}")

if output.count("unknown mode") < 6:
    missing.append("four retired, one unknown, and one mixed-case parser rejection")

if output.count("SYS_MODE_TRANSITION_REJECTED") < 5:
    missing.append("transition rejection events for guarded shell requests")

if missing:
    sys.stderr.write(output)
    raise SystemExit(f"mode model v2 hosted probe missing expected output: {missing}")

if "mode=NOMINAL" in output or "mode=LOW_POWER" in output or "mode=DEBUG" in output or "mode=UPDATE" in output:
    sys.stderr.write(output)
    raise SystemExit("mode model v2 hosted probe observed a retired primary mode name")

if "mode=HELL " in output:
    sys.stderr.write(output)
    raise SystemExit("mode model v2 hosted probe observed operator entry to HELL")

print(f"mode_model_v2_hosted_probe: PASS log={obc_log}")
PY
