#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import json
import os
import pathlib
import re
import statistics
import subprocess
import sys
import time
import traceback
from dataclasses import dataclass

import run_target_can_matrix_probe as target_probe
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario
from run_target_can_matrix_probe import ensure_no_legacy_aliases


ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
DEFAULT_TARGET_BASELINE = ROOT_DIR / "scripts" / "ensure_target_comm_lab_baseline.sh"
DEFAULT_GROUND_BASELINE = ROOT_DIR / "scripts" / "ensure_ground_dual_gds_baseline.sh"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    parser.add_argument("--repetitions", type=int, default=3)
    return parser.parse_args()


def median_or_zero(values: list[int]) -> float:
    return float(statistics.median(values)) if values else 0.0


def max_or_zero(values: list[int]) -> int:
    return max(values) if values else 0


def median_or_none(values: list[int | float]) -> float | None:
    return float(statistics.median(values)) if values else None


def max_or_none(values: list[int | float]) -> int | float | None:
    return max(values) if values else None


def read_json(path: pathlib.Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def run_logged_command(
    *,
    args: list[str],
    cwd: pathlib.Path,
    env: dict[str, str],
    log_path: pathlib.Path,
    timeout_sec: int,
) -> tuple[bool, int | None, bool]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as handle:
        handle.write(f"$ {' '.join(args)}\n")
        handle.flush()
        try:
            result = subprocess.run(
                args,
                cwd=str(cwd),
                env=env,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=timeout_sec,
                check=False,
            )
            handle.write(f"\nreturncode={result.returncode}\n")
            return result.returncode == 0, result.returncode, False
        except subprocess.TimeoutExpired:
            handle.write(f"\ntimed out after {timeout_sec}s\n")
            return False, None, True


@dataclass
class BaselineResult:
    ready: bool
    payload: dict[str, object]
    log_path: str


class SecureLiveCommandScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path, *, case_name: str, nonquiet: bool) -> None:
        super().__init__(probe_root)
        self.mode = "uhf-primary-secure-live-benchmark"
        self.profile = "sband"
        self.case_name = case_name
        self.nonquiet = nonquiet
        self.disable_packet_quiet_dropin_name = os.getenv(
            "DISABLE_UHF_PRIMARY_PACKET_QUIET_DROPIN_NAME",
            "57-uhf-primary-live-packets.conf",
        )
        self.disable_packet_quiet_override_applied = False
        self.switched_to_uhf = False
        self.uhf_primary_session: target_probe.SecureAuthSession | None = None
        self.configure_ground_artifact_layout(self.sband)
        self.configure_ground_artifact_layout(self.uhf)

    @staticmethod
    def configure_ground_artifact_layout(ground: target_probe.GroundPath) -> None:
        ground.gds_runtime_dir = ground.root / "gds-runtime"
        ground.file_storage = ground.gds_runtime_dir / "gds-files"
        ground.gds_log = ground.gds_runtime_dir / "gds.log"
        ground.gateway_log = ground.gds_runtime_dir / "gateway.log"
        ground.process_control_dir = ground.root / "process-control"
        ground.events_log = ground.process_control_dir / "passive-cli-stdout.log"
        ground.channels_log = ground.process_control_dir / "control-query-stdout.log"
        ground.passive_cli_control_log = ground.process_control_dir / "passive-cli.log"
        ground.control_query_log = ground.process_control_dir / "control-queries.log"
        ground.raw_command_log = ground.process_control_dir / "raw-command.log"
        ground.cli_log_dir = ground.root / "native-cli"
        ground.passive_cli_log_dir = ground.cli_log_dir

    def gateway_capture_dir_for_ground(self, ground: target_probe.GroundPath) -> pathlib.Path | None:
        link_name = "sband" if ground is self.sband else "uhf"
        capture_dir = self.capture_dir / link_name
        capture_dir.mkdir(parents=True, exist_ok=True)
        return capture_dir

    def prepare_case_baseline(self) -> None:
        self.checkpoint(
            "benchmark-case-begin",
            "info",
            case=self.case_name,
            nonquiet=self.nonquiet,
            mode=self.mode,
            profile=self.profile,
            comm_baudrate=self.comm_baudrate,
        )
        self.checkpoint(
            "benchmark-external-baseline-assumed-ready",
            "pass",
            reason="A/B baseline managers completed before this case",
        )
        self.apply_obc_groundlink_diagnostics_override()
        if self.nonquiet:
            self.apply_disable_packet_quiet_override()
        self.wait_for_target_ready_for_comm(False)

    def prepare_sband_secure_auth_case(self) -> None:
        self.prepare_case_baseline()
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
        self.ensure_sband_ground_ready()
        self.prepare_ground_window(self.sband)

    @staticmethod
    def parse_channel_stream_log(path: pathlib.Path) -> dict[str, object]:
        latest: dict[str, object] = {}
        if not path.exists():
            return latest
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            if not line or line.startswith("$") or line.startswith("["):
                continue
            parts = line.split(",", 4)
            if len(parts) < 5:
                continue
            channel_name = parts[2].strip()
            raw_value = parts[4].strip()
            value: object = raw_value
            if raw_value.startswith("[") and raw_value.endswith("]"):
                try:
                    value = ast.literal_eval(raw_value)
                except Exception:
                    value = raw_value
            else:
                try:
                    value = int(raw_value)
                except ValueError:
                    try:
                        value = float(raw_value)
                    except ValueError:
                        value = raw_value
            latest[channel_name] = value
        return latest

    def collect_sband_channel_state(self) -> dict[str, object]:
        latest = self.parse_channel_stream_log(self.sband.native_channel_log)
        return self._collect_channel_state(latest)

    def collect_uhf_channel_state(self) -> dict[str, object]:
        latest = self.parse_channel_stream_log(self.uhf.native_channel_log)
        return self._collect_channel_state(latest)

    def _collect_channel_state(self, latest: dict[str, object]) -> dict[str, object]:
        interesting = {
            "uhfGroundLinkRxErrors": "OBCApp.uhfGroundLinkDriver.GROUND_LINK_RX_ERRORS",
            "uhfGroundLinkTxErrors": "OBCApp.uhfGroundLinkDriver.GROUND_LINK_TX_ERRORS",
            "uartRxErrors": "OBCApp.uartDriver.UART_RX_ERRORS",
            "uartTxErrors": "OBCApp.uartDriver.UART_TX_ERRORS",
            "rateGroupCycleSlips": "OBCApp.rateGroup1Comp.RgCycleSlips",
            "comQueueDepth": "ComCcsds.comQueue.comQueueDepth",
            "buffQueueDepth": "ComCcsds.comQueue.buffQueueDepth",
            "primaryCommandLink": "OBCApp.commController.COMM_PRIMARY_COMMAND_LINK",
            "sysMode": "OBCApp.modeManager.SYS_MODE",
        }
        snapshots: dict[str, object] = {}
        values: dict[str, object] = {}
        for key, channel_name in interesting.items():
            matched = channel_name in latest
            value = latest.get(channel_name)
            snapshots[key] = {
                "label": f"{self.case_name}-{key}",
                "search": channel_name,
                "matched": matched,
                "matches": [f"{channel_name}={value}"] if matched else [],
                "value": value,
                "returncode": 0 if matched else None,
                "timedOut": False,
                "parsedJson": None,
            }
            values[key] = value
        return {"channelSnapshots": snapshots, "values": values}

    def collect_ground_native_metrics(
        self,
        ground: target_probe.GroundPath,
        *,
        capture_summary: dict[str, object],
    ) -> dict[str, object]:
        latest = self.parse_channel_stream_log(ground.native_channel_log)
        events_text = target_probe.read_text(ground.native_event_log)
        gds_log_text = target_probe.read_text(ground.gds_log)
        ground_link_prefix = "OBCApp.uhfGroundLinkDriver" if ground is self.uhf else "OBCApp.groundLinkDriver"
        transport_visible = (
            capture_summary["southboundToGds"].get("present", False)
            and int(capture_summary["southboundToGds"].get("size", 0) or 0) > 0
        )
        return {
            "eventVisible": bool(re.search(r"^\d{4}-\d{2}-\d{2}T.*OBCApp\.", events_text, re.MULTILINE)),
            "channelVisible": bool(latest),
            "transportVisible": transport_visible,
            "checksumWarningCount": gds_log_text.count("Checksum validation failed"),
            "groundLinkUpCount": events_text.count(ground.link_up_fragment),
            "groundLinkDownCount": events_text.count(ground.link_down_fragment),
            "groundLinkChurnCount": min(
                events_text.count(ground.link_up_fragment),
                events_text.count(ground.link_down_fragment),
            ),
            "groundLinkRxErrors": latest.get(f"{ground_link_prefix}.GROUND_LINK_RX_ERRORS"),
            "groundLinkTxErrors": latest.get(f"{ground_link_prefix}.GROUND_LINK_TX_ERRORS"),
            "uartRxErrors": latest.get("OBCApp.uartDriver.UART_RX_ERRORS"),
            "uartTxErrors": latest.get("OBCApp.uartDriver.UART_TX_ERRORS"),
            "rateGroupCycleSlipCount": events_text.count("RateGroupCycleSlip"),
        }

    def secure_switch_to_uhf_primary(self, sband_session: target_probe.SecureAuthSession) -> tuple[int, str]:
        sequence, source = self.send_secure_command_name(
            self.sband,
            sband_session,
            f"{self.case_name}-sband-secure-switch-to-uhf-primary",
            "OBCApp.commController.COMM_SET_ACTIVE",
            "UHF",
            accept_sequence=True,
            journal_fragments=(
                "COMM_PRIMARY_LINK_CHANGED",
                "command UHF",
                "telemetry UHF",
                "file UHF",
                "reason 1",
            ),
            timeout=35.0,
        )
        self.switched_to_uhf = True
        return sequence, source

    def secure_restore_sband_primary(self, uhf_session: target_probe.SecureAuthSession) -> tuple[int, str]:
        sequence, source = self.send_secure_command_name(
            self.uhf,
            uhf_session,
            f"{self.case_name}-uhf-primary-secure-switch-back-to-sband",
            "OBCApp.commController.COMM_SET_ACTIVE",
            "SBAND",
            accept_sequence=True,
            journal_fragments=(
                "COMM_PRIMARY_LINK_CHANGED",
                "command SBAND",
                "telemetry SBAND",
                "file SBAND",
                "reason 1",
            ),
            timeout=35.0,
        )
        self.switched_to_uhf = False
        return sequence, source

    def apply_disable_packet_quiet_override(self) -> None:
        baseline_env = target_probe.service_environment(self.obc_target, self.obc_service)
        if baseline_env.get("DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET", "0") not in ("", "0"):
            if target_probe.service_override_exists(
                self.obc_target,
                self.obc_service,
                self.disable_packet_quiet_dropin_name,
            ):
                target_probe.remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.disable_packet_quiet_dropin_name,
                )
                target_probe.wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                baseline_env = target_probe.service_environment(self.obc_target, self.obc_service)
            if baseline_env.get("DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET", "0") not in ("", "0"):
                raise target_probe.ProbeFailure(
                    "target service already has DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET enabled"
                )
        target_probe.apply_service_override(
            self.obc_target,
            self.obc_service,
            self.disable_packet_quiet_dropin_name,
            {"DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET": "1"},
        )
        self.disable_packet_quiet_override_applied = True
        target_probe.wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "uhf-primary-live-packets-override-applied")
        self.checkpoint(
            "uhf-primary-live-packets-override-applied",
            "pass",
            service=self.obc_service,
            override_dropin=self.disable_packet_quiet_dropin_name,
        )

    def remove_disable_packet_quiet_override(self) -> None:
        if not self.disable_packet_quiet_override_applied:
            return
        target_probe.remove_service_override(
            self.obc_target,
            self.obc_service,
            self.disable_packet_quiet_dropin_name,
        )
        target_probe.wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        restored_env = target_probe.service_environment(self.obc_target, self.obc_service)
        if restored_env.get("DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET", "0") not in ("", "0"):
            raise target_probe.ProbeFailure(
                "failed to restore DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET baseline"
            )
        self.snapshot_service(self.obc_target, self.obc_service, "uhf-primary-live-packets-override-removed")
        self.checkpoint(
            "uhf-primary-live-packets-override-removed",
            "pass",
            service=self.obc_service,
            override_dropin=self.disable_packet_quiet_dropin_name,
        )
        self.disable_packet_quiet_override_applied = False

    def collect_runtime_metrics(self) -> dict[str, object]:
        channel_state = self.collect_uhf_channel_state()
        channel_queries = channel_state["channelSnapshots"]
        formal = self.summarize_nonquiet_formal_cleanliness()
        capture_summary = {
            "sband": self.secure_capture_summary()["sband"],
            "uhf": self.secure_capture_summary()["uhf"],
        }
        sband_metrics = self.collect_ground_native_metrics(self.sband, capture_summary=capture_summary["sband"])
        uhf_metrics = self.collect_ground_native_metrics(self.uhf, capture_summary=capture_summary["uhf"])
        active_ground_path = "uhf" if self.nonquiet else "sband"
        active_ground_metrics = uhf_metrics if self.nonquiet else sband_metrics
        return {
            "activeGroundPath": active_ground_path,
            "eventVisible": bool(active_ground_metrics["eventVisible"]),
            "channelVisible": bool(active_ground_metrics["channelVisible"]),
            "transportVisible": bool(active_ground_metrics["transportVisible"]),
            "channelSnapshots": channel_queries,
            "captureSummary": capture_summary,
            "groundPaths": {
                "sband": sband_metrics,
                "uhf": uhf_metrics,
            },
            "noise": {
                "checksumWarningCount": int(active_ground_metrics["checksumWarningCount"] or 0),
                "groundLinkChurnCount": int(active_ground_metrics["groundLinkChurnCount"] or 0),
                "groundLinkUpCount": int(active_ground_metrics["groundLinkUpCount"] or 0),
                "groundLinkDownCount": int(active_ground_metrics["groundLinkDownCount"] or 0),
                "groundLinkRxErrors": active_ground_metrics.get("groundLinkRxErrors"),
                "groundLinkTxErrors": active_ground_metrics.get("groundLinkTxErrors"),
                "uartRxErrors": active_ground_metrics.get("uartRxErrors"),
                "uartTxErrors": active_ground_metrics.get("uartTxErrors"),
                "rateGroupCycleSlipCount": int(active_ground_metrics["rateGroupCycleSlipCount"] or 0),
            },
            "auxiliaryUhfFormalLinkCleanliness": formal,
        }

    def best_effort_restore_sband_primary(self) -> None:
        if not self.switched_to_uhf:
            return
        session = self.uhf_primary_session
        if session is None:
            session = self.authenticate_secure_service(
                self.uhf,
                service_id=target_probe.SERVICE_ID_UHF,
                ingress_port=1,
                role_fragment="identity 2 role 3",
                initial_sequence=91,
            )
            self.uhf_primary_session = session
            self.checkpoint("benchmark-cleanup-uhf-reauth", "pass")
        self.secure_restore_sband_primary(session)
        self.checkpoint("benchmark-cleanup-restore-sband", "pass")

    def cleanup(self, run_error: Exception | None) -> None:
        cleanup_errors: list[str] = []

        def cleanup_step(label: str, action) -> None:
            try:
                action()
                self.checkpoint(label, "pass")
            except Exception as exc:  # pragma: no cover - cleanup is best effort
                cleanup_errors.append(f"{label}: {exc}")
                self.checkpoint(label, "fail", error=str(exc))

        if self.switched_to_uhf:
            cleanup_step("benchmark-cleanup-restore-sband", self.best_effort_restore_sband_primary)

        if self.disable_packet_quiet_override_applied:
            cleanup_step("uhf-primary-live-packets-override-removed", self.remove_disable_packet_quiet_override)

        if self.uhf_ingress_diagnostics_override_applied:
            def remove_uhf_ingress() -> None:
                target_probe.remove_service_override(
                    self.subsystem_target,
                    self.uhf_comm_service,
                    self.uhf_ingress_diagnostics_override_dropin_name,
                )
                target_probe.wait_service_active(self.subsystem_target, self.uhf_comm_service, self.restart_timeout)
                self.uhf_ingress_diagnostics_override_applied = False

            cleanup_step("uhf-ingress-diagnostics-override-removed", remove_uhf_ingress)

        if self.sband_ingress_diagnostics_override_applied:
            def remove_sband_ingress() -> None:
                target_probe.remove_service_override(
                    self.subsystem_target,
                    self.sband_comm_service,
                    self.sband_ingress_diagnostics_override_dropin_name,
                )
                target_probe.wait_service_active(self.subsystem_target, self.sband_comm_service, self.restart_timeout)
                self.sband_ingress_diagnostics_override_applied = False

            cleanup_step("sband-ingress-diagnostics-override-removed", remove_sband_ingress)

        if self.obc_groundlink_diagnostics_override_applied:
            def remove_obc_diag() -> None:
                target_probe.remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.obc_groundlink_diagnostics_override_dropin_name,
                )
                target_probe.wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.obc_groundlink_diagnostics_override_applied = False

            cleanup_step("obc-groundlink-diagnostics-override-removed", remove_obc_diag)

        if self.obc_groundlink_timeout_override_applied:
            def remove_obc_timeout() -> None:
                target_probe.remove_service_override(
                    self.obc_target,
                    self.obc_service,
                    self.obc_groundlink_timeout_override_dropin_name,
                )
                target_probe.wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                self.obc_groundlink_timeout_override_applied = False

            cleanup_step("obc-groundlink-timeout-override-removed", remove_obc_timeout)

        cleanup_step("subsystem-environment-restored", self.restore_subsystem_environment)
        cleanup_step("sband-ground-stopped", self.sband.stop)
        cleanup_step("uhf-ground-stopped", self.uhf.stop)
        self.write_json_artifact(
            self.diagnostics_dir / "cleanup-status.json",
            {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                "mode": "uhf-primary-secure-live-benchmark",
                "case": self.case_name,
                "nonquiet": self.nonquiet,
                "verdict": "PASS" if not cleanup_errors else "FAIL",
                "errors": cleanup_errors,
            },
        )
        if cleanup_errors and run_error is None:
            raise target_probe.ProbeFailure("cleanup failed: " + "; ".join(cleanup_errors))

    def run_command_case(self) -> dict[str, object]:
        result: dict[str, object] = {
            "case": self.case_name,
            "nonquiet": self.nonquiet,
        }
        run_error: Exception | None = None
        try:
            self.start()
            self.prepare_sband_secure_auth_case()
            if self.include_malformed_advisory:
                self.verify_malformed_secure_handshake_fail_closed(self.sband)

            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=target_probe.SERVICE_ID_SBAND,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            sband_sequence, sband_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                f"{self.case_name}-sband-secure-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )

            self.apply_uhf_ingress_diagnostics_override()
            self.prepare_uhf_service_for_switch(
                require_pre_switch_ping=False,
                boundary=f"{self.case_name}-switch-boundary",
            )
            self.start_ground_paths(need_sband=False, need_uhf=True)
            switch_sequence, switch_source = self.secure_switch_to_uhf_primary(sband_session)

            uhf_session = self.authenticate_secure_service(
                self.uhf,
                service_id=target_probe.SERVICE_ID_UHF,
                ingress_port=1,
                role_fragment="identity 2 role 3",
                initial_sequence=41,
            )
            self.uhf_primary_session = uhf_session
            uhf_sequence, uhf_source = self.send_secure_command_name(
                self.uhf,
                uhf_session,
                f"{self.case_name}-uhf-primary-secure-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )

            metrics = self.collect_runtime_metrics()
            restore_sequence, restore_source = self.secure_restore_sband_primary(uhf_session)

            observability = (
                "PASS"
                if metrics["eventVisible"] and metrics["channelVisible"] and metrics["transportVisible"]
                else ("DEGRADED" if metrics["transportVisible"] else "FAIL")
            )
            noise = metrics["noise"]
            noise_verdict = (
                "CLEAN"
                if noise["checksumWarningCount"] == 0
                and noise["groundLinkChurnCount"] == 0
                and noise["rateGroupCycleSlipCount"] == 0
                else "NOISY"
            )

            result.update(
                {
                    "targetRuntimeVerdict": "PASS",
                    "groundObservabilityVerdict": observability,
                    "noiseVerdict": noise_verdict,
                    "authoritativeTruthSource": "target-journal",
                    "targetJournalRequired": True,
                    "liveEventVisible": metrics["eventVisible"],
                    "liveChannelVisible": metrics["channelVisible"],
                    "captureSummary": metrics["captureSummary"],
                    "metrics": metrics,
                    "sequences": {
                        "sbandGetResetCause": sband_sequence,
                        "switchToUhf": switch_sequence,
                        "uhfGetResetCause": uhf_sequence,
                        "restoreSband": restore_sequence,
                    },
                    "sources": {
                        "sbandCommand": sband_source,
                        "switch": switch_source,
                        "uhfCommand": uhf_source,
                        "restore": restore_source,
                    },
                    "artifacts": {
                        "checkpoints": str(self.checkpoints_log),
                        "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
                        "sbandNativeCliDir": str(self.sband.passive_cli_log_dir),
                        "sbandPassiveCliControlLog": str(self.sband.passive_cli_control_log),
                        "sbandControlQueryLog": str(self.sband.control_query_log),
                        "sbandGatewayLog": str(self.sband.gateway_log),
                        "sbandGdsLog": str(self.sband.gds_log),
                        "uhfNativeCliDir": str(self.uhf.passive_cli_log_dir),
                        "uhfPassiveCliControlLog": str(self.uhf.passive_cli_control_log),
                        "uhfControlQueryLog": str(self.uhf.control_query_log),
                        "uhfGatewayLog": str(self.uhf.gateway_log),
                        "uhfGdsLog": str(self.uhf.gds_log),
                    },
                }
            )
            return result
        except Exception as exc:
            run_error = exc
            best_effort_metrics: dict[str, object] | None = None
            try:
                if self.uhf.root.exists():
                    best_effort_metrics = self.collect_runtime_metrics()
            except Exception:
                best_effort_metrics = None
            result.update(
                {
                    "targetRuntimeVerdict": "FAIL",
                    "authoritativeTruthSource": "target-journal",
                    "targetJournalRequired": True,
                    "error": str(exc),
                    "traceback": traceback.format_exc(),
                    "metrics": best_effort_metrics or {},
                    "captureSummary": (best_effort_metrics or {}).get("captureSummary", {}),
                    "liveEventVisible": (best_effort_metrics or {}).get("eventVisible", False),
                    "liveChannelVisible": (best_effort_metrics or {}).get("channelVisible", False),
                }
            )
            if best_effort_metrics:
                result["groundObservabilityVerdict"] = (
                    "PASS"
                    if best_effort_metrics.get("eventVisible")
                    and best_effort_metrics.get("channelVisible")
                    and best_effort_metrics.get("transportVisible")
                    else ("DEGRADED" if best_effort_metrics.get("transportVisible") else "FAIL")
                )
                noise = best_effort_metrics.get("noise", {})
                result["noiseVerdict"] = (
                    "CLEAN"
                    if int(noise.get("checksumWarningCount", 0) or 0) == 0
                    and int(noise.get("groundLinkChurnCount", 0) or 0) == 0
                    and int(noise.get("rateGroupCycleSlipCount", 0) or 0) == 0
                    else "SEVERE"
                )
            else:
                result["groundObservabilityVerdict"] = "FAIL"
                result["noiseVerdict"] = "SEVERE"
            return result
        finally:
            try:
                self.cleanup(run_error)
                result["cleanupVerdict"] = "PASS"
            except Exception as cleanup_exc:  # pragma: no cover - best effort
                result["cleanupVerdict"] = "FAIL"
                result["cleanupError"] = str(cleanup_exc)
                if run_error is None:
                    result["targetRuntimeVerdict"] = "FAIL"
                    result["groundObservabilityVerdict"] = "FAIL"
                    result["noiseVerdict"] = "SEVERE"
                    result["error"] = f"cleanup failed: {cleanup_exc}"
            self.write_json_artifact(self.probe_root / "result.json", result)
            ensure_no_legacy_aliases(self.root_dir)


class SecureLiveBenchmarkCampaign:
    def __init__(self, probe_root: pathlib.Path, repetitions: int) -> None:
        self.probe_root = probe_root
        self.repetitions = repetitions
        self.target_baseline_script = pathlib.Path(
            os.getenv("ENSURE_TARGET_BASELINE_SCRIPT", str(DEFAULT_TARGET_BASELINE))
        )
        self.ground_baseline_script = pathlib.Path(
            os.getenv("ENSURE_GROUND_BASELINE_SCRIPT", str(DEFAULT_GROUND_BASELINE))
        )

    def run_baseline_script(
        self,
        *,
        script: pathlib.Path,
        json_path: pathlib.Path,
        log_path: pathlib.Path,
        extra_env: dict[str, str] | None = None,
    ) -> BaselineResult:
        env = os.environ.copy()
        env["JSON_OUT"] = str(json_path)
        if extra_env:
            env.update(extra_env)
        ok, returncode, timed_out = run_logged_command(
            args=["bash", str(script)],
            cwd=ROOT_DIR,
            env=env,
            log_path=log_path,
            timeout_sec=180,
        )
        if json_path.exists():
            payload = read_json(json_path)
        else:
            payload = {
                "verdict": "blocked",
                "error": "summary-missing",
                "returncode": returncode,
                "timedOut": timed_out,
            }
        ready = ok and str(payload.get("verdict", "")).lower() in ("ready", "repaired")
        return BaselineResult(ready=ready, payload=payload, log_path=str(log_path))

    def run_preflight(self, repetition_root: pathlib.Path, *, label: str) -> dict[str, object]:
        preflight_dir = repetition_root / "preflight"
        preflight_dir.mkdir(parents=True, exist_ok=True)
        target = self.run_baseline_script(
            script=self.target_baseline_script,
            json_path=preflight_dir / f"target-{label}.json",
            log_path=preflight_dir / f"target-{label}.log",
            extra_env={"TARGET_BASELINE_REQUIRE_UHF_SERVICE": "1"},
        )
        ground = self.run_baseline_script(
            script=self.ground_baseline_script,
            json_path=preflight_dir / f"ground-{label}.json",
            log_path=preflight_dir / f"ground-{label}.log",
            extra_env={"GROUND_BASELINE_EXEMPT_PIDS": str(os.getpid())},
        )
        return {
            "label": label,
            "verdict": "READY" if target.ready and ground.ready else "BLOCKED",
            "target": {"ready": target.ready, "payload": target.payload, "logPath": target.log_path},
            "ground": {"ready": ground.ready, "payload": ground.payload, "logPath": ground.log_path},
        }

    def run_postflight(self, repetition_root: pathlib.Path) -> dict[str, object]:
        target = self.run_baseline_script(
            script=self.target_baseline_script,
            json_path=repetition_root / "target-after.json",
            log_path=repetition_root / "target-after.log",
            extra_env={
                "TARGET_BASELINE_REQUIRE_UHF_SERVICE": "1",
                "TARGET_BASELINE_IGNORE_GPS_STATE_MISSING": "1",
            },
        )
        ground = self.run_baseline_script(
            script=self.ground_baseline_script,
            json_path=repetition_root / "ground-after.json",
            log_path=repetition_root / "ground-after.log",
            extra_env={"GROUND_BASELINE_EXEMPT_PIDS": str(os.getpid())},
        )
        return {
            "verdict": "READY" if target.ready and ground.ready else "BLOCKED",
            "target": {"ready": target.ready, "payload": target.payload, "logPath": target.log_path},
            "ground": {"ready": ground.ready, "payload": ground.payload, "logPath": ground.log_path},
        }

    def run_case(self, repetition_root: pathlib.Path, case_name: str, nonquiet: bool) -> dict[str, object]:
        case_root = repetition_root / case_name
        scenario = SecureLiveCommandScenario(case_root, case_name=case_name, nonquiet=nonquiet)
        target_probe.install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
        return scenario.run_command_case()

    @staticmethod
    def build_case_aggregate(payloads: list[dict[str, object]]) -> dict[str, object]:
        def numeric_values(extractor) -> list[int | float]:
            values: list[int | float] = []
            for payload in payloads:
                value = extractor(payload)
                if isinstance(value, bool):
                    values.append(int(value))
                elif isinstance(value, (int, float)):
                    values.append(value)
            return values

        def capture_sizes(ground: str, direction: str) -> list[int]:
            values: list[int] = []
            for payload in payloads:
                try:
                    size = int(
                        payload.get("captureSummary", {})
                        .get(ground, {})
                        .get(direction, {})
                        .get("size", 0)
                        or 0
                    )
                except Exception:
                    size = 0
                values.append(size)
            return values

        per_ground: dict[str, object] = {}
        for ground in ("sband", "uhf"):
            per_ground[ground] = {
                "eventVisibleCount": sum(
                    1
                    for payload in payloads
                    if payload.get("metrics", {}).get("groundPaths", {}).get(ground, {}).get("eventVisible")
                ),
                "channelVisibleCount": sum(
                    1
                    for payload in payloads
                    if payload.get("metrics", {}).get("groundPaths", {}).get(ground, {}).get("channelVisible")
                ),
                "transportVisibleCount": sum(
                    1
                    for payload in payloads
                    if payload.get("metrics", {}).get("groundPaths", {}).get(ground, {}).get("transportVisible")
                ),
                "captureBytes": {
                    "gdsToSouthbound": {
                        "median": median_or_zero(capture_sizes(ground, "gdsToSouthbound")),
                        "max": max_or_zero(capture_sizes(ground, "gdsToSouthbound")),
                    },
                    "southboundToGds": {
                        "median": median_or_zero(capture_sizes(ground, "southboundToGds")),
                        "max": max_or_zero(capture_sizes(ground, "southboundToGds")),
                    },
                },
                "noiseMetrics": {
                    name: {
                        "median": median_or_none(
                            numeric_values(
                                lambda payload, ground=ground, name=name: payload.get("metrics", {})
                                .get("groundPaths", {})
                                .get(ground, {})
                                .get(name)
                            )
                        ),
                        "max": max_or_none(
                            numeric_values(
                                lambda payload, ground=ground, name=name: payload.get("metrics", {})
                                .get("groundPaths", {})
                                .get(ground, {})
                                .get(name)
                            )
                        ),
                    }
                    for name in (
                        "checksumWarningCount",
                        "groundLinkUpCount",
                        "groundLinkDownCount",
                        "groundLinkChurnCount",
                        "groundLinkRxErrors",
                        "groundLinkTxErrors",
                        "uartRxErrors",
                        "uartTxErrors",
                        "rateGroupCycleSlipCount",
                    )
                },
            }
        return {
            "count": len(payloads),
            "runtimePassCount": sum(1 for payload in payloads if payload.get("targetRuntimeVerdict") == "PASS"),
            "observabilityCounts": {
                verdict: sum(1 for payload in payloads if payload.get("groundObservabilityVerdict") == verdict)
                for verdict in ("PASS", "DEGRADED", "FAIL")
            },
            "activeGroundPath": payloads[0].get("metrics", {}).get("activeGroundPath") if payloads else None,
            "activeEventVisibleCount": sum(1 for payload in payloads if payload.get("liveEventVisible")),
            "activeChannelVisibleCount": sum(1 for payload in payloads if payload.get("liveChannelVisible")),
            "noiseVerdictCounts": {
                verdict: sum(1 for payload in payloads if payload.get("noiseVerdict") == verdict)
                for verdict in ("CLEAN", "NOISY", "SEVERE")
            },
            "perGround": per_ground,
        }

    def build_summary(self, repetitions: list[dict[str, object]]) -> dict[str, object]:
        counted = [item for item in repetitions if item.get("counted", False)]
        quiet_payloads = [
            item["quiet-command"]["payload"]
            for item in counted
            if "quiet-command" in item and isinstance(item["quiet-command"].get("payload"), dict)
        ]
        nonquiet_payloads = [
            item["nonquiet-command"]["payload"]
            for item in counted
            if "nonquiet-command" in item and isinstance(item["nonquiet-command"].get("payload"), dict)
        ]
        noise_values = {
            "checksumWarningCount": [
                int(payload.get("metrics", {}).get("noise", {}).get("checksumWarningCount", 0) or 0)
                for payload in nonquiet_payloads
            ],
            "groundLinkChurnCount": [
                int(payload.get("metrics", {}).get("noise", {}).get("groundLinkChurnCount", 0) or 0)
                for payload in nonquiet_payloads
            ],
            "rateGroupCycleSlipCount": [
                int(payload.get("metrics", {}).get("noise", {}).get("rateGroupCycleSlipCount", 0) or 0)
                for payload in nonquiet_payloads
            ],
        }
        summary = {
            "repetitions": repetitions,
            "aggregate": {
                "requestedRepetitions": self.repetitions,
                "countedRepetitions": len(counted),
                "environmentBlockerCount": sum(1 for item in repetitions if item.get("blocked", False)),
                "quietCommandSuccessCount": sum(
                    1
                    for item in counted
                    if item.get("quiet-command", {}).get("payload", {}).get("targetRuntimeVerdict") == "PASS"
                ),
                "nonquietCommandSuccessCount": sum(
                    1
                    for item in counted
                    if item.get("nonquiet-command", {}).get("payload", {}).get("targetRuntimeVerdict") == "PASS"
                ),
                "liveObservabilityCounts": {
                    verdict: sum(
                        1
                        for payload in nonquiet_payloads
                        if payload.get("groundObservabilityVerdict") == verdict
                    )
                    for verdict in ("PASS", "DEGRADED", "FAIL")
                },
                "noiseMetrics": {
                    name: {
                        "median": median_or_zero(values),
                        "max": max_or_zero(values),
                    }
                    for name, values in noise_values.items()
                },
                "caseComparison": {
                    "quiet": self.build_case_aggregate(quiet_payloads),
                    "nonquiet": self.build_case_aggregate(nonquiet_payloads),
                },
                "nonquietDegradedCommandReliability": any(
                    item.get("quiet-command", {}).get("payload", {}).get("targetRuntimeVerdict") == "PASS"
                    and item.get("nonquiet-command", {}).get("payload", {}).get("targetRuntimeVerdict") != "PASS"
                    for item in counted
                ),
                "nonquietDegradedLiveObservability": any(
                    payload.get("groundObservabilityVerdict") != "PASS"
                    for payload in nonquiet_payloads
                ),
                "nonquietIncreasedLinkNoise": any(
                    payload.get("noiseVerdict") in ("NOISY", "SEVERE")
                    for payload in nonquiet_payloads
                ),
            },
        }
        return summary

    def write_summary_md(self, payload: dict[str, object]) -> None:
        aggregate = payload["aggregate"]
        lines = [
            "# UHF Primary Secure-Live Benchmark V1",
            "",
            f"- requested-repetitions={aggregate['requestedRepetitions']}",
            f"- counted-repetitions={aggregate['countedRepetitions']}",
            f"- environment-blockers={aggregate['environmentBlockerCount']}",
            f"- quiet-command-pass={aggregate['quietCommandSuccessCount']}",
            f"- nonquiet-command-pass={aggregate['nonquietCommandSuccessCount']}",
            f"- observability-pass={aggregate['liveObservabilityCounts']['PASS']}",
            f"- observability-degraded={aggregate['liveObservabilityCounts']['DEGRADED']}",
            f"- observability-fail={aggregate['liveObservabilityCounts']['FAIL']}",
            f"- nonquiet-degraded-command-reliability={aggregate['nonquietDegradedCommandReliability']}",
            f"- nonquiet-degraded-live-observability={aggregate['nonquietDegradedLiveObservability']}",
            f"- nonquiet-increased-link-noise={aggregate['nonquietIncreasedLinkNoise']}",
            "",
            "## Quiet Vs Nonquiet Aggregate",
            "",
            f"- quiet-active-ground={aggregate['caseComparison']['quiet']['activeGroundPath']}",
            f"- quiet-pass={aggregate['caseComparison']['quiet']['runtimePassCount']}/{aggregate['caseComparison']['quiet']['count']}",
            f"- quiet-observability-pass={aggregate['caseComparison']['quiet']['observabilityCounts']['PASS']}",
            f"- quiet-sband-downlink-median={aggregate['caseComparison']['quiet']['perGround']['sband']['captureBytes']['southboundToGds']['median']}",
            f"- quiet-uhf-downlink-median={aggregate['caseComparison']['quiet']['perGround']['uhf']['captureBytes']['southboundToGds']['median']}",
            f"- quiet-sband-checksum-median={aggregate['caseComparison']['quiet']['perGround']['sband']['noiseMetrics']['checksumWarningCount']['median']}",
            f"- quiet-sband-cycleslip-median={aggregate['caseComparison']['quiet']['perGround']['sband']['noiseMetrics']['rateGroupCycleSlipCount']['median']}",
            f"- nonquiet-active-ground={aggregate['caseComparison']['nonquiet']['activeGroundPath']}",
            f"- nonquiet-pass={aggregate['caseComparison']['nonquiet']['runtimePassCount']}/{aggregate['caseComparison']['nonquiet']['count']}",
            f"- nonquiet-observability-pass={aggregate['caseComparison']['nonquiet']['observabilityCounts']['PASS']}",
            f"- nonquiet-sband-downlink-median={aggregate['caseComparison']['nonquiet']['perGround']['sband']['captureBytes']['southboundToGds']['median']}",
            f"- nonquiet-uhf-downlink-median={aggregate['caseComparison']['nonquiet']['perGround']['uhf']['captureBytes']['southboundToGds']['median']}",
            f"- nonquiet-uhf-checksum-median={aggregate['caseComparison']['nonquiet']['perGround']['uhf']['noiseMetrics']['checksumWarningCount']['median']}",
            f"- nonquiet-uhf-cycleslip-median={aggregate['caseComparison']['nonquiet']['perGround']['uhf']['noiseMetrics']['rateGroupCycleSlipCount']['median']}",
            "",
            "## Repetitions",
        ]
        for item in payload["repetitions"]:
            lines.extend(
                [
                    f"- {item['name']}: counted={item.get('counted', False)} blocked={item.get('blocked', False)}",
                    f"  quiet={item.get('quiet-command', {}).get('payload', {}).get('targetRuntimeVerdict', 'N/A')}",
                    f"  nonquiet={item.get('nonquiet-command', {}).get('payload', {}).get('targetRuntimeVerdict', 'N/A')}",
                    f"  observability={item.get('nonquiet-command', {}).get('payload', {}).get('groundObservabilityVerdict', 'N/A')}",
                ]
            )
        (self.probe_root / "summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    def run(self) -> dict[str, object]:
        repetitions: list[dict[str, object]] = []
        for index in range(1, self.repetitions + 1):
            repetition_root = self.probe_root / f"paired-rerun-{index}"
            repetition_root.mkdir(parents=True, exist_ok=True)
            repetition: dict[str, object] = {
                "name": f"paired-rerun-{index}",
                "counted": False,
                "blocked": False,
            }

            quiet_preflight = self.run_preflight(repetition_root, label="before-quiet")
            repetition.setdefault("preflight", {})["before-quiet"] = quiet_preflight
            if quiet_preflight["verdict"] != "READY":
                repetition["blocked"] = True
                repetitions.append(repetition)
                continue

            quiet_payload = self.run_case(repetition_root, "quiet-command", False)
            repetition["quiet-command"] = {"payload": quiet_payload}

            nonquiet_preflight = self.run_preflight(repetition_root, label="before-nonquiet")
            repetition["preflight"]["before-nonquiet"] = nonquiet_preflight
            if nonquiet_preflight["verdict"] != "READY":
                repetition["blocked"] = True
                repetition["postflight"] = self.run_postflight(repetition_root)
                repetitions.append(repetition)
                continue

            nonquiet_payload = self.run_case(repetition_root, "nonquiet-command", True)
            repetition["nonquiet-command"] = {"payload": nonquiet_payload}

            postflight = self.run_postflight(repetition_root)
            repetition["postflight"] = postflight
            repetition["blocked"] = postflight["verdict"] != "READY"
            repetition["counted"] = not repetition["blocked"]
            repetitions.append(repetition)

        payload = self.build_summary(repetitions)
        (self.probe_root / "campaign-summary.json").write_text(
            json.dumps(payload, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        self.write_summary_md(payload)
        return payload


def main() -> int:
    args = parse_args()
    campaign = SecureLiveBenchmarkCampaign(pathlib.Path(args.probe_root), args.repetitions)
    try:
        payload = campaign.run()
    except Exception:
        sys.stderr.write(traceback.format_exc())
        sys.stderr.flush()
        return 1
    print("target-uhf-primary-secure-live-benchmark: PASS")
    print(f"benchmark-root={args.probe_root}")
    print(f"summary-json={pathlib.Path(args.probe_root) / 'campaign-summary.json'}")
    print(f"summary-md={pathlib.Path(args.probe_root) / 'summary.md'}")
    print(f"counted-repetitions={payload['aggregate']['countedRepetitions']}")
    print(f"environment-blockers={payload['aggregate']['environmentBlockerCount']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
