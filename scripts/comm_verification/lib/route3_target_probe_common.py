#!/usr/bin/env python3
from __future__ import annotations

import os
import pathlib
import shlex
import subprocess
import time

from run_target_can_matrix_probe import (
    ProbeFailure,
    service_environment,
    ssh_capture,
    systemctl_show,
)


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise ProbeFailure(f"missing required environment variable: {name}")
    return value


def shq(value: str) -> str:
    return shlex.quote(value)


def ssh_is_up(target: str) -> bool:
    result = subprocess.run(
        ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", target, "true"],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return result.returncode == 0


def wait_for_ssh_down(target: str, timeout: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if not ssh_is_up(target):
            return
        time.sleep(1.0)
    raise ProbeFailure(f"timed out waiting for SSH disconnect on {target}")


def wait_for_ssh_up(target: str, timeout: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if ssh_is_up(target):
            return
        time.sleep(2.0)
    raise ProbeFailure(f"timed out waiting for SSH reconnect on {target}")


def read_metadata(target: str, metadata_path: str) -> dict[str, str]:
    text = ssh_capture(target, f"cat {shq(metadata_path)}", check=False)
    metadata: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            metadata[key.strip()] = value.strip()
    return metadata


def read_boot_marker(target: str) -> str:
    boot_id = ssh_capture(
        target,
        "journalctl --list-boots --no-pager 2>/dev/null | awk '$1 == \"0\" { print $2; exit }'",
        check=False,
    ).strip()
    if boot_id:
        return f"journal:{boot_id}"
    boot_time = ssh_capture(target, "who -b 2>/dev/null || true", check=False).strip()
    if boot_time:
        return f"who:{boot_time}"
    raise ProbeFailure("unable to determine remote boot marker from journalctl --list-boots or who -b")


def capture_target_status(
    *,
    obc_target: str,
    obc_service: str,
    metadata_path: str,
    service_log: pathlib.Path,
    label: str,
) -> None:
    commands = [
        f"echo '=== {label}: OBC service ==='",
        f"systemctl --no-pager --full status {shq(obc_service)} || true",
        f"systemctl show -p MainPID -p NRestarts -p ExecMainStatus -p Result -p ActiveState {shq(obc_service)} || true",
        f"journalctl -u {shq(obc_service)} -n 160 --no-pager || true",
        "echo '=== list boots ==='",
        "journalctl --list-boots --no-pager || true",
        "echo '=== metadata ==='",
        f"cat {shq(metadata_path)} || true",
    ]
    text = ssh_capture(obc_target, " ; ".join(commands), check=False)
    service_log.parent.mkdir(parents=True, exist_ok=True)
    with service_log.open("a", encoding="utf-8") as handle:
        handle.write(text)
        handle.write("\n")


def run_secure_auth_command_path_subprobe(
    *,
    root_dir: pathlib.Path,
    probe_root: pathlib.Path,
    require_uhf: bool,
    extra_readbacks: tuple[str, ...] = (),
) -> tuple[str, pathlib.Path]:
    python_bin = root_dir / "fprime-venv" / "bin" / "python"
    probe_script = root_dir / "scripts" / "comm_verification" / "lib" / "run_target_secure_auth_command_path_probe.py"
    summary_path = probe_root / "diagnostics" / "secure-auth-command-path-summary.json"
    env = os.environ.copy()
    env.update(
        {
            "TARGET_BASELINE_MANAGED_EXTERNALLY": "1",
            "TARGET_BASELINE_REQUIRE_UHF_SERVICE": "1" if require_uhf else "0",
            "OBC_GROUNDLINK_DIAGNOSTICS": "0",
            "SBAND_COMM_NODE_INGRESS_DIAGNOSTICS": "0",
            "UHF_COMM_NODE_INGRESS_DIAGNOSTICS": "0",
            "TARGET_SECURE_AUTH_EXTRA_READBACKS": ",".join(extra_readbacks),
        }
    )
    result = subprocess.run(
        [str(python_bin), str(probe_script), "--probe-root", str(probe_root)],
        check=False,
        capture_output=True,
        text=True,
        env=env,
    )
    if result.returncode != 0:
        raise ProbeFailure(
            "secure-auth command path probe failed: "
            f"rc={result.returncode} summary={summary_path}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    return result.stdout.strip(), summary_path


def wait_r2_metadata(target: str, metadata_path: str, timeout: int) -> dict[str, str]:
    deadline = time.time() + timeout
    last: dict[str, str] = {}
    while time.time() < deadline:
        last = read_metadata(target, metadata_path)
        if (
            last.get("reset_cause") == "3"
            and last.get("last_recovery_source") == "6"
            and last.get("last_recovery_level") == "2"
            and last.get("recovery_reset_pending") == "0"
        ):
            return last
        time.sleep(1.0)
    raise ProbeFailure(f"timed out waiting for ADCS R2 metadata in {metadata_path}; last={last}")


def wait_watchdog_reboot_metadata(
    *,
    target: str,
    metadata_path: str,
    baseline_boot_count: int,
    recovery_source_code: str,
    timeout: int,
) -> dict[str, str]:
    deadline = time.time() + timeout
    last: dict[str, str] = {}
    while time.time() < deadline:
        last = read_metadata(target, metadata_path)
        boot_count = int(last.get("boot_count", "0") or "0")
        if (
            boot_count > baseline_boot_count
            and last.get("last_recovery_source") == recovery_source_code
            and last.get("last_recovery_level") == "6"
            and last.get("recovery_reset_pending") == "0"
        ):
            return last
        time.sleep(1.0)
    raise ProbeFailure(f"timed out waiting for watchdog reboot metadata in {metadata_path}; last={last}")


def current_metadata_path(obc_target: str, obc_service: str, runtime_root_override: str) -> tuple[str, dict[str, str]]:
    baseline_env = service_environment(obc_target, obc_service)
    runtime_root = runtime_root_override or baseline_env.get(
        "RUNTIME_ROOT",
        "/home/operator/obc-deploy/runtime/comm-csp-lab-obc",
    )
    metadata_path = f"{runtime_root}/persistent-data/boot/metadata-v1.txt"
    return metadata_path, baseline_env


def current_service_snapshot(obc_target: str, obc_service: str) -> dict[str, str]:
    return systemctl_show(obc_target, obc_service, ("MainPID", "NRestarts", "ActiveState", "InvocationID"))


def wait_current_invocation_ready(
    *,
    scenario,
    require_uhf: bool,
    timeout: int = 20,
) -> None:
    scenario.wait_for_target_current_ready_state(
        require_uhf=require_uhf,
        timeout=timeout,
        label="current target readiness after reboot",
    )
    time.sleep(2.0)
