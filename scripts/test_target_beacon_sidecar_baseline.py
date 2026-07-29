#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import pathlib
import shlex
import sys
import unittest
from unittest import mock


LIB_DIR = pathlib.Path(__file__).resolve().parent / "comm_verification" / "lib"
if str(LIB_DIR) not in sys.path:
    sys.path.insert(0, str(LIB_DIR))

import ensure_target_comm_lab_baseline as baseline


class TargetBeaconSidecarBaselineTest(unittest.TestCase):
    def test_run_revalidates_baseline_after_sidecar_service_repair(self) -> None:
        environment = {"OBC_SSH_TARGET": "obc", "SUBSYSTEM_SIM_SSH_TARGET": "subsystem"}
        with mock.patch.dict(os.environ, environment, clear=True):
            manager = baseline.TargetBaselineManager()
            sidecar = {"ready": True, "ptyDevice": "/dev/pts/42"}
            with (
                mock.patch.object(manager, "collect_state", side_effect=[{"before": True}, {"after": True}]) as collect,
                mock.patch.object(manager, "analyze", side_effect=[[], []]) as analyze,
                mock.patch.object(
                    manager,
                    "ensure_beacon_sidecar",
                    side_effect=lambda repairs: (repairs.append("applied-target-beacon-sidecar:obc"), sidecar)[1],
                ),
                mock.patch.object(baseline.time, "sleep") as sleep,
            ):
                result = manager.run()
            self.assertEqual(result["verdict"], "repaired")
            self.assertEqual(result["targetBeaconSidecar"], sidecar)
            self.assertEqual(collect.call_count, 2)
            self.assertEqual(analyze.call_count, 2)
            sleep.assert_called_once_with(manager.repair_settle_sec)

    def test_run_blocks_when_sidecar_service_repair_breaks_link_readiness(self) -> None:
        environment = {"OBC_SSH_TARGET": "obc", "SUBSYSTEM_SIM_SSH_TARGET": "subsystem"}
        with mock.patch.dict(os.environ, environment, clear=True):
            manager = baseline.TargetBaselineManager()
            with (
                mock.patch.object(manager, "collect_state", side_effect=[{"before": True}, {"after": True}]) as collect,
                mock.patch.object(
                    manager,
                    "analyze",
                    side_effect=[[], ["journal:obc-ground-link-config-missing"]],
                ) as analyze,
                mock.patch.object(
                    manager,
                    "ensure_beacon_sidecar",
                    side_effect=lambda repairs: repairs.append("applied-target-beacon-sidecar:obc"),
                ),
                mock.patch.object(baseline.time, "sleep"),
            ):
                result = manager.run()
            self.assertEqual(result["verdict"], "blocked")
            self.assertEqual(collect.call_count, 2)
            self.assertEqual(analyze.call_count, 2)

    def test_ready_sidecar_reuses_effective_service_environment_without_restart(self) -> None:
        environment = {
            "OBC_SSH_TARGET": "obc",
            "SUBSYSTEM_SIM_SSH_TARGET": "subsystem",
        }
        sidecar = {
            "ready": True,
            "ptyDevice": "/dev/pts/42",
            "sourceKind": "target-remote-sidecar",
            "sourceBand": "uhf-backup",
            "capturePath": "/tmp/target-beacon-sidecar-baseline-v1/uhf-beacon.bin",
            "frameSize": 108,
            "instanceLabel": "target-beacon-sidecar-baseline-v1",
            "bridgePid": 10,
            "capturePid": 11,
            "subsystemTarget": "subsystem",
            "uhfService": "subsystem-uhf-csp.service",
        }
        with mock.patch.dict(os.environ, environment, clear=True):
            manager = baseline.TargetBaselineManager()
            remote_commands: list[str] = []

            def capture(_target: str, command: str) -> str:
                if command.startswith("systemctl show"):
                    return "/workspace\n"
                remote_commands.append(command)
                return json.dumps(sidecar)

            with (
                mock.patch.object(baseline, "ssh_capture", side_effect=capture),
                mock.patch.object(
                    baseline,
                    "service_environment",
                    side_effect=[
                        {"SUBSYSTEM_SIM_COMM_BEACON_DEVICE": "/dev/pts/42"},
                        {"UHF_BEACON_CSP_NODE": "6"},
                    ],
                ),
                mock.patch.object(baseline, "apply_service_override") as apply_override,
                mock.patch.object(baseline, "wait_service_active") as wait_active,
            ):
                repairs: list[str] = []
                self.assertEqual(manager.ensure_beacon_sidecar(repairs), sidecar)
            apply_override.assert_not_called()
            wait_active.assert_not_called()
            self.assertEqual(repairs, [])
            self.assertEqual(len(remote_commands), 1)
            self.assertIn("def matching_process", remote_commands[0])
            self.assertIn("def terminate_matching_processes", remote_commands[0])
            self.assertIn("def bridge_metadata_ready", remote_commands[0])
            self.assertIn("/proc", remote_commands[0])
            self.assertIn("or not bridge_metadata_ready()", remote_commands[0])
            self.assertIn("PTY path is unavailable", remote_commands[0])
            self.assertIn("if not capture.exists() or not matching_process(capture_pid", remote_commands[0])
            compile(shlex.split(remote_commands[0])[2], "target-beacon-sidecar", "exec")


if __name__ == "__main__":
    unittest.main()
