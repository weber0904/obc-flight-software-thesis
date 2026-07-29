#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_BINARY_NAME="${OBC_BINARY_NAME:-OBC}"
COMMAND_PREFIX="${COMMAND_PREFIX:-OBCApp}"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="${DICT_PATH:-$(obc_find_dictionary_path "${ROOT_DIR}" "${OBC_BINARY_NAME}" || true)}"
if [[ -z "${DICT_PATH}" ]]; then
  echo "Dictionary not found. Run fprime-util build first." >&2
  exit 1
fi

find_local_tool() {
  local tool_name="${1:?tool_name is required}"
  if [[ -x "${ROOT_DIR}/fprime-venv/bin/${tool_name}" ]]; then
    printf '%s\n' "${ROOT_DIR}/fprime-venv/bin/${tool_name}"
    return 0
  fi
  command -v "${tool_name}"
}

FPRIME_CLI_BIN="$(find_local_tool fprime-cli || true)"
if [[ -z "${FPRIME_CLI_BIN}" ]]; then
  echo "fprime-cli not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi

FPRIME_DP_BIN="$(find_local_tool fprime-dp || true)"
FPRIME_DP_WRITE_BIN="$(find_local_tool fprime-dp-write || true)"
if [[ -z "${FPRIME_DP_BIN}" && -z "${FPRIME_DP_WRITE_BIN}" ]]; then
  echo "Neither fprime-dp nor fprime-dp-write was found. Install one in fprime-venv or PATH." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/hk-data-product-alignment-v2.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/hk-data-product-alignment-v2-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-${PROBE_TMP_DIR}/gds-downlink}"
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC:-55}"
EXPECTED_HK_TREND_VERSION="${EXPECTED_HK_TREND_VERSION:-6}"
GDS_SCID="${GDS_SCID:-68}"
GDS_VCID="${GDS_VCID:-1}"
GDS_FRAME_SIZE="${GDS_FRAME_SIZE:-1024}"
COMM_CSP_NODE="${COMM_CSP_NODE:-5}"
SBAND_TCP_HOST="${SBAND_TCP_HOST:-127.0.0.1}"

mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
FPRIME_DP_BIN="${FPRIME_DP_BIN}" \
FPRIME_DP_WRITE_BIN="${FPRIME_DP_WRITE_BIN}" \
OBC_BINARY_NAME="${OBC_BINARY_NAME}" \
COMMAND_PREFIX="${COMMAND_PREFIX}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC}" \
EXPECTED_HK_TREND_VERSION="${EXPECTED_HK_TREND_VERSION}" \
GDS_SCID="${GDS_SCID}" \
GDS_VCID="${GDS_VCID}" \
GDS_FRAME_SIZE="${GDS_FRAME_SIZE}" \
COMM_CSP_NODE="${COMM_CSP_NODE}" \
SBAND_TCP_HOST="${SBAND_TCP_HOST}" \
python3 - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import re
import shutil
import struct
import subprocess
import time

def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"Required environment variable is missing: {name}")
    return value


root_dir = pathlib.Path(require_env("ROOT_DIR"))
bin_dir = pathlib.Path(require_env("BIN_DIR"))
dict_path = pathlib.Path(require_env("DICT_PATH"))
fprime_dp_bin = pathlib.Path(os.environ["FPRIME_DP_BIN"]) if os.environ.get("FPRIME_DP_BIN") else None
fprime_dp_write_bin = pathlib.Path(os.environ["FPRIME_DP_WRITE_BIN"]) if os.environ.get("FPRIME_DP_WRITE_BIN") else None
obc_binary_name = require_env("OBC_BINARY_NAME")
command_prefix = require_env("COMMAND_PREFIX")
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
observe_timeout_sec = int(require_env("OBSERVE_TIMEOUT_SEC"))
expected_hk_trend_version = int(require_env("EXPECTED_HK_TREND_VERSION"))
gds_scid = require_env("GDS_SCID")
gds_vcid = require_env("GDS_VCID")
gds_frame_size = require_env("GDS_FRAME_SIZE")
comm_csp_node = require_env("COMM_CSP_NODE")
sband_tcp_host = require_env("SBAND_TCP_HOST")
python_bin = pathlib.Path(shutil.which("python3") or "python3")

fdp_decode_log = probe_tmp_dir / "fdp-decode.log"
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
stack_root = pathlib.Path("/tmp") / f"hka-{probe_tmp_dir.name[-6:]}"
stack_runtime_root = stack_root / "r"

os.environ["OBC_DATA_PRODUCTS_QUOTA_BYTES"] = "1048576"
os.environ["DICT_PATH"] = str(dict_path)

import sys

sys.path.insert(0, str(root_dir / "scripts"))

from hosted_secure_command_helpers import authenticate_service, send_secure_command_with_retry, wait_for_socket
from per_band_stock_ground_stacks import HostedPerBandStockStacks, find_fprime_cli
from probe_process_utils import cleanup_managed_processes, start_managed_process
from secure_link_auth_lib import SERVICE_ID_SBAND


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"Refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_bytes().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def wait_for_log(path: pathlib.Path, fragment: str, timeout_sec: float) -> bool:
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        if fragment in read_text(path):
            return True
        time.sleep(0.2)
    return False


def dictionary_command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"Command not found in dictionary: {name}")


def file_sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_matching_received_fdp(source_files: list[pathlib.Path], received_root: pathlib.Path, timeout_sec: float) -> tuple[pathlib.Path, pathlib.Path]:
    source_by_hash = {file_sha256(path): path for path in source_files}
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        for received in sorted(received_root.rglob("*.fdp")):
            if not received.is_file():
                continue
            source = source_by_hash.get(file_sha256(received))
            if source is not None:
                return source, received
        time.sleep(0.5)
    received_files = ", ".join(str(path) for path in sorted(received_root.rglob("*.fdp")))
    raise RuntimeError(f"Did not find a byte-matching GDS-received .fdp file; received=[{received_files}]")


def find_hk_trend_record() -> dict[str, object]:
    dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
    for record in dictionary.get("records", []):
        if "HkTrendRecord" in str(record.get("name", "")):
            return record
    raise RuntimeError("Dictionary does not contain HkTrendRecord")


def find_hk_trend_chunk_meta_record() -> dict[str, object]:
    dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
    for record in dictionary.get("records", []):
        if "HkTrendChunkMeta" in str(record.get("name", "")):
            return record
    raise RuntimeError("Dictionary does not contain HkTrendChunkMeta")


def contains_key_value(value: object, key: str, expected: object) -> bool:
    if isinstance(value, dict):
        if value.get(key) == expected:
            return True
        return any(contains_key_value(child, key, expected) for child in value.values())
    if isinstance(value, list):
        return any(contains_key_value(child, key, expected) for child in value)
    return False


def decode_with_fprime_dp(received: pathlib.Path, decode_dir: pathlib.Path) -> pathlib.Path | None:
    if fprime_dp_bin is None:
        return None
    output_json = decode_dir / "fprime-dp-decode.json"
    args = [
        str(fprime_dp_bin),
        "decode",
        "--bin-file",
        str(received),
        "--dictionary",
        str(dict_path),
        "--output",
        str(output_json),
    ]
    with fdp_decode_log.open("a", encoding="utf-8") as log:
        log.write("$ " + " ".join(args) + "\n")
        log.flush()
        result = subprocess.run(args, cwd=decode_dir, stdout=log, stderr=subprocess.STDOUT, text=True, check=False)
    if result.returncode == 0 and output_json.is_file():
        return output_json
    candidates = sorted(decode_dir.rglob("*.json"))
    return candidates[0] if candidates else None


def decode_with_fprime_dp_write(received: pathlib.Path, decode_dir: pathlib.Path) -> pathlib.Path:
    if fprime_dp_write_bin is None:
        raise RuntimeError("fprime-dp-write is unavailable and fprime-dp decode did not produce JSON")
    args = [str(fprime_dp_write_bin), str(received), str(dict_path)]
    with fdp_decode_log.open("a", encoding="utf-8") as log:
        log.write("$ " + " ".join(args) + "\n")
        log.flush()
        result = subprocess.run(args, cwd=decode_dir, stdout=log, stderr=subprocess.STDOUT, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"fprime-dp-write failed for {received}; see {fdp_decode_log}")
    output = decode_dir / (received.stem + ".json")
    if not output.exists():
        raise RuntimeError(f"fprime-dp-write did not produce expected JSON: {output}")
    return output


def decode_received_fdp(received: pathlib.Path) -> pathlib.Path:
    decode_dir = probe_tmp_dir / "fdp-decode"
    clean_dir(decode_dir)
    decoded = decode_with_fprime_dp(received, decode_dir)
    if decoded is None:
        decoded = decode_with_fprime_dp_write(received, decode_dir)
    hk_record = find_hk_trend_record()
    chunk_meta_record = find_hk_trend_chunk_meta_record()
    decoded_json = json.loads(decoded.read_text(encoding="utf-8"))
    hk_record_id = hk_record.get("id")
    chunk_meta_record_id = chunk_meta_record.get("id")
    decoded_ids = [entry.get("dataId") for entry in decoded_json if isinstance(entry, dict)]
    if hk_record_id not in decoded_ids:
        raise RuntimeError(f"Decoded .fdp did not contain HK trend record id {hk_record_id}; decoded ids={decoded_ids}")
    if chunk_meta_record_id not in decoded_ids:
        raise RuntimeError(
            f"Decoded .fdp did not contain HK trend chunk meta id {chunk_meta_record_id}; decoded ids={decoded_ids}"
        )
    if not contains_key_value(decoded_json, "version", expected_hk_trend_version):
        raise RuntimeError(f"Decoded HK trend .fdp did not contain payload version={expected_hk_trend_version}")
    if '"sampleCount"' not in json.dumps(decoded_json):
        raise RuntimeError("Decoded HK trend .fdp did not contain HkTrendChunkMeta sampleCount")
    return decoded


clean_dir(probe_tmp_dir)
clean_dir(runtime_root)
(runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)
processes = []
dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
stack: HostedPerBandStockStacks | None = None

try:
    security_server_socket.parent.mkdir(parents=True, exist_ok=True)
    processes.append(
        start_managed_process(
            "security_server_sim",
            [str(python_bin), str(root_dir / "scripts/security_server_sim.py"), "--socket-path", str(security_server_socket)],
            security_server_log,
            env=os.environ.copy(),
            cwd=root_dir,
            stale_match_groups=((str(security_server_socket),),),
            stale_match_markers=("security_server_sim.py",),
        )
    )
    wait_for_socket(security_server_socket, 20.0)
    stack = HostedPerBandStockStacks(
        mode_name="hk-alignment",
        root_dir=root_dir,
        bin_dir=bin_dir,
        dictionary_path=dict_path,
        cli_path=find_fprime_cli(root_dir),
        stack_root=stack_root,
        runtime_root=stack_runtime_root,
        expose_sband_surface=True,
        expose_uhf_surface=False,
        preserve_sband_primary=True,
        auto_ports=True,
        command_authority_profile="sband-primary",
    )
    stack.start()
    if stack.sband is None:
        raise RuntimeError("Hosted stack did not expose the S-band surface")
    sband = stack.sband
    active_runtime_root = stack.runtime_root
    obc_log = stack.obc_log
    if not wait_for_log(stack.sband_process_log, "COMM node startup: executable=sband_comm_csp_node", 20):
        raise RuntimeError("Hosted S-band COMM CSP node did not start")
    if not wait_for_log(stack.sband_process_log, f"node={comm_csp_node}", 2):
        raise RuntimeError(f"Hosted S-band COMM CSP node did not use node {comm_csp_node}")
    if not wait_for_log(stack.sband_gateway_log, "gds-connected", 20):
        raise RuntimeError("Ground TTC gateway did not start on the hosted S-band path")

    session = authenticate_service(
        capture_path=stack.captures_root / "sband-southbound-to-gds.bin",
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        scid=sband.scid,
        vcid=sband.vcid,
        frame_size=sband.frame_size,
        security_server_socket=security_server_socket,
        obc_log=obc_log,
        service_id=SERVICE_ID_SBAND,
        ingress_port=0,
        expected_open_pattern=re.escape("Command session opened ingress 0 identity 1 role 1 session ") + r".*replaced 0",
        auth_timeout=10.0,
        establish_timeout=30.0,
    )
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=dictionary_command_opcode(dictionary, f"{command_prefix}.hkTrendProductProducer.HK_TREND_FLUSH"),
        log_waits=[(obc_log, "HK_TREND_PRODUCT_WRITTEN")],
        attempts=6,
        timeout=8.0,
        label="hk-trend-flush",
    )
    if not wait_for_log(obc_log, "FileWritten", 10):
        raise RuntimeError("OBC log did not contain DpWriter FileWritten evidence after HK_TREND_FLUSH")

    dp_files = sorted((active_runtime_root / "data-products").glob("*.fdp"))
    if not dp_files:
        raise RuntimeError("Did not observe official F' .fdp data product files under <runtime-root>/data-products")
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=dictionary_command_opcode(dictionary, f"{command_prefix}.dpCatalog.BUILD_CATALOG"),
        log_waits=[(obc_log, "CatalogBuildComplete")],
        attempts=4,
        timeout=8.0,
        label="dp-build-catalog",
    )
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=dictionary_command_opcode(dictionary, f"{command_prefix}.dpCatalog.START_XMIT_CATALOG"),
        args=struct.pack(">B", 1),
        log_waits=[(obc_log, "SendingProduct")],
        attempts=4,
        timeout=8.0,
        label="dp-start-xmit-catalog",
    )

    dp_files = sorted((active_runtime_root / "data-products").glob("*.fdp"))
    matched_source, received_fdp = find_matching_received_fdp(dp_files, sband.file_storage, 30)
    decoded_fdp_json = decode_received_fdp(received_fdp)

    if (active_runtime_root / "data-products" / "catalog.csv").exists():
        raise RuntimeError("Legacy data-products/catalog.csv was produced unexpectedly")
    if (active_runtime_root / "data-products" / "hk" / "index.csv").exists():
        raise RuntimeError("Fallback hk/index.csv appeared inside the official data-products catalog root")
    if not (active_runtime_root / "data-products" / "DpState.dat").exists():
        raise RuntimeError("DpCatalog did not create DpState.dat under <runtime-root>/data-products")

    print("hk-data-product-alignment-v2-ccsds-sband-fdp-probe: PASS")
    print("formal-verdict=hk-data-product-alignment-v2")
    print(f"obc-binary={obc_binary_name}")
    print(f"command-prefix={command_prefix}")
    print("link-mode=hosted-sband-tcp")
    print(f"comm-node={comm_csp_node}")
    print("framing=space-packet-space-data-link")
    print(f"scid={gds_scid}")
    print(f"vcid={gds_vcid}")
    print(f"frame-size={gds_frame_size}")
    print("dpcatalog-build=PASS")
    print("dpcatalog-xmit-queue=PASS")
    print(f"fdp-received-byte-match=PASS source={matched_source} received={received_fdp}")
    print(f"fdp-decode=PASS version={expected_hk_trend_version} json={decoded_fdp_json}")
    print("fallback-hk-ring-official-catalog=ABSENT")
    print("legacy-catalog-absent=PASS")
    print(f"sband-tcp-endpoint={sband_tcp_host}:{stack.sband_tcp_port}")
    print(f"gds-port={sband.gds_port}")
    print(f"gds-tts-port={sband.gds_tts_port}")
    print(f"runtime-root={active_runtime_root}")
    print(f"gds-file-storage-dir={sband.file_storage}")
    print(f"logs={probe_tmp_dir}")
finally:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)
PY
