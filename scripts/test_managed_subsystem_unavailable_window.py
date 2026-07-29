#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT_DIR = pathlib.Path(__file__).resolve().parents[1]
COMM_LIB_DIR = ROOT_DIR / "scripts" / "comm_verification" / "lib"
if str(COMM_LIB_DIR) not in sys.path:
    sys.path.insert(0, str(COMM_LIB_DIR))

import managed_subsystem_unavailable_window as helper  # noqa: E402


class ManagedSubsystemUnavailableWindowTest(unittest.TestCase):
    def test_trigger_and_restore_success(self) -> None:
        ssh_calls: list[str] = []

        def fake_ssh_capture(target: str, command: str, check: bool = True) -> str:
            ssh_calls.append(command)
            if "date '+%Y-%m-%d %H:%M:%S'" in command:
                return "2026-06-27 12:00:00\n"
            return ""

        journals = iter(
            [
                "noise only",
                "RECOVERY_INCIDENT_OPENED\nADCS_POLL_TRANSPORT\nSUBSYSTEM_INTERFACE_RESET\n",
            ]
        )

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=fake_ssh_capture), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(helper, "current_service_journal", side_effect=lambda *args, **kwargs: next(journals)), \
            mock.patch.object(helper, "wait_service_active") as wait_service_active:
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = helper.drive_managed_subsystem_unavailable_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-adcs-csp.service",
                restart_timeout=10,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "ADCS_POLL_TRANSPORT", "SUBSYSTEM_INTERFACE_RESET"),
                diagnostic_fragments=("ADCS_COMM_ERROR",),
                artifact_path=artifact,
            )
            self.assertTrue(artifact.exists())

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(payload["restoreVerdict"], "PASS")
        self.assertEqual(payload["observedFragmentCounts"]["ADCS_POLL_TRANSPORT"], 1)
        self.assertEqual(payload["diagnosticCounts"]["ADCS_COMM_ERROR"], 0)
        wait_service_active.assert_called_once_with("subsystem", "subsystem-adcs-csp.service", 10)
        self.assertTrue(any("systemctl stop" in call for call in ssh_calls))
        self.assertTrue(any("systemctl start" in call for call in ssh_calls))

    def test_restore_failure_flips_pass_to_fail(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=["2026-06-27 12:00:00\n", "", ""]), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(
                helper,
                "current_service_journal",
                return_value="RECOVERY_INCIDENT_OPENED\nEPS_TIMEOUT\nSUBSYSTEM_INTERFACE_RESET\n",
            ), \
            mock.patch.object(helper, "wait_service_active", side_effect=RuntimeError("restore failed")):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = helper.drive_managed_subsystem_unavailable_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-eps-csp.service",
                restart_timeout=10,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "EPS_TIMEOUT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=artifact,
            )

        self.assertEqual(payload["verdict"], "FAIL")
        self.assertEqual(payload["restoreVerdict"], "FAIL")
        self.assertIn("restoration failed", payload["error"])

    def test_failure_records_observed_and_diagnostic_counts(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=["2026-06-27 12:00:00\n", "", ""]), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(
                helper,
                "current_service_journal",
                return_value="EPS_COMM_ERROR\nMODE_SAFETY_EPS_UNAVAILABLE\n",
            ), \
            mock.patch.object(helper, "wait_service_active"):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            with self.assertRaises(helper.ProbeFailure) as excinfo:
                helper.drive_managed_subsystem_unavailable_window(
                    obc_target="obc",
                    obc_service="obc.service",
                    subsystem_target="subsystem",
                    subsystem_service="subsystem-eps-csp.service",
                    restart_timeout=10,
                    trigger_timeout=1,
                    journal_fragments=("RECOVERY_INCIDENT_OPENED", "EPS_TIMEOUT", "SUBSYSTEM_INTERFACE_RESET"),
                    diagnostic_fragments=("EPS_COMM_ERROR", "MODE_SAFETY_EPS_UNAVAILABLE"),
                    artifact_path=artifact,
                )
            payload = json.loads(artifact.read_text(encoding="utf-8"))
            self.assertEqual(payload["verdict"], "FAIL")
            self.assertEqual(payload["observedFragmentCounts"]["EPS_TIMEOUT"], 0)
            self.assertEqual(payload["diagnosticCounts"]["EPS_COMM_ERROR"], 1)
            self.assertEqual(payload["diagnosticCounts"]["MODE_SAFETY_EPS_UNAVAILABLE"], 1)
            self.assertIn("diagnostic=", str(excinfo.exception))

    def test_outage_probe_payload_is_recorded(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=["2026-06-27 12:00:00\n", "", ""]), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(
                helper,
                "current_service_journal",
                return_value="RECOVERY_INCIDENT_OPENED\nADCS_POLL_TRANSPORT\nSUBSYSTEM_INTERFACE_RESET\n",
            ), \
            mock.patch.object(helper, "wait_service_active"):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = helper.drive_managed_subsystem_unavailable_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-adcs-csp.service",
                restart_timeout=10,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "ADCS_POLL_TRANSPORT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=artifact,
                outage_probe=lambda current_payload: {
                    "verdict": "FAIL",
                    "label": "in-outage sync probe",
                    "holdStrategy": current_payload["holdStrategy"],
                },
            )

        self.assertEqual(payload["outageProbe"]["verdict"], "FAIL")
        self.assertEqual(payload["outageProbe"]["holdStrategy"], "service-down-until-journal-or-timeout")

    def test_restore_before_post_trigger_probe_runs_restore_first(self) -> None:
        ssh_calls: list[str] = []

        def fake_ssh_capture(target: str, command: str, check: bool = True) -> str:
            ssh_calls.append(command)
            if "date '+%Y-%m-%d %H:%M:%S'" in command:
                return "2026-06-27 12:00:00\n"
            return ""

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=fake_ssh_capture), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(
                helper,
                "current_service_journal",
                return_value="RECOVERY_INCIDENT_OPENED\nEPS_TIMEOUT\nSUBSYSTEM_INTERFACE_RESET\nRECOVERY_ACTION_EXECUTED\nSAFE_FALLBACK\n",
            ), \
            mock.patch.object(helper, "wait_service_active"):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = helper.drive_managed_subsystem_unavailable_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-eps-csp.service",
                restart_timeout=10,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "EPS_TIMEOUT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=artifact,
                post_trigger_fragments=("RECOVERY_ACTION_EXECUTED", "SAFE_FALLBACK"),
                restore_before_post_trigger_probe=True,
                post_trigger_probe=lambda current_payload: {
                    "verdict": "PASS",
                    "restoreAlreadyDone": current_payload.get("restoreVerdict") == "PASS",
                },
            )

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(payload["restoreVerdict"], "PASS")
        self.assertTrue(payload["postTriggerProbe"]["restoreAlreadyDone"])
        stop_index = next(index for index, call in enumerate(ssh_calls) if "systemctl stop" in call)
        start_index = next(index for index, call in enumerate(ssh_calls) if "systemctl start" in call)
        self.assertGreater(start_index, stop_index)

    def test_uses_configured_poll_interval(self) -> None:
        sleep_calls: list[float] = []

        def fake_sleep(value: float) -> None:
            sleep_calls.append(value)

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(helper, "ssh_capture", side_effect=["2026-06-27 12:00:00\n", "", ""]), \
            mock.patch.object(helper, "service_invocation_id", return_value="inv-1"), \
            mock.patch.object(
                helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(
                helper,
                "current_service_journal",
                side_effect=["noise only", "RECOVERY_INCIDENT_OPENED\nEPS_TIMEOUT\nSUBSYSTEM_INTERFACE_RESET\n"],
            ), \
            mock.patch.object(helper, "wait_service_active"), \
            mock.patch.object(helper.time, "sleep", side_effect=fake_sleep):
            payload = helper.drive_managed_subsystem_unavailable_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-eps-csp.service",
                restart_timeout=10,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "EPS_TIMEOUT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=pathlib.Path(tmpdir) / "artifact.json",
                poll_interval=0.5,
            )

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(sleep_calls, [0.5])


if __name__ == "__main__":
    unittest.main()
