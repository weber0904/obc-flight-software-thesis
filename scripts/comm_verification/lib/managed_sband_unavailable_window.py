#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import shlex
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import (  # noqa: E402
    ProbeFailure,
    current_service_journal,
    service_invocation_id,
    systemctl_show,
    ssh_capture,
    wait_service_active,
)


def shq(value: str) -> str:
    return shlex.quote(value)


def journal_fragments() -> tuple[str, ...]:
    return (
        "RECOVERY_INCIDENT_OPENED",
        "COMM_PRIMARY_UNAVAILABLE",
        "COMM_PRIMARY_LINK_CHANGED",
        "command UHF",
        "telemetry UHF",
        "file UHF",
        "reason 2",
    )


def drive_managed_sband_unavailable_window(
    *,
    obc_target: str,
    obc_service: str,
    subsystem_target: str,
    sband_comm_service: str,
    restart_timeout: int,
    failover_timeout: int,
    artifact_path: pathlib.Path | None = None,
) -> dict[str, object]:
    journal_since = ssh_capture(obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
    baseline_invocation = service_invocation_id(obc_target, obc_service)
    baseline_sband_state = systemctl_show(
        subsystem_target,
        sband_comm_service,
        ("ActiveState", "SubState", "MainPID", "UnitFileState"),
    )
    started_at = time.strftime("%Y-%m-%dT%H:%M:%S")
    payload: dict[str, object] = {
        "verdict": "FAIL",
        "startedAt": started_at,
        "obcTarget": obc_target,
        "obcService": obc_service,
        "subsystemTarget": subsystem_target,
        "sbandCommService": sband_comm_service,
        "baselineInvocationId": baseline_invocation,
        "baselineSbandServiceState": baseline_sband_state,
        "journalSince": journal_since,
        "expectedFragments": list(journal_fragments()),
    }
    restore_attempted = False
    try:
        ssh_capture(subsystem_target, f"sudo -n systemctl stop {shq(sband_comm_service)}")
        payload["stopIssued"] = True
        deadline = time.time() + failover_timeout
        while time.time() < deadline:
            journal = current_service_journal(
                obc_target,
                obc_service,
                fallback_lines=500,
                since=journal_since,
            )
            if all(fragment in journal for fragment in journal_fragments()):
                payload["verdict"] = "PASS"
                payload["failoverSource"] = "target-journal"
                payload["journalExcerpt"] = journal[-4000:]
                payload["observedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
                break
            time.sleep(0.5)
        if payload.get("verdict") != "PASS":
            payload["lastJournalTail"] = current_service_journal(
                obc_target,
                obc_service,
                fallback_lines=500,
                since=journal_since,
            )[-4000:]
            raise ProbeFailure("timed out waiting for target autonomous failover journal markers")
    finally:
        restore_attempted = True
        restore_error = ""
        try:
            ssh_capture(subsystem_target, f"sudo -n systemctl start {shq(sband_comm_service)}", check=False)
            wait_service_active(subsystem_target, sband_comm_service, restart_timeout)
            payload["restoreVerdict"] = "PASS"
        except Exception as exc:  # pragma: no cover - target restore best effort
            restore_error = str(exc)
            payload["restoreVerdict"] = "FAIL"
            payload["restoreError"] = restore_error
            if payload.get("verdict") == "PASS":
                payload["verdict"] = "FAIL"
                payload["error"] = f"S-band restoration failed: {restore_error}"
        payload["restoreAttempted"] = restore_attempted
        payload["finishedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        if artifact_path is not None:
            artifact_path.parent.mkdir(parents=True, exist_ok=True)
            artifact_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return payload


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-path", required=True)
    parser.add_argument("--obc-target", required=True)
    parser.add_argument("--obc-service", required=True)
    parser.add_argument("--subsystem-target", required=True)
    parser.add_argument("--sband-comm-service", required=True)
    parser.add_argument("--restart-timeout", type=int, default=120)
    parser.add_argument("--failover-timeout", type=int, default=60)
    args = parser.parse_args()

    artifact_path = pathlib.Path(args.artifact_path)
    try:
        payload = drive_managed_sband_unavailable_window(
            obc_target=args.obc_target,
            obc_service=args.obc_service,
            subsystem_target=args.subsystem_target,
            sband_comm_service=args.sband_comm_service,
            restart_timeout=args.restart_timeout,
            failover_timeout=args.failover_timeout,
            artifact_path=artifact_path,
        )
    except ProbeFailure as exc:
        artifact_path.parent.mkdir(parents=True, exist_ok=True)
        artifact_path.write_text(
            json.dumps(
                {
                    "verdict": "FAIL",
                    "error": str(exc),
                    "finishedAt": time.strftime("%Y-%m-%dT%H:%M:%S"),
                },
                indent=2,
                sort_keys=True,
            )
            + "\n",
            encoding="utf-8",
        )
        print(f"managed-sband-unavailable-window: FAIL {exc}", file=sys.stderr)
        return 1

    print(f"managed-sband-unavailable-window: {payload['verdict']}")
    print(f"artifact={artifact_path}")
    if payload.get("verdict") != "PASS":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
