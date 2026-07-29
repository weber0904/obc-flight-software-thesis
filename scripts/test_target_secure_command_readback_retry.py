#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import tempfile
import unittest
from types import SimpleNamespace
from unittest import mock

from comm_verification.lib import run_target_can_matrix_probe as probe_lib


class _FakeScenario:
    send_secure_command_until_ground_readback = probe_lib.TargetCanScenario.send_secure_command_until_ground_readback
    _send_secure_command_packet_once = probe_lib.TargetCanScenario._send_secure_command_packet_once
    _analyze_secure_command_retry_journal = probe_lib.TargetCanScenario._analyze_secure_command_retry_journal
    build_ground_event_readback_oracle = probe_lib.TargetCanScenario.build_ground_event_readback_oracle
    _text_has_all_fragments = staticmethod(probe_lib.TargetCanScenario._text_has_all_fragments)

    def __init__(self, temp_root: pathlib.Path) -> None:
        self.temp_root = temp_root
        self.obc_target = "fake-obc"
        self.obc_service = "fake-obc.service"
        self.opcodes = {"OBCApp.bootManager.GET_RESET_CAUSE": 0x10010000}
        self._capture_path = temp_root / "southbound-to-gds.bin"
        self._capture_path.write_bytes(b"")
        self.checkpoints: list[dict[str, object]] = []

    def prepare_ground_window(self, ground, timeout: float = 8.0) -> None:
        return None

    def encode_inner_command(self, command_name: str, *args: str) -> bytes:
        return b"\x00\x00\x00\x01"

    def capture_path(self, ground, name: str) -> pathlib.Path:
        assert name == "southbound-to-gds"
        return self._capture_path

    def checkpoint(self, name: str, verdict: str, **kwargs) -> None:
        self.checkpoints.append({"name": name, "verdict": verdict, **kwargs})


def _fake_ground(temp_root: pathlib.Path):
    raw_command_log = temp_root / "raw-command.log"
    raw_command_log.write_text("", encoding="utf-8")
    events_log = temp_root / "events.log"
    events_log.write_text("", encoding="utf-8")
    native_event_log = temp_root / "native-events.log"
    native_event_log.write_text("", encoding="utf-8")
    native_channel_log = temp_root / "native-channels.log"
    native_channel_log.write_text("", encoding="utf-8")
    return SimpleNamespace(
        name="sband-ground",
        gds_tts_port=50151,
        raw_command_log=raw_command_log,
        events_log=events_log,
        native_event_log=native_event_log,
        native_channel_log=native_channel_log,
    )


class SecureCommandReadbackRetryTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.temp_root = pathlib.Path(self.temp_dir.name)
        self.scenario = _FakeScenario(self.temp_root)
        self.ground = _fake_ground(self.temp_root)
        self.session = probe_lib.SecureAuthSession(
            service_id=1,
            ingress_port=0,
            role_fragment="identity 1 role 1",
            session_key=b"\x11" * 32,
            next_sequence=41,
        )
        self.sent_sequences: list[int] = []
        self.journal_by_attempt: dict[int, str] = {}

    def _patch_transport(self):
        def fake_send(_port: int, _payload: bytes) -> None:
            return None

        def fake_ssh_capture(_target: str, command: str, check: bool = True) -> str:
            if "date '+%Y-%m-%d %H:%M:%S'" in command:
                return "2026-06-25 12:00:00\n"
            if "journalctl -u" in command:
                return self.journal_by_attempt.get(len(self.sent_sequences), "")
            raise AssertionError(f"unexpected ssh command: {command}")

        patches = (
            mock.patch.object(probe_lib, "send_tts_raw_packet", side_effect=fake_send),
            mock.patch.object(probe_lib, "ssh_capture", side_effect=fake_ssh_capture),
            mock.patch.object(probe_lib.time, "sleep", side_effect=lambda *_args, **_kwargs: None),
        )
        for patch in patches:
            patch.start()
            self.addCleanup(patch.stop)

    def _run_helper(self, oracle):
        original_send = self.scenario._send_secure_command_packet_once

        def wrapped_send(*args, **kwargs):
            context = original_send(*args, **kwargs)
            self.sent_sequences.append(context.sequence)
            return context

        with mock.patch.object(self.scenario, "_send_secure_command_packet_once", side_effect=wrapped_send):
            return self.scenario.send_secure_command_until_ground_readback(
                self.ground,
                self.session,
                "test-readback",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_success_oracle=oracle,
                attempt_limit=30,
                per_attempt_timeout=0.01,
            )

    def test_first_attempt_ground_success_stops_immediately(self) -> None:
        self._patch_transport()

        def oracle(attempt, timeout: float):
            self.assertEqual(attempt.attempt, 1)
            return {"source": "ground-native-event-log"} if attempt.attempt == 1 else None

        outcome = self._run_helper(oracle)
        self.assertEqual(outcome["attemptCount"], 1)
        self.assertEqual(self.sent_sequences, [41])
        self.assertEqual(outcome["source"], "ground-native-event-log")

    def test_same_sequence_retry_when_ground_and_journal_are_silent(self) -> None:
        self._patch_transport()

        def oracle(attempt, timeout: float):
            if attempt.attempt == 3:
                return {"source": "ground-native-event-log"}
            return None

        outcome = self._run_helper(oracle)
        self.assertEqual(outcome["attemptCount"], 3)
        self.assertEqual(self.sent_sequences, [41, 41, 41])
        self.assertEqual(outcome["attempts"][-1]["sequencePolicy"], "same-seq-retry")

    def test_journal_acceptance_switches_next_attempt_to_new_sequence(self) -> None:
        self._patch_transport()
        self.journal_by_attempt[1] = "BOOT_RECOVERY_STATUS accepted\n"

        def oracle(attempt, timeout: float):
            if attempt.attempt == 2:
                return {"source": "ground-native-event-log"}
            return None

        outcome = self._run_helper(oracle)
        self.assertEqual(outcome["attemptCount"], 2)
        self.assertEqual(self.sent_sequences, [41, 42])
        self.assertTrue(outcome["journalAcceptedSeen"])
        self.assertEqual(outcome["attempts"][-1]["sequencePolicy"], "new-seq-retry")

    def test_duplicate_reject_switches_next_attempt_to_new_sequence(self) -> None:
        self._patch_transport()
        self.journal_by_attempt[1] = (
            "COMMAND_SEQUENCE_REJECTED ingress 0 sequence 41 inner opcode 0x10010000\n"
        )

        def oracle(attempt, timeout: float):
            if attempt.attempt == 2:
                return {"source": "ground-native-event-log"}
            return None

        outcome = self._run_helper(oracle)
        self.assertEqual(outcome["attemptCount"], 2)
        self.assertEqual(self.sent_sequences, [41, 42])
        self.assertTrue(outcome["duplicateRejectSeen"])

    def test_attempt_limit_exhaustion_exposes_failure_classification(self) -> None:
        self._patch_transport()
        self.journal_by_attempt[1] = "BOOT_RECOVERY_STATUS accepted\n"

        def oracle(attempt, timeout: float):
            return None

        with self.assertRaises(probe_lib.ProbeFailure) as ctx:
            self._run_helper(oracle)
        retry_details = getattr(ctx.exception, "retry_details", None)
        assert retry_details is not None
        self.assertEqual(retry_details["failureClass"], "target-accepted-but-ground-readback-missing")
        self.assertEqual(len(retry_details["attempts"]), 30)

    def test_late_ground_success_after_journal_check_stops_without_resend(self) -> None:
        self._patch_transport()
        self.journal_by_attempt[1] = "BOOT_RECOVERY_STATUS accepted\n"
        oracle_calls: list[tuple[int, float]] = []

        def oracle(attempt, timeout: float):
            oracle_calls.append((attempt.attempt, timeout))
            if len(oracle_calls) == 2:
                return {"source": "ground-native-event-log", "lateCheck": True}
            return None

        outcome = self._run_helper(oracle)
        self.assertEqual(outcome["attemptCount"], 1)
        self.assertEqual(self.sent_sequences, [41])
        self.assertEqual(oracle_calls, [(1, 0.01), (1, 0.0)])
        self.assertTrue(outcome["lateCheck"])

    def test_event_oracle_requires_capture_growth_when_requested(self) -> None:
        event_path = self.temp_root / "native-events.log"
        event_path.write_text("BOOT_RECOVERY_STATUS\n", encoding="utf-8")
        capture_path = self.temp_root / "southbound-to-gds.bin"
        capture_path.write_bytes(b"")
        attempt = probe_lib.SecureCommandReadbackAttemptContext(
            attempt=1,
            sequence=41,
            sequence_policy="initial",
            command_name="OBCApp.bootManager.GET_RESET_CAUSE",
            command_args=(),
            ground=self.ground,
            journal_since="2026-06-25 12:00:00",
            events_log_line_count=0,
            events_log_offset=0,
            native_event_log_offset=0,
            native_channel_log_offset=0,
            capture_offset=0,
            session_events_log_offset=0,
            session_native_event_log_offset=0,
            session_native_channel_log_offset=0,
            session_capture_offset=0,
        )
        oracle = self.scenario.build_ground_event_readback_oracle(
            event_log_path=event_path,
            event_offset_attr="native_event_log_offset",
            ground_fragments=("BOOT_RECOVERY_STATUS",),
            capture_path=capture_path,
            require_capture_growth=True,
            source_name="ground-native-event-log",
        )
        outcome = oracle(attempt, 0.01)
        self.assertIsNone(outcome)

        capture_path.write_bytes(b"\x01\x02\x03\x04")
        outcome = oracle(attempt, 0.01)
        self.assertIsNotNone(outcome)
        assert outcome is not None
        self.assertEqual(outcome["captureGrowth"], 4)
        self.assertEqual(outcome["source"], "ground-native-event-log")


if __name__ == "__main__":
    raise SystemExit(unittest.main())
