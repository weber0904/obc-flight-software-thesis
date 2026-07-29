#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
VENV_PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python3"
HELPER_BIN="${BIN_DIR}/payload_camera_backend_helper"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${VENV_PYTHON_BIN}" || ! -x "${HELPER_BIN}" ]]; then
  echo "Required native OBC build outputs are missing. Build native OBC first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/payload-persistent-session-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/payload-persistent-session-hosted-rt}"
STACK_ROOT="${STACK_ROOT:-/tmp/payload-persistent-session-hosted-stack}"

mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
VENV_PYTHON_BIN="${VENV_PYTHON_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
STACK_ROOT="${STACK_ROOT}" \
"${VENV_PYTHON_BIN}" - <<'PY'
from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
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
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
stack_root = pathlib.Path(require_env("STACK_ROOT"))
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
python_bin = venv_python_bin

sys.path.insert(0, str(root_dir / "scripts"))

from hosted_secure_command_helpers import authenticate_service, send_secure_command, wait_for_socket
from per_band_stock_ground_stacks import HostedPerBandStockStacks, find_fprime_cli
from probe_process_utils import cleanup_managed_processes, install_signal_cleanup, start_managed_process
from secure_link_auth_lib import SERVICE_ID_SBAND

PAYLOAD_MASK_EXPOSURE_USEC = 0x0010
PAYLOAD_MASK_GAIN_X100 = 0x0020
DETAIL_SESSION_REPREPARE_REQUIRED = 25


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


def read_text_since(path: pathlib.Path, offset: int) -> str:
    try:
        payload = path.read_bytes()
    except FileNotFoundError:
        return ""
    bounded_offset = max(0, min(offset, len(payload)))
    return payload[bounded_offset:].replace(b"\0", b"\n").decode("utf-8", errors="replace")


def file_size(path: pathlib.Path) -> int:
    try:
        return path.stat().st_size
    except FileNotFoundError:
        return 0


def wait_text_since(path: pathlib.Path, offset: int, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text_since(path, offset)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path} from offset {offset}")


def wait_any_event(events_log: pathlib.Path, fragments: list[str], timeout: float, start: int) -> tuple[str, str]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            for fragment in fragments:
                if fragment in line:
                    return fragment, line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for one of {fragments!r}")


def wait_event_fragment(events_log: pathlib.Path, fragment: str, timeout: float, start: int) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            if fragment in line:
                return line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for event fragment {fragment!r}")


def parse_payload_capture_metadata(line: str) -> dict[str, object]:
    match = re.search(
        r"(?:policy|session)\s+(?P<capturePolicy>\S+)\s+"
        r"capture\s+(?P<captureId>\d+)\s+"
        r"index\s+(?P<captureIndex>\d+)\s+"
        r"requested\s+(?P<requestedMask>\d+)\s+"
        r"applied\s+(?P<appliedMask>\d+)\s+"
        r"actualExp\s+(?P<actualExposureUsec>\d+)\s+"
        r"actualGain\s+(?P<actualGainX100>\d+)\s+"
        r"awbValid\s+(?P<actualAwbValid>True|False)\s+"
        r"awbTempK\s+(?P<actualAwbColorTemperatureK>\d+)\s+"
        r"awbRedX1000\s+(?P<actualAwbRedGainX1000>\d+)\s+"
        r"awbBlueX1000\s+(?P<actualAwbBlueGainX1000>\d+)\s+"
        r"raw\s+(?P<rawRelativePath>.*?)\s+"
        r"preview\s+(?P<previewRelativePath>.*?)\s+"
        r"previewDp\s+(?P<previewDataProductPath>.*?)\s+"
        r"rawDp\s+(?P<rawDataProductPath>.*?)\s+"
        r"previewPublished\s+(?P<previewPublished>True|False)\s+"
        r"rawPublished\s+(?P<rawPublished>True|False)$",
        line,
    )
    if match is None:
        raise RuntimeError(f"could not parse PAYLOAD_CAPTURE_METADATA line: {line}")
    return {
        "capturePolicy": match.group("capturePolicy"),
        "captureId": int(match.group("captureId")),
        "captureIndex": int(match.group("captureIndex")),
        "requestedMask": int(match.group("requestedMask")),
        "appliedMask": int(match.group("appliedMask")),
        "actualExposureUsec": int(match.group("actualExposureUsec")),
        "actualGainX100": int(match.group("actualGainX100")),
        "actualAwbValid": match.group("actualAwbValid") == "True",
        "actualAwbColorTemperatureK": int(match.group("actualAwbColorTemperatureK")),
        "actualAwbRedGainX1000": int(match.group("actualAwbRedGainX1000")),
        "actualAwbBlueGainX1000": int(match.group("actualAwbBlueGainX1000")),
        "rawRelativePath": match.group("rawRelativePath"),
        "previewRelativePath": match.group("previewRelativePath"),
        "previewDataProductPath": match.group("previewDataProductPath"),
        "rawDataProductPath": match.group("rawDataProductPath"),
        "previewPublished": match.group("previewPublished") == "True",
        "rawPublished": match.group("rawPublished") == "True",
    }


def parse_failure_detail(line: str) -> tuple[str, int]:
    match = re.search(r"result\s+(?P<result>\S+)\s+detail\s+(?P<detail>\d+)", line)
    if match is None:
        raise RuntimeError(f"could not parse failure detail from line: {line}")
    return match.group("result"), int(match.group("detail"))


def command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"command {name!r} not found in dictionary")


def capture_metadata_snapshot(
    *,
    session,
    stack,
    opcodes: dict[str, int],
    label: str,
    timeout: float = 10.0,
) -> tuple[str, dict[str, object]]:
    start = len(read_text(stack.sband.events_log).splitlines())
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
        timeout=timeout,
        label=label,
        log_waits=[(stack.obc_log, f"Opcode 0x{opcodes['OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA']:08x} completed")],
    )
    line = wait_event_fragment(stack.sband.events_log, "PAYLOAD_CAPTURE_METADATA", timeout, start)
    return line, parse_payload_capture_metadata(line)


clean_dir(probe_tmp_dir)
clean_dir(runtime_root)
clean_dir(stack_root)

dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
dictionaries = Dictionaries()
dictionaries.load_dictionaries(str(dict_path), None, None)
cmd_encoder = CmdEncoder()
opcodes = {
    name: command_opcode(dictionary, name)
    for name in (
        "OBCApp.modeManager.MODE_SET",
        "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
        "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS",
        "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
        "OBCApp.payloadOpsController.PAYLOAD_PREPARE",
        "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO",
        "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC",
        "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
        "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
    )
}


def encode_command_args(command: str, *args: str) -> bytes:
    template = dictionaries.command_name[command]
    encoded = cmd_encoder.encode_api(CmdData(tuple(args), template))
    return encoded[14:]


processes = []
stack = None


def cleanup() -> None:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)


def send_named_command(
    *,
    session,
    stack,
    opcodes: dict[str, int],
    command: str,
    args: tuple[str, ...] = (),
    timeout: float = 10.0,
    label: str | None = None,
    log_waits: list[tuple[pathlib.Path, str]] | tuple[tuple[pathlib.Path, str], ...] = (),
) -> None:
    send_secure_command(
        raw_command_log=stack.sband.raw_command_log,
        gds_tts_port=stack.sband.gds_tts_port,
        session=session,
        opcode=opcodes[command],
        args=encode_command_args(command, *args),
        timeout=timeout,
        label=label or command,
        log_waits=log_waits,
    )


install_signal_cleanup(cleanup)

try:
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
        mode_name="payload-persistent-session",
        root_dir=root_dir,
        bin_dir=bin_dir,
        dictionary_path=dict_path,
        cli_path=find_fprime_cli(root_dir),
        stack_root=stack_root,
        runtime_root=runtime_root,
        expose_sband_surface=True,
        expose_uhf_surface=False,
        preserve_sband_primary=True,
        auto_ports=True,
        command_authority_profile="sband-primary",
    )
    stack.start()
    if stack.sband is None:
        raise RuntimeError("hosted stack did not expose S-band surface")
    wait_text_since(stack.sband_process_log, 0, "node=5", 20.0)
    wait_text_since(stack.sband_gateway_log, 0, "gds-connected", 20.0)

    session = authenticate_service(
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

    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.modeManager.MODE_SET",
        args=("IDLE",),
        log_waits=[(stack.obc_log, "System mode changed to IDLE")],
        timeout=12.0,
        label="mode-set-idle",
    )

    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.modeManager.MODE_SET",
        args=("PAYLOAD",),
        log_waits=[(stack.obc_log, "System mode changed to PAYLOAD")],
        timeout=12.0,
        label="mode-set-payload",
    )

    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
        args=("PRESET_VGA_640X480", "90", "false", "false"),
        timeout=10.0,
        label="payload-set-camera-defaults-vga",
    )
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS",
        args=("AWB_AUTO", "METER_CENTRE", "0"),
        timeout=10.0,
        label="payload-set-auto-defaults-vga",
    )
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
        args=("30000", "400"),
        timeout=10.0,
        label="payload-set-deterministic-defaults-vga",
    )

    prepare_events_start = len(read_text(stack.sband.events_log).splitlines())
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_PREPARE",
        log_waits=[(stack.obc_log, f"Opcode 0x{opcodes['OBCApp.payloadOpsController.PAYLOAD_PREPARE']:08x} completed")],
        timeout=20.0,
        label="payload-prepare",
    )
    wait_event_fragment(stack.sband.events_log, "Payload state PSTATE_READY result PRESULT_OK", 20.0, prepare_events_start)
    prepare_event_count = read_text(stack.sband.events_log).count("Payload state PSTATE_READY result PRESULT_OK")

    auto_start = len(read_text(stack.sband.events_log).splitlines())
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO",
        args=("48", "auto-capture", "0", "AWB_AUTO", "METER_CENTRE", "0"),
        timeout=20.0,
        label="payload-capture-auto",
    )
    auto_fragment, auto_line = wait_any_event(stack.sband.events_log, ["PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"], 45.0, auto_start)
    if auto_fragment != "PAYLOAD_CAPTURED":
        raise RuntimeError(f"AUTO capture failed unexpectedly: {auto_line}")
    auto_metadata_line, auto_metadata = capture_metadata_snapshot(
        session=session,
        stack=stack,
        opcodes=opcodes,
        label="payload-get-last-metadata-auto",
    )
    auto_preview = stack.runtime_root / str(auto_metadata["previewRelativePath"])
    auto_raw = stack.runtime_root / str(auto_metadata["rawRelativePath"])
    if not auto_preview.is_file() or not auto_raw.is_file():
        raise RuntimeError("AUTO capture did not create hosted source artifacts")
    if str(auto_metadata["capturePolicy"]) != "CAPTURE_AUTO":
        raise RuntimeError(f"unexpected AUTO capture policy: {auto_metadata['capturePolicy']}")
    if int(auto_metadata["actualExposureUsec"]) <= 0 or int(auto_metadata["actualGainX100"]) <= 0:
        raise RuntimeError(f"AUTO metadata did not report positive actual exposure/gain: {auto_metadata_line}")
    if not bool(auto_metadata["actualAwbValid"]):
        raise RuntimeError(f"AUTO metadata did not report valid AWB: {auto_metadata_line}")

    det_start = len(read_text(stack.sband.events_log).splitlines())
    det_exposure = int(auto_metadata["actualExposureUsec"])
    det_gain = int(auto_metadata["actualGainX100"])
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC",
        args=(
            "49",
            "det-capture",
            str(PAYLOAD_MASK_EXPOSURE_USEC | PAYLOAD_MASK_GAIN_X100),
            str(det_exposure),
            str(det_gain),
        ),
        timeout=20.0,
        label="payload-capture-deterministic",
    )
    det_fragment, det_line = wait_any_event(stack.sband.events_log, ["PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"], 45.0, det_start)
    if det_fragment != "PAYLOAD_CAPTURED":
        raise RuntimeError(f"DETERMINISTIC capture failed unexpectedly: {det_line}")
    det_metadata_line, det_metadata = capture_metadata_snapshot(
        session=session,
        stack=stack,
        opcodes=opcodes,
        label="payload-get-last-metadata-deterministic",
    )
    det_preview = stack.runtime_root / str(det_metadata["previewRelativePath"])
    det_raw = stack.runtime_root / str(det_metadata["rawRelativePath"])
    if not det_preview.is_file() or not det_raw.is_file():
        raise RuntimeError("DETERMINISTIC capture did not create hosted source artifacts")
    if str(det_metadata["capturePolicy"]) != "CAPTURE_DETERMINISTIC":
        raise RuntimeError(f"unexpected DETERMINISTIC capture policy: {det_metadata['capturePolicy']}")
    if int(det_metadata["actualExposureUsec"]) != det_exposure or int(det_metadata["actualGainX100"]) != det_gain:
        raise RuntimeError(
            "DETERMINISTIC actual exposure/gain did not align with requested values: "
            f"{det_metadata_line}"
        )
    if bool(det_metadata["actualAwbValid"]):
        raise RuntimeError(f"DETERMINISTIC metadata unexpectedly reported AWB valid: {det_metadata_line}")

    mismatch_start = len(read_text(stack.sband.events_log).splitlines())
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
        args=("PRESET_HD_1280X720", "80", "false", "false"),
        timeout=20.0,
        label="payload-set-camera-defaults-while-prepared",
    )
    mismatch_fragment, mismatch_line = wait_any_event(stack.sband.events_log, ["PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"], 45.0, mismatch_start)
    if mismatch_fragment != "PAYLOAD_OPERATION_FAILED":
        raise RuntimeError(f"camera-defaults update while prepared should reject, got: {mismatch_line}")
    failure_result, failure_detail = parse_failure_detail(mismatch_line)
    if failure_result != "PRESULT_REJECTED_BUSY":
        raise RuntimeError(
            "camera-defaults update while prepared returned unexpected result: "
            f"result={failure_result} detail={failure_detail}"
        )
    send_named_command(
        session=session,
        stack=stack,
        opcodes=opcodes,
        command="OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
        log_waits=[(stack.obc_log, "Payload state PSTATE_OFF")],
        timeout=20.0,
        label="payload-shutdown",
    )

    summary = {
        "formalVerdict": "payload-persistent-session-hosted=PASS",
        "probeRoot": str(probe_tmp_dir),
        "stackRoot": str(stack_root),
        "runtimeRoot": str(stack.runtime_root),
        "prepareReadyCount": prepare_event_count,
        "sharedReadyPath": "PAYLOAD_SET_CAMERA_DEFAULTS -> PAYLOAD_SET_AUTO_DEFAULTS -> PAYLOAD_SET_DETERMINISTIC_DEFAULTS -> PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_AUTO -> PAYLOAD_GET_LAST_CAPTURE_METADATA -> PAYLOAD_CAPTURE_DETERMINISTIC -> PAYLOAD_GET_LAST_CAPTURE_METADATA -> PAYLOAD_SHUTDOWN",
        "sameSessionNoReprepare": True,
        "cameraDefaultsWhilePreparedReject": True,
        "auto": auto_metadata,
        "deterministic": det_metadata,
        "mismatchReject": {
            "result": failure_result,
            "detail": failure_detail,
        },
        "logs": {
            "events": str(stack.sband.events_log),
            "rawCommand": str(stack.sband.raw_command_log),
            "obc": str(stack.obc_log),
        },
    }
    summary_path = probe_tmp_dir / "payload-persistent-session-hosted-summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True), encoding="utf-8")
    summary_lines = [
        "payload-persistent-session-hosted: PASS",
        f"probe-root={probe_tmp_dir}",
        f"stack-root={stack_root}",
        f"runtime-root={stack.runtime_root}",
        "shared-ready-session=PASS",
        "auto-capture=PASS",
        "auto-metadata-actual=PASS",
        "deterministic-capture=PASS",
        "deterministic-alignment=PASS",
        "same-session-no-reprepare=PASS",
        "camera-defaults-while-prepared-reject=PASS",
        f"prepare-ready-count={prepare_event_count}",
        f"summary-json={summary_path}",
        "payload-persistent-session-hosted: PASS",
    ]
    summary_log = probe_tmp_dir / "summary.log"
    summary_log.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
    print(summary_log.read_text(encoding="utf-8"), end="")
finally:
    cleanup()
PY
