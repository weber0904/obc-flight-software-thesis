#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
from unittest import mock

ROOT_DIR = pathlib.Path(__file__).resolve().parents[1]
COMM_LIB_DIR = ROOT_DIR / "scripts" / "comm_verification" / "lib"
if str(COMM_LIB_DIR) not in sys.path:
    sys.path.insert(0, str(COMM_LIB_DIR))

import managed_adcs_state_drop_window as adcs_helper  # noqa: E402
import managed_eps_status_drop_window as eps_helper  # noqa: E402


class ManagedSimDropWindowTest(unittest.TestCase):
    def test_adcs_pass_still_clears_drop_state(self) -> None:
        commands: list[int] = []

        def fake_send(_target: str, _socket_path: str, count: int) -> str:
            commands.append(count)
            return f"OK count={count}"

        journals = iter(
            [
                "noise only",
                "RECOVERY_INCIDENT_OPENED\nADCS_POLL_TRANSPORT\nSUBSYSTEM_INTERFACE_RESET\n",
            ]
        )

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(adcs_helper, "ssh_capture", return_value="2026-06-28 12:00:00\n"), \
            mock.patch.object(
                adcs_helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(adcs_helper, "current_service_journal", side_effect=lambda *args, **kwargs: next(journals)), \
            mock.patch.object(adcs_helper, "send_adcs_drop_state_command", side_effect=fake_send):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = adcs_helper.drive_managed_adcs_state_drop_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-adcs-csp.service",
                control_socket_path="/tmp/adcs.sock",
                drop_count=4,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "ADCS_POLL_TRANSPORT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=artifact,
            )

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(commands, [4, 0])
        self.assertEqual(payload["cleanupControlResponse"], "OK count=0")

    def test_eps_pass_still_clears_drop_status(self) -> None:
        commands: list[int] = []

        def fake_send(_target: str, _socket_path: str, count: int) -> str:
            commands.append(count)
            return f"OK count={count}"

        journals = iter(
            [
                "noise only",
                "RECOVERY_INCIDENT_OPENED\nEPS_TIMEOUT\nSUBSYSTEM_INTERFACE_RESET\nSYS_MODE_CHANGE\nSAFE (0)\n",
            ]
        )

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(eps_helper, "ssh_capture", return_value="2026-06-28 12:00:00\n"), \
            mock.patch.object(
                eps_helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(eps_helper, "current_service_journal", side_effect=lambda *args, **kwargs: next(journals)), \
            mock.patch.object(eps_helper, "send_eps_drop_status_command", side_effect=fake_send):
            artifact = pathlib.Path(tmpdir) / "artifact.json"
            payload = eps_helper.drive_managed_eps_status_drop_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-eps-csp.service",
                control_socket_path="/tmp/eps.sock",
                drop_count=4,
                trigger_timeout=5,
                journal_fragments=(
                    "RECOVERY_INCIDENT_OPENED",
                    "EPS_TIMEOUT",
                    "SUBSYSTEM_INTERFACE_RESET",
                    "SYS_MODE_CHANGE",
                    "SAFE (0)",
                ),
                artifact_path=artifact,
            )

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(commands, [4, 0])
        self.assertEqual(payload["cleanupControlResponse"], "OK count=0")

    def test_helpers_use_configured_poll_interval(self) -> None:
        sleep_calls: list[float] = []

        def fake_sleep(value: float) -> None:
            sleep_calls.append(value)

        with tempfile.TemporaryDirectory() as tmpdir, \
            mock.patch.object(adcs_helper, "ssh_capture", return_value="2026-06-28 12:00:00\n"), \
            mock.patch.object(
                adcs_helper,
                "systemctl_show",
                return_value={"ActiveState": "active", "SubState": "running", "MainPID": "123", "UnitFileState": "enabled"},
            ), \
            mock.patch.object(adcs_helper, "current_service_journal", side_effect=["noise only", "RECOVERY_INCIDENT_OPENED\nADCS_POLL_TRANSPORT\nSUBSYSTEM_INTERFACE_RESET\n"]), \
            mock.patch.object(adcs_helper, "send_adcs_drop_state_command", side_effect=["OK count=4", "OK count=0"]), \
            mock.patch.object(adcs_helper.time, "sleep", side_effect=fake_sleep):
            payload = adcs_helper.drive_managed_adcs_state_drop_window(
                obc_target="obc",
                obc_service="obc.service",
                subsystem_target="subsystem",
                subsystem_service="subsystem-adcs-csp.service",
                control_socket_path="/tmp/adcs.sock",
                drop_count=4,
                trigger_timeout=5,
                journal_fragments=("RECOVERY_INCIDENT_OPENED", "ADCS_POLL_TRANSPORT", "SUBSYSTEM_INTERFACE_RESET"),
                artifact_path=pathlib.Path(tmpdir) / "adcs-artifact.json",
                poll_interval=0.75,
            )

        self.assertEqual(payload["verdict"], "PASS")
        self.assertEqual(sleep_calls, [0.75])


if __name__ == "__main__":
    unittest.main()
