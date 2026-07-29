#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import shlex
import sys
import time
from typing import Callable

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import (  # noqa: E402
    ProbeFailure,
    current_service_journal,
    service_invocation_id,
    ssh_capture,
    systemctl_show,
    wait_service_active,
)


def shq(value: str) -> str:
    return shlex.quote(value)


def summarize_fragment_counts(journal: str, fragments: tuple[str, ...]) -> dict[str, int]:
    return {fragment: journal.count(fragment) for fragment in fragments}


def drive_managed_subsystem_unavailable_window(
    *,
    obc_target: str,
    obc_service: str,
    subsystem_target: str,
    subsystem_service: str,
    restart_timeout: int,
    trigger_timeout: int,
    journal_fragments: tuple[str, ...],
    diagnostic_fragments: tuple[str, ...] = (),
    artifact_path: pathlib.Path | None = None,
    outage_probe: Callable[[dict[str, object]], dict[str, object]] | None = None,
    mid_window_probe: Callable[[dict[str, object]], dict[str, object]] | None = None,
    post_trigger_probe: Callable[[dict[str, object]], dict[str, object]] | None = None,
    post_trigger_fragments: tuple[str, ...] | None = None,
    restore_before_post_trigger_probe: bool = False,
    poll_interval: float = 1.0,
) -> dict[str, object]:
    journal_since = ssh_capture(obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
    payload: dict[str, object] = {
        "verdict": "FAIL",
        "startedAt": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "obcTarget": obc_target,
        "obcService": obc_service,
        "subsystemTarget": subsystem_target,
        "subsystemService": subsystem_service,
        "baselineInvocationId": service_invocation_id(obc_target, obc_service),
        "baselineSubsystemServiceState": systemctl_show(
            subsystem_target,
            subsystem_service,
            ("ActiveState", "SubState", "MainPID", "UnitFileState"),
        ),
        "journalSince": journal_since,
        "expectedFragments": list(journal_fragments),
        "triggerTimeoutSec": trigger_timeout,
        "holdStrategy": "service-down-until-journal-or-timeout",
    }
    restored = False

    def restore_service() -> None:
        nonlocal restored
        if restored:
            return
        ssh_capture(subsystem_target, f"sudo -n systemctl start {shq(subsystem_service)}", check=False)
        wait_service_active(subsystem_target, subsystem_service, restart_timeout)
        payload["restoreVerdict"] = "PASS"
        payload["restoreAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        restored = True
    try:
        ssh_capture(subsystem_target, f"sudo -n systemctl stop {shq(subsystem_service)}")
        payload["stopIssued"] = True
        payload["stopIssuedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        hold_started = time.monotonic()
        if outage_probe is not None:
            payload["outageProbe"] = outage_probe(payload)
        if mid_window_probe is not None:
            payload["midWindowProbe"] = mid_window_probe(payload)
        deadline = time.time() + trigger_timeout
        poll_count = 0
        post_trigger_done = False
        while time.time() < deadline:
            poll_count += 1
            journal = current_service_journal(
                obc_target,
                obc_service,
                fallback_lines=500,
                since=journal_since,
            )
            if (
                not post_trigger_done
                and post_trigger_probe is not None
                and post_trigger_fragments is not None
                and all(fragment in journal for fragment in post_trigger_fragments)
            ):
                if restore_before_post_trigger_probe:
                    restore_service()
                payload["postTriggerProbe"] = post_trigger_probe(payload)
                post_trigger_done = True
            if all(fragment in journal for fragment in journal_fragments):
                payload["verdict"] = "PASS"
                payload["triggerVerdict"] = "PASS"
                payload["triggerSource"] = "target-journal"
                payload["pollCount"] = poll_count
                payload["holdDurationSec"] = round(time.monotonic() - hold_started, 3)
                payload["observedFragmentCounts"] = summarize_fragment_counts(journal, journal_fragments)
                if diagnostic_fragments:
                    payload["diagnosticCounts"] = summarize_fragment_counts(journal, diagnostic_fragments)
                payload["journalExcerpt"] = journal[-4000:]
                payload["triggerObservedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
                if not post_trigger_done and post_trigger_probe is not None:
                    payload["postTriggerProbe"] = post_trigger_probe(payload)
                    post_trigger_done = True
                break
            time.sleep(poll_interval)
        if payload.get("verdict") != "PASS":
            journal = current_service_journal(
                obc_target,
                obc_service,
                fallback_lines=500,
                since=journal_since,
            )
            payload["triggerVerdict"] = "FAIL"
            payload["pollCount"] = poll_count
            payload["holdDurationSec"] = round(time.monotonic() - hold_started, 3)
            payload["observedFragmentCounts"] = summarize_fragment_counts(journal, journal_fragments)
            if diagnostic_fragments:
                payload["diagnosticCounts"] = summarize_fragment_counts(journal, diagnostic_fragments)
            payload["lastJournalTail"] = journal[-4000:]
            raise ProbeFailure(
                "timed out waiting for recovery journal markers while "
                f"{subsystem_service} was unavailable; "
                f"observed={payload['observedFragmentCounts']}"
                + (
                    f" diagnostic={payload['diagnosticCounts']}"
                    if payload.get("diagnosticCounts") is not None
                    else ""
                )
            )
    finally:
        try:
            restore_service()
        except Exception as exc:  # pragma: no cover - target restore best effort
            payload["restoreVerdict"] = "FAIL"
            payload["restoreError"] = str(exc)
            if payload.get("verdict") == "PASS":
                payload["verdict"] = "FAIL"
                payload["error"] = f"{subsystem_service} restoration failed: {exc}"
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
    parser.add_argument("--subsystem-service", required=True)
    parser.add_argument("--restart-timeout", type=int, default=120)
    parser.add_argument("--trigger-timeout", type=int, default=60)
    parser.add_argument("--journal-fragment", action="append", dest="journal_fragments", default=[])
    args = parser.parse_args()

    if not args.journal_fragments:
        print("at least one --journal-fragment is required", file=sys.stderr)
        return 2

    artifact_path = pathlib.Path(args.artifact_path)
    try:
        payload = drive_managed_subsystem_unavailable_window(
            obc_target=args.obc_target,
            obc_service=args.obc_service,
            subsystem_target=args.subsystem_target,
            subsystem_service=args.subsystem_service,
            restart_timeout=args.restart_timeout,
            trigger_timeout=args.trigger_timeout,
            journal_fragments=tuple(args.journal_fragments),
            artifact_path=artifact_path,
        )
    except ProbeFailure as exc:
        if not artifact_path.exists():
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
        print(f"managed-subsystem-unavailable-window: FAIL {exc}", file=sys.stderr)
        return 1

    print(f"managed-subsystem-unavailable-window: {payload['verdict']}")
    print(f"artifact={artifact_path}")
    if payload.get("verdict") != "PASS":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
