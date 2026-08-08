#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import tempfile
import unittest
from unittest import mock

LIB_DIR = pathlib.Path(__file__).resolve().parent / "manual_ops" / "lib"
if str(LIB_DIR) not in sys.path:
    sys.path.insert(0, str(LIB_DIR))

import surface_owner


class TargetManualBeaconCleanupTest(unittest.TestCase):
    def test_empty_remote_capture_clears_stale_local_mirror(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            owner = surface_owner.TargetGroundManualSurfaceOwner(
                pathlib.Path(temporary_directory) / "surface",
                "headless",
                auto_ports=False,
            )
            owner.target_beacon_sidecar = {
                "ready": True,
                "subsystemTarget": "subsystem",
                "capturePath": "/tmp/target-uhf-beacon.bin",
            }
            owner.beacon_local_capture_path.parent.mkdir(parents=True)
            owner.beacon_local_capture_path.write_bytes(b"stale")
            owner.remote_beacon_size_bytes = 108
            with mock.patch.object(surface_owner, "ssh_capture", return_value="0\n"):
                owner.refresh_beacon_mirror()
            self.assertFalse(owner.beacon_local_capture_path.exists())
            self.assertEqual(owner.remote_beacon_size_bytes, -1)

    def test_failed_remote_copy_clears_mirror_without_advancing_marker(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            owner = surface_owner.TargetGroundManualSurfaceOwner(
                pathlib.Path(temporary_directory) / "surface",
                "headless",
                auto_ports=False,
            )
            owner.target_beacon_sidecar = {
                "ready": True,
                "subsystemTarget": "subsystem",
                "capturePath": "/tmp/target-uhf-beacon.bin",
            }
            owner.beacon_local_capture_path.parent.mkdir(parents=True)
            owner.beacon_local_capture_path.write_bytes(b"stale")
            with (
                mock.patch.object(surface_owner, "ssh_capture", return_value="108\n"),
                mock.patch.object(surface_owner, "ssh_capture_bytes", side_effect=RuntimeError("copy failed")),
            ):
                owner.refresh_beacon_mirror()
            self.assertFalse(owner.beacon_local_capture_path.exists())
            self.assertEqual(owner.remote_beacon_size_bytes, -1)

    def test_same_size_recreated_remote_capture_is_recopied(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            owner = surface_owner.TargetGroundManualSurfaceOwner(
                pathlib.Path(temporary_directory) / "surface",
                "headless",
                auto_ports=False,
            )
            owner.target_beacon_sidecar = {
                "ready": True,
                "subsystemTarget": "subsystem",
                "capturePath": "/tmp/target-uhf-beacon.bin",
            }
            owner.beacon_local_capture_path.parent.mkdir(parents=True)
            owner.beacon_local_capture_path.write_bytes(b"old" * 36)
            owner.remote_beacon_size_bytes = 108
            owner.remote_beacon_capture_marker = (108, 100, 10)
            fresh = b"new" * 36
            with (
                mock.patch.object(surface_owner, "ssh_capture", return_value="108 200 11\n"),
                mock.patch.object(surface_owner, "ssh_capture_bytes", return_value=fresh),
            ):
                owner.refresh_beacon_mirror()
            self.assertEqual(owner.beacon_local_capture_path.read_bytes(), fresh)
            self.assertEqual(owner.remote_beacon_capture_marker, (108, 200, 11))

    def test_cleanup_matches_only_its_own_bridge_instance_label(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            owner = surface_owner.TargetGroundManualSurfaceOwner(
                pathlib.Path(temporary_directory) / "surface",
                "headless",
                auto_ports=False,
            )
            owner.remote_beacon_working_directory = "/workspace"
            owner.remote_beacon_capture_path = "/tmp/owner-capture.bin"
            owner.remote_beacon_instance_label = "mission-console-beacon-test-owner"
            with mock.patch.object(surface_owner, "ssh_capture") as ssh_capture:
                owner.stop_beacon_sidecar()
            cleanup_command = ssh_capture.call_args.args[1]
            self.assertIn("--instance-label mission-console-beacon-test-owner", cleanup_command)
            self.assertNotIn("build-fprime-automatic-native/bin/Linux/pty_pair_bridge", cleanup_command)

    def test_manual_owner_cleanup_does_not_stop_the_baseline_owned_sidecar(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            owner = surface_owner.TargetGroundManualSurfaceOwner(
                pathlib.Path(temporary_directory) / "surface",
                "headless",
                auto_ports=False,
            )
            owner.stop_beacon_sidecar = mock.Mock(side_effect=AssertionError("manual cleanup must not own sidecar"))
            with mock.patch.object(surface_owner, "cleanup_managed_processes"):
                owner.cleanup(write_terminal_state=False)
            owner.stop_beacon_sidecar.assert_not_called()


if __name__ == "__main__":
    unittest.main()
