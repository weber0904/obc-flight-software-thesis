#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import sys
import time
import traceback

import run_target_can_matrix_probe as target_probe
from run_payload_e2e_downlink_closure_v1_target_probe import PayloadDownlinkTargetScenario
from run_target_can_matrix_probe import ProbeFailure, command_opcode, ensure_no_legacy_aliases


class HkTrendDownlinkTargetScenario(PayloadDownlinkTargetScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "node5-comm-csp-downlink-v3-hk-target"
        self.target_file_bytes = int(os.environ.get("HK_TREND_TARGET_FILE_BYTES", "10240"))
        self.min_fdp_bytes = int(os.environ.get("HK_TREND_MIN_FDP_BYTES", "8192"))
        self.pending_wait_timeout_sec = float(os.environ.get("HK_TREND_PENDING_WAIT_TIMEOUT_SEC", "240"))
        self.reuse_existing_source = os.environ.get("HK_TREND_REUSE_EXISTING_SOURCE", "0") == "1"

    def load_hk_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.hkTrendProductProducer.HK_TREND_GET_STATUS": command_opcode(
                    dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_GET_STATUS"
                ),
                "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET": command_opcode(
                    dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET"
                ),
                "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE": command_opcode(
                    dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE"
                ),
            }
        )

    def start(self) -> None:
        super().start()
        self.load_hk_opcodes()

    @staticmethod
    def parse_hk_status_line(line: str) -> dict[str, int]:
        match = re.search(
            r"HK trend status target (?P<target>\d+) pendingSamples (?P<samples>\d+) pendingBytes (?P<bytes>\d+) "
            r"nextSample (?P<next_sample>\d+) nextChunk (?P<next_chunk>\d+)",
            line,
        )
        if match is None:
            raise ProbeFailure(f"could not parse HK_TREND_STATUS line: {line}")
        return {
            "targetFileBytes": int(match.group("target")),
            "pendingSampleCount": int(match.group("samples")),
            "pendingEstimatedBytes": int(match.group("bytes")),
            "nextSampleSequence": int(match.group("next_sample")),
            "nextChunkSequence": int(match.group("next_chunk")),
        }

    def wait_for_pending_hk_bytes(self, session: target_probe.SecureAuthSession) -> dict[str, int]:
        deadline = time.time() + self.pending_wait_timeout_sec
        last_status: dict[str, int] | None = None
        while time.time() < deadline:
            _, _, line = self.send_secure_command_until_fragments(
                session,
                "hk-trend-get-status",
                "OBCApp.hkTrendProductProducer.HK_TREND_GET_STATUS",
                event_fragments=("HK_TREND_STATUS",),
                journal_fragments=("HK_TREND_STATUS",),
                timeout=20.0,
            )
            last_status = self.parse_hk_status_line(line)
            self.checkpoint(
                "hk-trend-pending-status",
                "pass",
                target_file_bytes=last_status["targetFileBytes"],
                pending_sample_count=last_status["pendingSampleCount"],
                pending_estimated_bytes=last_status["pendingEstimatedBytes"],
                next_sample_sequence=last_status["nextSampleSequence"],
                next_chunk_sequence=last_status["nextChunkSequence"],
            )
            if last_status["targetFileBytes"] != self.target_file_bytes:
                raise ProbeFailure(
                    f"HK_TREND_TARGET_FILE_BYTES did not latch to {self.target_file_bytes}: {last_status}"
                )
            if last_status["pendingEstimatedBytes"] >= self.min_fdp_bytes:
                return last_status
            time.sleep(5.0)
        raise ProbeFailure(
            "HK trend pending bytes did not reach required minimum before flush: "
            f"required={self.min_fdp_bytes} last={last_status}"
        )

    def remote_file_size(self, remote_path: str) -> int:
        output = target_probe.ssh_capture(
            self.obc_target,
            f"python3 - <<'PY'\nimport os\nprint(os.path.getsize({remote_path!r}))\nPY",
        ).strip()
        return int(output)

    def list_existing_source_paths(self) -> dict[str, int]:
        source_paths = self.list_remote_data_product_files()
        eligible: dict[str, int] = {}
        for source_path in source_paths:
            source_size = self.remote_file_size(source_path)
            if source_size >= self.min_fdp_bytes:
                eligible[source_path] = source_size
        if not eligible:
            raise ProbeFailure(
                "HK_TREND_REUSE_EXISTING_SOURCE requested but no existing remote .fdp met the minimum size: "
                f"required={self.min_fdp_bytes} observed={source_paths}"
            )
        return eligible

    def choose_existing_source_path(self, eligible_sources: dict[str, int]) -> tuple[str, int]:
        chosen_path, chosen_size = max(
            eligible_sources.items(),
            key=lambda item: (item[1], item[0]),
        )
        return chosen_path, chosen_size

    def prune_remote_data_products_to(self, keep_source_path: str) -> None:
        data_products_root = self.runtime_root + "/data-products"
        target_probe.ssh_capture(
            self.obc_target,
            (
                f"set -euo pipefail; mkdir -p {target_probe.shq(data_products_root)}; "
                f"find {target_probe.shq(data_products_root)} -maxdepth 1 -type f -name 'Dp_*.fdp' "
                f"! -path {target_probe.shq(keep_source_path)} -delete; "
                f"rm -f {target_probe.shq(data_products_root + '/DpState.dat')}"
            ),
        )

    def write_summary(
        self,
        *,
        case_results: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": self.mode,
            "profile": "sband",
            "verdict": "FAIL" if failure else "PASS",
            "targetPathUnderTest": "node-5 official HK .fdp via DpCatalog -> FileDownlink -> ComCcsds -> GroundLinkDriver -> COMM_CSP v3",
            "cases": case_results,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "obcJournal": str(self.journal_snapshot_dir / "node5-comm-csp-downlink-v3-hk-target-obc.log"),
                "sbandServiceJournal": str(
                    self.journal_snapshot_dir / "node5-comm-csp-downlink-v3-hk-target-sband-service.log"
                ),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
            "constraints": {
                "targetFileBytes": self.target_file_bytes,
                "minFdpBytes": self.min_fdp_bytes,
            },
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "node5-comm-csp-downlink-v3-hk-target-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def run_probe(self) -> list[str]:
        summary: list[str] = []
        case_results: list[dict[str, object]] = []
        failure_stage = "hk-target-bootstrap"
        try:
            failure_stage = "hk-target-begin-profile"
            self.begin_profile()

            failure_stage = "hk-target-provenance"
            self.record_secure_auth_provenance()
            case_results.append({"case": "installed-release-keystore-provenance", "verdict": "PASS"})

            failure_stage = "hk-target-sband-ingress"
            self.apply_sband_ingress_diagnostics_override()
            self.start_security_server()
            self.wait_for_sband_tcp_reachability(10.0)
            self.start_ground_paths(need_sband=True, need_uhf=False)
            readiness = self.ensure_sband_ground_ready()
            summary.append(f"sband-ground-readiness={readiness}")

            failure_stage = "hk-target-authenticate"
            session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            case_results.append({"case": "sband-apid-00fe-secure-auth", "verdict": "PASS"})

            pending_status: dict[str, object]
            failure_stage = "hk-target-prepare-source"
            if self.reuse_existing_source:
                reuse_existing_sources = self.list_existing_source_paths()
                source_path, source_size = self.choose_existing_source_path(reuse_existing_sources)
                self.prune_remote_data_products_to(source_path)
                pending_status = {
                    "reuseExistingSource": True,
                    "eligibleSourceCount": len(reuse_existing_sources),
                    "eligibleSourcePaths": list(reuse_existing_sources.keys()),
                    "selectedSourcePath": source_path,
                    "selectedSourceSizeBytes": source_size,
                }
                self.checkpoint(
                    "hk-trend-existing-source-selected",
                    "pass",
                    eligible_source_count=len(reuse_existing_sources),
                    eligible_source_paths=list(reuse_existing_sources.keys()),
                    selected_source_path=source_path,
                    selected_source_size_bytes=source_size,
                    min_fdp_bytes=self.min_fdp_bytes,
                )
            else:
                failure_stage = "hk-target-set-target-file-bytes"
                self.send_secure_command_until_fragments(
                    session,
                    "hk-trend-target-file-bytes-set",
                    "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET",
                    str(self.target_file_bytes),
                    event_fragments=(
                        f"Opcode 0x{self.opcodes['OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET']:08x} completed",
                    ),
                    journal_fragments=(
                        f"Opcode 0x{self.opcodes['OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET']:08x} completed",
                    ),
                )
                self.send_secure_command_until_fragments(
                    session,
                    "hk-trend-target-file-bytes-save",
                    "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE",
                    event_fragments=(
                        f"Opcode 0x{self.opcodes['OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE']:08x} completed",
                    ),
                    journal_fragments=(
                        f"Opcode 0x{self.opcodes['OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE']:08x} completed",
                    ),
                )
                pending_status = self.wait_for_pending_hk_bytes(session)

                failure_stage = "hk-target-flush-and-downlink"
                self.remove_received_fdp_files(self.sband)
                self.reset_remote_data_products()
                self.send_secure_command_until_fragments(
                    session,
                    "hk-trend-flush",
                    "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH",
                    event_fragments=("HK_TREND_PRODUCT_WRITTEN",),
                    journal_fragments=("HK_TREND_PRODUCT_WRITTEN",),
                    timeout=30.0,
                )
                source_paths = self.wait_for_remote_data_product_files(30)
                if len(source_paths) != 1:
                    raise ProbeFailure(f"expected exactly one remote HK .fdp after reset+flush, observed {source_paths}")
                source_path = source_paths[0]
                source_size = self.remote_file_size(source_path)
                if source_size < self.min_fdp_bytes:
                    raise ProbeFailure(
                        f"generated HK .fdp smaller than required debug floor: size={source_size} required={self.min_fdp_bytes}"
                    )

            failure_stage = "hk-target-flush-and-downlink"
            self.remove_received_fdp_files(self.sband)

            self.send_secure_command_until_fragments(
                session,
                "hk-build-catalog",
                "OBCApp.dpCatalog.BUILD_CATALOG",
                event_fragments=("CatalogBuildComplete",),
                journal_fragments=("CatalogBuildComplete",),
                timeout=30.0,
            )
            status_before_xmit = self.run_remote_downlink_v3_status_probe("hk-downlink-v3-status-before-xmit.log")
            if self.reuse_existing_source:
                sequence_number, _ = self.send_secure_command_name(
                    self.sband,
                    session,
                    "sband-ground secure START_XMIT_CATALOG NO_WAIT",
                    "OBCApp.dpCatalog.START_XMIT_CATALOG",
                    "NO_WAIT",
                    accept_sequence=True,
                    journal_fragments=(),
                    timeout=25.0,
                )
                self.checkpoint(
                    "hk-trend-existing-source-confirmed",
                    "pass",
                    source_path=source_path,
                    source_size_bytes=source_size,
                    min_fdp_bytes=self.min_fdp_bytes,
                    sequence_number=sequence_number,
                    oracle="pre-pruned-source",
                )
            else:
                start_xmit_source_path = self.run_start_xmit_catalog_secure(self.sband, session)
                if start_xmit_source_path != source_path:
                    raise ProbeFailure(
                        f"START_XMIT_CATALOG selected unexpected source path: expected={source_path} observed={start_xmit_source_path}"
                    )
            expected_snapshots = self.snapshot_remote_source_files([source_path], "hk-target-source")
            matched_entry, matched_received_path, matched_hash, matched_size = self.wait_for_any_matching_file(
                self.sband,
                expected_snapshots,
                240,
            )
            status_after_match = self.run_remote_downlink_v3_status_probe("hk-downlink-v3-status-after-match.log")
            if int(status_after_match.get("acceptedBytes", 0)) <= int(status_before_xmit.get("acceptedBytes", 0)):
                raise ProbeFailure("node-5 acceptedBytes did not advance across HK official downlink")
            case_results.append(
                {
                    "case": "hk-official-downlink",
                    "verdict": "PASS",
                    "sourcePath": source_path,
                    "sourceSizeBytes": source_size,
                    "receivedPath": str(matched_received_path),
                    "receivedSizeBytes": matched_size,
                    "receivedSha256": matched_hash,
                    "pendingStatusBeforeFlush": pending_status,
                    "sourceSnapshot": str(matched_entry["snapshot_path"]),
                    "statusBeforeXmit": status_before_xmit,
                    "statusAfterMatch": status_after_match,
                }
            )

            self.snapshot_journal(self.obc_target, self.obc_service, "node5-comm-csp-downlink-v3-hk-target-obc", lines=400)
            self.snapshot_journal(
                self.subsystem_target,
                self.sband_comm_service,
                "node5-comm-csp-downlink-v3-hk-target-sband-service",
                lines=240,
            )
            summary_path = self.write_summary(case_results=case_results)
            summary.extend(
                [
                    f"summary={summary_path}",
                    f"source-path={source_path}",
                    f"source-size-bytes={source_size}",
                    f"received-path={matched_received_path}",
                    f"received-size-bytes={matched_size}",
                    "hk-official-byte-match=PASS",
                    "node5-transport=v3",
                    "target-node5-hk-official-downlink=PASS",
                ]
            )
            ensure_no_legacy_aliases(self.root_dir)
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "node5-comm-csp-downlink-v3-hk-target-obc", lines=400)
            self.snapshot_journal(
                self.subsystem_target,
                self.sband_comm_service,
                "node5-comm-csp-downlink-v3-hk-target-sband-service",
                lines=240,
            )
            summary_path = self.write_summary(
                case_results=case_results,
                failure={"stage": failure_stage, "error": str(exc)},
            )
            self.checkpoint(
                "node5-comm-csp-downlink-v3-hk-target-summary-written",
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

    scenario = HkTrendDownlinkTargetScenario(pathlib.Path(args.probe_root))
    target_probe.install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        lines = [
            "node5-comm-csp-downlink-v3-hk-target-probe: PASS",
            f"probe-root={args.probe_root}",
        ]
        lines.extend(scenario.run_probe())
        output = "\n".join(lines) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"node5-comm-csp-downlink-v3-hk-target-probe: FAIL {exc}\n"
        exit_code = 1
    except Exception as exc:  # pragma: no cover - top-level trap
        run_error = exc
        output = "node5-comm-csp-downlink-v3-hk-target-probe: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"node5-comm-csp-downlink-v3-hk-target-probe: FAIL cleanup {cleanup_exc}\n"
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
