#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

obc_force_cleanup_process_pattern 'payload-raw-preview-dual-artifact-v1-hosted\.'
obc_force_cleanup_process_pattern "${ROOT_DIR}/fprime-venv/bin/python[^ ]* -u -m fprime_gds\\.executables\\.(comm|tcpserver)"
trap 'obc_force_cleanup_process_pattern '\''payload-raw-preview-dual-artifact-v1-hosted\.'\''' EXIT

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
FPRIME_CLI_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-cli"
FPRIME_GDS_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-gds"
FPRIME_DP_WRITE_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-dp-write"
VENV_PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python3"
PAYLOAD_FDP_EXTRACT_BIN="${ROOT_DIR}/scripts/payload_fdp_extract.py"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${FPRIME_CLI_BIN}" || ! -x "${FPRIME_GDS_BIN}" || ! -x "${FPRIME_DP_WRITE_BIN}" || ! -x "${VENV_PYTHON_BIN}" || ! -f "${PAYLOAD_FDP_EXTRACT_BIN}" ]]; then
  echo "Required build outputs or tooling are missing. Build native OBC and ensure payload FDP tooling exists first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/payload-raw-preview-dual-artifact-v1-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/pedc-hosted-rt}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-${PROBE_TMP_DIR}/gds-downlink}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
FPRIME_GDS_BIN="${FPRIME_GDS_BIN}" \
FPRIME_DP_WRITE_BIN="${FPRIME_DP_WRITE_BIN}" \
VENV_PYTHON_BIN="${VENV_PYTHON_BIN}" \
PAYLOAD_FDP_EXTRACT_BIN="${PAYLOAD_FDP_EXTRACT_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
"${VENV_PYTHON_BIN}" - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import re
import shutil
import signal
import socket
import struct
import subprocess
import time
from datetime import datetime, timezone

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.models.dictionaries import Dictionaries

import sys


COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32
FILE_DOWNLINK_PATH_MAX = 100
PROFILE_AUTH = {
    "sband-primary": (1, 1, bytes.fromhex("101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F")),
}
COMMAND_SETTLE_SEC = 1.0

CASE_DEFS = [
    {
        "name": "vga",
        "capture_index": 0x10,
        "resolution": "PRESET_VGA_640X480",
        "jpeg_quality": 85,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": True,
        "publish_raw": True,
        "downlink_preview": True,
        "downlink_raw": True,
    },
    {
        "name": "hd",
        "capture_index": 0x11,
        "resolution": "PRESET_HD_1280X720",
        "jpeg_quality": 85,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": True,
        "publish_raw": True,
        "downlink_preview": True,
        "downlink_raw": False,
    },
    {
        "name": "full",
        "capture_index": 0x12,
        "resolution": "PRESET_FULL_3280X2464",
        "jpeg_quality": 85,
        "exposure_usec": 30000,
        "gain_x100": 400,
        "required": False,
        "publish_raw": True,
        "downlink_preview": False,
        "downlink_raw": False,
    },
]


def selected_case_defs() -> list[dict[str, object]]:
    if DOWNLINK_PROFILE == "preview-only":
        return [case for case in CASE_DEFS if bool(case.get("downlink_preview", False))]
    return list(CASE_DEFS)


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
FPRIME_CLI = pathlib.Path(require_env("FPRIME_CLI_BIN"))
FPRIME_GDS = pathlib.Path(require_env("FPRIME_GDS_BIN"))
FPRIME_DP_WRITE = pathlib.Path(require_env("FPRIME_DP_WRITE_BIN"))
VENV_PYTHON = pathlib.Path(require_env("VENV_PYTHON_BIN"))
PAYLOAD_FDP_EXTRACT = pathlib.Path(require_env("PAYLOAD_FDP_EXTRACT_BIN"))
COMM_DOWNLINK_V3_STATUS_PROBE = BIN_DIR / "comm_downlink_v3_status_probe"
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = pathlib.Path(require_env("RUNTIME_ROOT"))
GDS_FILE_STORAGE_DIR = pathlib.Path(require_env("GDS_FILE_STORAGE_DIR"))
DOWNLINK_MATCH_TIMEOUT_SEC = float(os.environ.get("DOWNLINK_MATCH_TIMEOUT_SEC", "1800"))
DOWNLINK_PROFILE = os.environ.get("PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE", "full").strip().lower()

if not COMM_DOWNLINK_V3_STATUS_PROBE.is_file():
    raise RuntimeError(f"required v3 status probe binary is missing: {COMM_DOWNLINK_V3_STATUS_PROBE}")

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from hosted_secure_command_helpers import authenticate_service, send_secure_command, wait_for_socket  # noqa: E402
from secure_link_auth_lib import SERVICE_ID_SBAND  # noqa: E402

events_log = PROBE_TMP_DIR / "events.log"
obc_log = PROBE_TMP_DIR / "obc.log"
summary_log = PROBE_TMP_DIR / "summary.log"
command_log = PROBE_TMP_DIR / "command.log"
gds_log = PROBE_TMP_DIR / "gds.log"
security_server_log = PROBE_TMP_DIR / "security-server.log"
security_server_socket = PROBE_TMP_DIR / "security-server" / "secure-server.sock"
gds_to_southbound_capture = PROBE_TMP_DIR / "gds-to-southbound.bin"
southbound_to_gds_capture = PROBE_TMP_DIR / "southbound-to-gds.bin"


def list_repo_root_aliases() -> set[pathlib.Path]:
    aliases: set[pathlib.Path] = set()
    for alias in list(ROOT_DIR.glob(".adm-*")) + list(ROOT_DIR.glob(".stg-*")) + [
        ROOT_DIR / ".sequence-staging",
        ROOT_DIR / ".sequence-admitted",
    ]:
        if alias.exists() or alias.is_symlink():
            aliases.add(alias)
    return aliases


PREEXISTING_ROOT_ALIASES = list_repo_root_aliases()


def cleanup_owned_repo_root_aliases() -> None:
    for alias in list_repo_root_aliases():
        if alias in PREEXISTING_ROOT_ALIASES:
            continue
        if alias.is_symlink() or alias.is_file():
            alias.unlink(missing_ok=True)
            continue
        if alias.is_dir():
            shutil.rmtree(alias)


def assert_no_new_repo_root_aliases() -> None:
    residue = sorted(str(alias) for alias in (list_repo_root_aliases() - PREEXISTING_ROOT_ALIASES))
    if residue:
        raise RuntimeError(f"repo root alias residue should not be created: {residue}")


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def validate_runtime_root_path_budget() -> None:
    sample_payload = RUNTIME_ROOT / "data-products" / "Dp_268673025_12345678_12345678.fdp"
    sample_hk = RUNTIME_ROOT / "data-products" / "Dp_268693505_12345678_12345678.fdp"
    for label, sample in (("payload", sample_payload), ("hk", sample_hk)):
        if len(str(sample)) >= FILE_DOWNLINK_PATH_MAX:
            raise RuntimeError(
                f"{label} downlink source path would exceed FileDownlink's {FILE_DOWNLINK_PATH_MAX}-byte limit: {sample}"
            )


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
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


def artifact_downlink_required(*, preview_requested: bool, raw_requested: bool, artifact_kind: str) -> bool:
    if artifact_kind == "PREVIEW_JPEG":
        return preview_requested
    if artifact_kind == "RAW_FRAME":
        if DOWNLINK_PROFILE == "preview-only":
            return False
        if DOWNLINK_PROFILE == "full":
            return raw_requested
        raise RuntimeError(f"unsupported PAYLOAD_DUAL_ARTIFACT_DOWNLINK_PROFILE: {DOWNLINK_PROFILE}")
    raise RuntimeError(f"unsupported artifact kind: {artifact_kind}")


def iso_utc(ts: float) -> str:
    return datetime.fromtimestamp(ts, tz=timezone.utc).isoformat().replace("+00:00", "Z")


def run_downlink_v3_status_probe(log_name: str) -> dict[str, object]:
    args = [
        str(COMM_DOWNLINK_V3_STATUS_PROBE),
        "--target-node",
        "5",
        "--local-node",
        "7",
        "--timeout-ms",
        "1000",
        "--interface-name",
        "PAYV3PRB",
    ]
    result = subprocess.run(
        args,
        cwd=str(ROOT_DIR),
        env=base_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    log_path = PROBE_TMP_DIR / log_name
    log_path.write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(f"comm_downlink_v3_status_probe failed; see {log_path}")
    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not lines:
        raise RuntimeError(f"comm_downlink_v3_status_probe produced no output; see {log_path}")
    try:
        status = json.loads(lines[-1])
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"comm_downlink_v3_status_probe did not emit JSON; see {log_path}") from exc
    if int(status.get("targetNode", 0)) != 5:
        raise RuntimeError(f"unexpected target node in v3 status probe output: {status}")
    return status


def wait_event_fragment(fragment: str, timeout: float, start: int) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            if fragment in line:
                return line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for event fragment {fragment!r}")


def wait_event_or_obc_fragment(fragment: str, timeout: float, event_start: int, obc_start: int) -> tuple[str, str]:
    deadline = time.time() + timeout
    last_event_line = ""
    last_obc_line = ""
    while time.time() < deadline:
        event_lines = read_text(events_log).splitlines()
        for line in event_lines[event_start:]:
            last_event_line = line
            if fragment in line:
                return "events-log", line
        obc_lines = read_text(obc_log).splitlines()
        for line in obc_lines[obc_start:]:
            last_obc_line = line
            if fragment in line:
                return "obc-log", line
        time.sleep(0.2)
    raise RuntimeError(
        f"timed out waiting for fragment {fragment!r}; "
        f"last-events-line={last_event_line!r} last-obc-line={last_obc_line!r}"
    )


def wait_any_event(fragments: list[str], timeout: float, start: int) -> tuple[str, str]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            for fragment in fragments:
                if fragment in line:
                    return fragment, line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for any event fragment from {fragments}")


def wait_all_event_fragments(fragments: tuple[str, ...], timeout: float, start: int) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        lines = read_text(events_log).splitlines()
        for line in lines[start:]:
            if all(fragment in line for fragment in fragments):
                return line
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for event line containing all fragments {fragments}")


def history_size(path: pathlib.Path) -> int:
    return len(read_text(path).splitlines())


def start(name: str, args: list[str], log: pathlib.Path, env: dict[str, str] | None = None) -> subprocess.Popen[str]:
    handle = log.open("w", encoding="utf-8", buffering=1)
    handle.write("$ " + " ".join(args) + "\n")
    handle.flush()
    process = subprocess.Popen(
        args,
        env=env,
        cwd=str(ROOT_DIR),
        stdin=subprocess.DEVNULL,
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )
    processes.append((name, process, handle))
    return process


def gds_command_packet(opcode: int, args: bytes = b"") -> bytes:
    packet = struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args
    return struct.pack(">II", COMMAND_DESCRIPTOR, len(packet)) + packet


def send_gds_payload(payload: bytes) -> None:
    with socket.create_connection(("127.0.0.1", gds_tts_port), timeout=5.0) as sock:
        sock.sendall(b"Register GUI\n")
        time.sleep(0.1)
        sock.sendall(b"A5A5 FSW " + payload)
        time.sleep(0.2)


def encode_inner_command(command: str, *args: str) -> bytes:
    template = dictionaries.command_name[command]
    encoded = cmd_encoder.encode_api(CmdData(tuple(args), template))
    return encoded[8:]


def encode_command_args(command: str, *args: str) -> bytes:
    inner = encode_inner_command(command, *args)
    return inner[6:]


def send_enveloped_command(command: str, *args: str, sequence_number: int | None = None) -> int:
    del sequence_number
    if secure_session is None:
        raise RuntimeError("secure session has not been established")
    event_start = history_size(events_log)
    opcode = command_opcodes[command]
    send_secure_command(
        raw_command_log=command_log,
        gds_tts_port=gds_tts_port,
        session=secure_session,
        opcode=opcode,
        args=encode_command_args(command, *args),
        timeout=10.0,
        label=f"{command} args={list(args)}",
    )
    return event_start


def send_enveloped_until_event(command: str, expected_fragment: str, timeout: float, attempts: int, *args: str) -> int:
    last_start = 0
    for _ in range(attempts):
        last_start = send_enveloped_command(command, *args)
        try:
            wait_event_fragment(expected_fragment, timeout, last_start)
            time.sleep(COMMAND_SETTLE_SEC)
            return last_start
        except RuntimeError:
            time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for {expected_fragment!r} after {command}")


def send_enveloped_and_settle(command: str, *args: str, settle_sec: float = COMMAND_SETTLE_SEC) -> int:
    start_index = send_enveloped_command(command, *args)
    time.sleep(settle_sec)
    return start_index


def open_session_until_opened(timeout: float, attempts: int) -> None:
    del timeout, attempts
    global secure_session
    secure_session = authenticate_service(
        capture_path=southbound_to_gds_capture,
        raw_command_log=command_log,
        gds_tts_port=gds_tts_port,
        scid=68,
        vcid=1,
        frame_size=4096,
        security_server_socket=security_server_socket,
        obc_log=obc_log,
        service_id=SERVICE_ID_SBAND,
        ingress_port=0,
        expected_open_pattern=re.escape("Command session opened ingress 0 identity 1 role 1 session ") + r".*replaced 0",
        auth_timeout=10.0,
        establish_timeout=30.0,
    )
    time.sleep(COMMAND_SETTLE_SEC)


def wait_channel_contains(channel: str, fragment: str, timeout: float) -> None:
    args = [
        str(FPRIME_CLI),
        "channels",
        "--dictionary",
        str(DICT_PATH),
        "--no-zmq",
        "--tts-port",
        str(gds_tts_port),
        channel,
    ]
    deadline = time.time() + timeout
    while time.time() < deadline:
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
        if fragment in result.stdout:
            return
        time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for channel {channel} to contain {fragment!r}")


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


def parse_failure_detail(line: str) -> tuple[str, int]:
    match = re.search(r"result (?P<result>\S+) detail (?P<detail>\d+)", line)
    if match is None:
        raise RuntimeError(f"could not parse payload failure line: {line}")
    return match.group("result"), int(match.group("detail"))


def extract_summary(summary_path: pathlib.Path) -> dict[str, object]:
    return json.loads(summary_path.read_text(encoding="utf-8"))


def run_payload_extract(
    *,
    received_fdp: pathlib.Path,
    output_dir: pathlib.Path,
    source_artifact: pathlib.Path,
    expected_capture_id: int,
    expected_capture_index: int,
    expected_artifact_kind: str,
    expected_relative_path: str,
    expected_relative_data_product_path: str,
    expected_resolution: str,
    expected_capture_policy: str,
    require_valid_jpeg: bool,
) -> dict[str, object]:
    args = [
        str(VENV_PYTHON),
        str(PAYLOAD_FDP_EXTRACT),
        "--fdp-file",
        str(received_fdp),
        "--dictionary",
        str(DICT_PATH),
        "--output-dir",
        str(output_dir),
        "--dp-writer",
        str(FPRIME_DP_WRITE),
        "--source-artifact",
        str(source_artifact),
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
        "--require-published",
    ]
    if require_valid_jpeg:
        args.append("--require-valid-jpeg")
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    (output_dir / "extract.log").write_text(result.stdout, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(f"payload_fdp_extract.py failed for {received_fdp}; see {output_dir / 'extract.log'}")
    return extract_summary(output_dir / "payload-fdp-summary.json")


def build_source_family_summaries(artifacts: list[dict[str, object]]) -> None:
    for artifact in artifacts:
        extract_dir = PROBE_TMP_DIR / "source-decode" / str(artifact["name"]) / str(artifact["artifactKind"])
        extract_result = run_payload_extract(
            received_fdp=pathlib.Path(str(artifact["sourceFdp"])),
            output_dir=extract_dir,
            source_artifact=pathlib.Path(str(artifact["sourceArtifact"])),
            expected_capture_id=int(artifact["captureId"]),
            expected_capture_index=int(artifact["captureIndex"]),
            expected_artifact_kind=str(artifact["artifactKind"]),
            expected_relative_path=str(artifact["relativePath"]),
            expected_relative_data_product_path=str(artifact["relativeDataProductPath"]),
            expected_resolution=str(artifact["resolution"]),
            expected_capture_policy=str(artifact["capturePolicy"]),
            require_valid_jpeg=bool(artifact["artifactKind"] == "PREVIEW_JPEG"),
        )
        artifact["sourceFamilySummary"] = extract_result
        artifact["sourceFdpFamilyBytes"] = sum(pathlib.Path(path).stat().st_size for path in extract_result["familyFdpFiles"])


def find_matching_received_fdp(artifacts: list[dict[str, object]], timeout_sec: float) -> dict[tuple[str, str], dict[str, object]]:
    unmatched = {(str(artifact["name"]), str(artifact["artifactKind"])): artifact for artifact in artifacts}
    matched: dict[tuple[str, str], dict[str, object]] = {}
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        for artifact_key, artifact in list(unmatched.items()):
            source_summary = artifact["sourceFamilySummary"]
            expected_family_sha = {str(sha) for sha in source_summary["familyFdpSha256"]}
            for received in sorted(GDS_FILE_STORAGE_DIR.rglob("*.fdp")):
                if not received.is_file():
                    continue
                if file_sha256(received) not in expected_family_sha:
                    continue
                extract_dir = PROBE_TMP_DIR / "received-decode" / str(artifact["name"]) / str(artifact["artifactKind"])
                try:
                    extract_result = run_payload_extract(
                        received_fdp=received,
                        output_dir=extract_dir,
                        source_artifact=pathlib.Path(str(artifact["sourceArtifact"])),
                        expected_capture_id=int(artifact["captureId"]),
                        expected_capture_index=int(artifact["captureIndex"]),
                        expected_artifact_kind=str(artifact["artifactKind"]),
                        expected_relative_path=str(artifact["relativePath"]),
                        expected_relative_data_product_path=str(artifact["relativeDataProductPath"]),
                        expected_resolution=str(artifact["resolution"]),
                        expected_capture_policy=str(artifact["capturePolicy"]),
                        require_valid_jpeg=bool(artifact["artifactKind"] == "PREVIEW_JPEG"),
                    )
                except RuntimeError:
                    continue
                if extract_result["familyFdpSha256"] != source_summary["familyFdpSha256"]:
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
    received_files = ", ".join(str(path) for path in sorted(GDS_FILE_STORAGE_DIR.rglob("*.fdp")))
    raise RuntimeError(
        "did not find byte-matching GDS-received .fdp families for all payload artifacts; "
        f"matched={len(matched)}/{len(artifacts)} received=[{received_files}]"
    )


def prune_catalog_to_payload_products(payload_source_fdps: list[pathlib.Path]) -> None:
    allowed = {path.resolve() for path in payload_source_fdps}
    data_products_root = RUNTIME_ROOT / "data-products"
    for candidate in data_products_root.glob("*.fdp"):
        if candidate.resolve() not in allowed:
            candidate.unlink()


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
    source_artifact: pathlib.Path,
    source_fdp: pathlib.Path,
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
        "sourceArtifact": str(source_artifact),
        "sourceArtifactBytes": source_artifact.stat().st_size,
        "sourceArtifactSha256": file_sha256(source_artifact),
        "sourceFdp": str(source_fdp),
        "sourceFdpFirstSliceBytes": source_fdp.stat().st_size,
        "sourceFdpFirstSliceSha256": file_sha256(source_fdp),
    }


def capture_case(case: dict[str, object]) -> dict[str, object]:
    case_name = str(case["name"])
    capture_index = int(case["capture_index"])
    resolution = str(case["resolution"])
    jpeg_quality = int(case["jpeg_quality"])
    exposure_usec = int(case["exposure_usec"])
    gain_x100 = int(case["gain_x100"])
    required = bool(case["required"])
    publish_raw = bool(case["publish_raw"]) and DOWNLINK_PROFILE != "preview-only"
    downlink_preview = bool(case.get("downlink_preview", False))
    downlink_raw = bool(case.get("downlink_raw", False))
    tag = f"{case_name}-capture"

    send_enveloped_and_settle(
        "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS",
        resolution,
        str(jpeg_quality),
        "false",
        "false",
    )
    send_enveloped_and_settle(
        "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS",
        str(exposure_usec),
        str(gain_x100),
    )

    send_enveloped_until_event(
        "OBCApp.payloadOpsController.PAYLOAD_PREPARE",
        "Payload state PSTATE_READY result PRESULT_OK",
        20.0,
        3,
    )

    capture_start = send_enveloped_command(
        "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC",
        str(capture_index),
        tag,
        str((1 << 4) | (1 << 5)),
        str(exposure_usec),
        str(gain_x100),
    )
    fragment, line = wait_any_event(["PAYLOAD_CAPTURED", "PAYLOAD_OPERATION_FAILED"], 45.0, capture_start)

    metadata_start = send_enveloped_until_event(
        "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
        "PAYLOAD_CAPTURE_METADATA",
        10.0,
        3,
    )
    metadata_line = wait_event_fragment("PAYLOAD_CAPTURE_METADATA", 10.0, metadata_start)
    metadata = parse_metadata_line(metadata_line)
    preview_path = RUNTIME_ROOT / str(metadata["previewRelativePath"])
    raw_path = RUNTIME_ROOT / str(metadata["rawRelativePath"])
    preview_fdp_path = RUNTIME_ROOT / str(metadata["previewDataProductPath"])
    raw_fdp_path = RUNTIME_ROOT / str(metadata["rawDataProductPath"])

    result: dict[str, object] = {
        "name": case_name,
        "resolution": resolution,
        "jpegQuality": jpeg_quality,
        "required": required,
        "publishRaw": publish_raw,
        "captureId": int(metadata["captureId"]),
        "captureIndex": int(metadata["captureIndex"]),
        "capturePolicy": str(metadata["capturePolicy"]),
        "rawRelativePath": str(metadata["rawRelativePath"]),
        "previewRelativePath": str(metadata["previewRelativePath"]),
        "previewDataProductPath": str(metadata["previewDataProductPath"]),
        "rawDataProductPath": str(metadata["rawDataProductPath"]),
        "previewPublished": bool(metadata["previewPublished"]),
        "rawPublished": bool(metadata["rawPublished"]),
        "sourceRaw": str(raw_path),
        "sourcePreview": str(preview_path),
        "localRawBytes": raw_path.stat().st_size if raw_path.is_file() else 0,
        "localPreviewBytes": preview_path.stat().st_size if preview_path.is_file() else 0,
        "artifacts": [],
    }

    if fragment == "PAYLOAD_CAPTURED":
        if not preview_fdp_path.is_file():
            raise RuntimeError(f"payload capture reported success but preview source .fdp was missing: {preview_fdp_path}")
        if not bool(metadata["previewPublished"]):
            raise RuntimeError(f"payload capture {case_name} reported success but previewPublished=false")
        result["artifacts"].append(
            build_artifact_entry(
                case_name=case_name,
                resolution=resolution,
                capture_id=int(metadata["captureId"]),
                capture_index=int(metadata["captureIndex"]),
                capture_policy=str(metadata["capturePolicy"]),
                artifact_kind="PREVIEW_JPEG",
                downlink_required=artifact_downlink_required(
                    preview_requested=downlink_preview,
                    raw_requested=downlink_raw,
                    artifact_kind="PREVIEW_JPEG",
                ),
                relative_path=str(metadata["previewRelativePath"]),
                relative_data_product_path=str(metadata["previewDataProductPath"]),
                source_artifact=preview_path,
                source_fdp=preview_fdp_path,
            )
        )
    else:
        failure_result, failure_detail = parse_failure_detail(line)
        result.update(
            {
                "status": "failed",
                "failureResult": failure_result,
                "failureDetail": failure_detail,
            }
        )
        if required:
            raise RuntimeError(
                f"required payload capture case {case_name} failed with {failure_result} detail={failure_detail}"
            )
        send_enveloped_until_event(
            "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
            "Payload state PSTATE_OFF",
            20.0,
            3,
        )
        return result

    result["status"] = "preview-published"

    if publish_raw:
        raw_publish_start = send_enveloped_command(
            "OBCApp.payloadOpsController.PAYLOAD_PUBLISH_CAPTURE",
            str(capture_index),
            "RAW_FRAME",
        )
        raw_fragment, raw_line = wait_any_event(["rawPublished True", "PAYLOAD_OPERATION_FAILED"], 180.0, raw_publish_start)
        if raw_fragment == "PAYLOAD_OPERATION_FAILED":
            raw_result, raw_detail = parse_failure_detail(raw_line)
            result["rawPublishStatus"] = "failed"
            result["rawFailureResult"] = raw_result
            result["rawFailureDetail"] = raw_detail
            if resolution == "PRESET_FULL_3280X2464":
                if raw_result != "PRESULT_STORAGE_FAILED" or raw_detail != 24:
                    raise RuntimeError(
                        f"full raw publish should fail with deferred boundary, got {raw_result} detail={raw_detail}"
                    )
                result["rawPublishStatus"] = "bounded-full-raw-deferred"
            elif required:
                raise RuntimeError(f"required raw publish case {case_name} failed with {raw_result} detail={raw_detail}")
        else:
            raw_metadata_start = send_enveloped_until_event(
                "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
                "rawPublished True",
                10.0,
                3,
            )
            raw_metadata_line = wait_all_event_fragments(("PAYLOAD_CAPTURE_METADATA", "rawPublished True"), 20.0, raw_metadata_start)
            raw_metadata = parse_metadata_line(raw_metadata_line)
            raw_fdp_path = RUNTIME_ROOT / str(raw_metadata["rawDataProductPath"])
            if not raw_fdp_path.is_file():
                raise RuntimeError(f"payload raw publish reported success but raw source .fdp was missing: {raw_fdp_path}")
            if not bool(raw_metadata["rawPublished"]):
                raise RuntimeError(f"payload raw publish {case_name} reported success but rawPublished=false")
            result["rawPublished"] = True
            result["rawDataProductPath"] = str(raw_metadata["rawDataProductPath"])
            result["artifacts"].append(
                build_artifact_entry(
                    case_name=case_name,
                    resolution=resolution,
                    capture_id=int(raw_metadata["captureId"]),
                    capture_index=int(raw_metadata["captureIndex"]),
                    capture_policy=str(raw_metadata["capturePolicy"]),
                    artifact_kind="RAW_FRAME",
                    downlink_required=artifact_downlink_required(
                        preview_requested=downlink_preview,
                        raw_requested=downlink_raw,
                        artifact_kind="RAW_FRAME",
                    ),
                    relative_path=str(raw_metadata["rawRelativePath"]),
                    relative_data_product_path=str(raw_metadata["rawDataProductPath"]),
                    source_artifact=raw_path,
                    source_fdp=raw_fdp_path,
                )
            )
            result["rawPublishStatus"] = "published"

    send_enveloped_until_event(
        "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN",
        "Payload state PSTATE_OFF",
        20.0,
        3,
    )
    return result


clean_dir(PROBE_TMP_DIR)
clean_dir(RUNTIME_ROOT)
clean_dir(GDS_FILE_STORAGE_DIR)
validate_runtime_root_path_budget()
(RUNTIME_ROOT / "persistent-data").mkdir(parents=True, exist_ok=True)
(RUNTIME_ROOT / "staging").mkdir(parents=True, exist_ok=True)

gds_port = free_port()
gds_tts_port = free_port()
csp_sub_port = free_port()
csp_pub_port = free_port()
radio_port = free_port()
sband_port = free_port()
uhf_port = free_port()

base_env = os.environ.copy()
base_env.update(
    {
        "DICT_PATH": str(DICT_PATH),
        "RUNTIME_ROOT": str(RUNTIME_ROOT),
        "GDS_FILE_STORAGE_DIR": str(GDS_FILE_STORAGE_DIR),
        "GDS_HOST": "127.0.0.1",
        "GDS_PORT": str(gds_port),
        "GDS_TTS_PORT": str(gds_tts_port),
        "GDS_FRAMING_SELECTION": "space-packet-space-data-link",
        "GDS_SCID": "68",
        "GDS_VCID": "1",
        "GDS_FRAME_SIZE": "4096",
        "RADIO_PORT": str(radio_port),
        "GROUND_LINK_MODE": "comm-csp",
        "COMM_CSP_NODE": "5",
        "SBAND_TCP_HOST": "127.0.0.1",
        "SBAND_TCP_PORT": str(sband_port),
        "UHF_TCP_HOST": "127.0.0.1",
        "UHF_TCP_PORT": str(uhf_port),
        "CSP_TRANSPORT": "zmqhub",
        "CSP_HUB_HOST": "127.0.0.1",
        "CSP_HUB_SUB_PORT": str(csp_sub_port),
        "CSP_HUB_PUB_PORT": str(csp_pub_port),
        "EPS_CSP_NODE_ID": "2",
        "ADCS_CSP_NODE_ID": "3",
        "HEADLESS": "1",
    }
)

processes: list[tuple[str, subprocess.Popen[str], object]] = []
secure_session = None
dictionaries = Dictionaries()
dictionaries.load_dictionaries(str(DICT_PATH), None, None)
cmd_encoder = CmdEncoder()
dictionary_json = json.loads(DICT_PATH.read_text(encoding="utf-8"))
command_opcodes = {entry["name"]: int(entry["opcode"]) for entry in dictionary_json.get("commands", [])}

try:
    security_server_socket.parent.mkdir(parents=True, exist_ok=True)
    start(
        "security_server_sim",
        [str(VENV_PYTHON), str(ROOT_DIR / "scripts/security_server_sim.py"), "--socket-path", str(security_server_socket)],
        security_server_log,
        base_env,
    )
    wait_for_socket(security_server_socket, 20.0)

    start(
        "fprime-gds",
        [
            str(FPRIME_GDS),
            "-n",
            "-g",
            "none",
            "--framing-selection",
            "space-packet-space-data-link",
            "--scid",
            "68",
            "--vcid",
            "1",
            "--frame-size",
            "4096",
            "--dictionary",
            str(DICT_PATH),
            "--no-zmq",
            "--ip-address",
            "127.0.0.1",
            "--ip-port",
            str(gds_port),
            "--tts-port",
            str(gds_tts_port),
            "--tts-addr",
            "127.0.0.1",
            "--file-storage-directory",
            str(GDS_FILE_STORAGE_DIR),
            "--log-directly",
            "--logs",
            str(PROBE_TMP_DIR / "gds-logs"),
        ],
        gds_log,
    )
    wait_port(gds_port, 20.0)
    wait_port(gds_tts_port, 20.0)

    start(
        "fprime-cli-events",
        [str(FPRIME_CLI), "events", "--dictionary", str(DICT_PATH), "--no-zmq", "--tts-port", str(gds_tts_port)],
        events_log,
        {**base_env, "PYTHONUNBUFFERED": "1"},
    )
    time.sleep(1.0)

    start(
        "csp_zmqproxy",
        [str(BIN_DIR / "csp_zmqproxy"), "-s", f"tcp://0.0.0.0:{csp_sub_port}", "-p", f"tcp://0.0.0.0:{csp_pub_port}"],
        PROBE_TMP_DIR / "csp_zmqproxy.log",
        base_env,
    )
    start(
        "eps_simulator",
        [str(BIN_DIR / "eps_simulator"), "--node-id", "2", "--initial-soc", "80.0"],
        PROBE_TMP_DIR / "eps_simulator.log",
        base_env,
    )
    start(
        "adcs_simulator",
        [str(BIN_DIR / "adcs_simulator"), "--node-id", "3"],
        PROBE_TMP_DIR / "adcs_simulator.log",
        base_env,
    )
    start(
        "radio_mock_server",
        [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
        PROBE_TMP_DIR / "radio_mock_server.log",
        base_env,
    )
    start(
        "sband_comm_csp_node",
        [
            str(BIN_DIR / "sband_comm_csp_node"),
            "--tcp-listen-host",
            "127.0.0.1",
            "--tcp-listen-port",
            str(sband_port),
            "--node-id",
            "5",
        ],
        PROBE_TMP_DIR / "sband_comm_csp_node.log",
        base_env,
    )
    start(
        "uhf_comm_csp_node",
        [
            str(BIN_DIR / "uhf_comm_csp_node"),
            "--tcp-listen-host",
            "127.0.0.1",
            "--tcp-listen-port",
            str(uhf_port),
            "--node-id",
            "6",
        ],
        PROBE_TMP_DIR / "uhf_comm_csp_node.log",
        base_env,
    )
    time.sleep(4.0)
    wait_port(sband_port, 20.0)
    wait_port(uhf_port, 20.0)
    time.sleep(2.0)
    start(
        "ground_ttc_gateway",
        [
            str(BIN_DIR / "ground_ttc_gateway"),
            "--rf-tcp-host",
            "127.0.0.1",
            "--rf-tcp-port",
            str(sband_port),
            "--link-identity",
            "sband",
            "--gds-host",
            "127.0.0.1",
            "--gds-port",
            str(gds_port),
            "--capture-gds-to-southbound",
            str(gds_to_southbound_capture),
            "--capture-southbound-to-gds",
            str(southbound_to_gds_capture),
        ],
        PROBE_TMP_DIR / "ground_ttc_gateway.log",
        base_env,
    )
    time.sleep(1.0)
    pre_obc_status = run_downlink_v3_status_probe("downlink-v3-status-before-obc.log")

    start(
        "OBC",
        [
            str(BIN_DIR / "OBC"),
            "--comm",
            "tcp",
            "--comm-host",
            "127.0.0.1",
            "--comm-port",
            str(radio_port),
            "--radio-protocol",
            "mock-text",
            "--ground-link",
            "comm-csp",
            "--comm-csp-node",
            "5",
            "--gds-host",
            "127.0.0.1",
            "--gds-port",
            str(gds_port),
            "--runtime-root",
            str(RUNTIME_ROOT),
            "--persistent-root",
            str(RUNTIME_ROOT / "persistent-data"),
            "--staging-root",
            str(RUNTIME_ROOT / "staging"),
            "--command-authority-profile",
            "sband-primary",
            "--tick-ms",
            "250",
            "--headless",
        ],
        obc_log,
        base_env,
    )
    wait_text(obc_log, "Runtime mode: headless", 20.0)
    wait_text(obc_log, "GROUND_LINK_UP", 10.0)
    wait_text(obc_log, "COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available 1", 10.0)
    time.sleep(1.5)

    open_session_until_opened(10.0, 3)
    send_enveloped_until_event("OBCApp.modeManager.MODE_SET", "System mode changed to IDLE", 10.0, 3, "IDLE")
    send_enveloped_until_event("OBCApp.modeManager.MODE_SET", "System mode changed to PAYLOAD", 10.0, 3, "PAYLOAD")

    capture_results: list[dict[str, object]] = []
    for case in selected_case_defs():
        capture_results.append(capture_case(case))

    published_artifacts = [artifact for case in capture_results for artifact in case["artifacts"]]
    build_source_family_summaries(published_artifacts)
    downlink_artifacts = [artifact for artifact in published_artifacts if bool(artifact.get("downlinkRequired"))]
    if not downlink_artifacts:
        raise RuntimeError("no hosted payload artifacts were selected for governed downlink proof")
    downlink_source_fdps = [
        pathlib.Path(path)
        for artifact in downlink_artifacts
        for path in artifact["sourceFamilySummary"]["familyFdpFiles"]
    ]
    prune_catalog_to_payload_products(downlink_source_fdps)

    send_enveloped_command("OBCApp.dpCatalog.BUILD_CATALOG")
    wait_text(obc_log, "CatalogBuildComplete", 20.0)
    status_before_xmit = run_downlink_v3_status_probe("downlink-v3-status-before-xmit.log")
    start_xmit_event_start = history_size(events_log)
    start_xmit_obc_start = history_size(obc_log)
    start_xmit_requested_ts = time.time()
    send_enveloped_command("OBCApp.dpCatalog.START_XMIT_CATALOG", "NO_WAIT")
    wait_text(obc_log, "SendingProduct", 20.0)
    catalog_complete_source, catalog_complete_line = wait_event_or_obc_fragment(
        "CatalogXmitCompleted",
        DOWNLINK_MATCH_TIMEOUT_SEC,
        start_xmit_event_start,
        start_xmit_obc_start,
    )
    catalog_complete_observed_ts = time.time()
    status_at_catalog_complete = run_downlink_v3_status_probe("downlink-v3-status-at-catalog-complete.log")

    matched_received = find_matching_received_fdp(downlink_artifacts, DOWNLINK_MATCH_TIMEOUT_SEC)
    final_gds_arrival_ts = max(float(matched["familyLastArrivalUnixSec"]) for matched in matched_received.values())
    status_after_gds_match = run_downlink_v3_status_probe("downlink-v3-status-after-gds-match.log")

    verified_artifacts: list[dict[str, object]] = []
    for artifact in downlink_artifacts:
        matched = matched_received[(str(artifact["name"]), str(artifact["artifactKind"]))]
        received_fdp = pathlib.Path(str(matched["receivedFdp"]))
        verified_artifact = dict(artifact)
        verified_artifact.update(
            {
                "receivedFdp": str(received_fdp),
                "receivedFdpSha256": file_sha256(received_fdp),
                "extractSummary": matched["extractSummary"],
                "receivedFamilySliceCount": matched["familySliceCount"],
                "receivedFamilyFirstArrivalUtc": iso_utc(float(matched["familyFirstArrivalUnixSec"])),
                "receivedFamilyLastArrivalUtc": iso_utc(float(matched["familyLastArrivalUnixSec"])),
            }
        )
        verified_artifacts.append(verified_artifact)

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

    proof_summary = {
        "formalVerdict": "payload-raw-preview-dual-artifact-v1-hosted=PASS",
        "probeRoot": str(PROBE_TMP_DIR),
        "runtimeRoot": str(RUNTIME_ROOT),
        "gdsFileStorageDir": str(GDS_FILE_STORAGE_DIR),
        "hostedCommandPath": "authenticated envelope -> GDS(CCSDS) -> ground_ttc_gateway -> sband_comm_csp_node(node 5) -> OBC",
        "officialDeliveryPath": "PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink",
        "downlinkProfile": DOWNLINK_PROFILE,
        "node5DownlinkTransport": {
            "selectedMode": "v3",
            "statusBeforeObc": pre_obc_status,
            "statusBeforeXmit": status_before_xmit,
            "statusAtCatalogComplete": status_at_catalog_complete,
            "statusAfterGdsMatch": status_after_gds_match,
        },
        "timing": timing_summary,
        "captureResults": capture_results,
        "publishedArtifacts": published_artifacts,
        "downlinkedArtifacts": verified_artifacts,
        "diagnosticCases": [
            case for case in capture_results if case["status"] == "failed" or case.get("rawPublishStatus") == "bounded-full-raw-deferred"
        ],
    }
    summary_json = PROBE_TMP_DIR / "payload-raw-preview-dual-artifact-summary.json"
    summary_json.write_text(json.dumps(proof_summary, indent=2) + "\n", encoding="utf-8")

    summary_lines = [
        "payload-raw-preview-dual-artifact-v1-hosted: PASS",
        f"probe-root={PROBE_TMP_DIR}",
        f"runtime-root={RUNTIME_ROOT}",
        f"gds-file-storage-dir={GDS_FILE_STORAGE_DIR}",
        "official-path=PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink",
        f"downlink-profile={DOWNLINK_PROFILE}",
        "downlink-queue=BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)",
        f"downlink-match-timeout-sec={DOWNLINK_MATCH_TIMEOUT_SEC:g}",
        f"node5-transport=v3 status-before-obc=PASS connected={str(bool(pre_obc_status['connected'])).lower()} "
        f"accepted={pre_obc_status['acceptedBytes']} flushed={pre_obc_status['flushedBytes']}",
        f"node5-transport=v3 status-before-xmit=PASS connected={str(bool(status_before_xmit['connected'])).lower()} "
        f"accepted={status_before_xmit['acceptedBytes']} flushed={status_before_xmit['flushedBytes']}",
        f"timing-start-xmit-requested-utc={timing_summary['startXmitRequestedUtc']}",
        f"timing-catalog-xmit-completed-observed-utc={timing_summary['catalogXmitCompletedObservedUtc']} "
        f"elapsed-sec={timing_summary['startToCatalogCompletedSec']} source={timing_summary['catalogXmitCompletedSource']}",
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
            f"capture-index=0x{int(case['captureIndex']):02X} preview-bytes={case['localPreviewBytes']} "
            f"raw-bytes={case['localRawBytes']} preview-status={case['status']} "
            f"raw-publish-status={case.get('rawPublishStatus', 'not-requested')}"
        )
    for artifact in verified_artifacts:
        summary_lines.append(
            f"downlink-artifact={artifact['name']} kind={artifact['artifactKind']} resolution={artifact['resolution']} "
            f"artifact-bytes={artifact['sourceArtifactBytes']} fdp-family-bytes={artifact['sourceFdpFamilyBytes']} "
            f"fdp-byte-match=PASS source={artifact['sourceFdp']} received={artifact['receivedFdp']} "
            f"family-first-arrival-utc={artifact['receivedFamilyFirstArrivalUtc']} "
            f"family-last-arrival-utc={artifact['receivedFamilyLastArrivalUtc']} "
            f"family-slices={artifact['receivedFamilySliceCount']}"
        )
        summary_lines.append(
            f"downlink-artifact={artifact['name']} kind={artifact['artifactKind']} extract-hash=PASS "
            f"sha256={artifact['extractSummary']['extractedArtifactSha256']}"
        )
    for case in proof_summary["diagnosticCases"]:
        summary_lines.append(
            f"diagnostic-case={case['name']} resolution={case['resolution']} preview-status={case['status']} "
            f"raw-publish-status={case.get('rawPublishStatus', 'n/a')} "
            f"result={case.get('failureResult', case.get('rawFailureResult', 'n/a'))} "
            f"detail={case.get('failureDetail', case.get('rawFailureDetail', 'n/a'))}"
        )
    summary_lines.append(f"summary-json={summary_json}")
    summary_log.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
    print(summary_log.read_text(encoding="utf-8"), end="")
finally:
    for _, process, handle in reversed(processes):
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
        if process.poll() is None:
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(os.getpgid(process.pid), signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait(timeout=5.0)
        handle.close()
    cleanup_owned_repo_root_aliases()
    assert_no_new_repo_root_aliases()
PY
trap - EXIT
obc_force_cleanup_process_pattern 'payload-raw-preview-dual-artifact-v1-hosted\.'
obc_force_cleanup_process_pattern "${ROOT_DIR}/fprime-venv/bin/python[^ ]* -u -m fprime_gds\\.executables\\.(comm|tcpserver)"
obc_force_cleanup_process_pattern "fprime_gds\.executables\.(comm|tcpserver).*${PROBE_TMP_DIR}"
