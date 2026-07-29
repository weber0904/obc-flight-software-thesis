#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import traceback

import run_target_can_matrix_probe as target_probe
from payload_image_sanity import evaluate_luma_sanity
from run_payload_e2e_downlink_closure_v1_target_probe import PayloadDownlinkTargetScenario, ProbeFailure, file_sha256


AUTO_CAPTURE_INDEX = 0x30
DETERMINISTIC_CAPTURE_INDEX = 0x31
SEQUENCE_WAIT_AFTER_CAPTURE_SEC = 30
SEQUENCE_PREFLIGHT_TIMEOUT_SEC = 5.0
SEQUENCE_RESULT_TIMEOUT_SEC = 240.0
METADATA_WAIT_TIMEOUT_SEC = 180.0


class Route1SequencePayloadTargetScenario(PayloadDownlinkTargetScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "chapter5-route1-sequence-target"

    def load_payload_opcodes(self) -> None:
        super().load_payload_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO"
                ),
            }
        )

    def resolve_eps_control_socket(self) -> str:
        for env_source in (
            target_probe.service_environment(self.subsystem_target, self.eps_service),
            target_probe.service_process_environment(self.subsystem_target, self.eps_service),
        ):
            value = env_source.get("EPS_SIM_CONTROL_SOCKET", "").strip()
            if value:
                return value
        raise ProbeFailure(
            f"{self.eps_service} on {self.subsystem_target} does not expose EPS_SIM_CONTROL_SOCKET; "
            "install the updated subsystem EPS service baseline first"
        )

    def remote_set_soc(self, socket_path: str, value: float, transition_sec: float = 0.0) -> str:
        script = """
import socket
import sys

socket_path = sys.argv[1]
command = f"set-soc {float(sys.argv[2]):.2f} {float(sys.argv[3]):.2f}\\n".encode("utf-8")
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(socket_path)
sock.sendall(command)
response = sock.recv(4096).decode("utf-8", errors="replace").strip()
sock.close()
print(response)
if not response.startswith("OK "):
    raise SystemExit(2)
""".strip()
        return target_probe.ssh_capture(
            self.subsystem_target,
            f"python3 -c {target_probe.shlex.quote(script)} {target_probe.shlex.quote(socket_path)} {value:.2f} {transition_sec:.2f}",
        ).strip()

    def build_sequence_artifact(self, name: str) -> target_probe.SequenceArtifact:
        source = self.sequence_src_dir / f"{name}.seq"
        source.write_text(
            "\n".join(
                [
                    "; Route 1 target payload sequence demo",
                    "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS PRESET_VGA_640X480 90 false false",
                    "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS AWB_AUTO METER_CENTRE 0",
                    "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS 30000 400",
                    "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_PREPARE",
                    f'R00:00:00 OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO {AUTO_CAPTURE_INDEX} "route1-auto" 0 AWB_AUTO METER_CENTRE 0',
                    f"R00:00:{SEQUENCE_WAIT_AFTER_CAPTURE_SEC:02d} OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                    f'R00:00:05 OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC {DETERMINISTIC_CAPTURE_INDEX} "route1-det" {AUTO_CAPTURE_INDEX} 30000 400',
                    f"R00:00:{SEQUENCE_WAIT_AFTER_CAPTURE_SEC:02d} OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        binary = self.sequence_bin_dir / f"{name}.bin"
        target_probe.subprocess.run(
            [str(self.seqgen_path), "--dictionary", str(self.dictionary_path), str(source), str(binary)],
            check=True,
            cwd=str(self.root_dir),
            stdout=target_probe.subprocess.DEVNULL,
            stderr=target_probe.subprocess.DEVNULL,
        )
        return target_probe.SequenceArtifact(source=source, binary=binary, destination=f".sequence-staging/{name}.bin")

    def collect_case_result(
        self,
        *,
        case_name: str,
        metadata: dict[str, object],
    ) -> dict[str, object]:
        source_local_dir = self.probe_root / "source-artifacts" / case_name
        mirrored = self.mirror_remote_capture_artifacts(
            relative_raw=str(metadata["rawRelativePath"]),
            relative_preview=str(metadata["previewRelativePath"]),
            local_dir=source_local_dir,
        )
        preview_local_path = pathlib.Path(mirrored["localPreviewPath"])
        raw_local_path = pathlib.Path(mirrored["localRawPath"])
        preview_fdp_path = pathlib.Path(mirrored["localDataProductsDir"]) / pathlib.Path(str(metadata["previewDataProductPath"])).name
        if not preview_fdp_path.is_file():
            raise ProbeFailure(f"{case_name} preview .fdp missing after mirror: {preview_fdp_path}")
        decode_dir = self.probe_root / "source-decode" / case_name / "PREVIEW_JPEG"
        extract_result = self.run_payload_extract(
            received_fdp=preview_fdp_path,
            output_dir=decode_dir,
            expected_capture_id=int(metadata["captureId"]),
            expected_capture_index=int(metadata["captureIndex"]),
            expected_artifact_kind="PREVIEW_JPEG",
            expected_relative_path=str(metadata["previewRelativePath"]),
            expected_relative_data_product_path=str(metadata["previewDataProductPath"]),
            expected_resolution="PRESET_VGA_640X480",
            expected_capture_policy=str(metadata["capturePolicy"]),
            expected_source_artifact_sha256=file_sha256(preview_local_path),
            require_valid_jpeg=True,
        )
        luma = evaluate_luma_sanity(
            raw_local_path.read_bytes(),
            str(extract_result["pixelFormat"]),
            int(extract_result["imageWidth"]),
            int(extract_result["imageHeight"]),
            mean_threshold=1.0,
            p95_threshold=8,
        )
        if not bool(luma["passed"]):
            raise ProbeFailure(
                f"{case_name} preview image failed luma sanity: mean={float(luma['meanLuma']):.3f} "
                f"p95={int(luma['p95Luma'])}"
            )
        preview_artifact = self.build_artifact_entry(
            case_name=case_name,
            resolution="PRESET_VGA_640X480",
            capture_id=int(metadata["captureId"]),
            capture_index=int(metadata["captureIndex"]),
            capture_policy=str(metadata["capturePolicy"]),
            artifact_kind="PREVIEW_JPEG",
            downlink_required=case_name == "deterministic",
            relative_path=str(metadata["previewRelativePath"]),
            relative_data_product_path=str(metadata["previewDataProductPath"]),
            source_artifact_path=preview_local_path,
            source_fdp_path=preview_fdp_path,
        )
        self.build_source_family_summaries([preview_artifact])
        return {
            "name": case_name,
            "capturePolicy": str(metadata["capturePolicy"]),
            "captureId": int(metadata["captureId"]),
            "captureIndex": int(metadata["captureIndex"]),
            "actualExposureUsec": int(metadata["actualExposureUsec"]),
            "actualGainX100": int(metadata["actualGainX100"]),
            "actualAwbValid": bool(metadata["actualAwbValid"]),
            "previewPublished": bool(metadata["previewPublished"]),
            "rawPublished": bool(metadata["rawPublished"]),
            "previewRelativePath": str(metadata["previewRelativePath"]),
            "previewDataProductPath": str(metadata["previewDataProductPath"]),
            "sourcePreviewPath": str(preview_local_path),
            "sourcePreviewBytes": preview_local_path.stat().st_size,
            "sourcePreviewSha256": file_sha256(preview_local_path),
            "sourceRawPath": str(raw_local_path),
            "sourceRawBytes": raw_local_path.stat().st_size,
            "sourceRawSha256": file_sha256(raw_local_path),
            "extractSummary": extract_result,
            "lumaSanity": luma,
            "artifacts": [preview_artifact],
        }

    def set_mode_payload(self, session: target_probe.SecureAuthSession) -> None:
        eps_socket = self.resolve_eps_control_socket()
        self.remote_set_soc(eps_socket, 80.0)
        self.send_secure_command_name(
            self.sband,
            session,
            "route1-sequence-eps-refresh",
            "OBCApp.epsBridge.EPS_GET_STATUS",
            accept_sequence=True,
            journal_fragments=("EPS_STATUS_RECEIVED",),
            timeout=20.0,
        )
        self.send_secure_command_until_fragments(
            session,
            "route1-sequence-mode-set-idle",
            "OBCApp.modeManager.MODE_SET",
            "IDLE",
            event_fragments=("SYS_MODE_CHANGE", "IDLE (1)"),
            journal_fragments=("SYS_MODE_CHANGE", "IDLE (1)"),
            timeout=20.0,
        )
        self.send_secure_command_until_fragments(
            session,
            "route1-sequence-mode-set-payload",
            "OBCApp.modeManager.MODE_SET",
            "PAYLOAD",
            event_fragments=("SYS_MODE_CHANGE", "PAYLOAD (3)"),
            journal_fragments=("SYS_MODE_CHANGE", "PAYLOAD (3)"),
            timeout=20.0,
        )

    def run_route1_sequence(self) -> list[str]:
        self.reset_remote_runtime()
        session = self.prepare_payload_target_case()
        capabilities_line = self.collect_payload_capabilities(session)
        self.set_mode_payload(session)

        sequence_artifact = self.build_sequence_artifact("route1-sequence-demo")
        self.upload_sequence(self.sband, sequence_artifact)

        validate_journal_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        validate_start, validate_sequence = self.send_secure_command_without_wait(
            session,
            "route1-seq-validate",
            "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
            sequence_artifact.destination,
        )
        self.sband.assert_no_event("SEQUENCE_CONTROL_REJECTED", timeout=SEQUENCE_PREFLIGHT_TIMEOUT_SEC, start=validate_start)
        self.sband.assert_no_event("COMMAND_AUTHORITY_REJECTED", timeout=SEQUENCE_PREFLIGHT_TIMEOUT_SEC, start=validate_start)
        self.sband.assert_no_event("COMMAND_SEQUENCE_REJECTED", timeout=SEQUENCE_PREFLIGHT_TIMEOUT_SEC, start=validate_start)
        validate_journal = target_probe.ssh_capture(
            self.obc_target,
            f"journalctl -u {target_probe.shlex.quote(self.obc_service)} --since {target_probe.shlex.quote(validate_journal_since)} --no-pager || true",
            check=False,
        )
        # The preflight decision is target-journal based.  Retain that exact
        # SSH response so the evidence pack contains the raw observation,
        # rather than only its derived checkpoint verdict.
        validate_journal_path = self.journal_snapshot_dir / "route1-seq-validate-obc.log"
        validate_journal_path.write_text(validate_journal, encoding="utf-8")
        for forbidden in ("SEQUENCE_CONTROL_REJECTED", "COMMAND_AUTHORITY_REJECTED", "COMMAND_SEQUENCE_REJECTED", "CS_CommandError"):
            if forbidden in validate_journal:
                raise ProbeFailure(f"route1 sequence validate preflight observed {forbidden}: {validate_journal[-4000:]}")
        self.checkpoint(
            "payload-secure-command-completed",
            "pass",
            label="route1-seq-validate",
            command="OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
            sequence=validate_sequence,
            source="bounded-nonreject-preflight",
        )

        run_journal_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        run_start, _ = self.send_secure_command_without_wait(
            session,
            "route1-seq-run",
            "OBCApp.sequenceAdmissionController.SEQ_RUN",
            sequence_artifact.destination,
            "WAIT",
        )
        self.sband.assert_no_event("SEQUENCE_CONTROL_REJECTED", timeout=3.0, start=run_start)
        self.sband.assert_no_event("COMMAND_AUTHORITY_REJECTED", timeout=3.0, start=run_start)
        self.sband.assert_no_event("COMMAND_SEQUENCE_REJECTED", timeout=3.0, start=run_start)
        self.wait_all_event_or_journal(
            event_start=run_start,
            event_fragments=("CS_SequenceComplete",),
            journal_since=run_journal_since,
            journal_fragments=("CS_SequenceComplete",),
            timeout=SEQUENCE_RESULT_TIMEOUT_SEC,
            label="route1 sequence complete",
            prefer_target_journal=True,
        )
        target_tail = target_probe.ssh_capture(
            self.obc_target,
            f"journalctl -u {target_probe.shlex.quote(self.obc_service)} --since {target_probe.shlex.quote(run_journal_since)} --no-pager || true",
            check=False,
        )
        if "SEQUENCE_CONTEXT_UPDATED" in target_tail and "state FAILED" in target_tail:
            raise ProbeFailure(f"route1 sequence reported FAILED: {target_tail[-4000:]}")
        if "CS_CommandError" in target_tail:
            raise ProbeFailure(f"route1 sequence reported CS_CommandError: {target_tail[-4000:]}")

        _, _, auto_metadata = self.wait_metadata_snapshot(
            event_start=run_start,
            journal_since=run_journal_since,
            timeout=METADATA_WAIT_TIMEOUT_SEC,
            label="route1 AUTO metadata",
            capture_index=AUTO_CAPTURE_INDEX,
            require_preview_published=True,
            prefer_target_journal=True,
        )
        _, _, det_metadata = self.wait_metadata_snapshot(
            event_start=run_start,
            journal_since=run_journal_since,
            timeout=METADATA_WAIT_TIMEOUT_SEC,
            label="route1 DETERMINISTIC metadata",
            capture_index=DETERMINISTIC_CAPTURE_INDEX,
            require_preview_published=True,
            prefer_target_journal=True,
        )
        if str(auto_metadata["capturePolicy"]) != "CAPTURE_AUTO":
            raise ProbeFailure(f"route1 AUTO metadata had wrong capture policy: {auto_metadata}")
        if str(det_metadata["capturePolicy"]) != "CAPTURE_DETERMINISTIC":
            raise ProbeFailure(f"route1 DETERMINISTIC metadata had wrong capture policy: {det_metadata}")

        auto_result = self.collect_case_result(case_name="auto", metadata=auto_metadata)
        deterministic_result = self.collect_case_result(case_name="deterministic", metadata=det_metadata)

        self.send_secure_command_until_fragments(
            session,
            "route1-seq-shutdown",
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
            event_fragments=("PSTATE_OFF",),
            journal_fragments=("PSTATE_OFF",),
            timeout=30.0,
        )

        det_preview_artifact = deterministic_result["artifacts"][0]
        source_family_summary = det_preview_artifact["sourceFamilySummary"]
        downlink_source_fdps = [
            pathlib.Path(str(path)) for path in source_family_summary["familyFdpFiles"]
        ]
        prune_summary = self.prune_remote_catalog_to_payload_products(downlink_source_fdps)
        self.checkpoint(
            "route1-sequence-remote-catalog-pruned",
            "pass",
            kept_count=len(prune_summary.get("kept", [])),
            removed_count=len(prune_summary.get("removed", [])),
        )

        self.send_secure_command_until_fragments(
            session,
            "route1-build-catalog",
            "OBCApp.dpCatalog.BUILD_CATALOG",
            event_fragments=("CatalogBuildComplete",),
            journal_fragments=("CatalogBuildComplete",),
            timeout=30.0,
        )
        start_xmit_journal_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        start_xmit_requested_ts = target_probe.time.time()
        start_xmit_event_start, _, _ = self.send_secure_command_until_fragments(
            session,
            "route1-start-xmit-catalog",
            "OBCApp.dpCatalog.START_XMIT_CATALOG",
            "NO_WAIT",
            event_fragments=("SendingProduct",),
            journal_fragments=("SendingProduct",),
            timeout=30.0,
        )
        catalog_complete_source, catalog_complete_line = self.wait_all_event_or_journal(
            event_start=start_xmit_event_start,
            event_fragments=("CatalogXmitCompleted",),
            journal_since=start_xmit_journal_since,
            journal_fragments=("CatalogXmitCompleted",),
            timeout=900.0,
            label="route1 catalog complete",
            prefer_target_journal=True,
        )
        catalog_complete_ts = target_probe.time.time()
        matched_received = self.find_matching_received_fdps([det_preview_artifact], 900.0)
        matched = matched_received[(str(det_preview_artifact["name"]), str(det_preview_artifact["artifactKind"]))]
        received_fdp = pathlib.Path(str(matched["receivedFdp"]))
        try:
            self.send_secure_command_until_fragments(
                session,
                "route1-stop-xmit-catalog",
                "OBCApp.dpCatalog.STOP_XMIT_CATALOG",
                event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.dpCatalog.STOP_XMIT_CATALOG']:08x} completed",),
                journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.dpCatalog.STOP_XMIT_CATALOG']:08x} completed",),
                timeout=30.0,
            )
        except ProbeFailure as exc:
            self.checkpoint("route1-stop-xmit-catalog", "info", error=str(exc))

        downlink_summary = {
            "sourcePreviewFdpPath": str(det_preview_artifact["sourceFdpPath"]),
            "receivedPreviewFdpPath": str(received_fdp),
            "catalogCompleteSource": catalog_complete_source,
            "catalogCompleteLine": catalog_complete_line,
            "startToCatalogCompletedSec": round(catalog_complete_ts - start_xmit_requested_ts, 3),
            "catalogCompletedToFinalGdsArrivalSec": round(
                float(matched["familyLastArrivalUnixSec"]) - catalog_complete_ts,
                3,
            ),
            "receivedExtractSummary": matched["extractSummary"],
        }

        payload = {
            "formalVerdict": "chapter5-route1-sequence-target=PASS",
            "probeRoot": str(self.probe_root),
            "targetPathUnderTest": "governed node-5 secure-auth payload sequence AUTO->metadata->DETERMINISTIC->metadata plus manual preview catalog downlink",
            "sequenceSource": str(sequence_artifact.source),
            "sequenceBinary": str(sequence_artifact.binary),
            "stagedSequencePath": sequence_artifact.destination,
            "capabilitiesLine": capabilities_line,
            "cases": [auto_result, deterministic_result],
            "downlink": downlink_summary,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "sequenceValidateTargetJournal": str(validate_journal_path),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
                "summaryJson": str(self.probe_root / "route1-sequence-target-summary.json"),
            },
        }
        self.write_json_artifact(self.probe_root / "route1-sequence-target-summary.json", payload)
        return [
            "chapter5-route1-sequence-target: PASS",
            f"probe-root={self.probe_root}",
            f"sequence-source={sequence_artifact.source}",
            f"sequence-path={sequence_artifact.destination}",
            "sequence-validate=PASS",
            "sequence-run-payload-auto-det=PASS",
            "capture-policies=AUTO(sequence)|DETERMINISTIC(sequence)",
            f"auto-preview-source={auto_result['sourcePreviewPath']}",
            f"det-preview-source={deterministic_result['sourcePreviewPath']}",
            f"det-preview-downlink={received_fdp}",
            f"timing-start-to-catalog-complete-sec={downlink_summary['startToCatalogCompletedSec']}",
            f"timing-catalog-complete-to-final-gds-arrival-sec={downlink_summary['catalogCompletedToFinalGdsArrivalSec']}",
            f"summary-json={self.probe_root / 'route1-sequence-target-summary.json'}",
        ]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    scenario = Route1SequencePayloadTargetScenario(pathlib.Path(args.probe_root))
    target_probe.install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        output = "\n".join(scenario.run_route1_sequence()) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"chapter5-route1-sequence-target: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        run_error = Exception("unexpected exception")
        output = "chapter5-route1-sequence-target: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"chapter5-route1-sequence-target: FAIL cleanup {cleanup_exc}\n"
                exit_code = 1
            else:
                output += f"\ncleanup-error: {cleanup_exc}\n"
        scenario.sband.force_stop()
        scenario.uhf.force_stop()
        if exit_code == 0:
            print(output, end="")
        else:
            print(output, end="", file=os.sys.stderr)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
