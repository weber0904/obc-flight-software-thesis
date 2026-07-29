#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys
import traceback

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import ProbeFailure, ensure_no_legacy_aliases, install_signal_cleanup  # noqa: E402
from target_autonomous_uhf_probe_common import AutonomousUhfScenario  # noqa: E402


class AutonomousFailoverProof(AutonomousUhfScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.include_hk_file = os.getenv("AUTONOMOUS_FAILOVER_INCLUDE_HK_FILE", "0") == "1"

    def summary_path(self) -> pathlib.Path:
        return self.diagnostics_dir / "autonomous-uhf-failover-summary.json"

    def write_summary(
        self,
        *,
        verdict: str,
        cases: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": "autonomous-uhf-failover",
            "profile": "sband-bootstrap-to-uhf-primary",
            "verdict": verdict,
            "targetPathUnderTest": "current detector-triggered COMM_PRIMARY_UNAVAILABLE -> executor-owned UHF primary failover on governed target/lab node-5/node-6 baseline",
            "cases": cases,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "sbandUnavailableWindow": str(self.sband_window_artifact),
                "gatewayCaptures": self.secure_capture_summary(),
                "obcJournal": str(self.journal_snapshot_dir / "autonomous-uhf-failover-obc.log"),
                "sbandServiceJournal": str(self.journal_snapshot_dir / "autonomous-uhf-failover-sband-service.log"),
                "uhfServiceJournal": str(self.journal_snapshot_dir / "autonomous-uhf-failover-uhf-service.log"),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.summary_path()
        self.write_json_artifact(path, payload)
        return path

    def run_probe(self) -> list[str]:
        summary: list[str] = []
        case_results: list[dict[str, object]] = []
        failure_stage = "bootstrap"
        try:
            readiness = self.prepare_sband_secure_auth_case()
            summary.append(f"sband-ground-readiness={readiness}")

            failure_stage = "sband-auth"
            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            case_results.append({"case": "sband-secure-auth-bootstrap", "verdict": "PASS"})

            failure_stage = "sband-precheck"
            sband_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("BOOT_RECOVERY_STATUS",),
                source_name="ground-native-event-log",
            )
            precheck = self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "autonomous-failover-sband-secure-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_success_oracle=sband_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            case_results.append(
                {
                    "case": "sband-precheck-get-reset-cause",
                    "verdict": "PASS",
                    **precheck,
                }
            )

            failure_stage = "prepare-uhf-ground"
            self.prepare_uhf_primary_ground(boundary="autonomous-failover")

            failure_stage = "autonomous-failover-window"
            failover = self.run_managed_sband_unavailable_window()
            case_results.append(
                {
                    "case": "managed-sband-unavailable-window",
                    "verdict": "PASS",
                    "source": failover.get("failoverSource", "target-journal"),
                    "artifact": str(self.sband_window_artifact),
                }
            )

            failure_stage = "uhf-primary-reauth"
            beacon_start_offset = self.log_length(self.uhf.native_event_log)
            uhf_session = self.authenticate_uhf_primary()
            case_results.append({"case": "uhf-primary-reauth-after-autonomous-failover", "verdict": "PASS"})

            failure_stage = "uhf-beacon-suppress-start"
            self.wait_for_log_fragments_since(
                self.uhf.native_event_log,
                beacon_start_offset,
                ("COMM_UHF_BEACON_SUPPRESS_STARTED",),
                timeout=10.0,
                label="uhf primary beacon suppress start",
            )
            case_results.append({"case": "uhf-primary-auth-starts-beacon-suppress", "verdict": "PASS"})

            failure_stage = "uhf-primary-get-reset-cause"
            get_reset = self.send_uhf_readback_with_ground_event(
                uhf_session,
                label="uhf-primary-autonomous-failover-get-reset-cause",
                command_name="OBCApp.bootManager.GET_RESET_CAUSE",
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_fragments=("BOOT_RECOVERY_STATUS",),
            )
            case_results.append({"case": "uhf-primary-get-reset-cause", "verdict": "PASS", **get_reset})

            failure_stage = "uhf-primary-get-persistent-fault-history"
            fault_history = self.send_uhf_readback_with_ground_event(
                uhf_session,
                label="uhf-primary-autonomous-failover-get-persistent-fault-history",
                command_name="OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                command_args=("4",),
                journal_fragments=("PERSISTENT_FAULT_HISTORY_STATUS",),
                ground_fragments=("PERSISTENT_FAULT_HISTORY_STATUS",),
            )
            case_results.append({"case": "uhf-primary-get-persistent-fault-history", "verdict": "PASS", **fault_history})

            if self.include_hk_file:
                failure_stage = "uhf-primary-hk-file"
                source_path, received_path, received_size = self.run_file_downlink_secure(
                    self.uhf,
                    uhf_session,
                    "autonomous-failover-uhf-primary-hk-file",
                )
                case_results.append(
                    {
                        "case": "uhf-primary-hk-file-downlink",
                        "verdict": "PASS",
                        "sourcePath": source_path,
                        "receivedPath": received_path,
                        "receivedSize": received_size,
                    }
                )

            self.snapshot_journal(self.obc_target, self.obc_service, "autonomous-uhf-failover-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "autonomous-uhf-failover-sband-service", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "autonomous-uhf-failover-uhf-service", lines=520)
            summary_path = self.write_summary(verdict="PASS", cases=case_results)
            summary.extend(
                [
                    "target-autonomous-uhf-failover: PASS",
                    f"autonomous-uhf-failover-summary={summary_path}",
                ]
            )
            ensure_no_legacy_aliases(ROOT_DIR)
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "autonomous-uhf-failover-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "autonomous-uhf-failover-sband-service", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "autonomous-uhf-failover-uhf-service", lines=520)
            summary_path = self.write_summary(
                verdict="FAIL",
                cases=case_results,
                failure={
                    "stage": failure_stage,
                    "error": str(exc),
                    **({"retryDetails": getattr(exc, "retry_details")} if hasattr(exc, "retry_details") else {}),
                },
            )
            self.checkpoint(
                "autonomous-uhf-failover-summary-written",
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

    scenario = AutonomousFailoverProof(pathlib.Path(args.probe_root))
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        output = "\n".join(scenario.run_probe()) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"target-autonomous-uhf-failover: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        run_error = RuntimeError("unexpected exception")
        output = "target-autonomous-uhf-failover: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"target-autonomous-uhf-failover: FAIL cleanup {cleanup_exc}\n"
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
