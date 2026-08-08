#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import shlex
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import ProbeFailure, current_service_journal, ssh_capture, systemctl_show  # noqa: E402


def shq(value: str) -> str:
    return shlex.quote(value)


def send_adcs_drop_state_command(subsystem_target: str, control_socket_path: str, count: int) -> str:
    script = """
import socket
import sys

socket_path = sys.argv[1]
command = f"drop-state {int(sys.argv[2])}\\n".encode("utf-8")
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(socket_path)
sock.sendall(command)
response = sock.recv(4096).decode("utf-8", errors="replace").strip()
sock.close()
print(response)
if not response.startswith("OK "):
    raise SystemExit(2)
""".strip()
    return ssh_capture(
        subsystem_target,
        f"python3 -c {shq(script)} {shq(control_socket_path)} {count}",
    ).strip()


def drive_managed_adcs_state_drop_window(
    *,
    obc_target: str,
    obc_service: str,
    subsystem_target: str,
    subsystem_service: str,
    control_socket_path: str,
    drop_count: int,
    trigger_timeout: int,
    journal_fragments: tuple[str, ...],
    artifact_path: pathlib.Path | None = None,
    outage_probe=None,
    mid_window_probe=None,
    post_trigger_probe=None,
    post_trigger_fragments: tuple[str, ...] | None = None,
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
        "controlSocketPath": control_socket_path,
        "dropCount": drop_count,
        "baselineSubsystemServiceState": systemctl_show(
            subsystem_target,
            subsystem_service,
            ("ActiveState", "SubState", "MainPID", "UnitFileState"),
        ),
        "journalSince": journal_since,
        "expectedFragments": list(journal_fragments),
    }
    try:
        payload["controlResponse"] = send_adcs_drop_state_command(subsystem_target, control_socket_path, drop_count)
        payload["triggerIssued"] = True
        if outage_probe is not None:
            payload["outageProbe"] = outage_probe(payload)
        if mid_window_probe is not None:
            payload["midWindowProbe"] = mid_window_probe(payload)
        deadline = time.time() + trigger_timeout
        post_trigger_done = False
        while time.time() < deadline:
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
                payload["postTriggerProbe"] = post_trigger_probe(payload)
                post_trigger_done = True
            if all(fragment in journal for fragment in journal_fragments):
                payload["verdict"] = "PASS"
                payload["triggerVerdict"] = "PASS"
                payload["triggerSource"] = "adcs-control-socket+target-journal"
                payload["journalExcerpt"] = journal[-4000:]
                payload["triggerObservedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
                if not post_trigger_done and post_trigger_probe is not None:
                    payload["postTriggerProbe"] = post_trigger_probe(payload)
                    post_trigger_done = True
                break
            time.sleep(poll_interval)
        if payload.get("verdict") != "PASS":
            payload["triggerVerdict"] = "FAIL"
            payload["lastJournalTail"] = current_service_journal(
                obc_target,
                obc_service,
                fallback_lines=500,
                since=journal_since,
            )[-4000:]
            raise ProbeFailure("timed out waiting for ADCS recovery journal markers after state-drop injection")
    finally:
        try:
            payload["cleanupControlResponse"] = send_adcs_drop_state_command(subsystem_target, control_socket_path, 0)
        except Exception as exc:  # pragma: no cover - best effort cleanup
            payload["cleanupError"] = str(exc)
        payload["finishedAt"] = time.strftime("%Y-%m-%dT%H:%M:%S")
        if artifact_path is not None:
            artifact_path.parent.mkdir(parents=True, exist_ok=True)
            artifact_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return payload
