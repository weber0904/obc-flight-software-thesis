#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import sys
import time
import traceback

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
if str(ROOT_DIR / "scripts") not in sys.path:
    sys.path.insert(0, str(ROOT_DIR / "scripts"))

from run_target_can_matrix_probe import ProbeFailure, ensure_no_legacy_aliases, install_signal_cleanup  # noqa: E402
from target_autonomous_uhf_probe_common import AutonomousUhfScenario  # noqa: E402


def classify_failure(error: str) -> str:
    lowered = error.lower()
    if "ground" in lowered or "capture growth" in lowered or "event visibility" in lowered:
        return "ground-observability-failure"
    if "baseline" in lowered or "readiness" in lowered or "service" in lowered:
        return "environment-baseline-failure"
    return "target-runtime-failure"


class UhfPrimaryNonquietRuntimeProbe(AutonomousUhfScenario):
    def __init__(self, probe_root: pathlib.Path, *, per_subcase_attempts: int, command_spacing_sec: float) -> None:
        super().__init__(probe_root)
        self.mode = "uhf-primary-nonquiet-runtime"
        self.per_subcase_attempts = per_subcase_attempts
        self.command_spacing_sec = command_spacing_sec

    def summary_path(self) -> pathlib.Path:
        return self.diagnostics_dir / "uhf-primary-nonquiet-runtime-summary.json"

    def record_attempt(
        self,
        *,
        case_id: str,
        attempt_index: int,
        action,
    ) -> dict[str, object]:
        attempt: dict[str, object] = {
            "case": case_id,
            "attempt": attempt_index,
            "verdict": "FAIL",
        }
        try:
            outcome = action()
            attempt.update(outcome)
            attempt["verdict"] = "PASS"
        except ProbeFailure as exc:
            attempt["error"] = str(exc)
            retry_details = getattr(exc, "retry_details", None)
            if retry_details is not None:
                attempt["retryDetails"] = retry_details
                attempt["failureClass"] = str(retry_details.get("failureClass", classify_failure(str(exc))))
            else:
                attempt["failureClass"] = classify_failure(str(exc))
        return attempt

    def run_repeated_single_case(self, session) -> dict[str, object]:
        attempts: list[dict[str, object]] = []
        for index in range(1, self.per_subcase_attempts + 1):
            attempts.append(
                self.record_attempt(
                    case_id="repeated-single-get-reset-cause",
                    attempt_index=index,
                    action=lambda index=index: self.send_uhf_readback_with_ground_event(
                        session,
                        label=f"uhf-nonquiet-repeated-get-reset-cause-{index}",
                        command_name="OBCApp.bootManager.GET_RESET_CAUSE",
                        journal_fragments=("BOOT_RECOVERY_STATUS",),
                        ground_fragments=("BOOT_RECOVERY_STATUS",),
                    ),
                )
            )
            time.sleep(self.command_spacing_sec)
        return self.aggregate_case("repeated-single-get-reset-cause", attempts)

    def run_interleaved_case(self, session) -> dict[str, object]:
        attempts: list[dict[str, object]] = []
        for index in range(1, self.per_subcase_attempts + 1):
            attempts.append(
                self.record_attempt(
                    case_id="interleaved-reset-and-fault-history",
                    attempt_index=index,
                    action=lambda index=index: self.run_interleaved_round(session, index),
                )
            )
            time.sleep(self.command_spacing_sec)
        return self.aggregate_case("interleaved-reset-and-fault-history", attempts)

    def run_interleaved_round(self, session, index: int) -> dict[str, object]:
        get_reset = self.send_uhf_readback_with_ground_event(
            session,
            label=f"uhf-nonquiet-interleaved-get-reset-cause-{index}",
            command_name="OBCApp.bootManager.GET_RESET_CAUSE",
            journal_fragments=("BOOT_RECOVERY_STATUS",),
            ground_fragments=("BOOT_RECOVERY_STATUS",),
        )
        time.sleep(1.0)
        fault_history = self.send_uhf_readback_with_ground_event(
            session,
            label=f"uhf-nonquiet-interleaved-get-persistent-fault-history-{index}",
            command_name="OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
            command_args=("4",),
            journal_fragments=("PERSISTENT_FAULT_HISTORY_STATUS",),
            ground_fragments=("PERSISTENT_FAULT_HISTORY_STATUS",),
        )
        return {
            "resetCause": get_reset,
            "faultHistory": fault_history,
        }

    def aggregate_case(self, case_id: str, attempts: list[dict[str, object]]) -> dict[str, object]:
        success_count = sum(1 for item in attempts if item.get("verdict") == "PASS")
        failure_counts = {
            name: sum(1 for item in attempts if item.get("failureClass") == name)
            for name in (
                "target-runtime-failure",
                "ground-observability-failure",
                "environment-baseline-failure",
            )
        }
        return {
            "case": case_id,
            "attemptCount": len(attempts),
            "successCount": success_count,
            "requiredSuccessCount": 7,
            "verdict": "PASS" if success_count >= 7 else "FAIL",
            "failureCounts": failure_counts,
            "attempts": attempts,
        }

    def write_summary(
        self,
        *,
        verdict: str,
        aggregate_cases: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": "uhf-primary-nonquiet-runtime",
            "verdict": verdict,
            "targetPathUnderTest": "current detector-triggered autonomous UHF primary runtime with non-quiet live readback on governed target/lab node-6 path",
            "attemptsPerSubcase": self.per_subcase_attempts,
            "commandSpacingSec": self.command_spacing_sec,
            "cases": aggregate_cases,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "sbandUnavailableWindow": str(self.sband_window_artifact),
                "gatewayCaptures": self.secure_capture_summary(),
                "obcJournal": str(self.journal_snapshot_dir / "uhf-primary-nonquiet-runtime-obc.log"),
                "sbandServiceJournal": str(self.journal_snapshot_dir / "uhf-primary-nonquiet-runtime-sband-service.log"),
                "uhfServiceJournal": str(self.journal_snapshot_dir / "uhf-primary-nonquiet-runtime-uhf-service.log"),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.summary_path()
        self.write_json_artifact(path, payload)
        return path

    def run_probe(self) -> list[str]:
        failure_stage = "bootstrap"
        aggregate_cases: list[dict[str, object]] = []
        try:
            self.prepare_sband_secure_auth_case()
            failure_stage = "sband-auth"
            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            sband_oracle = self.build_ground_event_readback_oracle(
                event_log_path=self.sband.native_event_log,
                event_offset_attr="native_event_log_offset",
                ground_fragments=("BOOT_RECOVERY_STATUS",),
                source_name="ground-native-event-log",
            )
            self.send_secure_command_until_ground_readback(
                self.sband,
                sband_session,
                "uhf-nonquiet-sband-precheck-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                ground_success_oracle=sband_oracle,
                attempt_limit=30,
                per_attempt_timeout=1.0,
            )
            failure_stage = "prepare-uhf-ground"
            self.prepare_uhf_primary_ground(boundary="uhf-primary-nonquiet-runtime")
            failure_stage = "autonomous-failover-window"
            self.run_managed_sband_unavailable_window()
            failure_stage = "uhf-primary-reauth"
            uhf_session = self.authenticate_uhf_primary()

            failure_stage = "repeated-single-case"
            aggregate_cases.append(self.run_repeated_single_case(uhf_session))

            failure_stage = "interleaved-case"
            aggregate_cases.append(self.run_interleaved_case(uhf_session))

            verdict = "PASS" if all(case["verdict"] == "PASS" for case in aggregate_cases) else "FAIL"
            self.snapshot_journal(self.obc_target, self.obc_service, "uhf-primary-nonquiet-runtime-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "uhf-primary-nonquiet-runtime-sband-service", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "uhf-primary-nonquiet-runtime-uhf-service", lines=520)
            summary_path = self.write_summary(verdict=verdict, aggregate_cases=aggregate_cases)
            if verdict != "PASS":
                raise ProbeFailure(f"non-quiet UHF primary stability gate failed; summary={summary_path}")
            ensure_no_legacy_aliases(ROOT_DIR)
            return [
                "target-uhf-primary-nonquiet-runtime: PASS",
                f"uhf-primary-nonquiet-runtime-summary={summary_path}",
            ]
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "uhf-primary-nonquiet-runtime-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "uhf-primary-nonquiet-runtime-sband-service", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "uhf-primary-nonquiet-runtime-uhf-service", lines=520)
            summary_path = self.write_summary(
                verdict="FAIL",
                aggregate_cases=aggregate_cases,
                failure={
                    "stage": failure_stage,
                    "error": str(exc),
                    **({"retryDetails": getattr(exc, "retry_details")} if hasattr(exc, "retry_details") else {}),
                },
            )
            self.checkpoint(
                "uhf-primary-nonquiet-runtime-summary-written",
                "fail",
                artifact=str(summary_path),
                stage=failure_stage,
                error=str(exc),
            )
            raise


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    parser.add_argument("--attempts", type=int, default=10)
    parser.add_argument("--command-spacing-sec", type=float, default=2.0)
    args = parser.parse_args()

    scenario = UhfPrimaryNonquietRuntimeProbe(
        pathlib.Path(args.probe_root),
        per_subcase_attempts=args.attempts,
        command_spacing_sec=args.command_spacing_sec,
    )
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        output = "\n".join(scenario.run_probe()) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"target-uhf-primary-nonquiet-runtime: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        run_error = RuntimeError("unexpected exception")
        output = "target-uhf-primary-nonquiet-runtime: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"target-uhf-primary-nonquiet-runtime: FAIL cleanup {cleanup_exc}\n"
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
