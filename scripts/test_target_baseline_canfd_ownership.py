#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import unittest
from unittest import mock


LIB_DIR = pathlib.Path(__file__).resolve().parent / "comm_verification" / "lib"
sys.path.insert(0, str(LIB_DIR))

import ensure_target_comm_lab_baseline as baseline  # noqa: E402
import run_target_can_matrix_probe as target_probe  # noqa: E402


EXPECTED_PROFILE = {
    "COMM_CSP_SOCKETCAN_USE_CANFD": "1",
    "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST": "5,6",
    "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST": "40",
}

EXPECTED_OBC_PROFILE = {
    "TARGET_COMM_PROFILE": "sband",
    "COMM_CSP_NODE": "5",
    "COMMAND_AUTHORITY_PROFILE": "sband-primary",
    "INITIAL_COMM_BAND": "sband",
    "ENABLE_PRIMARY_GROUND_LINK_DRIVER": "1",
    "ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR": "1",
}


class TargetBaselineCanFdOwnershipTest(unittest.TestCase):
    @staticmethod
    def make_scenario(*, externally_managed: bool) -> target_probe.TargetCanScenario:
        scenario = target_probe.TargetCanScenario.__new__(target_probe.TargetCanScenario)
        scenario.baseline_managed_externally = externally_managed
        scenario.obc_target = "obc"
        scenario.obc_service = "obc.service"
        scenario.target_service_profile = "sband"
        scenario.target_comm_csp_node = 5
        scenario.target_service_command_authority_profile = "sband-primary"
        scenario.target_service_initial_comm_band = "sband"
        scenario.target_service_enable_primary_ground_link_driver = "1"
        scenario.profile_override_dropin_name = "51-target-comm-profile-override.conf"
        scenario.profile_override_applied = False
        scenario.restart_timeout = 1
        scenario.checkpoint = mock.Mock()
        scenario.snapshot_service = mock.Mock()
        return scenario

    def test_a_forced_restart_reloads_ready_obc_service(self) -> None:
        manager = baseline.TargetBaselineManager.__new__(baseline.TargetBaselineManager)
        manager.force_obc_comm_restart = True
        manager.obc_target = "obc"
        manager.obc_service = "obc.service"
        manager.repair_settle_sec = 0
        manager.require_uhf_service = True
        manager.ignore_gps_state_missing = False
        manager.expected_comm_canfd_env = dict(EXPECTED_PROFILE)
        manager.collect_state = mock.Mock(return_value={"state": "ready"})
        manager.analyze = mock.Mock(return_value=[])
        manager.restart_service = mock.Mock()
        manager.ensure_beacon_sidecar = mock.Mock(return_value={"ready": True})

        summary = manager.run()

        manager.restart_service.assert_called_once_with("obc", "obc.service")
        self.assertEqual(summary["verdict"], "repaired")
        self.assertEqual(
            summary["repairsPerformed"],
            ["forced-restart:obc.service"],
        )
        self.assertTrue(summary["forceObcCommRestart"])

    def test_a_applies_profile_only_to_mismatched_comm_services(self) -> None:
        manager = baseline.TargetBaselineManager.__new__(baseline.TargetBaselineManager)
        manager.obc_target = "obc"
        manager.subsystem_target = "subsystem"
        manager.obc_service = "obc.service"
        manager.sband_service = "sband.service"
        manager.uhf_service = "uhf.service"
        manager.canfd_dropin_name = "57-csp-socketcan-canfd.conf"
        manager.expected_comm_canfd_env = dict(EXPECTED_PROFILE)
        manager.restart_timeout = 1
        manager.require_uhf_service = True

        observed = {
            ("subsystem", "sband.service"): dict(EXPECTED_PROFILE),
            ("subsystem", "uhf.service"): {},
            ("obc", "obc.service"): {},
        }
        repairs: list[str] = []
        with (
            mock.patch.object(baseline, "service_environment", side_effect=lambda target, service: observed[(target, service)]),
            mock.patch.object(baseline, "apply_service_override") as apply_override,
            mock.patch.object(baseline, "wait_service_active") as wait_active,
        ):
            manager.ensure_comm_canfd_profile(repairs)

        self.assertEqual(apply_override.call_count, 2)
        self.assertEqual(
            [call.args[1] for call in apply_override.call_args_list],
            ["uhf.service", "obc.service"],
        )
        for call in apply_override.call_args_list:
            self.assertEqual(call.args[3], EXPECTED_PROFILE)
        self.assertEqual(wait_active.call_count, 2)
        self.assertNotIn("sband.service", [call.args[1] for call in apply_override.call_args_list])

    def test_a_repairs_uhf_profile_even_if_legacy_flag_is_false(self) -> None:
        manager = baseline.TargetBaselineManager.__new__(baseline.TargetBaselineManager)
        manager.obc_target = "obc"
        manager.subsystem_target = "subsystem"
        manager.obc_service = "obc.service"
        manager.sband_service = "sband.service"
        manager.uhf_service = "uhf.service"
        manager.canfd_dropin_name = "57-csp-socketcan-canfd.conf"
        manager.expected_comm_canfd_env = dict(EXPECTED_PROFILE)
        manager.restart_timeout = 1
        manager.require_uhf_service = False

        with (
            mock.patch.object(baseline, "service_environment", return_value={}),
            mock.patch.object(baseline, "apply_service_override") as apply_override,
            mock.patch.object(baseline, "wait_service_active"),
        ):
            manager.ensure_comm_canfd_profile([])

        self.assertEqual(
            [call.args[1] for call in apply_override.call_args_list],
            ["sband.service", "uhf.service", "obc.service"],
        )

    def test_external_c_verifies_profile_without_applying_override(self) -> None:
        scenario = target_probe.TargetCanScenario.__new__(target_probe.TargetCanScenario)
        scenario.csp_socketcan_canfd_override_applied = False
        scenario.csp_socketcan_use_canfd = "1"
        scenario.csp_socketcan_canfd_dest_allowlist = "5,6"
        scenario.csp_socketcan_canfd_dport_allowlist = "40"
        scenario.baseline_managed_externally = True
        scenario.obc_target = "obc"
        scenario.subsystem_target = "subsystem"
        scenario.obc_service = "obc.service"
        scenario.sband_comm_service = "sband.service"
        scenario.uhf_comm_service = "uhf.service"
        scenario.checkpoint = mock.Mock()

        with (
            mock.patch.object(target_probe, "service_environment", return_value=dict(EXPECTED_PROFILE)),
            mock.patch.object(target_probe, "apply_service_override") as apply_override,
        ):
            scenario.apply_csp_socketcan_canfd_override()

        apply_override.assert_not_called()
        scenario.checkpoint.assert_called_once()
        self.assertEqual(scenario.checkpoint.call_args.args[:2], ("csp-socketcan-canfd-baseline-verified", "pass"))
        self.assertFalse(scenario.csp_socketcan_canfd_override_applied)

    def test_external_c_verifies_full_obc_profile_without_restart(self) -> None:
        scenario = self.make_scenario(externally_managed=True)

        with (
            mock.patch.object(target_probe, "service_environment", return_value=dict(EXPECTED_OBC_PROFILE)),
            mock.patch.object(target_probe, "apply_service_override") as apply_override,
            mock.patch.object(target_probe, "wait_service_active") as wait_active,
        ):
            scenario.apply_profile_override_if_needed()

        apply_override.assert_not_called()
        wait_active.assert_not_called()
        scenario.snapshot_service.assert_not_called()
        self.assertFalse(scenario.profile_override_applied)
        self.assertEqual(
            scenario.checkpoint.call_args.args[:2],
            ("profile-baseline-verified", "pass"),
        )

    def test_external_c_rejects_full_obc_profile_mismatch_without_restart(self) -> None:
        scenario = self.make_scenario(externally_managed=True)
        observed = dict(EXPECTED_OBC_PROFILE)
        observed["INITIAL_COMM_BAND"] = "uhf"

        with (
            mock.patch.object(target_probe, "service_environment", return_value=observed),
            mock.patch.object(target_probe, "apply_service_override") as apply_override,
            mock.patch.object(target_probe, "wait_service_active") as wait_active,
        ):
            with self.assertRaisesRegex(
                target_probe.ProbeFailure,
                "externally managed target baseline profile does not match",
            ):
                scenario.apply_profile_override_if_needed()

        apply_override.assert_not_called()
        wait_active.assert_not_called()
        scenario.snapshot_service.assert_not_called()
        self.assertFalse(scenario.profile_override_applied)
        self.assertEqual(
            scenario.checkpoint.call_args.args[:2],
            ("profile-baseline-mismatch", "fail"),
        )

    def test_probe_owned_profile_mismatch_keeps_existing_override_behavior(self) -> None:
        scenario = self.make_scenario(externally_managed=False)
        observed = dict(EXPECTED_OBC_PROFILE)
        observed["INITIAL_COMM_BAND"] = "uhf"

        with (
            mock.patch.object(target_probe, "service_environment", return_value=observed),
            mock.patch.object(target_probe, "apply_service_override") as apply_override,
            mock.patch.object(target_probe, "wait_service_active") as wait_active,
        ):
            scenario.apply_profile_override_if_needed()

        apply_override.assert_called_once_with(
            "obc",
            "obc.service",
            "51-target-comm-profile-override.conf",
            EXPECTED_OBC_PROFILE,
        )
        wait_active.assert_called_once_with("obc", "obc.service", 1)
        scenario.snapshot_service.assert_called_once_with(
            "obc",
            "obc.service",
            "profile-override-applied",
        )
        self.assertTrue(scenario.profile_override_applied)


if __name__ == "__main__":
    unittest.main()
