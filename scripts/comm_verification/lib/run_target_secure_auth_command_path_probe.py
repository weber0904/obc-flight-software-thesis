#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import shutil
import socket
import sys
import time
import traceback

from run_target_can_matrix_probe import (
    ProbeFailure,
    TargetCanScenario,
    apply_service_override,
    command_opcode,
    ensure_no_legacy_aliases,
    free_port,
    install_signal_cleanup,
    remove_service_override,
    wait_service_active,
)


class SecureAuthCommandPathScenario(TargetCanScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        # Reuse the secure-auth proof capture/oracle defaults while keeping the
        # flow independent from the full target secure-auth proof.
        super().__init__("secure-auth-proof", "sband", probe_root)
        self.include_malformed_advisory = os.getenv("SECURE_AUTH_PREFLIGHT_INCLUDE_MALFORMED", "0") == "1"
        self.sband_ingress_diagnostics_override_dropin_name = os.getenv(
            "SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME",
            "58-sband-ingress-diagnostics.conf",
        )
        self.sband_comm_node_ingress_diagnostics = os.getenv("SBAND_COMM_NODE_INGRESS_DIAGNOSTICS", "1")
        self.sband_ingress_diagnostics_override_applied = False

    def load_opcodes(self) -> None:
        super().load_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS": command_opcode(
                    dictionary, "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS"
                ),
                "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS": command_opcode(
                    dictionary, "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS"
                ),
            }
        )

    def apply_sband_ingress_diagnostics_override(self) -> None:
        if self.sband_ingress_diagnostics_override_applied:
            return
        if self.sband_comm_node_ingress_diagnostics in ("", "0"):
            self.checkpoint(
                "sband-ingress-diagnostics-override-skipped",
                "pass",
                reason="disabled",
                requested_value=self.sband_comm_node_ingress_diagnostics,
            )
            return
        apply_service_override(
            self.subsystem_target,
            self.sband_comm_service,
            self.sband_ingress_diagnostics_override_dropin_name,
            {"COMM_NODE_INGRESS_DIAGNOSTICS": self.sband_comm_node_ingress_diagnostics},
        )
        self.sband_ingress_diagnostics_override_applied = True
        wait_service_active(self.subsystem_target, self.sband_comm_service, self.restart_timeout)
        self.snapshot_service(self.subsystem_target, self.sband_comm_service, "sband-ingress-diagnostics-override-applied")
        self.wait_for_target_ready_for_comm(self.require_external_uhf_service)
        self.checkpoint(
            "sband-ingress-diagnostics-override-applied",
            "pass",
            service=self.sband_comm_service,
            value=self.sband_comm_node_ingress_diagnostics,
            override_dropin=self.sband_ingress_diagnostics_override_dropin_name,
        )

    def wait_for_sband_ground_readiness(self, timeout: float) -> str:
        deadline = time.time() + timeout
        downlink_capture = self.capture_path(self.sband, "southbound-to-gds")
        gateway_opened = False
        gateway_opened_since: float | None = None
        while time.time() < deadline:
            if self.sband.gateway_log.exists():
                gateway_text = self.sband.gateway_log.read_text(encoding="utf-8", errors="replace")
                if "southbound-opened mode=tcp-client" in gateway_text:
                    gateway_opened = True
                    if gateway_opened_since is None:
                        gateway_opened_since = time.time()
            if self.sband.latest_link_state() == "UP":
                return "link-up"
            if self.sband.events_log.exists():
                lines = self.sband.events_log.read_text(encoding="utf-8", errors="replace").splitlines()
                for line in lines[1:]:
                    if "OBCApp." in line or "CdhCore." in line:
                        return "event-stream"
            if downlink_capture.exists() and downlink_capture.stat().st_size > 0:
                return "downlink-bytes"
            if gateway_opened_since is not None and (time.time() - gateway_opened_since) >= 2.0:
                return "southbound-opened-stable"
            time.sleep(0.25)
        raise ProbeFailure(
            "S-band ground helper did not observe link-up, target event-stream, or downlink bytes before auth bootstrap"
            f" (southbound-opened={gateway_opened})"
        )

    def wait_for_sband_tcp_reachability(self, timeout: float) -> None:
        deadline = time.time() + timeout
        last_error = "unreached"
        while time.time() < deadline:
            try:
                with socket.create_connection((self.sband_tcp_host, self.sband_tcp_port), timeout=2.0):
                    return
            except OSError as exc:
                last_error = str(exc)
                time.sleep(0.5)
        raise ProbeFailure(
            f"S-band southbound listener {self.sband_tcp_host}:{self.sband_tcp_port} was not reachable from the ground host before auth bootstrap: {last_error}"
        )

    def ensure_sband_ground_ready(self) -> str:
        last_error: ProbeFailure | None = None
        for attempt in range(3):
            try:
                readiness = self.wait_for_sband_ground_readiness(20.0)
                self.checkpoint(
                    "sband-ground-readiness",
                    "pass",
                    attempt=attempt + 1,
                    readiness=readiness,
                )
                return readiness
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    "sband-ground-readiness",
                    "fail",
                    attempt=attempt + 1,
                    error=str(exc),
                )
                self.sband.force_stop()
                if attempt == 2:
                    break
                shutil.rmtree(self.sband.root, ignore_errors=True)
                self.sband.gds_port = free_port()
                self.sband.gds_tts_port = free_port()
                time.sleep(1.0)
                self.start_ground_paths(need_sband=True, need_uhf=False)
        assert last_error is not None
        raise last_error

    def write_summary(
        self,
        *,
        case_results: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": "secure-auth-command-path",
            "profile": "sband",
            "verdict": "FAIL" if failure else "PASS",
            "targetPathUnderTest": "entry-59 macOS fprime-gds + ground_ttc_gateway -> subsystem.local node 5 -> SocketCAN -> obc.local",
            "cases": case_results,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "gatewayCaptures": self.secure_capture_summary(),
                "obcJournal": str(self.journal_snapshot_dir / "secure-auth-command-path-obc.log"),
                "sbandServiceJournal": str(self.journal_snapshot_dir / "secure-auth-command-path-sband-service.log"),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
            "residualNonClaims": [
                "No UHF backup or UHF primary command/file behavior is claimed by this preflight.",
                "No staged-upload, sequence, reliable-transfer, encryption, RF, or one-GDS aggregation claim is made.",
                "This is a current secure-auth command-path preflight only; it does not replace the full target secure-auth proof.",
            ],
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "secure-auth-command-path-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def run_extra_readback_case(self, sband_session, case_name: str) -> tuple[dict[str, object], list[str]]:
        if case_name == "hw-watchdog-status":
            ground_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("HW_WATCHDOG_STATUS", "timeout 15"),
                source_name="ground-native-event-log",
            )
            outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-secure-get-hw-watchdog-status-command-path",
                "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
                accept_sequence=True,
                journal_fragments=("HW_WATCHDOG_STATUS", "timeout 15"),
                ground_success_oracle=ground_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            return (
                {
                    "case": "sband-secure-command-get-hw-watchdog-status",
                    "verdict": "PASS",
                    **outcome,
                },
                [
                    f"case-sband-secure-command-get-hw-watchdog-status-sequence={outcome['acceptedSequence']}",
                    f"sband-secure-command-get-hw-watchdog-status-source={outcome['source']}",
                    f"sband-secure-command-get-hw-watchdog-status-attempt-count={outcome['attemptCount']}",
                ],
            )
        if case_name == "watchdog-status":
            ground_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("WATCHDOG_STATUS", "WATCHDOG_SOURCE_STATUS"),
                source_name="ground-native-event-log",
            )
            outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-secure-get-watchdog-status-command-path",
                "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS",
                accept_sequence=True,
                journal_fragments=("WATCHDOG_STATUS", "WATCHDOG_SOURCE_STATUS"),
                ground_success_oracle=ground_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            return (
                {
                    "case": "sband-secure-command-get-watchdog-status",
                    "verdict": "PASS",
                    **outcome,
                },
                [
                    f"case-sband-secure-command-get-watchdog-status-sequence={outcome['acceptedSequence']}",
                    f"sband-secure-command-get-watchdog-status-source={outcome['source']}",
                    f"sband-secure-command-get-watchdog-status-attempt-count={outcome['attemptCount']}",
                ],
            )
        if case_name == "persistent-fault-history":
            history_limit = os.getenv("TARGET_SECURE_AUTH_PERSISTENT_FAULT_HISTORY_LIMIT", "8")
            ground_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("PERSISTENT_FAULT_HISTORY_STATUS", "PERSISTENT_FAULT_HISTORY_RECORD"),
                source_name="ground-native-event-log",
            )
            outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-secure-get-persistent-fault-history-command-path",
                "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                history_limit,
                accept_sequence=True,
                journal_fragments=("PERSISTENT_FAULT_HISTORY_STATUS", "PERSISTENT_FAULT_HISTORY_RECORD"),
                ground_success_oracle=ground_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            return (
                {
                    "case": "sband-secure-command-get-persistent-fault-history",
                    "verdict": "PASS",
                    "limit": int(history_limit),
                    **outcome,
                },
                [
                    f"case-sband-secure-command-get-persistent-fault-history-sequence={outcome['acceptedSequence']}",
                    f"sband-secure-command-get-persistent-fault-history-source={outcome['source']}",
                    f"sband-secure-command-get-persistent-fault-history-attempt-count={outcome['attemptCount']}",
                ],
            )
        raise ProbeFailure(f"unsupported extra readback case {case_name}")

    def run_command_path_preflight(self) -> list[str]:
        summary: list[str] = []
        case_results: list[dict[str, object]] = []
        failure_stage = "secure-auth-command-bootstrap"
        try:
            failure_stage = "secure-auth-command-begin-profile"
            self.begin_profile()

            failure_stage = "secure-auth-command-provenance"
            self.record_secure_auth_provenance()
            case_results.append({"case": "installed-release-keystore-provenance", "verdict": "PASS"})
            summary.append("case-installed-release-keystore-provenance=PASS")

            failure_stage = "secure-auth-command-server-start"
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
            summary.append(f"sband-ground-readiness={readiness}")
            self.prepare_ground_window(self.sband, timeout=8.0)

            malformed_verdict = "SKIPPED"
            malformed_error = ""
            if self.include_malformed_advisory:
                malformed_verdict = "PASS"
                failure_stage = "secure-auth-command-malformed-handshake"
                try:
                    self.verify_malformed_secure_handshake_fail_closed(self.sband)
                except ProbeFailure as malformed_exc:
                    malformed_verdict = "DEGRADED"
                    malformed_error = str(malformed_exc)
                    self.checkpoint(
                        "secure-auth-command-malformed-handshake-advisory",
                        "fail",
                        error=malformed_error,
                    )
            malformed_case: dict[str, object] = {
                "case": "sband-malformed-handshake-fail-closed-advisory",
                "verdict": malformed_verdict,
            }
            if malformed_error:
                malformed_case["error"] = malformed_error
            case_results.append(malformed_case)
            summary.append(f"case-sband-malformed-handshake-fail-closed-advisory={malformed_verdict}")

            failure_stage = "secure-auth-command-authenticate"
            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            case_results.append({"case": "sband-apid-00fe-secure-auth", "verdict": "PASS"})
            summary.append("case-sband-apid-00fe-secure-auth=PASS")

            failure_stage = "secure-auth-command-get-reset-cause"
            ground_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("BOOT_RECOVERY_STATUS",),
                source_name="ground-native-event-log",
            )
            outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-secure-get-reset-cause-command-path",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_success_oracle=ground_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            case_results.append(
                {
                    "case": "sband-secure-command-get-reset-cause",
                    "verdict": "PASS",
                    **outcome,
                }
            )
            summary.append(f"case-sband-secure-command-get-reset-cause-sequence={outcome['acceptedSequence']}")
            summary.append(f"sband-secure-command-source={outcome['source']}")
            summary.append(f"sband-secure-command-attempt-count={outcome['attemptCount']}")
            for case_name in parse_extra_readbacks():
                failure_stage = f"secure-auth-command-{case_name}"
                extra_case, extra_summary_lines = self.run_extra_readback_case(sband_session, case_name)
                case_results.append(extra_case)
                summary.extend(extra_summary_lines)

            self.snapshot_journal(self.obc_target, self.obc_service, "secure-auth-command-path-obc", lines=400)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "secure-auth-command-path-sband-service", lines=240)
            summary_path = self.write_summary(case_results=case_results)
            summary.append(f"secure-auth-command-path-summary={summary_path}")
            summary.append("target-secure-auth-command-path=PASS")
            ensure_no_legacy_aliases(self.root_dir)
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "secure-auth-command-path-obc", lines=400)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "secure-auth-command-path-sband-service", lines=240)
            summary_path = self.write_summary(
                case_results=case_results,
                failure={
                    "stage": failure_stage,
                    "error": str(exc),
                    **({"retryDetails": getattr(exc, "retry_details")} if hasattr(exc, "retry_details") else {}),
                },
            )
            self.checkpoint(
                "secure-auth-command-path-summary-written",
                "fail",
                artifact=str(summary_path),
                stage=failure_stage,
                error=str(exc),
            )
            raise

    def cleanup(self, run_error: Exception | None) -> None:
        cleanup_errors: list[str] = []

        def cleanup_step(label: str, action) -> None:
            try:
                action()
                self.checkpoint(label, "pass")
            except Exception as exc:  # pragma: no cover - target cleanup best effort
                cleanup_errors.append(f"{label}: {exc}")
                self.checkpoint(label, "fail", error=str(exc))

        if self.uhf_ingress_diagnostics_override_applied:
            def remove_uhf_ingress_diagnostics_override() -> None:
                remove_service_override(
                    self.subsystem_target,
                    self.uhf_comm_service,
                    self.uhf_ingress_diagnostics_override_dropin_name,
                )
                wait_service_active(self.subsystem_target, self.uhf_comm_service, self.restart_timeout)
                self.uhf_ingress_diagnostics_override_applied = False

            cleanup_step("uhf-ingress-diagnostics-override-removed", remove_uhf_ingress_diagnostics_override)

        if self.sband_ingress_diagnostics_override_applied:
            def remove_sband_ingress_diagnostics_override() -> None:
                remove_service_override(
                    self.subsystem_target,
                    self.sband_comm_service,
                    self.sband_ingress_diagnostics_override_dropin_name,
                )
                wait_service_active(self.subsystem_target, self.sband_comm_service, self.restart_timeout)
                self.sband_ingress_diagnostics_override_applied = False

            cleanup_step("sband-ingress-diagnostics-override-removed", remove_sband_ingress_diagnostics_override)

        cleanup_step("remote-beacon-capture-stopped", self.stop_remote_beacon_capture)

        if self.obc_groundlink_diagnostics_override_applied:
            def remove_obc_groundlink_diagnostics_override() -> None:
                remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.obc_groundlink_diagnostics_override_dropin_name,
                )
                wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.obc_groundlink_diagnostics_override_applied = False

            cleanup_step("obc-groundlink-diagnostics-override-removed", remove_obc_groundlink_diagnostics_override)

        if self.obc_groundlink_timeout_override_applied:
            def remove_obc_groundlink_timeout_override() -> None:
                remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.obc_groundlink_timeout_override_dropin_name,
                )
                wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.obc_groundlink_timeout_override_applied = False

            cleanup_step("obc-groundlink-timeout-override-removed", remove_obc_groundlink_timeout_override)

        if self.csp_socketcan_canfd_override_applied:
            def remove_csp_socketcan_canfd_override() -> None:
                subsystem_services = (
                    self.sband_comm_service,
                    self.uhf_comm_service,
                )
                for service_name in subsystem_services:
                    remove_service_override(
                        self.subsystem_target,
                        service_name,
                        self.csp_socketcan_canfd_override_dropin_name,
                    )
                    wait_service_active(self.subsystem_target, service_name, self.restart_timeout)
                remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.csp_socketcan_canfd_override_dropin_name,
                )
                wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.csp_socketcan_canfd_override_applied = False

            cleanup_step("csp-socketcan-canfd-override-removed", remove_csp_socketcan_canfd_override)

        if self.quiet_override_applied:
            cleanup_step("quiet-override-removed", self.remove_quiet_override)

        if self.profile_override_applied:
            def remove_profile_override() -> None:
                remove_service_override(self.obc_target, self.obc_service, self.profile_override_dropin_name)
                wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.profile_override_applied = False

            cleanup_step("profile-override-removed", remove_profile_override)

        cleanup_step("subsystem-environment-restored", self.restore_subsystem_environment)
        cleanup_step("sband-ground-stopped", self.sband.stop)
        cleanup_step("uhf-ground-stopped", self.uhf.stop)
        self.write_json_artifact(
            self.diagnostics_dir / "cleanup-status.json",
            {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "mode": "secure-auth-command-path",
                "profile": "sband",
                "verdict": "PASS" if not cleanup_errors else "FAIL",
                "errors": cleanup_errors,
            },
        )

        if cleanup_errors and run_error is None:
            raise ProbeFailure("cleanup failed: " + "; ".join(cleanup_errors))


def parse_extra_readbacks() -> tuple[str, ...]:
    value = os.getenv("TARGET_SECURE_AUTH_EXTRA_READBACKS", "")
    if not value.strip():
        return ()
    items = tuple(item.strip() for item in value.split(",") if item.strip())
    allowed = {"hw-watchdog-status", "watchdog-status", "persistent-fault-history"}
    invalid = [item for item in items if item not in allowed]
    if invalid:
        raise ProbeFailure(f"unsupported TARGET_SECURE_AUTH_EXTRA_READBACKS entries: {invalid}")
    return items


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    scenario = SecureAuthCommandPathScenario(pathlib.Path(args.probe_root))
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        summary_lines = [
            "target-secure-auth-command-path-probe: PASS",
            f"probe-root={args.probe_root}",
        ]
        summary_lines.extend(scenario.run_command_path_preflight())
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"target-secure-auth-command-path-probe: FAIL {exc}\n"
        exit_code = 1
    except Exception as exc:  # pragma: no cover - top-level trap
        run_error = exc
        output = "target-secure-auth-command-path-probe: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"target-secure-auth-command-path-probe: FAIL cleanup {cleanup_exc}\n"
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
