#!/usr/bin/env python3
from __future__ import annotations

import os
import pathlib
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from managed_sband_unavailable_window import drive_managed_sband_unavailable_window  # noqa: E402
from run_target_can_matrix_probe import (  # noqa: E402
    ProbeFailure,
    read_text,
)
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario  # noqa: E402


class AutonomousUhfScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "target-autonomous-uhf-failover"
        self.profile = "sband"
        self.require_uhf = True
        self.failover_timeout_sec = int(os.getenv("COMM_PRIMARY_UNAVAILABLE_TIMEOUT_SEC", "60"))
        self.sband_window_artifact = self.diagnostics_dir / "sband-unavailable-window.json"

    def prepare_sband_secure_auth_case(self) -> str:
        self.begin_profile()
        self.record_secure_auth_provenance()
        self.apply_sband_ingress_diagnostics_override()
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
        self.checkpoint("sband-ground-readiness-reused", "pass", readiness=readiness)
        return readiness

    def prepare_uhf_primary_ground(self, *, boundary: str) -> None:
        self.apply_uhf_ingress_diagnostics_override()
        self.prepare_uhf_service_for_switch(require_pre_switch_ping=False, boundary=boundary)
        self.start_ground_paths(need_sband=False, need_uhf=True)
        self.prepare_ground_window(self.uhf, timeout=8.0)
        self.checkpoint("uhf-ground-readiness-reused", "pass", boundary=boundary)

    def log_length(self, path: pathlib.Path) -> int:
        return len(read_text(path))

    def wait_for_log_fragments_since(
        self,
        path: pathlib.Path,
        start_length: int,
        fragments: tuple[str, ...],
        timeout: float,
        label: str,
    ) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            text = read_text(path)
            if len(text) >= start_length:
                appended = text[start_length:]
                if all(fragment in appended for fragment in fragments):
                    return
            time.sleep(0.25)
        raise ProbeFailure(f"timed out waiting for {label} in {path}")

    def wait_for_capture_growth(self, start_size: int, timeout: float, label: str) -> int:
        capture_path = self.capture_path(self.uhf, "southbound-to-gds")
        deadline = time.time() + timeout
        while time.time() < deadline:
            size = capture_path.stat().st_size if capture_path.exists() else 0
            if size > start_size:
                return size
            time.sleep(0.25)
        raise ProbeFailure(f"timed out waiting for {label} capture growth on UHF ground path")

    def run_managed_sband_unavailable_window(self) -> dict[str, object]:
        payload = drive_managed_sband_unavailable_window(
            obc_target=self.obc_target,
            obc_service=self.obc_service,
            subsystem_target=self.subsystem_target,
            sband_comm_service=self.sband_comm_service,
            restart_timeout=self.restart_timeout,
            failover_timeout=self.failover_timeout_sec,
            artifact_path=self.sband_window_artifact,
        )
        if payload.get("verdict") != "PASS":
            raise ProbeFailure(f"managed S-band unavailable window failed: {payload}")
        self.checkpoint(
            "managed-sband-unavailable-window",
            "pass",
            artifact=str(self.sband_window_artifact),
            source=payload.get("failoverSource", "target-journal"),
        )
        self.wait_for_target_current_ready_state(require_uhf=True, timeout=20, label="autonomous failover UHF readiness")
        return payload

    def authenticate_uhf_primary(self, *, initial_sequence: int = 61):
        self.prepare_ground_window(self.uhf, timeout=8.0)
        session = self.authenticate_secure_service(
            self.uhf,
            service_id=2,
            ingress_port=1,
            role_fragment="identity 2 role 3",
            initial_sequence=initial_sequence,
        )
        return session

    def send_uhf_readback_with_ground_event(
        self,
        session,
        *,
        label: str,
        command_name: str,
        journal_fragments: tuple[str, ...],
        ground_fragments: tuple[str, ...],
        command_args: tuple[str, ...] = (),
        timeout: float = 30.0,
    ) -> dict[str, object]:
        capture_path = self.capture_path(self.uhf, "southbound-to-gds")
        oracle = self.build_ground_event_readback_oracle(
            event_log_path=self.uhf.native_event_log,
            event_offset_attr="native_event_log_offset",
            ground_fragments=ground_fragments,
            capture_path=capture_path,
            require_capture_growth=True,
            source_name="ground-native-event-log",
        )
        outcome = self.send_secure_command_until_ground_readback(
            self.uhf,
            session,
            label,
            command_name,
            *command_args,
            accept_sequence=True,
            journal_fragments=journal_fragments,
            ground_success_oracle=oracle,
            attempt_limit=30,
            per_attempt_timeout=1.0,
        )
        return outcome
