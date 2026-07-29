#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
VENV_PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python3"
SEQGEN_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-seqgen"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${VENV_PYTHON_BIN}" || ! -x "${SEQGEN_BIN}" ]]; then
  echo "Required native OBC build outputs are missing. Build native OBC first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/ch5-r1-seq.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/ch5-r1-rt}"
STACK_ROOT="${STACK_ROOT:-/tmp/ch5-r1-stack}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
VENV_PYTHON_BIN="${VENV_PYTHON_BIN}" \
SEQGEN_BIN="${SEQGEN_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
STACK_ROOT="${STACK_ROOT}" \
"${VENV_PYTHON_BIN}" - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.models.dictionaries import Dictionaries


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


root_dir = pathlib.Path(require_env("ROOT_DIR"))
bin_dir = pathlib.Path(require_env("BIN_DIR"))
dict_path = pathlib.Path(require_env("DICT_PATH"))
venv_python_bin = require_env("VENV_PYTHON_BIN")
seqgen_bin = pathlib.Path(require_env("SEQGEN_BIN"))
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
stack_root = pathlib.Path(require_env("STACK_ROOT"))
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
python_bin = venv_python_bin
payload_extract = root_dir / "scripts/payload_fdp_extract.py"
fprime_dp_write = root_dir / "fprime-venv/bin/fprime-dp-write"
sys.path.insert(0, str(root_dir / "scripts"))

from hosted_secure_command_helpers import authenticate_service, send_secure_command, wait_for_socket
from per_band_stock_ground_stacks import HostedPerBandStockStacks, find_fprime_cli
from probe_process_utils import cleanup_managed_processes, install_signal_cleanup, start_managed_process
from secure_link_auth_lib import SERVICE_ID_SBAND


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_bytes().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text(path)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path}")


def wait_text_since(path: pathlib.Path, offset: int, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text(path)
        if fragment in text[offset:]:
            return text[offset:]
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path} from offset {offset}")


def history_size(path: pathlib.Path) -> int:
    return len(read_text(path).splitlines())


def wait_event_fragment(events_log: pathlib.Path, fragment: str, timeout: float, start: int) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            if fragment in line:
                return line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for event fragment {fragment!r}")


def wait_event_count(events_log: pathlib.Path, fragment: str, expected_count: int, timeout: float, start: int = 0) -> list[str]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = [line for line in read_text(events_log).splitlines()[start:] if fragment in line]
        if len(lines) >= expected_count:
            return lines
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {expected_count} instances of {fragment!r}")


def assert_no_event_fragment(events_log: pathlib.Path, fragment: str, timeout: float, start: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        if any(fragment in line for line in lines[start:]):
            raise RuntimeError(f"unexpected event fragment {fragment!r} observed after index {start}")
        time.sleep(0.2)


def wait_sequence_terminal_state(events_log: pathlib.Path, timeout: float, start: int) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            if "SEQUENCE_CONTEXT_UPDATED" not in line:
                continue
            if "state FAILED" in line:
                raise RuntimeError(f"sequence reported FAILED: {line}")
            if "state SUCCEEDED" in line:
                return line
        time.sleep(0.2)
    raise RuntimeError("timed out waiting for sequence terminal SUCCEEDED state")


def command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"command {name!r} not found in dictionary")


def file_sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


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
        raise RuntimeError(f"could not parse payload metadata line: {line}")
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


def run_payload_extract(
    *,
    received_fdp: pathlib.Path,
    output_dir: pathlib.Path,
    source_artifact: pathlib.Path,
    expected_capture_id: int,
    expected_capture_index: int,
    expected_relative_path: str,
    expected_relative_data_product_path: str,
    expected_capture_policy: str,
) -> dict[str, object]:
    args = [
        str(python_bin),
        str(payload_extract),
        "--fdp-file",
        str(received_fdp),
        "--dictionary",
        str(dict_path),
        "--output-dir",
        str(output_dir),
        "--dp-writer",
        str(fprime_dp_write),
        "--source-artifact",
        str(source_artifact),
        "--expected-capture-id",
        str(expected_capture_id),
        "--expected-capture-index",
        str(expected_capture_index),
        "--expected-artifact-kind",
        "PREVIEW_JPEG",
        "--expected-relative-path",
        expected_relative_path,
        "--expected-relative-data-product-path",
        expected_relative_data_product_path,
        "--expected-resolution",
        "PRESET_VGA_640X480",
        "--expected-capture-policy",
        expected_capture_policy,
        "--require-published",
        "--require-valid-jpeg",
    ]
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "extract.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(f"payload_fdp_extract.py failed for {received_fdp}; see {output_dir / 'extract.log'}")
    return json.loads((output_dir / "payload-fdp-summary.json").read_text(encoding="utf-8"))


def find_received_family_by_sha(expected_sha_set: set[str], timeout_sec: float) -> pathlib.Path:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        matched: dict[str, pathlib.Path] = {}
        for candidate in sorted(stack.sband.file_storage.rglob("*.fdp")):
            if not candidate.is_file():
                continue
            candidate_sha = file_sha256(candidate)
            if candidate_sha in expected_sha_set:
                matched[candidate_sha] = candidate
        if set(matched) == expected_sha_set:
            return matched[sorted(expected_sha_set)[0]]
        time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for received .fdp family matching {sorted(expected_sha_set)}")


dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
dictionaries = Dictionaries()
dictionaries.load_dictionaries(str(dict_path), None, None)
cmd_encoder = CmdEncoder()
opcodes = {
    name: command_opcode(dictionary, name)
    for name in (
        "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
        "OBCApp.sequenceAdmissionController.SEQ_RUN",
        "OBCApp.dpCatalog.BUILD_CATALOG",
        "OBCApp.dpCatalog.START_XMIT_CATALOG",
    )
}


def encode_command_args(command: str, *args: str) -> bytes:
    template = dictionaries.command_name[command]
    encoded = cmd_encoder.encode_api(CmdData(tuple(args), template))
    return encoded[14:]


processes = []
stack = None
secure_session = None


def cleanup() -> None:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)


def send_named_command(command: str, *args: str, timeout: float = 10.0, label: str | None = None, log_waits=()) -> int:
    if secure_session is None:
        raise RuntimeError("secure session has not been established")
    event_start = history_size(stack.sband.events_log)
    send_secure_command(
        raw_command_log=stack.sband.raw_command_log,
        gds_tts_port=stack.sband.gds_tts_port,
        session=secure_session,
        opcode=opcodes[command],
        args=encode_command_args(command, *args),
        timeout=timeout,
        label=label or command,
        log_waits=list(log_waits),
    )
    return event_start


def compile_sequence() -> tuple[pathlib.Path, pathlib.Path]:
    sequence_src = probe_tmp_dir / "route1-sequence-demo.seq"
    sequence_bin = probe_tmp_dir / "route1-sequence-demo.bin"
    sequence_src.write_text(
        "\n".join(
            [
                "; Route 1 hosted sequence demo",
                "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS PRESET_VGA_640X480 90 false false",
                "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS AWB_AUTO METER_CENTRE 0",
                "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS 30000 400",
                "R00:00:00 OBCApp.payloadOpsController.PAYLOAD_PREPARE",
                'R00:00:00 OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO 48 "route1-auto" 0 AWB_AUTO METER_CENTRE 0',
                "R00:00:05 OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                'R00:00:05 OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC 49 "route1-det" 48 30000 400',
                "R00:00:05 OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                "",
            ]
        ),
        encoding="utf-8",
    )
    result = subprocess.run(
        [str(seqgen_bin), "--dictionary", str(dict_path), str(sequence_src), str(sequence_bin)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    (probe_tmp_dir / "route1-sequence-demo.seqgen.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError("fprime-seqgen failed for route1 sequence demo")
    return sequence_src, sequence_bin


install_signal_cleanup(cleanup)

try:
    clean_dir(probe_tmp_dir)
    clean_dir(runtime_root)
    clean_dir(stack_root)
    security_server_socket.parent.mkdir(parents=True, exist_ok=True)
    processes.append(
        start_managed_process(
            "security_server_sim",
            [python_bin, str(root_dir / "scripts/security_server_sim.py"), "--socket-path", str(security_server_socket)],
            security_server_log,
            env=os.environ.copy(),
            cwd=root_dir,
            stale_match_groups=((str(security_server_socket),),),
            stale_match_markers=("security_server_sim.py",),
        )
    )
    wait_for_socket(security_server_socket, 20.0)

    stack = HostedPerBandStockStacks(
        mode_name="ch5-r1-seq",
        root_dir=root_dir,
        bin_dir=bin_dir,
        dictionary_path=dict_path,
        cli_path=find_fprime_cli(root_dir),
        stack_root=stack_root,
        runtime_root=runtime_root,
        expose_sband_surface=True,
        expose_uhf_surface=True,
        preserve_sband_primary=True,
        auto_ports=True,
        command_authority_profile="sband-primary",
    )
    stack.start()
    if stack.sband is None:
        raise RuntimeError("hosted stack did not expose S-band surface")
    active_runtime_root = stack.runtime_root
    wait_text(stack.sband_process_log, "node=5", 20.0)
    wait_text(stack.sband_gateway_log, "gds-connected", 20.0)

    secure_session = authenticate_service(
        capture_path=stack.captures_root / "sband-southbound-to-gds.bin",
        raw_command_log=stack.sband.raw_command_log,
        gds_tts_port=stack.sband.gds_tts_port,
        scid=stack.sband.scid,
        vcid=stack.sband.vcid,
        frame_size=stack.sband.frame_size,
        security_server_socket=security_server_socket,
        obc_log=stack.obc_log,
        service_id=SERVICE_ID_SBAND,
        ingress_port=0,
        expected_open_pattern=r"Command session opened ingress 0 identity 1 role 1 session .*replaced 0",
        auth_timeout=10.0,
        establish_timeout=30.0,
    )

    payload_command_opcodes = {
        name: command_opcode(dictionary, name)
        for name in (
            "OBCApp.modeManager.MODE_SET",
            "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
            "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS",
            "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
            "OBCApp.payloadOpsController.PAYLOAD_PREPARE",
            "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO",
            "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
        )
    }

    def send_payload_command(command: str, *args: str, timeout: float = 10.0, label: str | None = None) -> int:
        if secure_session is None:
            raise RuntimeError("secure session has not been established")
        event_start = history_size(stack.sband.events_log)
        template = dictionaries.command_name[command]
        encoded = cmd_encoder.encode_api(CmdData(tuple(args), template))
        send_secure_command(
            raw_command_log=stack.sband.raw_command_log,
            gds_tts_port=stack.sband.gds_tts_port,
            session=secure_session,
            opcode=payload_command_opcodes[command],
            args=encoded[14:],
            timeout=timeout,
            label=label or command,
        )
        return event_start

    mode_idle_start = send_payload_command("OBCApp.modeManager.MODE_SET", "IDLE", label="route1-mode-idle", timeout=12.0)
    wait_event_fragment(stack.sband.events_log, "System mode changed to IDLE", 15.0, mode_idle_start)
    mode_payload_start = send_payload_command("OBCApp.modeManager.MODE_SET", "PAYLOAD", label="route1-mode-payload", timeout=12.0)
    wait_event_fragment(stack.sband.events_log, "System mode changed to PAYLOAD", 15.0, mode_payload_start)

    sequence_src, sequence_bin = compile_sequence()
    staged_root = active_runtime_root / "sequences" / "staging"
    staged_root.mkdir(parents=True, exist_ok=True)
    staged_sequence = staged_root / sequence_bin.name
    shutil.copyfile(sequence_bin, staged_sequence)
    staged_sequence_path = f".sequence-staging/{sequence_bin.name}"

    validate_start = send_named_command(
        "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
        staged_sequence_path,
        timeout=10.0,
        label="route1-seq-validate",
    )
    assert_no_event_fragment(stack.sband.events_log, "SEQUENCE_CONTROL_REJECTED", 3.0, validate_start)
    assert_no_event_fragment(stack.sband.events_log, "COMMAND_AUTHORITY_REJECTED", 3.0, validate_start)
    assert_no_event_fragment(stack.sband.events_log, "COMMAND_SEQUENCE_REJECTED", 3.0, validate_start)

    run_start = send_named_command(
        "OBCApp.sequenceAdmissionController.SEQ_RUN",
        staged_sequence_path,
        "WAIT",
        timeout=90.0,
        label="route1-seq-run",
    )
    wait_event_count(stack.sband.events_log, "PAYLOAD_CAPTURED", 2, 45.0, run_start)
    metadata_lines = wait_event_count(stack.sband.events_log, "PAYLOAD_CAPTURE_METADATA", 2, 45.0, run_start)
    wait_sequence_terminal_state(stack.sband.events_log, 90.0, run_start)
    assert_no_event_fragment(stack.sband.events_log, "CS_CommandError", 2.0, run_start)
    auto_metadata = None
    det_metadata = None
    for line in metadata_lines:
        metadata = parse_metadata_line(line)
        if str(metadata["capturePolicy"]) == "CAPTURE_AUTO" and int(metadata["captureIndex"]) == 48:
            auto_metadata = metadata
        if str(metadata["capturePolicy"]) == "CAPTURE_DETERMINISTIC" and int(metadata["captureIndex"]) == 49:
            det_metadata = metadata
    if auto_metadata is None:
        raise RuntimeError(f"did not observe AUTO metadata in sequence run: {metadata_lines}")
    if det_metadata is None:
        raise RuntimeError(f"did not observe DETERMINISTIC metadata in sequence run: {metadata_lines}")
    if not bool(auto_metadata["previewPublished"]):
        raise RuntimeError(f"AUTO preview was not published: {auto_metadata}")
    if str(det_metadata["capturePolicy"]) != "CAPTURE_DETERMINISTIC":
        raise RuntimeError(f"unexpected DETERMINISTIC metadata payload: {det_metadata}")
    if not bool(det_metadata["previewPublished"]):
        raise RuntimeError(f"DETERMINISTIC preview was not published: {det_metadata}")
    if int(det_metadata["captureIndex"]) != 49:
        raise RuntimeError(f"unexpected DETERMINISTIC capture index: {det_metadata}")

    auto_preview = active_runtime_root / str(auto_metadata["previewRelativePath"])
    auto_preview_fdp = active_runtime_root / str(auto_metadata["previewDataProductPath"])

    det_shutdown_start = send_payload_command(
        "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
        label="route1-seq-shutdown",
        timeout=20.0,
    )
    wait_event_fragment(stack.sband.events_log, "Payload state PSTATE_OFF result PRESULT_OK", 20.0, det_shutdown_start)

    det_preview = active_runtime_root / str(det_metadata["previewRelativePath"])
    det_preview_fdp = active_runtime_root / str(det_metadata["previewDataProductPath"])
    for path in (auto_preview, det_preview, auto_preview_fdp, det_preview_fdp):
        if not path.is_file():
            raise RuntimeError(f"expected hosted payload artifact missing: {path}")

    det_source_extract = run_payload_extract(
        received_fdp=det_preview_fdp,
        output_dir=probe_tmp_dir / "source-decode" / "deterministic" / "PREVIEW_JPEG",
        source_artifact=det_preview,
        expected_capture_id=int(det_metadata["captureId"]),
        expected_capture_index=int(det_metadata["captureIndex"]),
        expected_relative_path=str(det_metadata["previewRelativePath"]),
        expected_relative_data_product_path=str(det_metadata["previewDataProductPath"]),
        expected_capture_policy=str(det_metadata["capturePolicy"]),
    )
    payload_source_fdps = [pathlib.Path(str(path)) for path in det_source_extract["familyFdpFiles"]]
    allowed = {path.resolve() for path in payload_source_fdps}
    for candidate in (active_runtime_root / "data-products").glob("*.fdp"):
        if candidate.resolve() not in allowed:
            candidate.unlink()

    build_start = send_named_command(
        "OBCApp.dpCatalog.BUILD_CATALOG",
        timeout=15.0,
        label="route1-build-catalog",
        log_waits=((stack.obc_log, f"Opcode 0x{opcodes['OBCApp.dpCatalog.BUILD_CATALOG']:08x} completed"),),
    )
    assert_no_event_fragment(stack.sband.events_log, "COMMAND_AUTHORITY_REJECTED", 3.0, build_start)
    wait_event_fragment(stack.sband.events_log, "CatalogBuildComplete", 20.0, build_start)

    xmit_start = send_named_command(
        "OBCApp.dpCatalog.START_XMIT_CATALOG",
        "NO_WAIT",
        timeout=15.0,
        label="route1-start-xmit-catalog",
        log_waits=((stack.obc_log, f"Opcode 0x{opcodes['OBCApp.dpCatalog.START_XMIT_CATALOG']:08x} completed"),),
    )
    assert_no_event_fragment(stack.sband.events_log, "COMMAND_AUTHORITY_REJECTED", 3.0, xmit_start)

    downlink_results = []
    for name, metadata, preview_path, preview_fdp in (
        ("deterministic", det_metadata, det_preview, det_preview_fdp),
    ):
        source_extract = det_source_extract
        received_fdp = find_received_family_by_sha(set(source_extract["familyFdpSha256"]), 180.0)
        received_extract = run_payload_extract(
            received_fdp=received_fdp,
            output_dir=probe_tmp_dir / "received-decode" / name / "PREVIEW_JPEG",
            source_artifact=preview_path,
            expected_capture_id=int(metadata["captureId"]),
            expected_capture_index=int(metadata["captureIndex"]),
            expected_relative_path=str(metadata["previewRelativePath"]),
            expected_relative_data_product_path=str(metadata["previewDataProductPath"]),
            expected_capture_policy=str(metadata["capturePolicy"]),
        )
        if source_extract["familyFdpSha256"] != received_extract["familyFdpSha256"]:
            raise RuntimeError(f"downlinked .fdp family sha mismatch for {name}")
        downlink_results.append(
            {
                "name": name,
                "captureId": int(metadata["captureId"]),
                "captureIndex": int(metadata["captureIndex"]),
                "capturePolicy": str(metadata["capturePolicy"]),
                "sourcePreview": str(preview_path),
                "sourcePreviewSha256": file_sha256(preview_path),
                "sourceFdp": str(preview_fdp),
                "receivedFdp": str(received_fdp),
                "familyFdpSha256": source_extract["familyFdpSha256"],
                "sourceExtract": source_extract,
                "receivedExtract": received_extract,
            }
        )

    summary = {
        "formalVerdict": "chapter5-route1-sequence-hosted=PASS",
        "probeRoot": str(probe_tmp_dir),
        "stackRoot": str(stack_root),
        "runtimeRoot": str(active_runtime_root),
        "sequenceSource": str(sequence_src),
        "sequenceBinary": str(sequence_bin),
        "stagedSequencePath": staged_sequence_path,
        "commandPath": "secure-auth hosted node-5 official sequence-driven AUTO -> metadata -> DETERMINISTIC -> metadata plus deterministic preview-only dpCatalog downlink",
        "downlinkProfile": "preview-only deterministic-preview",
        "auto": auto_metadata,
        "deterministic": det_metadata,
        "downlinks": downlink_results,
        "logs": {
            "events": str(stack.sband.events_log),
            "rawCommand": str(stack.sband.raw_command_log),
            "obc": str(stack.obc_log),
            "gateway": str(stack.sband_gateway_log),
        },
    }
    summary_path = probe_tmp_dir / "route1-sequence-hosted-summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True), encoding="utf-8")
    summary_lines = [
        "chapter5-route1-sequence-hosted: PASS",
        f"probe-root={probe_tmp_dir}",
        f"stack-root={stack_root}",
        f"runtime-root={active_runtime_root}",
        f"sequence-source={sequence_src}",
        f"sequence-path={staged_sequence_path}",
        "sequence-validate=PASS",
        "sequence-run-payload-auto-det=PASS",
        "capture-policies=AUTO(sequence)|DETERMINISTIC(sequence)",
        "downlink-profile=preview-only deterministic-preview",
        f"auto-preview-source={auto_preview}",
        f"det-preview-source={det_preview}",
        f"det-preview-downlink={downlink_results[0]['receivedFdp']}",
        f"summary-json={summary_path}",
        "chapter5-route1-sequence-hosted: PASS",
    ]
    summary_log = probe_tmp_dir / "summary.log"
    summary_log.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
    print(summary_log.read_text(encoding="utf-8"), end="")
finally:
    cleanup()
PY
