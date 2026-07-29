#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import subprocess
import traceback

import run_target_can_matrix_probe as target_probe
from payload_image_sanity import evaluate_luma_sanity
from payload_image_sanity import evaluate_preview_flip_effect
from run_payload_e2e_downlink_closure_v1_target_probe import (
    PayloadDownlinkTargetScenario,
    ProbeFailure,
    VENV_PYTHON,
    file_sha256,
    read_json,
)


AUTO_CASE = {
    "name": "auto",
    "capture_index": 0x30,
    "resolution": "PRESET_VGA_640X480",
    "jpeg_quality": 90,
    "capture_policy": "CAPTURE_AUTO",
    "hflip": False,
    "vflip": False,
    "awb_mode": "AWB_AUTO",
    "metering_mode": "METER_CENTRE",
}

DETERMINISTIC_CASE = {
    "name": "deterministic",
    "capture_index": 0x31,
    "resolution": "PRESET_VGA_640X480",
    "jpeg_quality": 90,
    "capture_policy": "CAPTURE_DETERMINISTIC",
    "hflip": False,
    "vflip": False,
    "exposure_usec": 30000,
    "gain_x100": 400,
}

FOLLOWUP_CASE = {
    "name": "deterministic-followup",
    "capture_index": 0x32,
    "resolution": "PRESET_VGA_640X480",
    "jpeg_quality": 90,
    "capture_policy": "CAPTURE_DETERMINISTIC",
    "hflip": False,
    "vflip": False,
    "exposure_usec": 30000,
    "gain_x100": 400,
}

FLIPPED_CASE = {
    "name": "deterministic-flipped",
    "capture_index": 0x33,
    "resolution": "PRESET_VGA_640X480",
    "jpeg_quality": 55,
    "capture_policy": "CAPTURE_DETERMINISTIC",
    "hflip": True,
    "vflip": False,
    "exposure_usec": 30000,
    "gain_x100": 400,
}

PAYLOAD_MASK_EXPOSURE_USEC = 1 << 4
PAYLOAD_MASK_GAIN_X100 = 1 << 5


class PayloadTargetCaptureSanityScenario(PayloadDownlinkTargetScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "payload-target-capture-sanity-v1"

    def load_payload_opcodes(self) -> None:
        super().load_payload_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC": target_probe.command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC"
                ),
            }
        )

    def configure_camera_defaults(
        self,
        session: target_probe.SecureAuthSession,
        case: dict[str, object],
    ) -> None:
        self.send_secure_command_until_fragments(
            session,
            f"payload-sanity-set-camera-defaults-{case['name']}",
            "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
            str(case["resolution"]),
            str(int(case["jpeg_quality"])),
            "true" if bool(case["hflip"]) else "false",
            "true" if bool(case["vflip"]) else "false",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS']:08x} completed",),
        )

    def configure_payload_defaults(
        self,
        session: target_probe.SecureAuthSession,
        auto_case: dict[str, object] = AUTO_CASE,
        deterministic_case: dict[str, object] = DETERMINISTIC_CASE,
    ) -> None:
        self.configure_camera_defaults(session, auto_case)
        if (
            str(auto_case["resolution"]) != str(deterministic_case["resolution"])
            or int(auto_case["jpeg_quality"]) != int(deterministic_case["jpeg_quality"])
            or bool(auto_case["hflip"]) != bool(deterministic_case["hflip"])
            or bool(auto_case["vflip"]) != bool(deterministic_case["vflip"])
        ):
            raise ProbeFailure("shared non-RAW camera defaults must match within one persistent-session proof")

        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-set-auto-defaults",
            "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS",
            str(auto_case["awb_mode"]),
            str(auto_case["metering_mode"]),
            "0",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS']:08x} completed",),
        )
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-set-deterministic-defaults",
            "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
            str(int(deterministic_case["exposure_usec"])),
            str(int(deterministic_case["gain_x100"])),
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
        )

    def prepare_camera_ready(self, session: target_probe.SecureAuthSession) -> None:
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-prepare",
            "OBCApp.payloadOpsController.PAYLOAD_PREPARE",
            event_fragments=("PSTATE_READY",),
            journal_fragments=("PSTATE_READY",),
            timeout=30.0,
        )

    def assert_actual_metadata(self, case_name: str, metadata: dict[str, object], case: dict[str, object]) -> None:
        actual_exposure = int(metadata["actualExposureUsec"])
        actual_gain = int(metadata["actualGainX100"])
        actual_awb_valid = bool(metadata["actualAwbValid"])
        if str(case["capture_policy"]) == "CAPTURE_AUTO":
            if actual_exposure <= 0 or actual_gain <= 0:
                raise ProbeFailure(
                    f"{case_name} actual AUTO metadata missing exposure/gain: exp={actual_exposure} gain={actual_gain}"
                )
            if not actual_awb_valid:
                raise ProbeFailure(f"{case_name} actual AUTO metadata did not report AWB validity")
            if int(metadata["actualAwbColorTemperatureK"]) <= 0:
                raise ProbeFailure(f"{case_name} actual AUTO metadata missing color temperature")
            if int(metadata["actualAwbRedGainX1000"]) <= 0 or int(metadata["actualAwbBlueGainX1000"]) <= 0:
                raise ProbeFailure(f"{case_name} actual AUTO metadata missing color gains")
            return

        expected_exposure = int(case["exposure_usec"])
        expected_gain = int(case["gain_x100"])
        if abs(actual_exposure - expected_exposure) > max(1000, expected_exposure // 10):
            raise ProbeFailure(
                f"{case_name} actual deterministic exposure drifted too far: "
                f"expected={expected_exposure} actual={actual_exposure}"
            )
        if abs(actual_gain - expected_gain) > max(25, expected_gain // 10):
            raise ProbeFailure(
                f"{case_name} actual deterministic gain drifted too far: expected={expected_gain} actual={actual_gain}"
            )
        if actual_awb_valid:
            raise ProbeFailure(f"{case_name} deterministic metadata unexpectedly reported AWB validity")

    def capture_rpicam_reference(self) -> dict[str, object]:
        remote_path = f"{self.runtime_root}/persistent-data/payload/camera/rpicam-reference-vga.jpg"
        command = (
            f"mkdir -p {target_probe.shq(self.runtime_root + '/persistent-data/payload/camera')} && "
            f"rpicam-still --shutter 30000 --gain 4 --width 640 --height 480 -n "
            f"-o {target_probe.shq(remote_path)} >/dev/null 2>&1 && "
            f"sha256sum {target_probe.shq(remote_path)}"
        )
        stdout = target_probe.ssh_capture(self.obc_target, command)
        parts = stdout.strip().split()
        if len(parts) < 2:
            raise ProbeFailure(f"failed to capture rpicam reference image: {stdout}")
        local_dir = self.probe_root / "reference-capture"
        local_dir.mkdir(parents=True, exist_ok=True)
        local_path = local_dir / "rpicam-reference-vga.jpg"
        result = subprocess.run(
            ["ssh", self.obc_target, f"cat {target_probe.shq(remote_path)}"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=60.0,
            cwd=str(target_probe.ROOT_DIR),
        )
        if result.returncode != 0:
            raise ProbeFailure(
                "failed to fetch rpicam reference image: "
                f"stdout={result.stdout.decode(errors='replace')} stderr={result.stderr.decode(errors='replace')}"
            )
        local_path.write_bytes(result.stdout)
        return {
            "remotePath": remote_path,
            "localPath": str(local_path),
            "bytes": local_path.stat().st_size,
            "sha256": file_sha256(local_path),
        }

    def set_mode_payload(self, session: target_probe.SecureAuthSession) -> None:
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-mode-set-idle",
            "OBCApp.modeManager.MODE_SET",
            "IDLE",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
        )
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-mode-set-payload",
            "OBCApp.modeManager.MODE_SET",
            "PAYLOAD",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
        )

    def run_case(self, session: target_probe.SecureAuthSession, case: dict[str, object]) -> dict[str, object]:
        case_name = str(case["name"])
        capture_index = int(case["capture_index"])
        resolution = str(case["resolution"])
        expected_capture_policy = str(case["capture_policy"])
        if expected_capture_policy == "CAPTURE_AUTO":
            accepted_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            capture_start, sequence = self.send_secure_command_without_wait(
                session,
                f"{case_name}-capture",
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO",
                str(capture_index),
                f"{case_name}-capture",
                "0",
                str(case["awb_mode"]),
                str(case["metering_mode"]),
                "0",
            )
        else:
            exposure_usec = int(case["exposure_usec"])
            gain_x100 = int(case["gain_x100"])
            accepted_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            capture_start, sequence = self.send_secure_command_without_wait(
                session,
                f"{case_name}-capture",
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC",
                str(capture_index),
                f"{case_name}-capture",
                str(PAYLOAD_MASK_EXPOSURE_USEC | PAYLOAD_MASK_GAIN_X100),
                str(exposure_usec),
                str(gain_x100),
            )

        result_source, result_fragment, result_line = self.wait_any_event_or_journal(
            event_start=capture_start,
            event_fragments=("PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"),
            journal_since=accepted_since,
            journal_fragments=("PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"),
            timeout=180.0,
            label=f"{case_name} final payload result",
            prefer_target_journal=True,
        )
        if result_fragment != "PAYLOAD_CAPTURED":
            failure_result, failure_detail = self.parse_failure_detail(result_line)
            raise ProbeFailure(
                f"target payload sanity case {case_name} failed with {failure_result} detail={failure_detail}"
            )

        _, metadata_line, metadata = self.wait_metadata_snapshot(
            event_start=capture_start,
            journal_since=accepted_since,
            timeout=180.0,
            label=f"{case_name} final capture metadata",
            capture_index=capture_index,
            require_preview_published=True,
            prefer_target_journal=False,
        )
        readback_start, _, _ = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-get-last-metadata",
            "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
            timeout=20.0,
            prefer_target_journal=True,
        )
        readback_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        _, metadata_readback_line, metadata_readback = self.wait_metadata_snapshot(
            event_start=readback_start,
            journal_since=readback_since,
            timeout=20.0,
            label=f"{case_name} metadata readback",
            capture_index=capture_index,
            require_preview_published=True,
            prefer_target_journal=False,
        )
        self.assert_actual_metadata(case_name, metadata_readback, case)
        artifact_summary = self.remote_capture_artifact_summary(
            relative_raw=str(metadata["rawRelativePath"]),
            relative_preview=str(metadata["previewRelativePath"]),
            relative_preview_fdp=str(metadata["previewDataProductPath"]),
            relative_raw_fdp="",
        )
        mirrored = self.mirror_remote_capture_artifacts(
            relative_raw=str(metadata["rawRelativePath"]),
            relative_preview=str(metadata["previewRelativePath"]),
            local_dir=self.probe_root / "source-artifacts" / case_name,
        )
        preview_local_path = pathlib.Path(mirrored["localPreviewPath"])
        raw_local_path = pathlib.Path(mirrored["localRawPath"])
        preview_fdp_local_path = pathlib.Path(mirrored["localDataProductsDir"]) / pathlib.Path(
            str(metadata["previewDataProductPath"])
        ).name
        if not artifact_summary["previewFdp"]["isFile"] or not preview_fdp_local_path.is_file():
            raise ProbeFailure(f"payload sanity case {case_name} preview source .fdp was missing")

        decode_dir = self.probe_root / "source-decode" / case_name / "PREVIEW_JPEG"
        extract_result = self.run_payload_extract(
            received_fdp=preview_fdp_local_path,
            output_dir=decode_dir,
            expected_capture_id=int(metadata["captureId"]),
            expected_capture_index=int(metadata["captureIndex"]),
            expected_artifact_kind="PREVIEW_JPEG",
            expected_relative_path=str(metadata["previewRelativePath"]),
            expected_relative_data_product_path=str(metadata["previewDataProductPath"]),
            expected_resolution=resolution,
            expected_capture_policy=expected_capture_policy,
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
                f"payload sanity case {case_name} failed luma oracle: "
                f"mean={luma['meanLuma']:.3f} p95={luma['p95Luma']}"
            )

        return {
            "name": case_name,
            "capturePolicy": expected_capture_policy,
            "commandSequence": sequence,
            "captureResultSource": result_source,
            "metadataLine": metadata_line,
            "metadataReadbackLine": metadata_readback_line,
            "captureId": int(metadata["captureId"]),
            "captureIndex": int(metadata["captureIndex"]),
            "jpegQuality": int(case["jpeg_quality"]),
            "hflip": bool(case.get("hflip", False)),
            "vflip": bool(case.get("vflip", False)),
            "actualExposureUsec": int(metadata_readback["actualExposureUsec"]),
            "actualGainX100": int(metadata_readback["actualGainX100"]),
            "actualAwbValid": bool(metadata_readback["actualAwbValid"]),
            "actualAwbColorTemperatureK": int(metadata_readback["actualAwbColorTemperatureK"]),
            "actualAwbRedGainX1000": int(metadata_readback["actualAwbRedGainX1000"]),
            "actualAwbBlueGainX1000": int(metadata_readback["actualAwbBlueGainX1000"]),
            "backend": "libcamera",
            "cameraModel": "OV5647",
            "sourceRawPath": str(raw_local_path),
            "sourcePreviewPath": str(preview_local_path),
            "sourcePreviewFdpPath": str(preview_fdp_local_path),
            "sourceRawBytes": raw_local_path.stat().st_size,
            "sourcePreviewBytes": preview_local_path.stat().st_size,
            "sourceRawSha256": file_sha256(raw_local_path),
            "sourcePreviewSha256": file_sha256(preview_local_path),
            "extractSummary": extract_result,
            "lumaSanity": luma,
        }

    def run_camera_defaults_busy_reject(self, session: target_probe.SecureAuthSession) -> dict[str, object]:
        accepted_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        capture_start, sequence = self.send_secure_command_without_wait(
            session,
            "prepared-set-camera-defaults-reject",
            "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
            "PRESET_HD_1280X720",
            "75",
            "false",
            "false",
        )
        result_source, result_fragment, result_line = self.wait_any_event_or_journal(
            event_start=capture_start,
            event_fragments=("PAYLOAD_OPERATION_FAILED",),
            journal_since=accepted_since,
            journal_fragments=("PAYLOAD_OPERATION_FAILED",),
            timeout=30.0,
            label="payload set-camera-defaults busy reject",
            prefer_target_journal=True,
        )
        if result_fragment != "PAYLOAD_OPERATION_FAILED":
            raise ProbeFailure(f"expected set-camera-defaults busy reject, saw {result_fragment}: {result_line}")
        failure_result, failure_detail = self.parse_failure_detail(result_line)
        if failure_result != "PRESULT_REJECTED_BUSY" or failure_detail != 0:
            raise ProbeFailure(
                "unexpected set-camera-defaults reject result: "
                f"result={failure_result} detail={failure_detail} line={result_line}"
            )
        return {
            "commandSequence": sequence,
            "resultSource": result_source,
            "resultLine": result_line,
            "resultCode": failure_result,
            "detail": failure_detail,
            "requestedResolution": "PRESET_HD_1280X720",
        }

    def run_capture_sanity(self) -> list[str]:
        self.reset_remote_runtime()
        reference_capture = self.capture_rpicam_reference()
        session = self.prepare_payload_target_case()
        capability_line = self.collect_payload_capabilities(session)
        self.set_mode_payload(session)
        self.configure_payload_defaults(session)
        self.prepare_camera_ready(session)
        auto_result = self.run_case(session, AUTO_CASE)
        deterministic_case = dict(DETERMINISTIC_CASE)
        deterministic_case["exposure_usec"] = int(auto_result["actualExposureUsec"])
        deterministic_case["gain_x100"] = int(auto_result["actualGainX100"])
        deterministic_result = self.run_case(session, deterministic_case)
        followup_case = dict(FOLLOWUP_CASE)
        followup_case["exposure_usec"] = int(auto_result["actualExposureUsec"])
        followup_case["gain_x100"] = int(auto_result["actualGainX100"])
        followup_result = self.run_case(session, followup_case)
        mismatch_reject = self.run_camera_defaults_busy_reject(session)
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-shutdown",
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
            event_fragments=("PSTATE_OFF",),
            journal_fragments=("PSTATE_OFF",),
            timeout=30.0,
        )
        flipped_case = dict(FLIPPED_CASE)
        flipped_case["exposure_usec"] = int(auto_result["actualExposureUsec"])
        flipped_case["gain_x100"] = int(auto_result["actualGainX100"])
        self.configure_camera_defaults(session, flipped_case)
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-flipped-set-deterministic-defaults",
            "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
            str(int(flipped_case["exposure_usec"])),
            str(int(flipped_case["gain_x100"])),
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
        )
        self.prepare_camera_ready(session)
        flipped_result = self.run_case(session, flipped_case)
        flipped_preview_oracle = evaluate_preview_flip_effect(
            pathlib.Path(auto_result["sourcePreviewPath"]),
            pathlib.Path(flipped_result["sourcePreviewPath"]),
            expected_hflip=bool(flipped_case["hflip"]),
            expected_vflip=bool(flipped_case["vflip"]),
        )
        if not bool(flipped_preview_oracle["passed"]):
            raise ProbeFailure(
                "flipped deterministic preview oracle did not corroborate the requested orientation: "
                f"baseline={float(flipped_preview_oracle['baselineDifference']):.4f} "
                f"corrected={float(flipped_preview_oracle['correctedDifference']):.4f} "
                f"ratio={float(flipped_preview_oracle['improvementRatio']):.3f}"
            )
        flipped_result["previewFlipOracle"] = flipped_preview_oracle
        self.send_secure_command_until_fragments(
            session,
            "payload-sanity-final-shutdown",
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
            event_fragments=("PSTATE_OFF",),
            journal_fragments=("PSTATE_OFF",),
            timeout=30.0,
        )
        case_results = [auto_result, deterministic_result, followup_result, flipped_result]
        payload = {
            "formalVerdict": "payload-target-capture-sanity-v1=PASS",
            "probeRoot": str(self.probe_root),
            "targetPathUnderTest": "governed A/B/C target secure-auth payload source-artifact sanity on OV5647 libcamera backend",
            "claims": {
                "secureAuth": True,
                "payloadModeEntry": True,
                "sourceArtifacts": True,
                "sourceImageContentValidity": True,
                "persistentWarmSession": True,
                "setCameraDefaultsBusyReject": True,
                "groundDownlink": False,
            },
            "referenceCapture": reference_capture,
            "capabilitiesLine": capability_line,
            "cases": case_results,
            "setCameraDefaultsBusyReject": mismatch_reject,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
                "summaryJson": str(self.probe_root / "payload-target-capture-sanity-summary.json"),
            },
        }
        self.write_json_artifact(self.probe_root / "payload-target-capture-sanity-summary.json", payload)
        lines = [
            "payload-target-capture-sanity-v1: PASS",
            f"probe-root={self.probe_root}",
            "target-path=secure-auth node5 -> PAYLOAD mode -> onboard source artifacts",
            f"reference-capture={reference_capture['localPath']}",
            f"reference-capture-bytes={reference_capture['bytes']}",
            f"reference-capture-sha256={reference_capture['sha256']}",
            f"capabilities-line={capability_line}",
            "same-session-capture-count=3",
            f"set-camera-defaults-busy-reject={mismatch_reject['resultCode']} detail={mismatch_reject['detail']}",
        ]
        for case in case_results:
            luma = case["lumaSanity"]
            lines.append(
                f"case={case['name']} policy={case['capturePolicy']} capture-id={case['captureId']} "
                f"capture-index=0x{int(case['captureIndex']):02X} pixel-format={case['extractSummary']['pixelFormat']} "
                f"resolution={case['extractSummary']['imageWidth']}x{case['extractSummary']['imageHeight']} "
                f"mean-luma={float(luma['meanLuma']):.3f} p95-luma={int(luma['p95Luma'])} "
                f"preview-bytes={case['sourcePreviewBytes']} raw-bytes={case['sourceRawBytes']}"
            )
            if "previewFlipOracle" in case:
                flip = case["previewFlipOracle"]
                lines.append(
                    f"case={case['name']} preview-flip baseline-diff={float(flip['baselineDifference']):.4f} "
                    f"corrected-diff={float(flip['correctedDifference']):.4f} "
                    f"ratio={float(flip['improvementRatio']):.3f}"
                )
        lines.append(f"summary-json={self.probe_root / 'payload-target-capture-sanity-summary.json'}")
        return lines


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    scenario = PayloadTargetCaptureSanityScenario(pathlib.Path(args.probe_root))
    target_probe.install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        output = "\n".join(scenario.run_capture_sanity()) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"payload-target-capture-sanity-v1: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        run_error = Exception("unexpected exception")
        output = "payload-target-capture-sanity-v1: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"payload-target-capture-sanity-v1: FAIL cleanup {cleanup_exc}\n"
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
