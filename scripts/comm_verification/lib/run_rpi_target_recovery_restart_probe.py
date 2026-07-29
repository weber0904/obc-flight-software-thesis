#!/usr/bin/env python3
from __future__ import annotations

import os
import pathlib
import shlex
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import ProbeFailure, ssh_capture, wait_service_active
from route3_target_probe_common import (
    capture_target_status,
    current_metadata_path,
    current_service_snapshot,
    require_env,
    run_secure_auth_command_path_subprobe,
    wait_r2_metadata,
)


def shq(value: str) -> str:
    return shlex.quote(value)


def main() -> int:
    obc_target = require_env("OBC_SSH_TARGET")
    subsystem_target = require_env("SUBSYSTEM_SIM_SSH_TARGET")
    obc_service = require_env("OBC_COMM_CSP_SERVICE_NAME")
    adcs_service = require_env("ADCS_SERVICE_NAME")
    sband_stack_target = require_env("SBAND_STACK_TARGET_NAME")
    uhf_stack_target = require_env("UHF_STACK_TARGET_NAME")
    sband_comm_service = require_env("SBAND_COMM_SERVICE_NAME")
    uhf_comm_service = require_env("UHF_COMM_SERVICE_NAME")
    restart_timeout = int(require_env("RESTART_TIMEOUT_SEC"))
    adcs_restore_timeout = int(require_env("ADCS_RESTORE_TIMEOUT_SEC"))
    require_uhf = require_env("TARGET_BASELINE_REQUIRE_UHF_SERVICE") == "1"
    runtime_root_override = os.environ.get("RUNTIME_ROOT", "")
    probe_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))

    metadata_path, baseline_env = current_metadata_path(obc_target, obc_service, runtime_root_override)
    baseline_target_comm_profile = baseline_env.get("TARGET_COMM_PROFILE", "sband")
    baseline = current_service_snapshot(obc_target, obc_service)
    baseline_pid = baseline.get("MainPID", "0")
    baseline_restarts = int(baseline.get("NRestarts", "0") or "0")
    service_log = probe_dir / "service-status.log"

    wait_service_active(obc_target, obc_service, restart_timeout)
    wait_service_active(subsystem_target, sband_stack_target, adcs_restore_timeout)
    wait_service_active(subsystem_target, sband_comm_service, adcs_restore_timeout)
    wait_service_active(subsystem_target, require_env("EPS_SERVICE_NAME"), adcs_restore_timeout)
    wait_service_active(subsystem_target, adcs_service, adcs_restore_timeout)
    if require_uhf:
        wait_service_active(subsystem_target, uhf_stack_target, adcs_restore_timeout)
        wait_service_active(subsystem_target, uhf_comm_service, adcs_restore_timeout)
    if baseline_target_comm_profile != os.environ.get("TARGET_COMM_PROFILE", "sband"):
        raise ProbeFailure(
            f"target service TARGET_COMM_PROFILE={baseline_target_comm_profile} does not match requested profile "
            f"{os.environ.get('TARGET_COMM_PROFILE', 'sband')}"
        )

    capture_target_status(
        obc_target=obc_target,
        obc_service=obc_service,
        metadata_path=metadata_path,
        service_log=service_log,
        label="baseline",
    )

    observed = dict(baseline)
    try:
        ssh_capture(subsystem_target, f"sudo -n systemctl stop {shq(adcs_service)}")
        deadline = time.time() + restart_timeout
        while time.time() < deadline:
            observed = current_service_snapshot(obc_target, obc_service)
            main_pid = observed.get("MainPID", "0")
            restarts = int(observed.get("NRestarts", "0") or "0")
            if observed.get("ActiveState") == "active" and main_pid not in ("0", baseline_pid) and restarts > baseline_restarts:
                break
            time.sleep(1.0)
        else:
            raise ProbeFailure(
                f"OBC service did not restart after ADCS stop; baseline={baseline} observed={observed}"
            )
    finally:
        ssh_capture(subsystem_target, f"sudo -n systemctl start {shq(adcs_service)}", check=False)

    wait_service_active(subsystem_target, adcs_service, adcs_restore_timeout)
    wait_service_active(obc_target, obc_service, restart_timeout)
    metadata = wait_r2_metadata(obc_target, metadata_path, restart_timeout)
    command_path_stdout, secure_auth_summary = run_secure_auth_command_path_subprobe(
        root_dir=ROOT_DIR,
        probe_root=probe_dir / "post-r2-secure-auth",
        require_uhf=require_uhf,
    )

    capture_target_status(
        obc_target=obc_target,
        obc_service=obc_service,
        metadata_path=metadata_path,
        service_log=service_log,
        label="after-r2-restart",
    )

    print("rpi target recovery restart probe PASS")
    print(f"obc-service={obc_service} target={obc_target}")
    print(f"adcs-service={adcs_service} target={subsystem_target}")
    print(f"subsystem-sband-stack={sband_stack_target}")
    print(f"subsystem-uhf-stack={uhf_stack_target}")
    print(f"subsystem-sband-comm-service={sband_comm_service}")
    print(f"subsystem-uhf-comm-service={uhf_comm_service}")
    print(f"target-comm-profile={os.environ.get('TARGET_COMM_PROFILE', 'sband')}")
    print("probe-mode=r2-recovery")
    print(f"baseline-main-pid={baseline_pid} restarted-main-pid={observed.get('MainPID')}")
    print(f"baseline-restarts={baseline_restarts} restarted-restarts={observed.get('NRestarts')}")
    print("reset_cause=RECOVERY_ADCS_FDIR")
    print("last_recovery_source=ADCS_POLL_TRANSPORT")
    print("last_recovery_level=R2_RESTART_SOFTWARE_COMPONENT")
    print(
        "command_path="
        f"secure-auth command path PASS summary={secure_auth_summary}; "
        f"stdout={command_path_stdout.replace(chr(10), ' | ')}"
    )
    print(f"boot_count={metadata.get('boot_count')}")
    print(f"consecutive_reset_count={metadata.get('consecutive_reset_count')}")
    print(f"log={service_log}")
    print(f"command-log={probe_dir / 'post-r2-secure-auth' / 'sband-ground' / 'raw-command.log'}")
    print(f"events-log={probe_dir / 'post-r2-secure-auth' / 'sband-ground' / 'events.log'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
