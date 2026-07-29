#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import pathlib
import re
import shlex
import statistics
import subprocess
import sys
import tarfile
import time
import traceback
from dataclasses import dataclass
from datetime import datetime, timezone

import run_target_can_matrix_probe as target_probe
from run_target_can_matrix_probe import ProbeFailure, command_opcode, ensure_no_legacy_aliases
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario


ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
DEFAULT_TARGET_BASELINE = ROOT_DIR / "scripts" / "ensure_target_comm_lab_baseline.sh"
DEFAULT_GROUND_BASELINE = ROOT_DIR / "scripts" / "ensure_ground_dual_gds_baseline.sh"
PAYLOAD_FDP_EXTRACT = pathlib.Path(os.environ.get("PAYLOAD_FDP_EXTRACT_BIN", ROOT_DIR / "scripts" / "payload_fdp_extract.py"))
FPRIME_DP_WRITE = pathlib.Path(os.environ.get("FPRIME_DP_WRITE_BIN", ROOT_DIR / "fprime-venv" / "bin" / "fprime-dp-write"))
VENV_PYTHON = pathlib.Path(os.environ.get("VENV_PYTHON_BIN", ROOT_DIR / "fprime-venv" / "bin" / "python3"))

CASE_DEFS = [
    {
        "name": "vga",
        "capture_index": 0x20,
        "resolution": "PRESET_VGA_640X480",
        "jpeg_quality": 90,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": True,
        "publish_raw": True,
        "downlink_required": True,
    },
    {
        "name": "hd",
        "capture_index": 0x21,
        "resolution": "PRESET_HD_1280X720",
        "jpeg_quality": 90,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": True,
        "publish_raw": False,
        "downlink_required": False,
    },
    {
        "name": "full",
        "capture_index": 0x22,
        "resolution": "PRESET_FULL_3280X2464",
        "jpeg_quality": 90,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": False,
        "publish_raw": True,
        "downlink_required": False,
    },
]

DOWNLINK_PROFILE = os.environ.get("PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE", "full").strip().lower()
REUSE_SUMMARY_JSON = os.environ.get("PAYLOAD_TARGET_REUSE_SUMMARY_JSON", "").strip()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    return parser.parse_args()


def read_json(path: pathlib.Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def file_sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def iso_utc(ts: float) -> str:
    return datetime.fromtimestamp(ts, tz=timezone.utc).isoformat().replace("+00:00", "Z")


def run_logged_command(
    *,
    args: list[str],
    cwd: pathlib.Path,
    env: dict[str, str],
    log_path: pathlib.Path,
    timeout_sec: int,
) -> tuple[bool, int | None, bool]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as handle:
        handle.write(f"$ {' '.join(args)}\n")
        handle.flush()
        try:
            result = subprocess.run(
                args,
                cwd=str(cwd),
                env=env,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=timeout_sec,
                check=False,
            )
            handle.write(f"\nreturncode={result.returncode}\n")
            return result.returncode == 0, result.returncode, False
        except subprocess.TimeoutExpired:
            handle.write(f"\ntimed out after {timeout_sec}s\n")
            return False, None, True


@dataclass
class BaselineResult:
    ready: bool
    payload: dict[str, object]
    log_path: str


class PayloadDownlinkTargetScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "payload-raw-preview-dual-artifact-v1-target"
        self.obc_groundlink_diagnostics = os.getenv("OBC_GROUNDLINK_DIAGNOSTICS", "1")
        self.profile = "sband"
        self.required_capture_count = sum(1 for case in CASE_DEFS if case["required"])
        self.configure_ground_artifact_layout(self.sband)
        self.configure_ground_artifact_layout(self.uhf)

    @staticmethod
    def configure_ground_artifact_layout(ground: target_probe.GroundPath) -> None:
        ground.gds_runtime_dir = ground.root / "gds-runtime"
        ground.file_storage = ground.gds_runtime_dir / "gds-files"
        ground.gds_log = ground.gds_runtime_dir / "gds.log"
        ground.gateway_log = ground.gds_runtime_dir / "gateway.log"
        ground.process_control_dir = ground.root / "process-control"
        ground.events_log = ground.process_control_dir / "passive-cli-stdout.log"
        ground.channels_log = ground.process_control_dir / "control-query-stdout.log"
        ground.passive_cli_control_log = ground.process_control_dir / "passive-cli.log"
        ground.control_query_log = ground.process_control_dir / "control-queries.log"
        ground.raw_command_log = ground.process_control_dir / "raw-command.log"
        ground.cli_log_dir = ground.root / "native-cli"
        ground.passive_cli_log_dir = ground.cli_log_dir

    def gateway_capture_dir_for_ground(self, ground: target_probe.GroundPath) -> pathlib.Path | None:
        capture_dir = self.capture_dir / ("sband" if ground is self.sband else "uhf")
        capture_dir.mkdir(parents=True, exist_ok=True)
        return capture_dir

    def load_payload_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_PREPARE": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_PREPARE"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_PUBLISH_CAPTURE": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_PUBLISH_CAPTURE"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA"
                ),
                "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN"
                ),
                "OBCApp.dpCatalog.STOP_XMIT_CATALOG": command_opcode(
                    dictionary, "OBCApp.dpCatalog.STOP_XMIT_CATALOG"
                ),
            }
        )

    @staticmethod
    def artifact_downlink_required(*, artifact_kind: str, case_downlink_required: bool) -> bool:
        if not case_downlink_required:
            return False
        if DOWNLINK_PROFILE == "preview-only":
            return artifact_kind == "PREVIEW_JPEG"
        if DOWNLINK_PROFILE == "full":
            return True
        raise ProbeFailure(f"unsupported PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE: {DOWNLINK_PROFILE}")

    def start(self) -> None:
        super().start()
        self.load_payload_opcodes()

    def wait_all_event_or_journal(
        self,
        *,
        event_start: int,
        event_fragments: tuple[str, ...],
        journal_since: str,
        journal_fragments: tuple[str, ...],
        timeout: float,
        label: str,
        prefer_target_journal: bool = False,
    ) -> tuple[str, str]:
        deadline = time.time() + timeout
        last_events = ""
        last_journal = ""
        while time.time() < deadline:
            event_text = target_probe.read_text(self.sband.native_event_log)
            event_lines = event_text.splitlines()[event_start:]
            last_events = "\n".join(event_lines)
            journal_text = target_probe.ssh_capture(
                self.obc_target,
                f"journalctl -u {shlex.quote(self.obc_service)} --since {shlex.quote(journal_since)} --no-pager || true",
                check=False,
            )
            last_journal = journal_text

            event_match = None
            if all(fragment in last_events for fragment in event_fragments):
                event_match = next(
                    (line for line in reversed(event_lines) if all(fragment in line for fragment in event_fragments)),
                    last_events,
                )
            journal_match = None
            if all(fragment in journal_text for fragment in journal_fragments):
                journal_match = next(
                    (
                        line
                        for line in reversed(journal_text.splitlines())
                        if all(fragment in line for fragment in journal_fragments)
                    ),
                    journal_text,
                )
            ordered = (("target-journal", journal_match), ("ground-events", event_match))
            if not prefer_target_journal:
                ordered = (("ground-events", event_match), ("target-journal", journal_match))
            for source, match in ordered:
                if match is not None:
                    return source, match
            time.sleep(0.5)
        raise ProbeFailure(
            f"timed out waiting for {label}; "
            f"event_fragments={event_fragments} journal_fragments={journal_fragments}\n"
            f"events_tail={last_events[-4000:]}\n"
            f"journal_tail={last_journal[-4000:]}"
        )

    def wait_any_event_or_journal(
        self,
        *,
        event_start: int,
        event_fragments: tuple[str, ...],
        journal_since: str,
        journal_fragments: tuple[str, ...],
        timeout: float,
        label: str,
        prefer_target_journal: bool = False,
    ) -> tuple[str, str, str]:
        deadline = time.time() + timeout
        last_events = ""
        last_journal = ""
        while time.time() < deadline:
            event_text = target_probe.read_text(self.sband.native_event_log)
            event_lines = event_text.splitlines()[event_start:]
            last_events = "\n".join(event_lines)
            event_match: tuple[str, str] | None = None
            for line in event_lines:
                for fragment in event_fragments:
                    if fragment in line:
                        event_match = (fragment, line)
                        break
                if event_match is not None:
                    break

            journal_text = target_probe.ssh_capture(
                self.obc_target,
                f"journalctl -u {shlex.quote(self.obc_service)} --since {shlex.quote(journal_since)} --no-pager || true",
                check=False,
            )
            last_journal = journal_text
            journal_match: tuple[str, str] | None = None
            for line in journal_text.splitlines():
                for fragment in journal_fragments:
                    if fragment in line:
                        journal_match = (fragment, line)
                        break
                if journal_match is not None:
                    break

            ordered = (("target-journal", journal_match), ("ground-events", event_match))
            if not prefer_target_journal:
                ordered = (("ground-events", event_match), ("target-journal", journal_match))
            for source, match in ordered:
                if match is not None:
                    return source, match[0], match[1]
            time.sleep(0.5)
        raise ProbeFailure(
            f"timed out waiting for any {label}; "
            f"event_fragments={event_fragments} journal_fragments={journal_fragments}\n"
            f"events_tail={last_events[-4000:]}\n"
            f"journal_tail={last_journal[-4000:]}"
        )

    def latest_metadata_candidate_(
        self,
        lines: list[str],
        *,
        capture_index: int | None,
        require_preview_published: bool | None,
        require_raw_published: bool | None,
    ) -> tuple[str, dict[str, object]] | None:
        for line in reversed(lines):
            if "PAYLOAD_CAPTURE_METADATA" not in line:
                continue
            try:
                metadata = self.parse_metadata_line(line)
            except ProbeFailure:
                continue
            if capture_index is not None and int(metadata["captureIndex"]) != capture_index:
                continue
            if require_preview_published is not None and bool(metadata["previewPublished"]) != require_preview_published:
                continue
            if require_raw_published is not None and bool(metadata["rawPublished"]) != require_raw_published:
                continue
            return line, metadata
        return None

    def wait_metadata_snapshot(
        self,
        *,
        event_start: int,
        journal_since: str,
        timeout: float,
        label: str,
        capture_index: int | None,
        require_preview_published: bool | None = None,
        require_raw_published: bool | None = None,
        prefer_target_journal: bool = False,
    ) -> tuple[str, str, dict[str, object]]:
        deadline = time.time() + timeout
        last_events = ""
        last_journal = ""
        while time.time() < deadline:
            event_text = target_probe.read_text(self.sband.native_event_log)
            event_lines = event_text.splitlines()[event_start:]
            last_events = "\n".join(event_lines)
            journal_text = target_probe.ssh_capture(
                self.obc_target,
                f"journalctl -u {shlex.quote(self.obc_service)} --since {shlex.quote(journal_since)} --no-pager || true",
                check=False,
            )
            last_journal = journal_text
            journal_lines = journal_text.splitlines()

            event_candidate = self.latest_metadata_candidate_(
                event_lines,
                capture_index=capture_index,
                require_preview_published=require_preview_published,
                require_raw_published=require_raw_published,
            )
            journal_candidate = self.latest_metadata_candidate_(
                journal_lines,
                capture_index=capture_index,
                require_preview_published=require_preview_published,
                require_raw_published=require_raw_published,
            )

            ordered = (("target-journal", journal_candidate), ("ground-events", event_candidate))
            if not prefer_target_journal:
                ordered = (("ground-events", event_candidate), ("target-journal", journal_candidate))
            for source, candidate in ordered:
                if candidate is not None:
                    return source, candidate[0], candidate[1]
            time.sleep(0.5)
        raise ProbeFailure(
            f"timed out waiting for {label}; "
            f"capture_index={capture_index} require_preview_published={require_preview_published} "
            f"require_raw_published={require_raw_published}\n"
            f"events_tail={last_events[-4000:]}\n"
            f"journal_tail={last_journal[-4000:]}"
        )

    def wait_raw_publish_result(
        self,
        *,
        event_start: int,
        journal_since: str,
        timeout: float,
        label: str,
        capture_index: int,
        prefer_target_journal: bool = True,
    ) -> tuple[str, str, str, dict[str, object] | None]:
        deadline = time.time() + timeout
        last_events = ""
        last_journal = ""
        while time.time() < deadline:
            event_text = target_probe.read_text(self.sband.native_event_log)
            event_lines = event_text.splitlines()[event_start:]
            last_events = "\n".join(event_lines)
            journal_text = target_probe.ssh_capture(
                self.obc_target,
                f"journalctl -u {shlex.quote(self.obc_service)} --since {shlex.quote(journal_since)} --no-pager || true",
                check=False,
            )
            last_journal = journal_text
            journal_lines = journal_text.splitlines()

            def failure_candidate(lines: list[str]) -> str | None:
                for line in reversed(lines):
                    if "PAYLOAD_OPERATION_FAILED" in line:
                        return line
                return None

            event_failure = failure_candidate(event_lines)
            journal_failure = failure_candidate(journal_lines)
            event_metadata = self.latest_metadata_candidate_(
                event_lines,
                capture_index=capture_index,
                require_preview_published=None,
                require_raw_published=True,
            )
            journal_metadata = self.latest_metadata_candidate_(
                journal_lines,
                capture_index=capture_index,
                require_preview_published=None,
                require_raw_published=True,
            )

            ordered = (
                ("target-journal", "PAYLOAD_OPERATION_FAILED", journal_failure, None),
                ("target-journal", "PAYLOAD_CAPTURE_METADATA", journal_metadata[0] if journal_metadata else None, journal_metadata[1] if journal_metadata else None),
                ("ground-events", "PAYLOAD_OPERATION_FAILED", event_failure, None),
                ("ground-events", "PAYLOAD_CAPTURE_METADATA", event_metadata[0] if event_metadata else None, event_metadata[1] if event_metadata else None),
            )
            if not prefer_target_journal:
                ordered = (
                    ("ground-events", "PAYLOAD_OPERATION_FAILED", event_failure, None),
                    ("ground-events", "PAYLOAD_CAPTURE_METADATA", event_metadata[0] if event_metadata else None, event_metadata[1] if event_metadata else None),
                    ("target-journal", "PAYLOAD_OPERATION_FAILED", journal_failure, None),
                    ("target-journal", "PAYLOAD_CAPTURE_METADATA", journal_metadata[0] if journal_metadata else None, journal_metadata[1] if journal_metadata else None),
                )

            for source, fragment, line, metadata in ordered:
                if line is not None:
                    return source, fragment, line, metadata
            time.sleep(0.5)
        raise ProbeFailure(
            f"timed out waiting for {label}; capture_index={capture_index}\n"
            f"events_tail={last_events[-4000:]}\n"
            f"journal_tail={last_journal[-4000:]}"
        )

    def send_secure_command_without_wait(
        self,
        session: target_probe.SecureAuthSession,
        label: str,
        command_name: str,
        *args: str,
        accept_sequence: bool = True,
    ) -> tuple[int, int]:
        self.prepare_ground_window(self.sband)
        time.sleep(0.3)
        sequence = session.accept_sequence() if accept_sequence else session.claim_sequence()
        inner = self.encode_inner_command(command_name, *args)
        payload = target_probe.build_secure_command_v2_packet(inner, session.session_key, sequence)
        opcode = self.opcodes[command_name]
        start = self.sband.event_count()
        with self.sband.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: secure-v2 service={session.service_id} ingress={session.ingress_port} "
                f"role={session.role_fragment} command={command_name} args={list(args)} "
                f"opcode=0x{opcode:x} seq={sequence} outer={payload.hex()}\n"
            )
        target_probe.send_tts_raw_packet(self.sband.gds_tts_port, payload)
        self.checkpoint(
            "payload-secure-command-sent",
            "pass",
            label=label,
            command=command_name,
            sequence=sequence,
            accept_sequence=accept_sequence,
        )
        return start, sequence

    def send_secure_command_until_fragments(
        self,
        session: target_probe.SecureAuthSession,
        label: str,
        command_name: str,
        *args: str,
        event_fragments: tuple[str, ...],
        journal_fragments: tuple[str, ...],
        timeout: float = 30.0,
        prefer_target_journal: bool = True,
    ) -> tuple[int, str, str]:
        journal_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        start, sequence = self.send_secure_command_without_wait(session, label, command_name, *args)
        source, line = self.wait_all_event_or_journal(
            event_start=start,
            event_fragments=event_fragments,
            journal_since=journal_since,
            journal_fragments=journal_fragments,
            timeout=timeout,
            label=label,
            prefer_target_journal=prefer_target_journal,
        )
        self.checkpoint(
            "payload-secure-command-completed",
            "pass",
            label=label,
            command=command_name,
            sequence=sequence,
            source=source,
        )
        return start, source, line

    @staticmethod
    def parse_metadata_line(line: str) -> dict[str, object]:
        match = re.search(
            r"(?:policy|session) (?P<capture_policy>\S+) capture (?P<capture>\d+) index (?P<index>\d+) "
            r"requested (?P<requested>\d+) applied (?P<applied>\d+) "
            r"actualExp (?P<actual_exp>\d+) actualGain (?P<actual_gain>\d+) "
            r"awbValid (?P<awb_valid>\S+) awbTempK (?P<awb_temp>\d+) "
            r"awbRedX1000 (?P<awb_red>\d+) awbBlueX1000 (?P<awb_blue>\d+) "
            r"raw (?P<raw>\S+) preview (?P<preview>\S+) "
            r"previewDp (?P<preview_dp>\S*) rawDp (?P<raw_dp>\S*) "
            r"previewPublished (?P<preview_published>\S+) rawPublished (?P<raw_published>\S+)",
            line,
        )
        if match is None:
            raise ProbeFailure(f"could not parse payload metadata line: {line}")
        return {
            "capturePolicy": match.group("capture_policy"),
            "captureId": int(match.group("capture")),
            "captureIndex": int(match.group("index")),
            "requestedMask": int(match.group("requested")),
            "appliedMask": int(match.group("applied")),
            "actualExposureUsec": int(match.group("actual_exp")),
            "actualGainX100": int(match.group("actual_gain")),
            "actualAwbValid": match.group("awb_valid").lower() == "true",
            "actualAwbColorTemperatureK": int(match.group("awb_temp")),
            "actualAwbRedGainX1000": int(match.group("awb_red")),
            "actualAwbBlueGainX1000": int(match.group("awb_blue")),
            "rawRelativePath": match.group("raw"),
            "previewRelativePath": match.group("preview"),
            "previewDataProductPath": match.group("preview_dp"),
            "rawDataProductPath": match.group("raw_dp"),
            "previewPublished": match.group("preview_published").lower() == "true",
            "rawPublished": match.group("raw_published").lower() == "true",
        }

    @staticmethod
    def parse_failure_detail(line: str) -> tuple[str, int]:
        match = re.search(r"result (?P<result>\S+)(?: \(\d+\))? detail (?P<detail>\d+)", line)
        if match is None:
            raise ProbeFailure(f"could not parse payload failure line: {line}")
        return match.group("result"), int(match.group("detail"))

    def ssh_python(self, args: list[str], script: str, *, timeout: float = 60.0) -> str:
        result = subprocess.run(
            ["ssh", self.obc_target, "python3", "-", *args],
            input=script,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
            timeout=timeout,
            cwd=str(ROOT_DIR),
        )
        if result.returncode != 0:
            raise ProbeFailure(f"remote python command failed: {result.stdout}")
        return result.stdout

    def reset_remote_runtime(self) -> dict[str, object]:
        script = """
import json
import pathlib
import sys

runtime_root = pathlib.Path(sys.argv[1])
removed = []
for pattern in (
    "data-products/*.fdp",
    "data-products/DpState.dat",
    "persistent-data/payload/camera/PIC*.bin",
    "persistent-data/payload/camera/PIC*.jpg",
    "persistent-data/payload/camera/catalog/PIC*.meta.bin",
    "persistent-data/payload/camera/capture-*.jpg",
    "persistent-data/payload/camera/capture-*.json",
):
    for path in runtime_root.glob(pattern):
        if path.is_file() or path.is_symlink():
            path.unlink(missing_ok=True)
            removed.append(str(path))

(runtime_root / "data-products").mkdir(parents=True, exist_ok=True)
(runtime_root / "persistent-data" / "payload" / "camera" / "catalog").mkdir(parents=True, exist_ok=True)
(runtime_root / "persistent-data" / "payload" / "camera").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)
print(json.dumps({"removed": removed}, sort_keys=True))
""".strip()
        return json.loads(self.ssh_python([self.runtime_root], script))

    def stage_local_fdp_files_to_remote_runtime(self, family_fdps: list[pathlib.Path]) -> dict[str, object]:
        archive = io.BytesIO()
        staged_names: list[str] = []
        with tarfile.open(fileobj=archive, mode="w:gz") as tar:
            for path in family_fdps:
                if not path.is_file():
                    raise ProbeFailure(f"reuse source family file missing: {path}")
                tar.add(path, arcname=path.name)
                staged_names.append(path.name)
        archive.seek(0)
        remote_command = (
            f"mkdir -p {shlex.quote(str(pathlib.Path(self.runtime_root) / 'data-products'))} && "
            f"tar -xzf - -C {shlex.quote(str(pathlib.Path(self.runtime_root) / 'data-products'))}"
        )
        result = subprocess.run(
            ["ssh", self.obc_target, remote_command],
            input=archive.getvalue(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=180.0,
            cwd=str(ROOT_DIR),
        )
        if result.returncode != 0:
            raise ProbeFailure(
                "failed to stage reused payload .fdp files to remote runtime: "
                f"stdout={result.stdout.decode(errors='replace')} stderr={result.stderr.decode(errors='replace')}"
            )
        return {"stagedCount": len(staged_names), "stagedBasenames": staged_names}

    def remote_capture_artifact_summary(
        self,
        *,
        relative_raw: str,
        relative_preview: str,
        relative_preview_fdp: str,
        relative_raw_fdp: str,
    ) -> dict[str, object]:
        empty_arg = "__PAYLOAD_NONE__"
        script = """
import hashlib
import json
import pathlib
import sys

runtime_root = pathlib.Path(sys.argv[1])
EMPTY = "__PAYLOAD_NONE__"

def decode_relative(index: int) -> pathlib.Path:
    value = sys.argv[index]
    if value == EMPTY:
        return pathlib.Path()
    return pathlib.Path(value)

relative_raw = decode_relative(2)
relative_preview = decode_relative(3)
relative_preview_fdp = decode_relative(4)
relative_raw_fdp = decode_relative(5)

def sha256_file(path: pathlib.Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

def info_for(relative_path: pathlib.Path) -> dict[str, object]:
    if relative_path.as_posix() in ("", "."):
        return {
            "relativePath": "",
            "path": "",
            "exists": False,
            "isFile": False,
            "size": 0,
            "sha256": None,
        }
    full = runtime_root / relative_path
    return {
        "relativePath": relative_path.as_posix(),
        "path": str(full),
        "exists": full.exists(),
        "isFile": full.is_file(),
        "size": full.stat().st_size if full.is_file() else 0,
        "sha256": sha256_file(full),
    }

summary = {
    "raw": info_for(relative_raw),
    "preview": info_for(relative_preview),
    "previewFdp": info_for(relative_preview_fdp),
    "rawFdp": info_for(relative_raw_fdp),
}
print(json.dumps(summary, sort_keys=True))
""".strip()
        return json.loads(
            self.ssh_python(
                [
                    self.runtime_root,
                    relative_raw or empty_arg,
                    relative_preview or empty_arg,
                    relative_preview_fdp or empty_arg,
                    relative_raw_fdp or empty_arg,
                ],
                script,
            )
        )

    def mirror_remote_capture_artifacts(self, *, relative_raw: str, relative_preview: str, local_dir: pathlib.Path) -> dict[str, str]:
        local_dir.mkdir(parents=True, exist_ok=True)
        command = (
            f"cd {shlex.quote(self.runtime_root)} && "
            f"tar -czf - {shlex.quote(relative_raw)} {shlex.quote(relative_preview)} data-products/*.fdp"
        )
        result = subprocess.run(
            ["ssh", self.obc_target, command],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=120.0,
            cwd=str(ROOT_DIR),
        )
        if result.returncode != 0:
            raise ProbeFailure(
                "failed to mirror remote payload artifacts: "
                f"stdout={result.stdout.decode(errors='replace')} stderr={result.stderr.decode(errors='replace')}"
            )
        with tarfile.open(fileobj=io.BytesIO(result.stdout), mode="r:gz") as archive:
            archive.extractall(local_dir)
        return {
            "localRawPath": str(local_dir / relative_raw),
            "localPreviewPath": str(local_dir / relative_preview),
            "localDataProductsDir": str(local_dir / "data-products"),
        }

    def run_payload_extract(
        self,
        *,
        received_fdp: pathlib.Path,
        output_dir: pathlib.Path,
        expected_capture_id: int,
        expected_capture_index: int,
        expected_artifact_kind: str,
        expected_relative_path: str,
        expected_relative_data_product_path: str,
        expected_resolution: str,
        expected_capture_policy: str,
        expected_source_artifact_sha256: str,
        require_valid_jpeg: bool,
    ) -> dict[str, object]:
        args = [
            str(VENV_PYTHON),
            str(PAYLOAD_FDP_EXTRACT),
            "--fdp-file",
            str(received_fdp),
            "--dictionary",
            str(self.dictionary_path),
            "--output-dir",
            str(output_dir),
            "--dp-writer",
            str(FPRIME_DP_WRITE),
            "--expected-capture-id",
            str(expected_capture_id),
            "--expected-capture-index",
            str(expected_capture_index),
            "--expected-artifact-kind",
            expected_artifact_kind,
            "--expected-relative-path",
            expected_relative_path,
            "--expected-relative-data-product-path",
            expected_relative_data_product_path,
            "--expected-resolution",
            expected_resolution,
            "--expected-capture-policy",
            expected_capture_policy,
            "--expected-source-artifact-sha256",
            expected_source_artifact_sha256,
            "--require-published",
        ]
        if require_valid_jpeg:
            args.append("--require-valid-jpeg")
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
        output_dir.mkdir(parents=True, exist_ok=True)
        (output_dir / "extract.log").write_text(result.stdout, encoding="utf-8")
        if result.returncode != 0:
            raise ProbeFailure(f"payload_fdp_extract.py failed for {received_fdp}; see {output_dir / 'extract.log'}")
        return read_json(output_dir / "payload-fdp-summary.json")

    def build_source_family_summaries(self, artifacts: list[dict[str, object]]) -> None:
        for artifact in artifacts:
            archived_summary = artifact.get("sourceFamilySummary")
            if isinstance(archived_summary, dict):
                family_files = archived_summary.get("familyFdpFiles", [])
                if not isinstance(family_files, list) or not family_files:
                    raise ProbeFailure(f"archived source family summary for {artifact['name']} {artifact['artifactKind']} was missing familyFdpFiles")
                rebound_family_files: list[str] = []
                for file_path in family_files:
                    rebound = pathlib.Path(str(artifact["sourceFdpPath"])).parent / pathlib.Path(str(file_path)).name
                    if not rebound.is_file():
                        raise ProbeFailure(f"reused source family slice missing: {rebound}")
                    rebound_family_files.append(str(rebound))
                archived_copy = json.loads(json.dumps(archived_summary))
                archived_copy["fdpFile"] = str(pathlib.Path(str(artifact["sourceFdpPath"])))
                archived_copy["fdpSha256"] = str(artifact["sourceFdpFirstSliceSha256"])
                archived_copy["familyFdpFiles"] = rebound_family_files
                archived_copy["familyFdpSha256"] = [file_sha256(pathlib.Path(path)) for path in rebound_family_files]
                artifact["sourceFamilySummary"] = archived_copy
                artifact["sourceFdpFamilyBytes"] = sum(pathlib.Path(path).stat().st_size for path in rebound_family_files)
                continue
            extract_dir = self.probe_root / "source-decode" / str(artifact["name"]) / str(artifact["artifactKind"])
            extract_result = self.run_payload_extract(
                received_fdp=pathlib.Path(str(artifact["sourceFdpPath"])),
                output_dir=extract_dir,
                expected_capture_id=int(artifact["captureId"]),
                expected_capture_index=int(artifact["captureIndex"]),
                expected_artifact_kind=str(artifact["artifactKind"]),
                expected_relative_path=str(artifact["relativePath"]),
                expected_relative_data_product_path=str(artifact["relativeDataProductPath"]),
                expected_resolution=str(artifact["resolution"]),
                expected_capture_policy=str(artifact["capturePolicy"]),
                expected_source_artifact_sha256=str(artifact["sourceArtifactSha256"]),
                require_valid_jpeg=bool(artifact["artifactKind"] == "PREVIEW_JPEG"),
            )
            artifact["sourceFamilySummary"] = extract_result
            artifact["sourceFdpFamilyBytes"] = sum(pathlib.Path(path).stat().st_size for path in extract_result["familyFdpFiles"])

    def load_reused_capture_results(self, summary_json: pathlib.Path) -> list[dict[str, object]]:
        payload = read_json(summary_json)
        source_root = summary_json.parent / "source-artifacts"
        if not source_root.is_dir():
            raise ProbeFailure(f"reused source-artifacts directory missing beside {summary_json}")
        case_defs = {str(case["name"]): case for case in CASE_DEFS}
        capture_results: list[dict[str, object]] = []

        for archived_case in payload.get("captureResults", []):
            case_name = str(archived_case["name"])
            case_def = case_defs.get(case_name)
            if case_def is None:
                raise ProbeFailure(f"reused summary referenced unknown payload case: {case_name}")
            capture_policy = str(archived_case.get("capturePolicy", archived_case.get("sessionKind", "")))
            if not capture_policy:
                raise ProbeFailure(f"reused summary case {case_name} was missing capture/session policy metadata")
            result: dict[str, object] = {
                "name": case_name,
                "resolution": str(archived_case["resolution"]),
                "jpegQuality": int(archived_case["jpegQuality"]),
                "exposureUsec": int(archived_case["exposureUsec"]),
                "gainX100": int(archived_case["gainX100"]),
                "required": bool(archived_case["required"]),
                "publishRaw": bool(archived_case["publishRaw"]),
                "downlink_required": bool(case_def.get("downlink_required", False)),
                "capturePolicy": capture_policy,
                "commandSequence": int(archived_case.get("commandSequence", 0)),
                "captureResultSource": "reused-summary",
                "captureId": int(archived_case["captureId"]),
                "captureIndex": int(archived_case["captureIndex"]),
                "rawRelativePath": str(archived_case["rawRelativePath"]),
                "previewRelativePath": str(archived_case["previewRelativePath"]),
                "previewDataProductPath": str(archived_case["previewDataProductPath"]),
                "rawDataProductPath": str(archived_case["rawDataProductPath"]),
                "previewPublished": bool(archived_case["previewPublished"]),
                "rawPublished": bool(archived_case.get("rawPublished", False)),
                "status": str(archived_case["status"]),
                "rawPublishStatus": str(archived_case.get("rawPublishStatus", "not-requested")),
                "artifacts": [],
            }
            if "failureResult" in archived_case:
                result["failureResult"] = str(archived_case["failureResult"])
            if "failureDetail" in archived_case:
                result["failureDetail"] = int(archived_case["failureDetail"])
            if "rawFailureResult" in archived_case:
                result["rawFailureResult"] = str(archived_case["rawFailureResult"])
            if "rawFailureDetail" in archived_case:
                result["rawFailureDetail"] = int(archived_case["rawFailureDetail"])

            preview_local_path = source_root / case_name / str(archived_case["previewRelativePath"])
            raw_local_path = source_root / case_name / str(archived_case["rawRelativePath"])
            if preview_local_path.is_file():
                result["sourcePreviewPath"] = str(preview_local_path)
                result["sourcePreviewBytes"] = preview_local_path.stat().st_size
                result["sourcePreviewSha256"] = file_sha256(preview_local_path)
            if raw_local_path.is_file():
                result["sourceRawPath"] = str(raw_local_path)
                result["sourceRawBytes"] = raw_local_path.stat().st_size
                result["sourceRawSha256"] = file_sha256(raw_local_path)

            for artifact in archived_case.get("artifacts", []):
                artifact_kind = str(artifact["artifactKind"])
                relative_data_product_path = str(artifact["relativeDataProductPath"])
                source_artifact_path = source_root / case_name / str(artifact["relativePath"])
                source_fdp_path = source_root / case_name / "data-products" / pathlib.Path(relative_data_product_path).name
                if not source_artifact_path.is_file():
                    raise ProbeFailure(f"reused source artifact missing: {source_artifact_path}")
                if not source_fdp_path.is_file():
                    raise ProbeFailure(f"reused source .fdp missing: {source_fdp_path}")
                artifact_entry = self.build_artifact_entry(
                    case_name=case_name,
                    resolution=str(archived_case["resolution"]),
                    capture_id=int(archived_case["captureId"]),
                    capture_index=int(archived_case["captureIndex"]),
                    capture_policy=capture_policy,
                    artifact_kind=artifact_kind,
                    downlink_required=self.artifact_downlink_required(
                        artifact_kind=artifact_kind,
                        case_downlink_required=bool(case_def.get("downlink_required", False)),
                    ),
                    relative_path=str(artifact["relativePath"]),
                    relative_data_product_path=relative_data_product_path,
                    source_artifact_path=source_artifact_path,
                    source_fdp_path=source_fdp_path,
                )
                archived_family_summary = artifact.get("sourceFamilySummary")
                if isinstance(archived_family_summary, dict):
                    artifact_entry["sourceFamilySummary"] = archived_family_summary
                result["artifacts"].append(artifact_entry)
            capture_results.append(result)

        return capture_results

    def prune_remote_catalog_to_payload_products(self, payload_source_fdps: list[pathlib.Path]) -> dict[str, object]:
        allowed_basenames = sorted({path.name for path in payload_source_fdps})
        script = """
import json
import pathlib
import sys

runtime_root = pathlib.Path(sys.argv[1])
allowed = set(sys.argv[2:])
removed = []
kept = []
for candidate in sorted((runtime_root / "data-products").glob("*.fdp")):
    if candidate.name in allowed:
        kept.append(candidate.name)
        continue
    candidate.unlink(missing_ok=True)
    removed.append(candidate.name)

print(json.dumps({"allowed": sorted(allowed), "kept": kept, "removed": removed}, sort_keys=True))
""".strip()
        return json.loads(self.ssh_python([self.runtime_root, *allowed_basenames], script))

    def run_remote_downlink_v3_status_probe(self, log_name: str) -> dict[str, object]:
        log_path = self.probe_root / log_name
        log_path.parent.mkdir(parents=True, exist_ok=True)
        service_env = target_probe.service_process_environment(self.subsystem_target, self.sband_comm_service)
        can_device = service_env.get("CSP_CAN_DEVICE") or service_env.get("SUBSYSTEM_SIM_COMM_CAN_DEVICE") or "can1"
        working_directory = target_probe.ssh_capture(
            self.subsystem_target,
            f"systemctl show {shlex.quote(self.sband_comm_service)} --property=WorkingDirectory --value",
            check=False,
        ).strip()
        if not working_directory:
            raise ProbeFailure(f"unable to resolve WorkingDirectory for {self.sband_comm_service}")
        remote_command = (
            f"cd {shlex.quote(working_directory)} && "
            "source scripts/_common.sh && "
            "BIN_DIR=$(obc_find_native_bin_dir .) && "
            "env "
            f"CSP_TRANSPORT=socketcan CSP_CAN_DEVICE={shlex.quote(can_device)} CSP_CAN_PROMISC=0 "
            "$BIN_DIR/comm_downlink_v3_status_probe "
            "--target-node 5 --local-node 7 --timeout-ms 2000 --interface-name DLV3PRB"
        )
        output = target_probe.ssh_capture(
            self.subsystem_target,
            f"bash -lc {shlex.quote(remote_command)}",
            check=False,
        )
        log_path.write_text(output, encoding="utf-8")
        json_line = ""
        for line in reversed(output.splitlines()):
            candidate = line.strip()
            if candidate.startswith("{") and candidate.endswith("}"):
                json_line = candidate
                break
        try:
            status = json.loads(json_line)
        except json.JSONDecodeError as exc:
            raise ProbeFailure(f"remote comm_downlink_v3_status_probe did not emit JSON; see {log_path}") from exc
        if int(status.get("targetNode", 0)) != 5:
            raise ProbeFailure(f"unexpected target node in remote downlink v3 status probe output: {status}")
        return status

    def find_matching_received_fdps(
        self, artifacts: list[dict[str, object]], timeout_sec: float
    ) -> dict[tuple[str, str], dict[str, object]]:
        unmatched = {
            (str(artifact["name"]), str(artifact["artifactKind"])): artifact for artifact in artifacts
        }
        matched: dict[tuple[str, str], dict[str, object]] = {}
        deadline = time.time() + timeout_sec
        while time.time() < deadline:
            for artifact_key, artifact in list(unmatched.items()):
                source_summary = artifact["sourceFamilySummary"]
                expected_family_sha = {str(sha) for sha in source_summary["familyFdpSha256"]}
                matching_family_files: list[pathlib.Path] = []
                matching_family_sha: list[str] = []
                for received in sorted(self.sband.file_storage.rglob("*.fdp")):
                    if not received.is_file():
                        continue
                    received_sha = file_sha256(received)
                    if received_sha not in expected_family_sha:
                        continue
                    matching_family_files.append(received)
                    matching_family_sha.append(received_sha)
                if set(matching_family_sha) != expected_family_sha:
                    continue
                for received in matching_family_files:
                    extract_dir = self.probe_root / "received-decode" / str(artifact["name"]) / str(artifact["artifactKind"])
                    try:
                        extract_result = self.run_payload_extract(
                            received_fdp=received,
                            output_dir=extract_dir,
                            expected_capture_id=int(artifact["captureId"]),
                            expected_capture_index=int(artifact["captureIndex"]),
                            expected_artifact_kind=str(artifact["artifactKind"]),
                            expected_relative_path=str(artifact["relativePath"]),
                            expected_relative_data_product_path=str(artifact["relativeDataProductPath"]),
                            expected_resolution=str(artifact["resolution"]),
                            expected_capture_policy=str(artifact["capturePolicy"]),
                            expected_source_artifact_sha256=str(artifact["sourceArtifactSha256"]),
                            require_valid_jpeg=bool(artifact["artifactKind"] == "PREVIEW_JPEG"),
                        )
                    except ProbeFailure:
                        if "extractedArtifactSha256" not in source_summary:
                            continue
                        extract_result = json.loads(json.dumps(source_summary))
                        extract_result["fdpFile"] = str(received)
                        extract_result["fdpSha256"] = file_sha256(received)
                        extract_result["familyFdpFiles"] = [str(path) for path in matching_family_files]
                        extract_result["familyFdpSha256"] = [file_sha256(path) for path in matching_family_files]
                        extract_result["decodeMode"] = "archived-summary-byte-match-fallback"
                    if set(str(sha) for sha in extract_result["familyFdpSha256"]) != expected_family_sha:
                        continue
                    family_file_paths = [pathlib.Path(path) for path in extract_result["familyFdpFiles"]]
                    family_mtimes = [path.stat().st_mtime for path in family_file_paths]
                    matched[artifact_key] = {
                        "receivedFdp": received,
                        "extractSummary": extract_result,
                        "familySliceCount": len(family_file_paths),
                        "familyFirstArrivalUnixSec": min(family_mtimes),
                        "familyLastArrivalUnixSec": max(family_mtimes),
                    }
                    del unmatched[artifact_key]
                    break
            if not unmatched:
                return matched
            time.sleep(0.5)
        received_files = ", ".join(str(path) for path in sorted(self.sband.file_storage.rglob("*.fdp")))
        raise ProbeFailure(
            "did not find byte-matching GDS-received .fdp families for all published payload artifacts; "
            f"matched={len(matched)}/{len(artifacts)} received=[{received_files}]"
        )

    def prepare_payload_target_case(self) -> target_probe.SecureAuthSession:
        self.begin_profile()
        self.record_secure_auth_provenance()
        self.apply_sband_ingress_diagnostics_override()
        self.start_security_server()
        self.wait_for_sband_tcp_reachability(10.0)
        self.start_ground_paths(need_sband=True, need_uhf=False)
        self.ensure_sband_ground_ready()
        self.wait_for_target_ready_for_comm(self.require_external_uhf_service)
        session = self.authenticate_secure_service(
            self.sband,
            service_id=1,
            ingress_port=0,
            role_fragment="identity 1 role 1",
            initial_sequence=41,
        )
        return session

    def collect_payload_capabilities(self, session: target_probe.SecureAuthSession) -> str:
        _, _, line = self.send_secure_command_until_fragments(
            session,
            "payload-get-capabilities",
            "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
            event_fragments=("PAYLOAD_CAPABILITIES",),
            journal_fragments=("PAYLOAD_CAPABILITIES",),
            timeout=20.0,
        )
        if "backend libcamera" not in line:
            raise ProbeFailure(f"target capabilities did not report libcamera backend: {line}")
        if "raw 0 real 1" not in line:
            raise ProbeFailure(f"target capabilities did not report raw 0 real 1: {line}")
        return line

    @staticmethod
    def build_artifact_entry(
        *,
        case_name: str,
        resolution: str,
        capture_id: int,
        capture_index: int,
        capture_policy: str,
        artifact_kind: str,
        downlink_required: bool,
        relative_path: str,
        relative_data_product_path: str,
        source_artifact_path: pathlib.Path,
        source_fdp_path: pathlib.Path,
    ) -> dict[str, object]:
        return {
            "name": case_name,
            "resolution": resolution,
            "captureId": capture_id,
            "captureIndex": capture_index,
            "capturePolicy": capture_policy,
            "artifactKind": artifact_kind,
            "downlinkRequired": downlink_required,
            "relativePath": relative_path,
            "relativeDataProductPath": relative_data_product_path,
            "sourceArtifactPath": str(source_artifact_path),
            "sourceArtifactBytes": source_artifact_path.stat().st_size,
            "sourceArtifactSha256": file_sha256(source_artifact_path),
            "sourceFdpPath": str(source_fdp_path),
            "sourceFdpFirstSliceBytes": source_fdp_path.stat().st_size,
            "sourceFdpFirstSliceSha256": file_sha256(source_fdp_path),
        }

    def capture_case(
        self, session: target_probe.SecureAuthSession, case: dict[str, object]
    ) -> dict[str, object]:
        case_name = str(case["name"])
        capture_index = int(case["capture_index"])
        resolution = str(case["resolution"])
        exposure_usec = int(case["exposure_usec"])
        gain_x100 = int(case["gain_x100"])
        required = bool(case["required"])
        publish_raw = bool(case["publish_raw"])
        downlink_required = bool(case.get("downlink_required", False))
        capture_tag = f"target-{case_name}"

        _, _, defaults_line = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-set-camera-defaults",
            "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
            resolution,
            str(int(case["jpeg_quality"])),
            "false",
            "false",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS']:08x} completed",),
        )
        self.checkpoint("payload-camera-defaults-applied", "pass", case=case_name, line=defaults_line)

        _, _, deterministic_defaults_line = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-set-deterministic-defaults",
            "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
            str(exposure_usec),
            str(gain_x100),
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS']:08x} completed",),
        )
        self.checkpoint("payload-deterministic-defaults-applied", "pass", case=case_name, line=deterministic_defaults_line)

        _, _, prepare_line = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-prepare",
            "OBCApp.payloadOpsController.PAYLOAD_PREPARE",
            event_fragments=("PSTATE_READY",),
            journal_fragments=("PSTATE_READY",),
            timeout=30.0,
        )
        self.checkpoint("payload-prepare-ready", "pass", case=case_name, line=prepare_line)

        accepted_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        capture_start, sequence = self.send_secure_command_without_wait(
            session,
            f"{case_name}-payload-capture-deterministic",
            "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC",
            str(capture_index),
            capture_tag,
            str((1 << 4) | (1 << 5)),
            str(exposure_usec),
            str(gain_x100),
        )
        self.wait_all_event_or_journal(
            event_start=capture_start,
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC']:08x} completed",),
            journal_since=accepted_since,
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC']:08x} completed",),
            timeout=20.0,
            label=f"{case_name} capture command completion",
            prefer_target_journal=True,
        )

        publish_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        publish_source, publish_fragment, publish_line = self.wait_any_event_or_journal(
            event_start=capture_start,
            event_fragments=("PSTATE_PUBLISHING", "PAYLOAD_OPERATION_FAILED", "PAYLOAD_CAPTURED"),
            journal_since=publish_since,
            journal_fragments=("PSTATE_PUBLISHING", "PAYLOAD_OPERATION_FAILED", "PAYLOAD_CAPTURED"),
            timeout=60.0,
            label=f"{case_name} publishing-or-terminal state",
            prefer_target_journal=True,
        )
        self.checkpoint(
            "payload-post-capture-state",
            "pass",
            case=case_name,
            source=publish_source,
            fragment=publish_fragment,
            line=publish_line,
        )

        result_source = publish_source
        result_fragment = publish_fragment
        result_line = publish_line
        if publish_fragment not in ("PAYLOAD_OPERATION_FAILED", "PAYLOAD_CAPTURED"):
            result_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            result_source, result_fragment, result_line = self.wait_any_event_or_journal(
                event_start=capture_start,
                event_fragments=("PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"),
                journal_since=result_since,
                journal_fragments=("PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"),
                timeout=180.0,
                label=f"{case_name} final payload result",
                prefer_target_journal=True,
            )

        _, metadata_line, metadata = self.wait_metadata_snapshot(
            event_start=capture_start,
            journal_since=accepted_since,
            timeout=180.0,
            label=f"{case_name} final capture metadata",
            capture_index=capture_index,
            require_preview_published=True if result_fragment == "PAYLOAD_CAPTURED" else None,
            prefer_target_journal=False,
        )
        _, metadata_readback_source, metadata_readback_line = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-get-last-metadata",
            "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
            journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
            timeout=20.0,
            prefer_target_journal=True,
        )
        self.checkpoint(
            "payload-metadata-readback-completed",
            "pass",
            case=case_name,
            source=metadata_readback_source,
            line=metadata_readback_line,
        )

        result: dict[str, object] = {
            "name": case_name,
            "resolution": resolution,
            "jpegQuality": int(case["jpeg_quality"]),
            "exposureUsec": exposure_usec,
            "gainX100": gain_x100,
            "required": required,
            "publishRaw": publish_raw,
            "downlink_required": downlink_required,
            "capturePolicy": str(metadata["capturePolicy"]),
            "commandSequence": sequence,
            "captureResultSource": result_source,
            "captureId": int(metadata["captureId"]),
            "captureIndex": int(metadata["captureIndex"]),
            "rawRelativePath": str(metadata["rawRelativePath"]),
            "previewRelativePath": str(metadata["previewRelativePath"]),
            "previewDataProductPath": str(metadata["previewDataProductPath"]),
            "rawDataProductPath": str(metadata["rawDataProductPath"]),
            "previewPublished": bool(metadata["previewPublished"]),
            "rawPublished": bool(metadata["rawPublished"]),
            "artifacts": [],
        }

        if result_fragment == "PAYLOAD_CAPTURED":
            if not bool(metadata["previewPublished"]):
                raise ProbeFailure(f"payload capture {case_name} reported success but previewPublished=false")
            result["status"] = "preview-published"
        else:
            failure_result, failure_detail = self.parse_failure_detail(result_line)
            result.update(
                {
                    "status": "failed",
                    "failureResult": failure_result,
                    "failureDetail": failure_detail,
                }
            )
            if required:
                raise ProbeFailure(
                    f"required target payload capture case {case_name} failed with {failure_result} detail={failure_detail}"
                )
            _, _, shutdown_line = self.send_secure_command_until_fragments(
                session,
                f"{case_name}-payload-shutdown",
                "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
                event_fragments=("PSTATE_OFF",),
                journal_fragments=("PSTATE_OFF",),
                timeout=30.0,
            )
            self.checkpoint("payload-shutdown", "pass", case=case_name, line=shutdown_line)
            return result

        if publish_raw:
            raw_publish_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            raw_publish_start, _ = self.send_secure_command_without_wait(
                session,
                f"{case_name}-payload-publish-raw",
                "OBCApp.payloadOpsController.PAYLOAD_PUBLISH_CAPTURE",
                str(capture_index),
                "RAW_FRAME",
            )
            raw_source, raw_fragment, raw_line, raw_metadata = self.wait_raw_publish_result(
                event_start=raw_publish_start,
                journal_since=raw_publish_since,
                timeout=180.0,
                label=f"{case_name} raw publish result",
                capture_index=capture_index,
                prefer_target_journal=True,
            )
            self.checkpoint(
                "payload-raw-publish-result",
                "pass",
                case=case_name,
                source=raw_source,
                fragment=raw_fragment,
                line=raw_line,
            )
            if raw_fragment == "PAYLOAD_OPERATION_FAILED":
                raw_result, raw_detail = self.parse_failure_detail(raw_line)
                result["rawPublishStatus"] = "failed"
                result["rawFailureResult"] = raw_result
                result["rawFailureDetail"] = raw_detail
                if resolution == "PRESET_FULL_3280X2464":
                    if raw_result != "PRESULT_STORAGE_FAILED" or raw_detail != 24:
                        raise ProbeFailure(
                            f"full raw publish should fail with deferred boundary, got {raw_result} detail={raw_detail}"
                        )
                    result["rawPublishStatus"] = "bounded-full-raw-deferred"
                elif required:
                    raise ProbeFailure(
                        f"required target raw publish case {case_name} failed with {raw_result} detail={raw_detail}"
                    )
            else:
                if raw_metadata is None or not bool(raw_metadata["rawPublished"]):
                    raise ProbeFailure(
                        f"{case_name} raw publish reported success but metadata was missing or rawPublished=false: {raw_line}"
                    )
                metadata = raw_metadata
                _, raw_readback_source, raw_readback_line = self.send_secure_command_until_fragments(
                    session,
                    f"{case_name}-payload-get-last-metadata-after-raw",
                    "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                    event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
                    journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed",),
                    timeout=20.0,
                    prefer_target_journal=True,
                )
                self.checkpoint(
                    "payload-raw-metadata-readback-completed",
                    "pass",
                    case=case_name,
                    source=raw_readback_source,
                    line=raw_readback_line,
                )
                result["rawPublished"] = bool(metadata["rawPublished"])
                result["rawDataProductPath"] = str(metadata["rawDataProductPath"])
                result["rawPublishStatus"] = "published"
        else:
            result["rawPublishStatus"] = "not-requested"

        artifact_summary = self.remote_capture_artifact_summary(
            relative_raw=str(metadata["rawRelativePath"]),
            relative_preview=str(metadata["previewRelativePath"]),
            relative_preview_fdp=str(metadata["previewDataProductPath"]),
            relative_raw_fdp=str(metadata["rawDataProductPath"]),
        )
        mirrored = self.mirror_remote_capture_artifacts(
            relative_raw=str(metadata["rawRelativePath"]),
            relative_preview=str(metadata["previewRelativePath"]),
            local_dir=self.probe_root / "source-artifacts" / case_name,
        )
        preview_local_path = pathlib.Path(mirrored["localPreviewPath"])
        raw_local_path = pathlib.Path(mirrored["localRawPath"])
        preview_fdp_local_path = pathlib.Path(mirrored["localDataProductsDir"]) / str(metadata["previewDataProductPath"]).split("/")[-1]
        result.update(
            {
                "sourcePreviewPath": str(preview_local_path),
                "sourceRawPath": str(raw_local_path),
                "sourcePreviewBytes": preview_local_path.stat().st_size,
                "sourceRawBytes": raw_local_path.stat().st_size,
                "sourcePreviewSha256": file_sha256(preview_local_path),
                "sourceRawSha256": file_sha256(raw_local_path),
            }
        )
        if not artifact_summary["previewFdp"]["isFile"] or not preview_fdp_local_path.is_file():
            raise ProbeFailure(f"payload capture {case_name} preview source .fdp was missing")
        result["artifacts"].append(
            self.build_artifact_entry(
                case_name=case_name,
                resolution=resolution,
                capture_id=int(metadata["captureId"]),
                capture_index=int(metadata["captureIndex"]),
                capture_policy=str(metadata["capturePolicy"]),
                artifact_kind="PREVIEW_JPEG",
                downlink_required=self.artifact_downlink_required(
                    artifact_kind="PREVIEW_JPEG",
                    case_downlink_required=downlink_required,
                ),
                relative_path=str(metadata["previewRelativePath"]),
                relative_data_product_path=str(metadata["previewDataProductPath"]),
                source_artifact_path=preview_local_path,
                source_fdp_path=preview_fdp_local_path,
            )
        )
        if result.get("rawPublishStatus") == "published":
            raw_fdp_local_path = pathlib.Path(mirrored["localDataProductsDir"]) / str(metadata["rawDataProductPath"]).split("/")[-1]
            if not artifact_summary["rawFdp"]["isFile"] or not raw_fdp_local_path.is_file():
                raise ProbeFailure(f"payload capture {case_name} raw source .fdp was missing after publish")
            result["artifacts"].append(
                self.build_artifact_entry(
                    case_name=case_name,
                    resolution=resolution,
                    capture_id=int(metadata["captureId"]),
                    capture_index=int(metadata["captureIndex"]),
                    capture_policy=str(metadata["capturePolicy"]),
                    artifact_kind="RAW_FRAME",
                    downlink_required=self.artifact_downlink_required(
                        artifact_kind="RAW_FRAME",
                        case_downlink_required=downlink_required,
                    ),
                    relative_path=str(metadata["rawRelativePath"]),
                    relative_data_product_path=str(metadata["rawDataProductPath"]),
                    source_artifact_path=raw_local_path,
                    source_fdp_path=raw_fdp_local_path,
                )
            )

        _, _, shutdown_line = self.send_secure_command_until_fragments(
            session,
            f"{case_name}-payload-shutdown",
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
            event_fragments=("PSTATE_OFF",),
            journal_fragments=("PSTATE_OFF",),
            timeout=30.0,
        )
        self.checkpoint("payload-shutdown", "pass", case=case_name, line=shutdown_line)
        return result

    def run_payload_proof(self) -> list[str]:
        self.reset_remote_runtime()
        session = self.prepare_payload_target_case()
        reuse_summary_json = pathlib.Path(REUSE_SUMMARY_JSON).resolve() if REUSE_SUMMARY_JSON else None
        if reuse_summary_json is not None:
            capture_results = self.load_reused_capture_results(reuse_summary_json)
            self.checkpoint(
                "payload-source-artifact-reuse",
                "pass",
                summaryJson=str(reuse_summary_json),
                mode="reuse-existing-route-artifacts",
            )
        else:
            self.collect_payload_capabilities(session)
            self.send_secure_command_until_fragments(
                session,
                "mode-set-idle",
                "OBCApp.modeManager.MODE_SET",
                "IDLE",
                event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
                journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
            )
            self.send_secure_command_until_fragments(
                session,
                "mode-set-payload",
                "OBCApp.modeManager.MODE_SET",
                "PAYLOAD",
                event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
                journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:08x} completed",),
            )
            capture_results = [self.capture_case(session, case) for case in CASE_DEFS]
        successful_cases = [case for case in capture_results if case["status"] != "failed"]
        if len(successful_cases) < self.required_capture_count:
            raise ProbeFailure(
                f"successful payload captures below required floor: required={self.required_capture_count} actual={len(successful_cases)}"
            )
        published_artifacts = [artifact for case in successful_cases for artifact in case["artifacts"]]
        self.build_source_family_summaries(published_artifacts)
        downlink_artifacts = [artifact for artifact in published_artifacts if bool(artifact.get("downlinkRequired"))]
        if not downlink_artifacts:
            raise ProbeFailure("no target payload cases were selected for governed downlink proof")
        downlink_source_fdps = [
            pathlib.Path(path)
            for artifact in downlink_artifacts
            for path in artifact["sourceFamilySummary"]["familyFdpFiles"]
        ]
        stage_summary = self.stage_local_fdp_files_to_remote_runtime(downlink_source_fdps)
        self.checkpoint(
            "payload-source-fdp-family-staged-remote",
            "pass",
            stagedCount=stage_summary["stagedCount"],
            stagedBasenames=stage_summary["stagedBasenames"],
        )
        prune_summary = self.prune_remote_catalog_to_payload_products(downlink_source_fdps)
        self.checkpoint(
            "payload-remote-catalog-pruned",
            "pass",
            downlink_artifacts=[f"{artifact['name']}:{artifact['artifactKind']}" for artifact in downlink_artifacts],
            kept_count=len(prune_summary.get("kept", [])),
            removed_count=len(prune_summary.get("removed", [])),
        )

        self.send_secure_command_until_fragments(
            session,
            "build-catalog",
            "OBCApp.dpCatalog.BUILD_CATALOG",
            event_fragments=("CatalogBuildComplete",),
            journal_fragments=("CatalogBuildComplete",),
            timeout=30.0,
        )
        status_before_xmit = self.run_remote_downlink_v3_status_probe("downlink-v3-status-before-xmit.log")
        start_xmit_requested_ts = time.time()
        start_xmit_journal_since = target_probe.ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        start_xmit_event_start, _, _ = self.send_secure_command_until_fragments(
            session,
            "start-xmit-catalog",
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
            timeout=2400.0,
            label="catalog-xmit-completed",
        )
        catalog_complete_observed_ts = time.time()
        status_at_catalog_complete = self.run_remote_downlink_v3_status_probe("downlink-v3-status-at-catalog-complete.log")

        matched_received = self.find_matching_received_fdps(downlink_artifacts, 2400.0)
        final_gds_arrival_ts = max(float(matched["familyLastArrivalUnixSec"]) for matched in matched_received.values())
        status_after_gds_match = self.run_remote_downlink_v3_status_probe("downlink-v3-status-after-gds-match.log")
        if int(status_after_gds_match.get("acceptedBytes", 0)) <= int(status_before_xmit.get("acceptedBytes", 0)):
            raise ProbeFailure(
                "target payload official proof did not observe node-5 v3 accepted-byte growth across START_XMIT_CATALOG"
            )
        try:
            self.send_secure_command_until_fragments(
                session,
                "stop-xmit-catalog",
                "OBCApp.dpCatalog.STOP_XMIT_CATALOG",
                event_fragments=(f"Opcode 0x{self.opcodes['OBCApp.dpCatalog.STOP_XMIT_CATALOG']:08x} completed",),
                journal_fragments=(f"Opcode 0x{self.opcodes['OBCApp.dpCatalog.STOP_XMIT_CATALOG']:08x} completed",),
                timeout=30.0,
            )
            self.checkpoint("payload-stop-xmit-catalog", "pass")
        except ProbeFailure as exc:
            self.checkpoint(
                "payload-stop-xmit-catalog",
                "info",
                detail="best-effort cleanup command did not complete before timeout",
                error=str(exc),
            )
        verified_artifacts: list[dict[str, object]] = []
        for artifact in downlink_artifacts:
            matched = matched_received[(str(artifact["name"]), str(artifact["artifactKind"]))]
            received_fdp = pathlib.Path(str(matched["receivedFdp"]))
            verified_artifact = dict(artifact)
            verified_artifact.update(
                {
                    "receivedFdpPath": str(received_fdp),
                    "receivedFdpBytes": received_fdp.stat().st_size,
                    "receivedFdpSha256": file_sha256(received_fdp),
                    "extractSummary": matched["extractSummary"],
                    "receivedFamilySliceCount": matched["familySliceCount"],
                    "receivedFamilyFirstArrivalUtc": iso_utc(float(matched["familyFirstArrivalUnixSec"])),
                    "receivedFamilyLastArrivalUtc": iso_utc(float(matched["familyLastArrivalUnixSec"])),
                }
            )
            verified_artifacts.append(verified_artifact)

        local_published_artifacts = [artifact for artifact in published_artifacts if not bool(artifact.get("downlinkRequired"))]
        timing_summary = {
            "startXmitRequestedUtc": iso_utc(start_xmit_requested_ts),
            "catalogXmitCompletedObservedUtc": iso_utc(catalog_complete_observed_ts),
            "catalogXmitCompletedSource": catalog_complete_source,
            "catalogXmitCompletedLine": catalog_complete_line,
            "finalGdsArrivalUtc": iso_utc(final_gds_arrival_ts),
            "startToCatalogCompletedSec": round(catalog_complete_observed_ts - start_xmit_requested_ts, 3),
            "startToFinalGdsArrivalSec": round(final_gds_arrival_ts - start_xmit_requested_ts, 3),
            "catalogCompletedToFinalGdsArrivalSec": round(final_gds_arrival_ts - catalog_complete_observed_ts, 3),
        }

        payload = {
            "formalVerdict": "payload-raw-preview-dual-artifact-v1-target=PASS",
            "probeRoot": str(self.probe_root),
            "targetPathUnderTest": "governed node 5 preview/raw payload .fdp closure over secure-auth command path",
            "remoteRuntimeRoot": self.runtime_root,
            "officialDeliveryPath": "PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink",
            "sourceProvisioningMode": "reuse-existing-summary" if reuse_summary_json is not None else "live-capture-and-publish",
            "sourceProvisioningSummaryJson": str(reuse_summary_json) if reuse_summary_json is not None else "",
            "downlinkProfile": DOWNLINK_PROFILE,
            "downlinkQueue": "BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)",
            "node5DownlinkTransport": {
                "selectedMode": "v3",
                "statusBeforeXmit": status_before_xmit,
                "statusAtCatalogComplete": status_at_catalog_complete,
                "statusAfterGdsMatch": status_after_gds_match,
            },
            "timing": timing_summary,
            "downlinkArtifacts": verified_artifacts,
            "captureResults": capture_results,
            "localPublishedArtifacts": local_published_artifacts,
            "diagnosticCases": [case for case in capture_results if case["status"] == "failed" or case.get("rawPublishStatus") == "bounded-full-raw-deferred"],
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
                "summaryJson": str(self.probe_root / "payload-raw-preview-dual-artifact-target-summary.json"),
            },
        }
        self.write_json_artifact(self.probe_root / "payload-raw-preview-dual-artifact-target-summary.json", payload)

        summary_lines = [
            "payload-raw-preview-dual-artifact-v1-target: PASS",
            f"probe-root={self.probe_root}",
            f"remote-runtime-root={self.runtime_root}",
            f"gds-file-storage-dir={self.sband.file_storage}",
            f"source-provisioning-mode={'reuse-existing-summary' if reuse_summary_json is not None else 'live-capture-and-publish'}",
            "official-path=PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink",
            f"downlink-profile={DOWNLINK_PROFILE}",
            "downlink-queue=BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)",
            f"node5-transport=v3 status-before-xmit accepted={status_before_xmit['acceptedBytes']} "
            f"flushed={status_before_xmit['flushedBytes']}",
            f"timing-start-xmit-requested-utc={timing_summary['startXmitRequestedUtc']}",
            f"timing-catalog-xmit-completed-observed-utc={timing_summary['catalogXmitCompletedObservedUtc']} "
            f"source={timing_summary['catalogXmitCompletedSource']} elapsed-sec={timing_summary['startToCatalogCompletedSec']}",
            f"timing-final-gds-arrival-utc={timing_summary['finalGdsArrivalUtc']} "
            f"elapsed-sec={timing_summary['startToFinalGdsArrivalSec']}",
            f"timing-catalog-completed-to-final-gds-arrival-sec={timing_summary['catalogCompletedToFinalGdsArrivalSec']}",
            f"node5-transport=v3 status-at-catalog-complete accepted={status_at_catalog_complete['acceptedBytes']} "
            f"flushed={status_at_catalog_complete['flushedBytes']} queued-frames={status_at_catalog_complete['drainQueuedFrames']} "
            f"duplicate-frames={status_at_catalog_complete['duplicateFrames']}",
            f"node5-transport=v3 status-after-gds-match accepted={status_after_gds_match['acceptedBytes']} "
            f"flushed={status_after_gds_match['flushedBytes']} queued-frames={status_after_gds_match['drainQueuedFrames']} "
            f"duplicate-frames={status_after_gds_match['duplicateFrames']}",
        ]
        for case in capture_results:
            summary_lines.append(
                f"capture-case={case['name']} resolution={case['resolution']} capture-id={case['captureId']} "
                f"exposure-usec={case['exposureUsec']} gain-x100={case['gainX100']} "
                f"capture-index=0x{int(case['captureIndex']):02X} preview-bytes={case.get('sourcePreviewBytes', 0)} "
                f"raw-bytes={case.get('sourceRawBytes', 0)} preview-status={case['status']} "
                f"raw-publish-status={case.get('rawPublishStatus', 'not-requested')}"
            )
        for artifact in verified_artifacts:
            summary_lines.append(
                f"downlink-artifact={artifact['name']} kind={artifact['artifactKind']} resolution={artifact['resolution']} "
                f"artifact-bytes={artifact['sourceArtifactBytes']} fdp-family-bytes={artifact['sourceFdpFamilyBytes']} "
                f"fdp-byte-match=PASS source={artifact['sourceFdpPath']} received={artifact['receivedFdpPath']} "
                f"family-first-arrival-utc={artifact['receivedFamilyFirstArrivalUtc']} "
                f"family-last-arrival-utc={artifact['receivedFamilyLastArrivalUtc']} "
                f"family-slices={artifact['receivedFamilySliceCount']}"
            )
            summary_lines.append(
                f"downlink-artifact={artifact['name']} kind={artifact['artifactKind']} extract-hash=PASS "
                f"sha256={artifact['extractSummary']['extractedArtifactSha256']}"
            )
        for artifact in payload["localPublishedArtifacts"]:
            summary_lines.append(
                f"local-artifact={artifact['name']} kind={artifact['artifactKind']} resolution={artifact['resolution']} "
                f"artifact-bytes={artifact['sourceArtifactBytes']} fdp-family-bytes={artifact['sourceFdpFamilyBytes']}"
            )
        for case in payload["diagnosticCases"]:
            summary_lines.append(
                f"diagnostic-case={case['name']} resolution={case['resolution']} preview-status={case['status']} "
                f"raw-publish-status={case.get('rawPublishStatus', 'n/a')} "
                f"result={case.get('failureResult', case.get('rawFailureResult', 'n/a'))} "
                f"detail={case.get('failureDetail', case.get('rawFailureDetail', 'n/a'))}"
            )
        summary_lines.append(f"summary-json={self.probe_root / 'payload-raw-preview-dual-artifact-target-summary.json'}")
        return summary_lines


class PayloadDownlinkTargetCampaign:
    def __init__(self, probe_root: pathlib.Path) -> None:
        self.probe_root = probe_root
        self.target_baseline_script = pathlib.Path(
            os.getenv("ENSURE_TARGET_BASELINE_SCRIPT", str(DEFAULT_TARGET_BASELINE))
        )
        self.ground_baseline_script = pathlib.Path(
            os.getenv("ENSURE_GROUND_BASELINE_SCRIPT", str(DEFAULT_GROUND_BASELINE))
        )

    def run_baseline_script(
        self,
        *,
        script: pathlib.Path,
        json_path: pathlib.Path,
        log_path: pathlib.Path,
        extra_env: dict[str, str] | None = None,
    ) -> BaselineResult:
        env = os.environ.copy()
        env["JSON_OUT"] = str(json_path)
        if extra_env:
            env.update(extra_env)
        ok, returncode, timed_out = run_logged_command(
            args=["bash", str(script)],
            cwd=ROOT_DIR,
            env=env,
            log_path=log_path,
            timeout_sec=240,
        )
        if json_path.exists():
            payload = read_json(json_path)
        else:
            payload = {
                "verdict": "blocked",
                "error": "summary-missing",
                "returncode": returncode,
                "timedOut": timed_out,
            }
        ready = ok and str(payload.get("verdict", "")).lower() in ("ready", "repaired")
        return BaselineResult(ready=ready, payload=payload, log_path=str(log_path))

    def run_preflight(self) -> dict[str, object]:
        preflight_dir = self.probe_root / "preflight"
        preflight_dir.mkdir(parents=True, exist_ok=True)
        target = self.run_baseline_script(
            script=self.target_baseline_script,
            json_path=preflight_dir / "target-before.json",
            log_path=preflight_dir / "target-before.log",
            extra_env={"TARGET_BASELINE_REQUIRE_UHF_SERVICE": "1"},
        )
        ground = self.run_baseline_script(
            script=self.ground_baseline_script,
            json_path=preflight_dir / "ground-before.json",
            log_path=preflight_dir / "ground-before.log",
            extra_env={"GROUND_BASELINE_EXEMPT_PIDS": str(os.getpid())},
        )
        return {
            "verdict": "READY" if target.ready and ground.ready else "BLOCKED",
            "target": {"ready": target.ready, "payload": target.payload, "logPath": target.log_path},
            "ground": {"ready": ground.ready, "payload": ground.payload, "logPath": ground.log_path},
        }

    def run_postflight(self) -> dict[str, object]:
        target = self.run_baseline_script(
            script=self.target_baseline_script,
            json_path=self.probe_root / "target-after.json",
            log_path=self.probe_root / "target-after.log",
            extra_env={
                "TARGET_BASELINE_REQUIRE_UHF_SERVICE": "1",
                "TARGET_BASELINE_IGNORE_GPS_STATE_MISSING": "1",
            },
        )
        ground = self.run_baseline_script(
            script=self.ground_baseline_script,
            json_path=self.probe_root / "ground-after.json",
            log_path=self.probe_root / "ground-after.log",
            extra_env={"GROUND_BASELINE_EXEMPT_PIDS": str(os.getpid())},
        )
        return {
            "verdict": "READY" if target.ready and ground.ready else "BLOCKED",
            "target": {"ready": target.ready, "payload": target.payload, "logPath": target.log_path},
            "ground": {"ready": ground.ready, "payload": ground.payload, "logPath": ground.log_path},
        }

    def run(self) -> tuple[list[str], dict[str, object]]:
        preflight = self.run_preflight()
        if preflight["verdict"] != "READY":
            raise ProbeFailure(f"preflight blocked: {json.dumps(preflight, sort_keys=True)}")

        scenario = PayloadDownlinkTargetScenario(self.probe_root / "case")
        target_probe.install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))

        run_error: Exception | None = None
        summary_lines: list[str] = []
        try:
            scenario.start()
            summary_lines = scenario.run_payload_proof()
        except Exception as exc:
            run_error = exc
            raise
        finally:
            try:
                scenario.cleanup(run_error)
            finally:
                scenario.sband.force_stop()
                scenario.uhf.force_stop()

        postflight = self.run_postflight()
        if postflight["verdict"] != "READY":
            raise ProbeFailure(f"postflight blocked: {json.dumps(postflight, sort_keys=True)}")

        campaign_summary = {
            "probeRoot": str(self.probe_root),
            "preflight": preflight,
            "postflight": postflight,
            "summaryLines": summary_lines,
        }
        summary_path = self.probe_root / "campaign-summary.json"
        summary_path.parent.mkdir(parents=True, exist_ok=True)
        summary_path.write_text(json.dumps(campaign_summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return summary_lines + [f"campaign-summary={summary_path}"], campaign_summary


def main() -> int:
    args = parse_args()

    if not PAYLOAD_FDP_EXTRACT.is_file():
        raise SystemExit(f"payload extractor not found: {PAYLOAD_FDP_EXTRACT}")
    if not FPRIME_DP_WRITE.exists():
        raise SystemExit(f"fprime-dp-write not found: {FPRIME_DP_WRITE}")
    if not VENV_PYTHON.exists():
        raise SystemExit(f"python3 not found: {VENV_PYTHON}")

    campaign = PayloadDownlinkTargetCampaign(pathlib.Path(args.probe_root).resolve())
    exit_code = 0
    output = ""
    try:
        summary_lines, _ = campaign.run()
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        output = f"payload-raw-preview-dual-artifact-v1-target: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        output = "payload-raw-preview-dual-artifact-v1-target: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        ensure_no_legacy_aliases(ROOT_DIR)
        if exit_code == 0:
            sys.stdout.write(output)
        else:
            sys.stderr.write(output)
        sys.stdout.flush()
        sys.stderr.flush()
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
