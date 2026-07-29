#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import shlex
import sys
import time
import traceback

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from managed_adcs_state_drop_window import drive_managed_adcs_state_drop_window
from managed_eps_status_drop_window import drive_managed_eps_status_drop_window
from run_target_can_matrix_probe import (  # noqa: E402
    ProbeFailure,
    command_opcode,
    file_size,
    install_signal_cleanup,
    read_text_range,
    service_environment,
    service_process_environment,
    ssh_capture,
    systemctl_show,
)
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario  # noqa: E402
from route3_target_probe_common import capture_target_status, current_metadata_path  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    parser.add_argument("--stage", choices=("adcs-r3", "eps-r3-safe"), required=True)
    return parser.parse_args()


class Route3PreRebootRecoveryScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path, stage: str) -> None:
        super().__init__(probe_root)
        self.stage = stage
        self.mode = f"chapter5-route3-target-{stage}"
        self.summary_path = self.diagnostics_dir / f"{stage}-summary.json"
        self.service_log = self.probe_root / "service-status.log"
        self.metadata_path = ""
        self.adcs_window_timeout_sec = int(os.getenv("ROUTE3_ADCS_WINDOW_TIMEOUT_SEC", "150"))
        self.eps_window_timeout_sec = int(os.getenv("ROUTE3_EPS_WINDOW_TIMEOUT_SEC", "150"))

    def load_route_opcodes(self) -> None:
        super().load_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS": command_opcode(
                    dictionary, "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS"
                ),
            }
        )

    def capture_status(self, label: str) -> None:
        capture_target_status(
            obc_target=self.obc_target,
            obc_service=self.obc_service,
            metadata_path=self.metadata_path,
            service_log=self.service_log,
            label=label,
        )

    def establish_route3_sband_session(self, summary: list[str]):
        self.begin_profile()
        self.record_secure_auth_provenance()
        self.start_security_server()
        self.wait_for_sband_tcp_reachability(10.0)
        self.checkpoint(
            "sband-southbound-tcp-reachability",
            "pass",
            host=self.sband_tcp_host,
            port=self.sband_tcp_port,
        )
        self.start_ground_paths(need_sband=True, need_uhf=False)
        readiness = self.ensure_sband_ground_ready()
        self.prepare_ground_window(self.sband, timeout=8.0)
        summary.append(f"sband-ground-readiness={readiness}")
        return self.authenticate_secure_service(
            self.sband,
            service_id=1,
            ingress_port=0,
            role_fragment="identity 1 role 1",
            initial_sequence=41,
        )

    def build_recovery_status_oracle(self, *fragments: str):
        return self.build_ground_event_readback_oracle(
            event_log_path=self.sband.native_event_log,
            event_offset_attr="native_event_log_offset",
            ground_fragments=("RECOVERY_STATUS",) + tuple(fragments),
            source_name="ground-native-event-log",
        )

    def send_secure_command_name_fast(
        self,
        ground,
        session,
        label: str,
        command_name: str,
        *args: str,
        accept_sequence: bool,
        journal_fragments: tuple[str, ...],
        timeout: float = 24.0,
        sequence_number: int | None = None,
    ) -> tuple[int, str]:
        effective_sequence = (
            sequence_number
            if sequence_number is not None
            else (session.accept_sequence() if accept_sequence else session.claim_sequence())
        )
        attempt_context = self._send_secure_command_packet_once(
            ground,
            session,
            label,
            command_name,
            args,
            sequence=effective_sequence,
            attempt=1,
            sequence_policy="single-shot",
        )
        source = "sent"
        if journal_fragments:
            source = self.wait_ground_or_journal(
                ground,
                attempt_context.events_log_line_count,
                attempt_context.journal_since,
                journal_fragments,
                journal_fragments,
                timeout,
                label,
                prefer_target_journal=True,
            )
        self.checkpoint(
            f"{ground.name}-secure-command",
            "pass",
            label=label,
            command=command_name,
            sequence=effective_sequence,
            source=source,
            accept_sequence=accept_sequence,
        )
        return effective_sequence, source

    def send_secure_command_until_ground_readback_fast(
        self,
        ground,
        session,
        label: str,
        command_name: str,
        *args: str,
        accept_sequence: bool,
        journal_fragments: tuple[str, ...],
        ground_success_oracle,
        attempt_limit: int = 30,
        per_attempt_timeout: float = 1.0,
        sequence_number: int | None = None,
    ) -> dict[str, object]:
        opcode = self.opcodes[command_name]
        current_sequence = (
            sequence_number
            if sequence_number is not None
            else (session.accept_sequence() if accept_sequence else session.claim_sequence())
        )
        next_policy = "initial"
        attempts: list[dict[str, object]] = []
        accepted_seen = False
        duplicate_seen = False
        secure_reject_seen = False
        session_reject_seen = False
        session_events_log_offset: int | None = None
        session_native_event_log_offset: int | None = None
        session_native_channel_log_offset: int | None = None
        session_capture_offset: int | None = None
        for attempt_index in range(1, attempt_limit + 1):
            if attempt_index > 1:
                if next_policy == "new-seq-retry":
                    if accept_sequence:
                        current_sequence = session.accept_sequence()
                    else:
                        current_sequence += 1
                        if session.next_sequence <= current_sequence:
                            session.next_sequence = current_sequence + 1
                sequence_policy = next_policy
            else:
                sequence_policy = "initial"
            attempt_context = self._send_secure_command_packet_once(
                ground,
                session,
                label,
                command_name,
                args,
                sequence=current_sequence,
                attempt=attempt_index,
                sequence_policy=sequence_policy,
                session_events_log_offset=session_events_log_offset,
                session_native_event_log_offset=session_native_event_log_offset,
                session_native_channel_log_offset=session_native_channel_log_offset,
                session_capture_offset=session_capture_offset,
            )
            if session_events_log_offset is None:
                session_events_log_offset = attempt_context.session_events_log_offset
                session_native_event_log_offset = attempt_context.session_native_event_log_offset
                session_native_channel_log_offset = attempt_context.session_native_channel_log_offset
                session_capture_offset = attempt_context.session_capture_offset
            ground_result = ground_success_oracle(attempt_context, per_attempt_timeout)
            journal_state = self._analyze_secure_command_retry_journal(
                journal_since=attempt_context.journal_since,
                journal_fragments=journal_fragments,
                session=session,
                sequence=current_sequence,
                opcode=opcode,
            )
            accepted_seen = accepted_seen or journal_state.accepted
            duplicate_seen = duplicate_seen or journal_state.duplicate_reject
            secure_reject_seen = secure_reject_seen or journal_state.secure_reject
            session_reject_seen = session_reject_seen or journal_state.session_reject
            if ground_result is None:
                ground_result = ground_success_oracle(attempt_context, 0.0)
            attempt_payload = {
                "attempt": attempt_index,
                "sequence": current_sequence,
                "sequencePolicy": sequence_policy,
                "groundSuccess": ground_result is not None,
                "journalAccepted": journal_state.accepted,
                "journalDuplicateReject": journal_state.duplicate_reject,
                "journalSecureReject": journal_state.secure_reject,
                "journalSessionReject": journal_state.session_reject,
            }
            if ground_result is not None:
                attempt_payload.update(ground_result)
                attempts.append(attempt_payload)
                self.checkpoint(
                    f"{ground.name}-secure-command-ground-readback",
                    "pass",
                    label=label,
                    command=command_name,
                    attempt=attempt_index,
                    sequence=current_sequence,
                    source=ground_result.get("source", "ground-readback"),
                    attemptsUsed=attempt_index,
                    sequencePolicy=sequence_policy,
                )
                return {
                    "acceptedSequence": current_sequence,
                    "source": ground_result.get("source", "ground-readback"),
                    "attemptCount": attempt_index,
                    "attempts": attempts,
                    "journalAcceptedSeen": accepted_seen,
                    "duplicateRejectSeen": duplicate_seen,
                    "secureRejectSeen": secure_reject_seen,
                    "sessionRejectSeen": session_reject_seen,
                    **ground_result,
                }
            attempt_payload["journalTail"] = journal_state.journal_tail
            attempts.append(attempt_payload)
            next_policy = (
                "new-seq-retry"
                if (
                    journal_state.accepted
                    or journal_state.duplicate_reject
                    or journal_state.secure_reject
                    or journal_state.session_reject
                )
                else "same-seq-retry"
            )

        failure_class = "uplink-target-acceptance-missing"
        if duplicate_seen or secure_reject_seen or session_reject_seen:
            failure_class = "duplicate-sequence-churn-after-prior-acceptance"
        elif accepted_seen:
            failure_class = "target-accepted-but-ground-readback-missing"
        failure = ProbeFailure(
            f"{ground.name}: timed out waiting for ground readback for {label} after {attempt_limit} sends; "
            f"failureClass={failure_class}"
        )
        setattr(
            failure,
            "retry_details",
            {
                "command": command_name,
                "attemptLimit": attempt_limit,
                "perAttemptTimeoutSec": per_attempt_timeout,
                "failureClass": failure_class,
                "attempts": attempts,
                "journalAcceptedSeen": accepted_seen,
                "duplicateRejectSeen": duplicate_seen,
                "secureRejectSeen": secure_reject_seen,
                "sessionRejectSeen": session_reject_seen,
            },
        )
        raise failure

    def send_get_recovery_status(self, session, label: str, *expected_fragments: str) -> dict[str, object]:
        return self.send_secure_command_until_ground_readback_fast(
            self.sband,
            session,
            label,
            "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
            accept_sequence=True,
            journal_fragments=("RECOVERY_STATUS",) + tuple(expected_fragments),
            ground_success_oracle=self.build_recovery_status_oracle(*expected_fragments),
            attempt_limit=30,
            per_attempt_timeout=1.0,
        )

    def send_get_recovery_status_after_delay(
        self,
        session,
        label: str,
        delay_sec: float,
        *expected_fragments: str,
    ) -> dict[str, object]:
        time.sleep(delay_sec)
        return self.send_get_recovery_status(session, label, *expected_fragments)

    def wait_ground_log_fragments(
        self,
        *,
        log_path: pathlib.Path,
        start_offset: int,
        label: str,
        fragments: tuple[str, ...],
        timeout: float = 12.0,
        source_name: str = "ground-log",
    ) -> dict[str, object]:
        deadline = time.time() + timeout
        while time.time() < deadline:
            text = read_text_range(log_path, start_byte=start_offset)
            if all(fragment in text for fragment in fragments):
                return {
                    "source": source_name,
                    "logPath": str(log_path),
                    "startOffset": start_offset,
                    "matchedFragments": list(fragments),
                }
            time.sleep(0.2)
        raise ProbeFailure(
            f"timed out waiting for {label} fragments in {log_path}: {list(fragments)}"
        )

    def probe_sync_command(
        self,
        session,
        *,
        label: str,
        command_name: str,
        journal_fragments: tuple[str, ...],
        timeout: float = 12.0,
    ) -> dict[str, object]:
        payload: dict[str, object] = {
            "label": label,
            "command": command_name,
            "journalFragments": list(journal_fragments),
            "timeoutSec": timeout,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
        }
        try:
            sequence, source = self.send_secure_command_name(
                self.sband,
                session,
                label,
                command_name,
                accept_sequence=True,
                journal_fragments=journal_fragments,
                timeout=timeout,
            )
            payload["verdict"] = "PASS"
            payload["sequence"] = sequence
            payload["source"] = source
        except ProbeFailure as exc:
            payload["verdict"] = "FAIL"
            payload["error"] = str(exc)
        return payload

    def probe_sync_command_fast(
        self,
        session,
        *,
        label: str,
        command_name: str,
        journal_fragments: tuple[str, ...],
        timeout: float = 12.0,
    ) -> dict[str, object]:
        payload: dict[str, object] = {
            "label": label,
            "command": command_name,
            "journalFragments": list(journal_fragments),
            "timeoutSec": timeout,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
        }
        try:
            sequence, source = self.send_secure_command_name_fast(
                self.sband,
                session,
                label,
                command_name,
                accept_sequence=True,
                journal_fragments=journal_fragments,
                timeout=timeout,
            )
            payload["verdict"] = "PASS"
            payload["sequence"] = sequence
            payload["source"] = source
        except ProbeFailure as exc:
            payload["verdict"] = "FAIL"
            payload["error"] = str(exc)
        return payload

    def resolve_eps_control_socket(self) -> str:
        for env_source in (
            service_environment(self.subsystem_target, self.eps_service),
            service_process_environment(self.subsystem_target, self.eps_service),
        ):
            value = env_source.get("EPS_SIM_CONTROL_SOCKET", "").strip()
            if value:
                return value
        raise ProbeFailure(
            f"{self.eps_service} on {self.subsystem_target} does not expose EPS_SIM_CONTROL_SOCKET; "
            "install the updated subsystem EPS service baseline first"
        )

    def resolve_adcs_control_socket(self) -> str:
        for env_source in (
            service_environment(self.subsystem_target, self.adcs_service),
            service_process_environment(self.subsystem_target, self.adcs_service),
        ):
            value = env_source.get("ADCS_SIM_CONTROL_SOCKET", "").strip()
            if value:
                return value
        raise ProbeFailure(
            f"{self.adcs_service} on {self.subsystem_target} does not expose ADCS_SIM_CONTROL_SOCKET; "
            "install the updated subsystem ADCS service baseline first"
        )

    def remote_set_soc(self, socket_path: str, value: float, transition_sec: float = 0.0) -> str:
        script = """
import socket
import sys

socket_path = sys.argv[1]
command = f"set-soc {float(sys.argv[2]):.2f} {float(sys.argv[3]):.2f}\\n".encode("utf-8")
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
            self.subsystem_target,
            f"python3 -c {shlex.quote(script)} {shlex.quote(socket_path)} {value:.2f} {transition_sec:.2f}",
        ).strip()

    def force_safe_then_idle(self, session, summary: list[str]) -> None:
        eps_socket = self.resolve_eps_control_socket()
        summary.append(f"eps-control-socket={eps_socket}")
        summary.append(f"soc-80-response={self.remote_set_soc(eps_socket, 80.0)}")
        eps_get_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        _, eps_source = self.send_secure_command_name(
            self.sband,
            session,
            "route3 eps status refresh before mode idle",
            "OBCApp.epsBridge.EPS_GET_STATUS",
            accept_sequence=True,
            journal_fragments=("EPS_STATUS_RECEIVED", f"Opcode 0x{eps_get_opcode:x} completed"),
            timeout=20.0,
        )
        summary.append(f"eps-refresh-source={eps_source}")
        try:
            _, safe_source = self.send_secure_command_name(
                self.sband,
                session,
                "route3 mode safe precondition",
                "OBCApp.modeManager.MODE_SET",
                "SAFE",
                accept_sequence=True,
                journal_fragments=("SYS_MODE_CHANGE", "SAFE (0)"),
                timeout=12.0,
            )
            summary.append(f"mode-safe-source={safe_source}")
        except ProbeFailure as exc:
            summary.append(f"mode-safe-precondition=best-effort-miss detail={exc}")
        _, idle_source = self.send_secure_command_name(
            self.sband,
            session,
            "route3 mode idle precondition",
            "OBCApp.modeManager.MODE_SET",
            "IDLE",
            accept_sequence=True,
            journal_fragments=("SYS_MODE_CHANGE", "IDLE (1)"),
            timeout=45.0,
        )
        summary.append(f"mode-idle-source={idle_source}")

    def run_adcs_stage(self) -> list[str]:
        summary: list[str] = []
        self.capture_status("baseline")
        session = self.establish_route3_sband_session(summary)
        baseline_service = systemctl_show(
            self.obc_target,
            self.obc_service,
            ("InvocationID", "MainPID", "NRestarts", "ActiveState"),
        )
        baseline_invocation = baseline_service.get("InvocationID", "")
        baseline_pid = baseline_service.get("MainPID", "0")
        baseline_restarts = baseline_service.get("NRestarts", "0")
        adcs_get_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
        pre_outage_probe = self.probe_sync_command(
            session,
            label="route3 adcs pre-outage attitude probe",
            command_name="OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
            journal_fragments=(f"Opcode 0x{adcs_get_opcode:x} completed",),
        )
        self.write_json_artifact(self.diagnostics_dir / "adcs-pre-outage-sync-probe.json", pre_outage_probe)
        if pre_outage_probe.get("verdict") != "PASS":
            raise ProbeFailure(f"ADCS pre-outage sync probe failed before outage: {pre_outage_probe}")

        adcs_control_socket = self.resolve_adcs_control_socket()
        summary.append(f"adcs-control-socket={adcs_control_socket}")
        ground_event_offset = file_size(self.sband.native_event_log)
        helper_artifact = self.diagnostics_dir / "managed-adcs-state-drop-window.json"
        try:
            helper_payload = drive_managed_adcs_state_drop_window(
                obc_target=self.obc_target,
                obc_service=self.obc_service,
                subsystem_target=self.subsystem_target,
                subsystem_service=self.adcs_service,
                control_socket_path=adcs_control_socket,
                drop_count=4,
                trigger_timeout=self.adcs_window_timeout_sec,
                journal_fragments=(
                    "RECOVERY_INCIDENT_OPENED",
                    "ADCS_POLL_TRANSPORT",
                    "SUBSYSTEM_INTERFACE_RESET",
                ),
                artifact_path=helper_artifact,
                post_trigger_fragments=(
                    "RECOVERY_INCIDENT_OPENED",
                    "ADCS_POLL_TRANSPORT",
                ),
                post_trigger_probe=lambda _payload: self.wait_ground_log_fragments(
                    log_path=self.sband.native_event_log,
                    start_offset=ground_event_offset,
                    label="route3 adcs active recovery event path",
                    fragments=(
                        "RECOVERY_INCIDENT_OPENED",
                        "source ADCS_POLL_TRANSPORT",
                        "RECOVERY_ACTION_EXECUTED",
                        "SUBSYSTEM_INTERFACE_RESET",
                    ),
                    timeout=12.0,
                    source_name="ground-native-event-log",
                ),
            )
        except ProbeFailure:
            self.capture_status("adcs-r3-window-failure")
            raise
        if helper_payload.get("verdict") != "PASS":
            raise ProbeFailure(f"managed ADCS state-drop window failed: payload={helper_payload}")
        active_outcome = helper_payload.get("postTriggerProbe", {})
        in_outage_probe = {
            "verdict": "SKIP",
            "reason": "current ADCS Route 3 closure prioritizes active recovery-status readback over auxiliary in-window sync probing",
        }
        self.write_json_artifact(self.diagnostics_dir / "adcs-in-outage-sync-probe.json", in_outage_probe)
        summary.append(f"managed-adcs-state-drop-window={helper_artifact}")
        summary.append(f"managed-adcs-window-timeout-sec={self.adcs_window_timeout_sec}")
        summary.append(f"adcs-pre-outage-sync-probe={pre_outage_probe}")
        summary.append(f"adcs-in-outage-sync-probe={in_outage_probe}")

        after_service = systemctl_show(
            self.obc_target,
            self.obc_service,
            ("InvocationID", "MainPID", "NRestarts", "ActiveState"),
        )
        if (
            after_service.get("InvocationID", "") != baseline_invocation
            or after_service.get("MainPID", "0") != baseline_pid
            or after_service.get("NRestarts", "0") != baseline_restarts
        ):
            raise ProbeFailure(
                "ADCS R3 target proof escalated past the intended no-restart window: "
                f"baseline_invocation={baseline_invocation} after_invocation={after_service.get('InvocationID')} "
                f"baseline_pid={baseline_pid} after_pid={after_service.get('MainPID')} "
                f"baseline_restarts={baseline_restarts} after_restarts={after_service.get('NRestarts')}"
            )
        summary.append(
            "no-r2-proof="
            f"InvocationID:{baseline_invocation} MainPID:{baseline_pid} NRestarts:{baseline_restarts}"
        )
        summary.append(f"adcs-active-recovery-event-proof={active_outcome}")

        self.wait_for_target_journal(
            self.obc_service,
            str(helper_payload["journalSince"]),
            ("RECOVERY_INCIDENT_CLEARED", "ADCS_POLL_TRANSPORT"),
            20,
            "ADCS recovery clear",
        )
        cleared_outcome = self.send_get_recovery_status(
            session,
            "route3 adcs cleared recovery status",
            "activeCount 0",
            "source NONE",
            "pendingProcessRestart False",
            "pendingReboot False",
        )
        summary.append(f"adcs-cleared-recovery-status={cleared_outcome}")
        self.capture_status("after-adcs-r3-stage")
        summary.append("chapter5-route3-target-adcs-r3: PASS")
        return summary

    def run_eps_stage(self) -> list[str]:
        summary: list[str] = []
        self.capture_status("baseline")
        session = self.establish_route3_sband_session(summary)
        self.force_safe_then_idle(session, summary)
        eps_get_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        pre_outage_probe = self.probe_sync_command(
            session,
            label="route3 eps pre-outage status probe",
            command_name="OBCApp.epsBridge.EPS_GET_STATUS",
            journal_fragments=("EPS_STATUS_RECEIVED", f"Opcode 0x{eps_get_opcode:x} completed"),
        )
        self.write_json_artifact(self.diagnostics_dir / "eps-pre-outage-sync-probe.json", pre_outage_probe)
        if pre_outage_probe.get("verdict") != "PASS":
            raise ProbeFailure(f"EPS pre-outage sync probe failed before outage: {pre_outage_probe}")

        ground_event_offset = file_size(self.sband.native_event_log)
        helper_artifact = self.diagnostics_dir / "managed-eps-unavailable-window.json"
        try:
            eps_control_socket = self.resolve_eps_control_socket()
            summary.append(f"eps-control-socket={eps_control_socket}")
            helper_payload = drive_managed_eps_status_drop_window(
                obc_target=self.obc_target,
                obc_service=self.obc_service,
                subsystem_target=self.subsystem_target,
                subsystem_service=self.eps_service,
                control_socket_path=eps_control_socket,
                drop_count=4,
                trigger_timeout=self.eps_window_timeout_sec,
                journal_fragments=(
                    "RECOVERY_INCIDENT_OPENED",
                    "EPS_TIMEOUT",
                    "SUBSYSTEM_INTERFACE_RESET",
                    "SYS_MODE_CHANGE",
                    "SAFE (0)",
                ),
                artifact_path=helper_artifact,
                outage_probe=lambda _payload: {
                    "verdict": "SKIP",
                    "reason": "current EPS Route 3 closure does not gate on auxiliary in-window sync probing",
                },
                post_trigger_fragments=(
                    "RECOVERY_INCIDENT_OPENED",
                    "EPS_TIMEOUT",
                ),
                post_trigger_probe=lambda _payload: self.wait_ground_log_fragments(
                    log_path=self.sband.native_event_log,
                    start_offset=ground_event_offset,
                    label="route3 eps active recovery event path",
                    fragments=(
                        "RECOVERY_INCIDENT_OPENED",
                        "source EPS_TIMEOUT",
                        "RECOVERY_ACTION_EXECUTED",
                        "SAFE_FALLBACK",
                        "SYS_MODE_CHANGE",
                        "System mode changed to SAFE",
                    ),
                    timeout=12.0,
                    source_name="ground-native-event-log",
                ),
            )
        except ProbeFailure:
            self.capture_status("eps-r3-safe-window-failure")
            raise
        if helper_payload.get("verdict") != "PASS":
            raise ProbeFailure(f"managed EPS unavailable window failed: payload={helper_payload}")
        summary.append(f"managed-eps-window={helper_artifact}")
        summary.append(f"managed-eps-window-timeout-sec={self.eps_window_timeout_sec}")
        summary.append(f"eps-pre-outage-sync-probe={pre_outage_probe}")
        summary.append(f"eps-in-outage-sync-probe={helper_payload.get('outageProbe')}")

        active_outcome = helper_payload.get("postTriggerProbe", {})
        summary.append(f"eps-active-recovery-event-proof={active_outcome}")

        self.wait_for_target_journal(
            self.obc_service,
            str(helper_payload["journalSince"]),
            ("RECOVERY_INCIDENT_CLEARED", "EPS_TIMEOUT"),
            20,
            "EPS recovery clear",
        )
        cleared_outcome = self.send_get_recovery_status(
            session,
            "route3 eps cleared recovery status",
            "activeCount 0",
            "source NONE",
            "pendingProcessRestart False",
            "pendingReboot False",
        )
        summary.append(f"eps-cleared-recovery-status={cleared_outcome}")
        self.capture_status("after-eps-r3-safe-stage")
        summary.append("chapter5-route3-target-eps-r3-safe: PASS")
        return summary

    def run(self) -> list[str]:
        self.metadata_path, _ = current_metadata_path(
            self.obc_target,
            self.obc_service,
            os.getenv("RUNTIME_ROOT", ""),
        )
        self.load_route_opcodes()
        if self.stage == "adcs-r3":
            return self.run_adcs_stage()
        if self.stage == "eps-r3-safe":
            return self.run_eps_stage()
        raise ProbeFailure(f"unsupported route3 stage {self.stage}")


def main() -> int:
    args = parse_args()
    scenario = Route3PreRebootRecoveryScenario(pathlib.Path(args.probe_root), args.stage)
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        summary_lines = scenario.run()
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"{scenario.mode}: FAIL {exc}\n"
        exit_code = 1
    except Exception:  # pragma: no cover - top-level trap
        run_error = RuntimeError("unexpected exception")
        output = f"{scenario.mode}: FAIL unexpected exception\n{traceback.format_exc()}"
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"{scenario.mode}: FAIL cleanup {cleanup_exc}\n"
                exit_code = 1
            else:
                output += f"\ncleanup-error: {cleanup_exc}\n"
        scenario.sband.force_stop()
        scenario.uhf.force_stop()
        if exit_code == 0:
            sys.stdout.write(output)
        else:
            sys.stderr.write(output)
        sys.stdout.flush()
        sys.stderr.flush()
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
