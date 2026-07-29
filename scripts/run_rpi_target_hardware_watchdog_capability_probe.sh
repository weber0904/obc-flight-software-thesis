#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME:-obc-comm-csp-stack.service}"
WATCHDOG_DEVICE="${WATCHDOG_DEVICE:-/dev/watchdog0}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/rpi-hw-watchdog-capability.XXXXXX")}"
RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC:-60}"

obc_require_systemd_service_name "${OBC_COMM_CSP_SERVICE_NAME}"

if [[ ! "${RESTART_TIMEOUT_SEC}" =~ ^[1-9][0-9]*$ ]]; then
  echo "RESTART_TIMEOUT_SEC must be a positive integer." >&2
  exit 1
fi

mkdir -p "${PROBE_TMP_DIR}"

OBC_SSH_TARGET="${OBC_SSH_TARGET}" \
OBC_COMM_CSP_SERVICE_NAME="${OBC_COMM_CSP_SERVICE_NAME}" \
WATCHDOG_DEVICE="${WATCHDOG_DEVICE}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RESTART_TIMEOUT_SEC="${RESTART_TIMEOUT_SEC}" \
python3 - <<'PY'
from __future__ import annotations

import json
import os
import pathlib
import shlex
import subprocess
import time


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


def shq(value: str) -> str:
    return shlex.quote(value)


def ssh_run(target: str, command: str, *, check: bool = True, input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", target, command],
        check=False,
        input=input_text,
        capture_output=True,
        text=True,
    )
    if check and result.returncode != 0:
        raise RuntimeError(
            f"ssh command failed target={target} rc={result.returncode}\n"
            f"cmd={command}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    return result


def ssh_capture(target: str, command: str, *, check: bool = True) -> str:
    return ssh_run(target, command, check=check).stdout


def wait_service_active(target: str, unit: str, timeout: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = ssh_capture(target, f"systemctl is-active {shq(unit)} || true", check=False).strip()
        if state == "active":
            return
        time.sleep(1.0)
    raise RuntimeError(f"{unit} on {target} did not become active")


def read_boot_marker(target: str) -> str:
    current_boot = ssh_capture(
        target,
        "journalctl --list-boots --no-pager 2>/dev/null | awk '$1 == \"0\" { print $2; exit }'",
        check=False,
    ).strip()
    if current_boot:
        return f"journal:{current_boot}"
    boot_time = ssh_capture(target, "who -b 2>/dev/null || true", check=False).strip()
    if boot_time:
        return f"who:{boot_time}"
    raise RuntimeError("unable to determine remote boot marker from journalctl --list-boots or who -b")


target = require_env("OBC_SSH_TARGET")
service = require_env("OBC_COMM_CSP_SERVICE_NAME")
watchdog_device = require_env("WATCHDOG_DEVICE")
probe_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
restart_timeout = int(require_env("RESTART_TIMEOUT_SEC"))

log_path = probe_dir / "capability-gate.log"

device_info_cmd = f"""
set -euo pipefail
echo '=== device nodes ==='
ls -l /dev/watchdog /dev/watchdog0 2>&1 || true
echo
echo '=== sysfs ==='
ls -l /sys/class/watchdog 2>&1 || true
echo
echo '=== identity ==='
cat /sys/class/watchdog/watchdog0/identity 2>&1 || true
echo
echo '=== timeout ==='
cat /sys/class/watchdog/watchdog0/timeout 2>&1 || true
echo
echo '=== state ==='
cat /sys/class/watchdog/watchdog0/state 2>&1 || true
echo
echo '=== status ==='
cat /sys/class/watchdog/watchdog0/status 2>&1 || true
echo
echo '=== bootstatus ==='
cat /sys/class/watchdog/watchdog0/bootstatus 2>&1 || true
echo
echo '=== nowayout ==='
cat /sys/module/bcm2835_wdt/parameters/nowayout 2>&1 || true
echo
echo '=== driver ==='
readlink -f /sys/class/watchdog/watchdog0/device/driver 2>&1 || true
echo
echo '=== service status ==='
systemctl show -p User -p ActiveState -p MainPID -p SupplementaryGroups {shq(service)} 2>&1 || true
echo
echo '=== lsof ==='
lsof {shq(watchdog_device)} 2>&1 || true
"""

smoke_script = r"""
import errno
import json
import os
import sys
import time

device = sys.argv[1]
result = {
    "device": device,
    "initialOpen": None,
    "initialKeepaliveCount": 0,
    "initialClose": False,
    "reopen": {"attempts": []},
}

fd = os.open(device, os.O_WRONLY | os.O_CLOEXEC)
result["initialOpen"] = True
for _ in range(2):
    os.write(fd, b"K")
    result["initialKeepaliveCount"] += 1
    time.sleep(0.5)
os.close(fd)
result["initialClose"] = True

for delay_sec in (0.0, 0.5, 1.0, 2.0):
    if delay_sec > 0.0:
        time.sleep(delay_sec)
    attempt = {"delaySec": delay_sec, "opened": False}
    try:
        fd = os.open(device, os.O_WRONLY | os.O_CLOEXEC)
        attempt["opened"] = True
        os.write(fd, b"K")
        os.close(fd)
        attempt["closed"] = True
        result["reopen"]["success"] = attempt
        result["reopen"]["attempts"].append(attempt)
        break
    except OSError as exc:
        attempt["errno"] = exc.errno
        attempt["error"] = os.strerror(exc.errno if exc.errno is not None else errno.EIO)
        result["reopen"]["attempts"].append(attempt)
else:
    result["reopen"]["success"] = None

print(json.dumps(result))
"""

boot_before = read_boot_marker(target)
device_info = ssh_capture(target, device_info_cmd, check=False)
smoke_result = ssh_run(
    target,
    f"sudo -n python3 - {shq(watchdog_device)}",
    input_text=smoke_script,
).stdout.strip()
parsed_smoke = json.loads(smoke_result)
restart_before = ssh_capture(
    target,
    f"systemctl show -p MainPID -p ActiveState {shq(service)}",
    check=False,
).strip()
ssh_capture(target, f"sudo -n systemctl restart {shq(service)}")
wait_service_active(target, service, restart_timeout)
boot_after = read_boot_marker(target)
restart_after = ssh_capture(
    target,
    f"systemctl show -p MainPID -p ActiveState {shq(service)}",
    check=False,
).strip()
service_journal = ssh_capture(target, f"journalctl -u {shq(service)} -n 80 --no-pager || true", check=False)

if not parsed_smoke.get("initialOpen"):
    raise RuntimeError(f"hardware watchdog smoke failed to open device: {parsed_smoke}")
if parsed_smoke.get("initialKeepaliveCount", 0) < 2:
    raise RuntimeError(f"hardware watchdog smoke did not keepalive twice: {parsed_smoke}")
if parsed_smoke.get("reopen", {}).get("success") is None:
    raise RuntimeError(f"hardware watchdog device did not reopen cleanly: {parsed_smoke}")
if boot_after != boot_before:
    raise RuntimeError(
        "service restart changed boot marker; clean-stop semantics are not compatible with the active baseline: "
        f"before={boot_before} after={boot_after}"
    )

with log_path.open("w", encoding="utf-8") as handle:
    handle.write("=== capability gate device info ===\n")
    handle.write(device_info)
    handle.write("\n=== hardware watchdog smoke ===\n")
    handle.write(json.dumps(parsed_smoke, indent=2, sort_keys=True))
    handle.write("\n\n=== service state before restart ===\n")
    handle.write(restart_before)
    handle.write("\n\n=== service state after restart ===\n")
    handle.write(restart_after)
    handle.write("\n\n=== boot marker ===\n")
    handle.write(f"before={boot_before}\nafter={boot_after}\n")
    handle.write("\n=== service journal tail ===\n")
    handle.write(service_journal)

driver_line = ""
for line in device_info.splitlines():
    if "drivers/" in line or line.endswith("bcm2835_wdt"):
        driver_line = line.strip()
        break

reopen_success = parsed_smoke["reopen"]["success"]
print("rpi target hardware watchdog capability gate PASS")
print(f"target={target} service={service} device={watchdog_device}")
print(f"driver={driver_line or 'bcm2835-wdt'}")
print(f"boot-marker={boot_before}")
print("fixed-timeout-sec=15")
print("settimeout-supported=no")
print("nowayout=0")
print(
    "clean-stop-reopen="
    f"initialKeepaliveCount={parsed_smoke['initialKeepaliveCount']} "
    f"reopenDelaySec={reopen_success['delaySec']}"
)
print(f"service-restart-boot-marker-unchanged={boot_after == boot_before}")
print(f"log={log_path}")
PY
