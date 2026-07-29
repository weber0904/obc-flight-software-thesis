#!/usr/bin/env python3
from __future__ import annotations

import argparse
import datetime as dt
import pathlib
import sys
import time
import traceback
from collections import Counter

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT_DIR / "scripts"))

from decode_ccsds_capture import packet_summary, parse_tm_frames
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario
from run_target_can_matrix_probe import ProbeFailure, file_size, install_signal_cleanup, read_text, ssh_capture

LIVE_DOWNLINK_APIDS = (1, 2, 4)


def parse_ground_runtime_marker(line: str) -> tuple[int, int] | None:
    import re

    match = re.search(r"\(\d+\(\d+\)-(\d+):(\d+)\)", line)
    if match is None:
        return None
    return int(match.group(1)), int(match.group(2))


def parse_target_journal_runtime_marker(line: str) -> tuple[int, int] | None:
    import re

    match = re.search(r"\(\d+:(\d+),(\d+)\)", line)
    if match is None:
        return None
    return int(match.group(1)), int(match.group(2))


class TargetSbandObservabilityGovernanceScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)

    @staticmethod
    def channel_output_has_expected_value(output: str, search: str, expected_tokens: tuple[str, ...]) -> bool:
        expected = {token.casefold() for token in expected_tokens}
        for raw_line in output.splitlines():
            line = raw_line.strip()
            if not line or search not in line or line.startswith("$ "):
                continue
            value = line.rsplit(",", maxsplit=1)[-1].strip() if "," in line else line.rsplit(maxsplit=1)[-1]
            if value.casefold() in expected:
                return True
        return False

    @staticmethod
    def count_fragment_since(path: pathlib.Path, offset: int, fragment: str) -> int:
        try:
            payload = path.read_bytes()
        except FileNotFoundError:
            return 0
        bounded_offset = max(0, min(offset, len(payload)))
        return payload[bounded_offset:].replace(b"\0", b"\n").decode("utf-8", errors="replace").count(fragment)

    @staticmethod
    def text_since(path: pathlib.Path, offset: int) -> str:
        try:
            payload = path.read_bytes()
        except FileNotFoundError:
            return ""
        bounded_offset = max(0, min(offset, len(payload)))
        return payload[bounded_offset:].replace(b"\0", b"\n").decode("utf-8", errors="replace")

    @staticmethod
    def assert_fragments_absent(text: str, fragments: tuple[str, ...], label: str) -> None:
        found = [fragment for fragment in fragments if fragment in text]
        if found:
            raise ProbeFailure(f"{label}: unexpected fragments present: {found}")

    def wait_native_channel_fragment(self, fragment: str, offset: int, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if fragment in self.text_since(self.sband.native_channel_log, offset):
                return
            time.sleep(0.25)
        raise ProbeFailure(f"timed out waiting for native channel fragment {fragment!r}")

    def wait_ground_channel_search_hit(self, ground, label: str, search: str, timeout: float) -> str:
        deadline = time.time() + timeout
        last_output = ""
        while time.time() < deadline:
            output = ground.channel_search(label, search)
            last_output = output
            for raw_line in output.splitlines():
                line = raw_line.strip()
                if line and search in line and not line.startswith("$ "):
                    return output
            time.sleep(0.5)
        raise ProbeFailure(f"timed out waiting for bounded ground-side channel search {search!r}; last={last_output}")

    @staticmethod
    def parse_log_timestamp(text: str) -> dt.datetime:
        try:
            return dt.datetime.fromisoformat(text)
        except ValueError:
            for fmt in ("%Y-%m-%d %H:%M:%S.%f", "%Y-%m-%d %H:%M:%S"):
                try:
                    return dt.datetime.strptime(text, fmt)
                except ValueError:
                    continue
        raise ProbeFailure(f"unsupported log timestamp format: {text!r}")

    @classmethod
    def parse_line_timestamp(cls, line: str) -> dt.datetime | None:
        timestamp_text = ""
        if ": " in line:
            timestamp_text, _ = line.split(": ", 1)
        elif "," in line:
            timestamp_text, _ = line.split(",", 1)
        else:
            return None
        try:
            return cls.parse_log_timestamp(timestamp_text.strip())
        except ProbeFailure:
            return None

    @classmethod
    def find_fragment_timestamp_since(cls, path: pathlib.Path, offset: int, fragment: str) -> dt.datetime:
        for raw_line in cls.text_since(path, offset).splitlines():
            if fragment not in raw_line:
                continue
            parsed = cls.parse_line_timestamp(raw_line.strip())
            if parsed is not None:
                return parsed
        raise ProbeFailure(f"failed to find timestamp for {fragment!r} in {path} from offset {offset}")

    @classmethod
    def wait_fragment_timestamp_since(
        cls,
        path: pathlib.Path,
        offset: int,
        fragment: str,
        timeout: float,
        label: str,
    ) -> dt.datetime:
        deadline = time.time() + timeout
        last_error = ""
        while time.time() < deadline:
            try:
                return cls.find_fragment_timestamp_since(path, offset, fragment)
            except ProbeFailure as exc:
                last_error = str(exc)
                time.sleep(0.25)
        raise ProbeFailure(f"{label}: {last_error}")

    @classmethod
    def latest_log_timestamp_since(cls, path: pathlib.Path, offset: int) -> dt.datetime | None:
        for raw_line in reversed(cls.text_since(path, offset).splitlines()):
            parsed = cls.parse_line_timestamp(raw_line.strip())
            if parsed is not None:
                return parsed
        return None

    def wait_channel_log_caught_up(
        self,
        path: pathlib.Path,
        offset: int,
        not_before: dt.datetime,
        timeout: float,
        label: str,
    ) -> dt.datetime:
        deadline = time.time() + timeout
        last_seen: dt.datetime | None = None
        while time.time() < deadline:
            last_seen = self.latest_log_timestamp_since(path, offset)
            if last_seen is not None and last_seen >= not_before:
                return last_seen
            time.sleep(0.25)
        raise ProbeFailure(f"{label}: channel log did not catch up to {not_before.isoformat()} (last={last_seen})")

    def assert_bounded_native_channel_fragment(self, fragment: str, offset: int, quiet_sec: float = 2.5) -> None:
        baseline_count = self.count_fragment_since(self.sband.native_channel_log, offset, fragment)
        time.sleep(quiet_sec)
        final_count = self.count_fragment_since(self.sband.native_channel_log, offset, fragment)
        if final_count > baseline_count:
            raise ProbeFailure(f"{fragment} kept growing in passive target live output after bounded GET retries completed")

    @staticmethod
    def wait_capture_stable(path: pathlib.Path, quiet_sec: float, timeout: float) -> None:
        quiet_start = time.time()
        deadline = time.time() + timeout
        last_size = path.stat().st_size if path.exists() else 0
        while time.time() < deadline:
            current_size = path.stat().st_size if path.exists() else 0
            if current_size != last_size:
                last_size = current_size
                quiet_start = time.time()
                time.sleep(0.25)
                continue
            if (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.25)
        raise ProbeFailure(f"downlink capture {path} did not go quiet within {timeout} sec")

    @staticmethod
    def wait_capture_growth(path: pathlib.Path, baseline_size: int, timeout: float) -> int:
        deadline = time.time() + timeout
        while time.time() < deadline:
            current_size = path.stat().st_size if path.exists() else 0
            if current_size > baseline_size:
                return current_size
            time.sleep(0.25)
        raise ProbeFailure(f"downlink capture {path} did not grow beyond {baseline_size} bytes")

    def build_eps_detailed_readback_oracle(self):
        capture_path = self.capture_path(self.sband, "southbound-to-gds")

        def oracle(attempt, timeout: float) -> dict[str, object] | None:
            deadline = time.time() + timeout
            while True:
                channel_text = self.text_since(self.sband.native_channel_log, attempt.session_native_channel_log_offset)
                if "OBCApp.epsBridge.EPS_IBAT" in channel_text:
                    capture_end = file_size(capture_path)
                    outcome = {
                        "source": "ground-native-channel",
                        "captureGrowth": capture_end - attempt.session_capture_offset,
                    }
                    try:
                        eps_not_before = self.find_fragment_timestamp_since(
                            self.sband.native_event_log,
                            attempt.session_native_event_log_offset,
                            "EPS_STATUS_RECEIVED",
                        )
                        outcome["eventTimestamp"] = eps_not_before.isoformat()
                    except ProbeFailure:
                        pass
                    return outcome
                if time.time() >= deadline:
                    return None
                time.sleep(0.1)

        return oracle

    def live_capture_apid_counts(self, path: pathlib.Path) -> Counter[int]:
        data = path.read_bytes() if path.exists() else b""
        frames = parse_tm_frames(data, self.sband.scid, self.sband.vcid, self.sband.frame_size)
        summary = packet_summary(frames)
        apid_counts = Counter({int(apid): int(count) for apid, count in summary["apid_counts"].items()})
        return Counter({apid: apid_counts.get(apid, 0) for apid in LIVE_DOWNLINK_APIDS})

    def wait_live_capture_quiet(
        self,
        path: pathlib.Path,
        baseline_counts: Counter[int],
        *,
        quiet_sec: float,
        timeout: float,
    ) -> None:
        deadline = time.time() + timeout
        quiet_start = time.time()
        last_counts = baseline_counts.copy()
        while time.time() < deadline:
            current_counts = self.live_capture_apid_counts(path)
            if current_counts != last_counts:
                last_counts = current_counts
                quiet_start = time.time()
                time.sleep(0.25)
                continue
            if (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.25)
        raise ProbeFailure(
            f"live downlink APIDs did not go quiet on {path}; baseline={dict(baseline_counts)} current={dict(last_counts)}"
        )

    def write_observability_summary(
        self,
        *,
        case_results: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": "target-sband-observability-governance",
            "profile": "sband",
            "verdict": "FAIL" if failure else "PASS",
            "targetPathUnderTest": "entry-70A macOS fprime-gds + ground_ttc_gateway -> subsystem.local node 5 -> SocketCAN -> obc.local",
            "cases": case_results,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "gatewayCaptures": self.secure_capture_summary(),
                "obcJournal": str(self.journal_snapshot_dir / "target-sband-observability-governance-obc.log"),
                "sbandServiceJournal": str(self.journal_snapshot_dir / "target-sband-observability-governance-sband-service.log"),
                "uhfServiceJournal": str(self.journal_snapshot_dir / "target-sband-observability-governance-uhf-service.log"),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
            "residualNonClaims": [
                "No pre-auth broad S-band live chatter requirement is claimed.",
                "No auth-free GET_* summary class is claimed; summary readback stays on the authenticated node-5 path in this v1 proof.",
                "No non-quiet UHF operator closure, reliable-transfer, RF, or one-GDS aggregation claim is made.",
            ],
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "target-sband-observability-governance-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def assert_sband_pre_auth_quiet(self, quiet_sec: float = 3.0) -> None:
        event_start = self.sband.event_count()
        capture_path = self.capture_path(self.sband, "southbound-to-gds")
        capture_size = capture_path.stat().st_size if capture_path.exists() else 0
        deadline = time.time() + quiet_sec
        while time.time() < deadline:
            if self.sband.event_count() != event_start:
                tail = "\n".join(read_text(self.sband.events_log).splitlines()[event_start:])
                raise ProbeFailure(f"unexpected pre-auth S-band event visibility:\n{tail}")
            current_capture_size = capture_path.stat().st_size if capture_path.exists() else 0
            if current_capture_size != capture_size:
                raise ProbeFailure(
                    f"unexpected pre-auth S-band downlink growth: expected {capture_size}, saw {current_capture_size}"
                )
            time.sleep(0.25)

    def wait_sband_channel_contains(self, search: str, expected_tokens: tuple[str, ...], timeout: float) -> str:
        deadline = time.time() + timeout
        last_output = ""
        while time.time() < deadline:
            output = self.sband.channel_search(search.lower(), search)
            passive_text = read_text(self.sband.native_channel_log)
            combined = output
            if passive_text:
                matching_lines = [line for line in passive_text.splitlines() if search in line]
                if matching_lines:
                    combined = output + ("\n" if output else "") + "\n".join(matching_lines[-5:])
            last_output = combined
            if self.channel_output_has_expected_value(combined, search, expected_tokens):
                return output
            time.sleep(0.5)
        raise ProbeFailure(
            f"S-band channel {search!r} did not contain one of {expected_tokens!r}; last={last_output}"
        )

    def assert_sband_passive_logs_stable(self, quiet_sec: float = 2.5, timeout: float = 12.0) -> None:
        event_expected = self.sband.event_count()
        channel_expected = self.sband.native_channel_log.stat().st_size if self.sband.native_channel_log.exists() else 0
        quiet_start = time.time()
        deadline = time.time() + timeout
        while time.time() < deadline:
            current_event_count = self.sband.event_count()
            current_channel_size = self.sband.native_channel_log.stat().st_size if self.sband.native_channel_log.exists() else 0
            if current_event_count != event_expected or current_channel_size != channel_expected:
                event_expected = current_event_count
                channel_expected = current_channel_size
                quiet_start = time.time()
                time.sleep(0.25)
                continue
            if (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.25)
        raise ProbeFailure(f"post-close S-band passive logs did not settle within {timeout} sec")

    def wait_target_journal_runtime_marker(self, journal_since: str, pattern: str, timeout: float) -> tuple[int, int]:
        import re

        compiled = re.compile(pattern)
        deadline = time.time() + timeout
        last_journal = ""
        while time.time() < deadline:
            last_journal = ssh_capture(
                self.obc_target,
                f"journalctl -u {self.obc_service} --since {journal_since!r} --no-pager || true",
                check=False,
            )
            for raw_line in last_journal.splitlines():
                if compiled.search(raw_line) is None:
                    continue
                marker = parse_target_journal_runtime_marker(raw_line)
                if marker is not None:
                    return marker
            time.sleep(0.25)
        raise ProbeFailure(f"timed out waiting for target journal runtime marker {pattern!r}; tail={last_journal[-2000:]}")

    def assert_no_post_close_visibility(
        self,
        *,
        event_offset: int,
        channel_offset: int,
        close_marker: tuple[int, int],
        settle_sec: float,
    ) -> None:
        deadline = time.time() + settle_sec
        while time.time() < deadline:
            for path, offset in (
                (self.sband.events_log, event_offset),
                (self.sband.native_channel_log, channel_offset),
            ):
                for raw_line in self.text_since(path, offset).splitlines():
                    marker = parse_ground_runtime_marker(raw_line)
                    if marker is not None and marker > close_marker:
                        raise ProbeFailure(f"post-close S-band visibility persisted in {path}: {raw_line}")
            time.sleep(0.25)

    def run_observability_proof(self) -> list[str]:
        summary: list[str] = []
        case_results: list[dict[str, object]] = []
        failure_stage = "observability-bootstrap"
        try:
            failure_stage = "observability-begin-profile"
            self.begin_profile()

            failure_stage = "observability-provenance"
            self.record_secure_auth_provenance()
            case_results.append({"case": "installed-release-keystore-provenance", "verdict": "PASS"})
            summary.append("case-installed-release-keystore-provenance=PASS")

            failure_stage = "observability-start-helpers"
            self.apply_sband_ingress_diagnostics_override()
            self.start_security_server()
            self.wait_for_sband_tcp_reachability(10.0)
            self.start_ground_paths(need_sband=True, need_uhf=False)
            readiness = self.ensure_sband_ground_ready()
            summary.append(f"sband-ground-readiness={readiness}")
            self.prepare_ground_window(self.sband, timeout=8.0)

            failure_stage = "observability-pre-auth-quiet"
            self.assert_sband_pre_auth_quiet()
            case_results.append({"case": "pre-auth-sband-live-quiet", "verdict": "PASS"})
            summary.append("case-pre-auth-sband-live-quiet=PASS")

            failure_stage = "observability-sband-auth"
            event_start = self.sband.event_count()
            auth_channel_offset = self.sband.native_channel_log.stat().st_size if self.sband.native_channel_log.exists() else 0
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            self.wait_ground_or_journal(
                self.sband,
                event_start,
                journal_since,
                ("COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED",),
                ("COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED",),
                20.0,
                "S-band live observability open",
                prefer_target_journal=False,
            )
            case_results.append({"case": "post-auth-sband-live-open", "verdict": "PASS"})
            summary.append("case-post-auth-sband-live-open=PASS")

            channel_offset = auth_channel_offset
            self.wait_native_channel_fragment("OBCApp.epsBridge.EPS_SOC", channel_offset, 15.0)
            self.wait_native_channel_fragment("OBCApp.adcsBridge.ADCS_MODE", channel_offset, 15.0)
            self.wait_native_channel_fragment("OBCApp.radioController.RADIO_STATUS_AGE_TICKS", channel_offset, 15.0)
            self.wait_native_channel_fragment("OBCApp.storageHealthBridge.STORAGE_WARNING_MASK", channel_offset, 30.0)
            self.wait_native_channel_fragment("OBCApp.watchdogSupervisor.SYS_CPU_USAGE", channel_offset, 15.0)
            self.wait_native_channel_fragment("OBCApp.watchdogSupervisor.SYS_MEM_RSS_MB", channel_offset, 15.0)
            self.wait_native_channel_fragment(
                "OBCApp.groundLinkHealthProvider.GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS",
                channel_offset,
                15.0,
            )
            self.wait_native_channel_fragment("OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES", channel_offset, 15.0)
            self.wait_native_channel_fragment(
                "OBCApp.commEgressMux.SBAND_ROUTED_EVENT_PACKETS", auth_channel_offset, 15.0
            )
            self.assert_fragments_absent(
                self.text_since(self.sband.native_channel_log, channel_offset),
                (
                    "OBCApp.epsBridge.EPS_IBAT",
                    "OBCApp.gpsBridge.GPS_LAT_DEG",
                    "OBCApp.adcsBridge.ADCS_Q0",
                    "OBCApp.radioController.RADIO_RSSI",
                    "OBCApp.storageHealthBridge.STORAGE_SCAN_COUNT",
                ),
                "post-auth curated live",
            )
            case_results.append({"case": "post-auth-summary-live-curated", "verdict": "PASS"})
            summary.append("case-post-auth-summary-live-curated=PASS")
            case_results.append({"case": "post-auth-resource-reviewable-surfaces", "verdict": "PASS"})
            summary.append("case-post-auth-resource-reviewable-surfaces=PASS")

            eps_offset = self.sband.native_channel_log.stat().st_size if self.sband.native_channel_log.exists() else 0
            eps_outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-observability-eps-get-status",
                "OBCApp.epsBridge.EPS_GET_STATUS",
                accept_sequence=True,
                journal_fragments=("EPS_STATUS_RECEIVED",),
                ground_success_oracle=self.build_eps_detailed_readback_oracle(),
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            self.assert_bounded_native_channel_fragment("OBCApp.epsBridge.EPS_IBAT", eps_offset)
            case_results.append({"case": "eps-get-bounded-detailed-readback", "verdict": "PASS", **eps_outcome})
            summary.append("case-eps-get-bounded-detailed-readback=PASS")

            failure_stage = "observability-get-reset-cause"
            reset_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("BOOT_RECOVERY_STATUS",),
                source_name="ground-native-event-log",
            )
            reset_outcome = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "sband-observability-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_success_oracle=reset_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            case_results.append(
                {
                    "case": "authenticated-get-reset-cause-summary-readback",
                    "verdict": "PASS",
                    **reset_outcome,
                }
            )
            summary.append("case-authenticated-get-reset-cause-summary-readback=PASS")

            failure_stage = "observability-prepare-uhf"
            self.apply_uhf_ingress_diagnostics_override()

            failure_stage = "observability-switch-close"
            switch_event_offset = self.sband.events_log.stat().st_size if self.sband.events_log.exists() else 0
            switch_channel_offset = self.sband.native_channel_log.stat().st_size if self.sband.native_channel_log.exists() else 0
            switch_journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            switch_sequence, switch_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                "sband-observability-switch-to-uhf",
                "OBCApp.commController.COMM_SET_ACTIVE",
                "UHF",
                accept_sequence=True,
                journal_fragments=(
                    "COMM_PRIMARY_LINK_CHANGED",
                    "command UHF",
                    "telemetry UHF",
                    "file UHF",
                    "reason 1",
                    "COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED",
                    "active 0",
                    "reason 3",
                ),
                timeout=35.0,
            )
            close_marker = self.wait_target_journal_runtime_marker(
                switch_journal_since,
                r"COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED.*active 0.*reason 3",
                5.0,
            )
            switch_capture_path = self.capture_path(self.sband, "southbound-to-gds")
            switch_capture_counts = self.live_capture_apid_counts(switch_capture_path)
            self.wait_live_capture_quiet(
                switch_capture_path,
                switch_capture_counts,
                quiet_sec=2.5,
                timeout=10.0,
            )
            self.assert_no_post_close_visibility(
                event_offset=switch_event_offset,
                channel_offset=switch_channel_offset,
                close_marker=close_marker,
                settle_sec=5.0,
            )
            case_results.append(
                {
                    "case": "primary-switch-closes-sband-live-observability",
                    "verdict": "PASS",
                    "switchSequence": switch_sequence,
                    "source": switch_source,
                }
            )
            summary.append("case-primary-switch-closes-sband-live-observability=PASS")

            failure_stage = "observability-uhf-reauth"
            self.start_ground_paths(need_sband=False, need_uhf=True)
            uhf_primary_session = self.authenticate_secure_service(
                self.uhf,
                service_id=2,
                ingress_port=1,
                role_fragment="identity 2 role 3",
                initial_sequence=41,
            )
            restore_sequence, restore_source = self.send_secure_command_name(
                self.uhf,
                uhf_primary_session,
                "uhf-observability-restore-sband",
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
            case_results.append(
                {
                    "case": "restore-sband-primary",
                    "verdict": "PASS",
                    "restoreSequence": restore_sequence,
                    "source": restore_source,
                }
            )
            summary.append("case-restore-sband-primary=PASS")

            self.snapshot_journal(self.obc_target, self.obc_service, "target-sband-observability-governance-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "target-sband-observability-governance-sband-service", lines=240)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "target-sband-observability-governance-uhf-service", lines=360)
            summary_path = self.write_observability_summary(case_results=case_results)
            summary.append(f"target-sband-observability-governance-summary={summary_path}")
            summary.append("target-sband-observability-governance=PASS")
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "target-sband-observability-governance-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "target-sband-observability-governance-sband-service", lines=240)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "target-sband-observability-governance-uhf-service", lines=360)
            summary_path = self.write_observability_summary(
                case_results=case_results,
                failure={
                    "stage": failure_stage,
                    "error": str(exc),
                    **({"retryDetails": getattr(exc, "retry_details")} if hasattr(exc, "retry_details") else {}),
                },
            )
            self.checkpoint(
                "target-sband-observability-governance-summary-written",
                "fail",
                artifact=str(summary_path),
                stage=failure_stage,
                error=str(exc),
            )
            raise


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    scenario = TargetSbandObservabilityGovernanceScenario(pathlib.Path(args.probe_root))
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        summary_lines = [
            "target-sband-observability-governance-probe: PASS",
            f"probe-root={args.probe_root}",
        ]
        summary_lines.extend(scenario.run_observability_proof())
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"target-sband-observability-governance-probe: FAIL {exc}\n"
        exit_code = 1
    except Exception as exc:  # pragma: no cover - top-level trap
        run_error = exc
        output = "target-sband-observability-governance-probe: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"target-sband-observability-governance-probe: FAIL cleanup {cleanup_exc}\n"
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
