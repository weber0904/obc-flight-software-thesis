#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" || true)"
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

FPRIME_GDS_BIN="$(find_local_tool fprime-gds || true)"
if [[ -z "${FPRIME_GDS_BIN}" ]]; then
  echo "fprime-gds not found. Install it in fprime-venv or PATH." >&2
  exit 1
fi

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

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/onboard-state-data-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/onboard-state-data-runtime-hosted}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-${PROBE_TMP_DIR}/gds-downlink}"
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC:-40}"
EXPECTED_HK_TREND_VERSION="${EXPECTED_HK_TREND_VERSION:-6}"

mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_GDS_BIN="${FPRIME_GDS_BIN}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
FPRIME_DP_BIN="${FPRIME_DP_BIN}" \
FPRIME_DP_WRITE_BIN="${FPRIME_DP_WRITE_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC}" \
EXPECTED_HK_TREND_VERSION="${EXPECTED_HK_TREND_VERSION}" \
python3 - <<'PY'
from __future__ import annotations

import json
import hashlib
import os
import pathlib
import re
import shutil
import signal
import struct
import subprocess
import sys
import time
import zlib

BEACON_MAGIC = b"OBC1"
BEACON_VERSION = 2
BEACON_FRAME_SIZE = 108


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
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
observe_timeout_sec = int(require_env("OBSERVE_TIMEOUT_SEC"))
expected_hk_trend_version = int(require_env("EXPECTED_HK_TREND_VERSION"))
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
stack_root = pathlib.Path("/tmp") / f"osd-{probe_tmp_dir.name[-6:]}"
stack_runtime_root = stack_root / "r"
python_bin = pathlib.Path(shutil.which("python3") or "python3")

fdp_decode_log = probe_tmp_dir / "fdp-decode.log"
beacon_capture_bin = probe_tmp_dir / "beacon-capture.bin"
decoded_beacon_json = probe_tmp_dir / "beacon-decoded.json"

os.environ["DICT_PATH"] = str(dict_path)
os.environ["OBC_DATA_PRODUCTS_QUOTA_BYTES"] = "1048576"

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


def start_process(args: list[str], log_path: pathlib.Path, env: dict[str, str] | None = None) -> subprocess.Popen:
    handle = log_path.open("w", encoding="utf-8", buffering=1)
    process = subprocess.Popen(
        args,
        env=env,
        stdin=subprocess.DEVNULL,
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
    )
    process._log_handle = handle  # type: ignore[attr-defined]
    return process


def stop_processes(processes: list[subprocess.Popen]) -> None:
    for process in reversed(processes):
        try:
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass
    for process in reversed(processes):
        if process.poll() is None:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(os.getpgid(process.pid), signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait(timeout=5)
        handle = getattr(process, "_log_handle", None)
        if handle is not None:
            handle.close()


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


def decode_beacon_frame(frame: bytes) -> dict[str, object]:
    if len(frame) != BEACON_FRAME_SIZE:
        raise RuntimeError(f"Beacon frame size mismatch: {len(frame)}")
    observed_crc = struct.unpack_from("<I", frame, len(frame) - 4)[0]
    expected_crc = zlib.crc32(frame[:-4]) & 0xFFFFFFFF
    if observed_crc != expected_crc:
        raise RuntimeError(f"Beacon CRC mismatch: observed=0x{observed_crc:08x} expected=0x{expected_crc:08x}")
    unpacked = struct.unpack("<IHHIIIIIBBHIIIIiiiiiBBBBIIIIIIIII", frame)
    (
        magic,
        version,
        reserved,
        sequence,
        time_base,
        time_context,
        time_seconds,
        time_useconds,
        mode,
        boot_slot,
        reboot_count,
        uptime_sec,
        health_mask,
        fault_mask,
        quality_mask,
        battery_soc_x100,
        battery_voltage_x100,
        battery_current_x100,
        battery_temp_x100,
        adcs_rate_x1e6,
        adcs_mode,
        gps_fix_valid,
        storage_warning_mask,
        storage_degraded_mask,
        comm_pass_remaining_sec,
        comm_total_passes,
        gps_accepted_sentences,
        gps_rejected_sentences,
        csp_tx_packets,
        csp_rx_packets,
        radio_tx_bytes,
        radio_rx_bytes,
        crc,
    ) = unpacked
    if magic != 0x3143424F:
        raise RuntimeError(f"Bad beacon magic: 0x{magic:08x}")
    if version != BEACON_VERSION:
        raise RuntimeError(f"Unsupported beacon version: {version}")
    return {
        "type": "BeaconV1",
        "version": version,
        "reserved": reserved,
        "sequence": sequence,
        "time": {
            "time_base": time_base,
            "context": time_context,
            "seconds": time_seconds,
            "useconds": time_useconds,
        },
        "mode": mode,
        "boot_slot": boot_slot,
        "reboot_count": reboot_count,
        "uptime_sec": uptime_sec,
        "health_mask": health_mask,
        "fault_mask": fault_mask,
        "quality_mask": quality_mask,
        "battery_soc": battery_soc_x100 / 100.0,
        "battery_voltage": battery_voltage_x100 / 100.0,
        "battery_current": battery_current_x100 / 100.0,
        "battery_temp_c": battery_temp_x100 / 100.0,
        "adcs_rate_norm": adcs_rate_x1e6 / 1_000_000.0,
        "adcs_mode": adcs_mode,
        "gps_fix_valid": gps_fix_valid != 0,
        "storage_warning_mask": storage_warning_mask,
        "storage_degraded_mask": storage_degraded_mask,
        "comm_pass_remaining_sec": comm_pass_remaining_sec,
        "comm_total_passes": comm_total_passes,
        "gps_accepted_sentences": gps_accepted_sentences,
        "gps_rejected_sentences": gps_rejected_sentences,
        "csp_tx_packets": csp_tx_packets,
        "csp_rx_packets": csp_rx_packets,
        "radio_tx_bytes": radio_tx_bytes,
        "radio_rx_bytes": radio_rx_bytes,
        "crc": crc,
    }


def read_beacon_frame(peer, timeout_sec: float) -> bytes:
    deadline = time.monotonic() + timeout_sec
    payload = bytearray()
    while time.monotonic() < deadline:
        try:
            chunk = os.read(peer.master_fd, BEACON_FRAME_SIZE - len(payload))
        except BlockingIOError:
            chunk = b""
        if chunk:
            payload.extend(chunk)
            if len(payload) == BEACON_FRAME_SIZE:
                return bytes(payload)
        else:
            time.sleep(0.1)
    raise RuntimeError(f"Timed out waiting for {BEACON_FRAME_SIZE} BeaconV1 bytes")


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
            received_hash = file_sha256(received)
            source = source_by_hash.get(received_hash)
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
    attempts = [
        [
            str(fprime_dp_bin),
            "decode",
            "--bin-file",
            str(received),
            "--dictionary",
            str(dict_path),
            "--output",
            str(output_json),
        ],
    ]
    with fdp_decode_log.open("a", encoding="utf-8") as log:
        for args in attempts:
            log.write("$ " + " ".join(args) + "\n")
            log.flush()
            result = subprocess.run(args, cwd=decode_dir, stdout=log, stderr=subprocess.STDOUT, text=True, check=False)
            if result.returncode != 0:
                continue
            if output_json.is_file():
                return output_json
            candidates = sorted(decode_dir.rglob("*.json"))
            if candidates:
                return candidates[0]
    return None


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
    hk_record_name = str(hk_record.get("name", ""))
    chunk_meta_record_id = chunk_meta_record.get("id")
    decoded_ids = [entry.get("dataId") for entry in decoded_json if isinstance(entry, dict)]
    if hk_record_id not in decoded_ids:
        raise RuntimeError(f"Decoded .fdp did not contain HK trend record id {hk_record_id}; decoded ids={decoded_ids}")
    if chunk_meta_record_id not in decoded_ids:
        raise RuntimeError(
            f"Decoded .fdp did not contain HK trend chunk meta id {chunk_meta_record_id}; decoded ids={decoded_ids}"
        )
    if "HkTrendRecord" not in hk_record_name:
        raise RuntimeError(f"Dictionary HK record name is unexpected: {hk_record_name}")
    if not contains_key_value(decoded_json, "version", expected_hk_trend_version):
        raise RuntimeError(
            f"Decoded HK trend .fdp did not contain payload version={expected_hk_trend_version}"
        )
    if '"sampleCount"' not in json.dumps(decoded_json):
        raise RuntimeError("Decoded HK trend .fdp did not contain HkTrendChunkMeta sampleCount")
    return decoded


clean_dir(probe_tmp_dir)
clean_dir(runtime_root)
(runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)
processes = []
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
        mode_name="onboard-state",
        root_dir=root_dir,
        bin_dir=bin_dir,
        dictionary_path=dict_path,
        cli_path=find_fprime_cli(root_dir),
        stack_root=stack_root,
        runtime_root=stack_runtime_root,
        expose_sband_surface=True,
        expose_uhf_surface=True,
        preserve_sband_primary=True,
        enable_uhf_beacon_side_channel=True,
        auto_ports=True,
        command_authority_profile="sband-primary",
    )
    stack.start()
    if stack.sband is None or stack.uhf_beacon_peer is None:
        raise RuntimeError("Hosted stack did not expose required S-band or beacon surfaces")
    sband = stack.sband
    active_runtime_root = stack.runtime_root
    obc_log = stack.obc_log
    if not wait_for_log(obc_log, "Runtime mode: headless", 30):
        raise RuntimeError("Hosted runtime did not start in headless mode")
    if not wait_for_log(stack.sband_process_log, "node=5", 20):
        raise RuntimeError("Hosted S-band COMM node did not start on the expected port")
    if not wait_for_log(obc_log, "UHF beacon CSP sink: node=6", 20):
        raise RuntimeError("OBC did not enable the UHF beacon CSP side channel")
    if not wait_for_log(obc_log, "GROUND_LINK_UP", 20):
        raise RuntimeError("Hosted ground link did not come up before probe observations")
    if not wait_for_log(obc_log, "BEACON_PACKET_EMITTED", observe_timeout_sec):
        raise RuntimeError("OBC log did not contain BEACON_PACKET_EMITTED")
    beacon_deadline = time.monotonic() + max(40.0, float(observe_timeout_sec))
    first_frame = b""
    first_decoded: dict[str, object] = {}
    while time.monotonic() < beacon_deadline:
        first_frame = read_beacon_frame(stack.uhf_beacon_peer, max(1.0, beacon_deadline - time.monotonic()))
        first_decoded = decode_beacon_frame(first_frame)
        if first_decoded["battery_voltage"] > 0.0 and first_decoded["battery_current"] > 0.0:
            break
    else:
        raise RuntimeError(
            "Captured beacon did not contain populated EPS raw measurements "
            f"(voltage={first_decoded.get('battery_voltage', 0.0)} current={first_decoded.get('battery_current', 0.0)})"
        )
    beacon_capture_bin.write_bytes(first_frame)
    decoded_beacon_json.write_text(json.dumps(first_decoded, indent=2, sort_keys=True), encoding="utf-8")

    dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
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
        opcode=dictionary_command_opcode(dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH"),
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
        opcode=dictionary_command_opcode(dictionary, "OBCApp.dpCatalog.BUILD_CATALOG"),
        log_waits=[(obc_log, "CatalogBuildComplete")],
        attempts=4,
        timeout=8.0,
        label="dp-build-catalog",
    )
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=dictionary_command_opcode(dictionary, "OBCApp.dpCatalog.START_XMIT_CATALOG"),
        args=struct.pack(">B", 1),
        log_waits=[(obc_log, "SendingProduct")],
        attempts=4,
        timeout=8.0,
        label="dp-start-xmit-catalog",
    )
    dp_files = sorted((active_runtime_root / "data-products").glob("*.fdp"))
    matched_source, received_fdp = find_matching_received_fdp(dp_files, sband.file_storage, 20)
    decoded_fdp_json = decode_received_fdp(received_fdp)

    if (active_runtime_root / "data-products" / "catalog.csv").exists():
        raise RuntimeError("Legacy data-products/catalog.csv was produced unexpectedly")
    if list((active_runtime_root / "data-products").glob("beacon-history-*.bin")):
        raise RuntimeError("Legacy BEACON_HISTORY product files were produced unexpectedly")

    state_file = active_runtime_root / "data-products" / "DpState.dat"
    if not state_file.exists():
        raise RuntimeError("DpCatalog did not create DpState.dat under <runtime-root>/data-products")
    data_products_root = active_runtime_root / "data-products"
    data_product_bytes = sum(path.stat().st_size for path in data_products_root.rglob("*") if path.is_file())

    print("onboard-state-data-fdp-parity-v1-hosted-probe: PASS")
    print("formal-verdict=onboard-state-data-fdp-parity-v1")
    print(
        "live-beacon-capture=PASS "
        f"sequence={first_decoded['sequence']} size={len(first_frame)} "
        f"battery_voltage={first_decoded['battery_voltage']} "
        f"battery_current={first_decoded['battery_current']} "
        f"crc=0x{first_decoded['crc']:08x}"
    )
    print(f"hk-official-dp-files=PASS count={len(dp_files)} first={dp_files[0]}")
    print("dpcatalog-build=PASS")
    print("dpcatalog-xmit-queue=PASS")
    print(f"fdp-received-byte-match=PASS source={matched_source} received={received_fdp}")
    print(f"fdp-decode=PASS version={expected_hk_trend_version} json={decoded_fdp_json}")
    print(
        "storage-data-products-root=PASS "
        f"files={sum(1 for path in data_products_root.rglob('*') if path.is_file())} "
        f"bytes={data_product_bytes}"
    )
    print("legacy-catalog-absent=PASS")
    print(f"runtime-root={active_runtime_root}")
    print(f"beacon-capture={beacon_capture_bin}")
    print(f"beacon-decoded={decoded_beacon_json}")
    print(f"logs={probe_tmp_dir}")
finally:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)
PY
