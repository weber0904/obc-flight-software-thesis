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

PROBE_MODE="${PROBE_MODE:-happy}"
case "${PROBE_MODE}" in
  happy|ack-loss|retry-exhausted) ;;
  *)
    echo "PROBE_MODE must be happy, ack-loss, or retry-exhausted." >&2
    exit 2
    ;;
esac

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/comm-reliable-transfer-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/comm-reliable-transfer-runtime}"
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR:-${PROBE_TMP_DIR}/gds-downlink}"
COMM_RT_OUTPUT_DIR="${COMM_RT_OUTPUT_DIR:-${PROBE_TMP_DIR}/rt-output}"
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC:-40}"

mkdir -p "${PROBE_TMP_DIR}" "${GDS_FILE_STORAGE_DIR}" "${COMM_RT_OUTPUT_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
PROBE_MODE="${PROBE_MODE}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
GDS_FILE_STORAGE_DIR="${GDS_FILE_STORAGE_DIR}" \
COMM_RT_OUTPUT_DIR="${COMM_RT_OUTPUT_DIR}" \
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC}" \
python3 - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import struct
import sys
import time

FW_WAIT_NO_WAIT = 1


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


root_dir = pathlib.Path(require_env("ROOT_DIR"))
bin_dir = pathlib.Path(require_env("BIN_DIR"))
dict_path = pathlib.Path(require_env("DICT_PATH"))
probe_mode = require_env("PROBE_MODE")
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
gds_file_storage_dir = pathlib.Path(require_env("GDS_FILE_STORAGE_DIR"))
rt_output_dir = pathlib.Path(require_env("COMM_RT_OUTPUT_DIR"))
observe_timeout_sec = int(require_env("OBSERVE_TIMEOUT_SEC"))
active_runtime_root = runtime_root
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
stack_root = pathlib.Path("/tmp") / f"crt-{probe_tmp_dir.name[-6:]}"
stack_runtime_root = stack_root / "r"
python_bin = shutil.which("python3") or "python3"

sys.path.insert(0, str(root_dir / "scripts"))

from hosted_secure_command_helpers import authenticate_service, send_secure_command_with_retry, wait_for_socket
from per_band_stock_ground_stacks import HostedPerBandStockStacks, find_fprime_cli
from probe_process_utils import (  # noqa: E402
    ManagedProcess,
    cleanup_managed_processes,
    install_signal_cleanup,
    start_managed_process,
)
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


def collect_legacy_aliases(root: pathlib.Path) -> set[str]:
    aliases: set[str] = set()
    for alias in root.glob(".adm-*"):
        if alias.exists() or alias.is_symlink():
            aliases.add(alias.name)
    for alias in root.glob(".stg-*"):
        if alias.exists() or alias.is_symlink():
            aliases.add(alias.name)
    for alias in (root / ".sequence-staging", root / ".sequence-admitted"):
        if alias.exists() or alias.is_symlink():
            aliases.add(alias.name)
    return aliases


def ensure_no_new_legacy_aliases(root: pathlib.Path, baseline: set[str]) -> None:
    current = collect_legacy_aliases(root)
    created = sorted(current - baseline)
    if created:
        raise RuntimeError("legacy checkout-level aliases should not be created: " + ", ".join(created))


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
    raise RuntimeError(f"command {name!r} not found in dictionary")


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def select_source_files() -> list[pathlib.Path]:
    deadline = time.monotonic() + observe_timeout_sec
    while time.monotonic() < deadline:
        files = sorted((active_runtime_root / "data-products").glob("Dp_*.fdp"))
        if files:
            return files[:2]
        time.sleep(0.5)
    raise RuntimeError("no official .fdp files appeared under runtime_root/data-products")


def wait_for_matching_received_file(expected_sources: list[pathlib.Path], timeout_sec: float) -> tuple[pathlib.Path, pathlib.Path, str, int]:
    expected_bytes = {path: path.read_bytes() for path in expected_sources}
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        for received_path in sorted(rt_output_dir.glob("*.fdp")):
            payload = received_path.read_bytes()
            for source_path, source_bytes in expected_bytes.items():
                if payload == source_bytes:
                    return source_path, received_path, sha256_file(received_path), received_path.stat().st_size
        time.sleep(0.2)
    names = ", ".join(path.name for path in expected_sources)
    raise RuntimeError(f"timed out waiting for any byte-matching RT output among [{names}]")


def gds_received_fdp_files(received_root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(received_root.rglob("*.fdp"))


ack_timeout_polls = {
    "happy": "0",
    "ack-loss": "1",
    "retry-exhausted": "4",
}[probe_mode]

clean_dir(probe_tmp_dir)
clean_dir(runtime_root)
clean_dir(gds_file_storage_dir)
clean_dir(rt_output_dir)
legacy_alias_baseline = collect_legacy_aliases(root_dir)

dictionary = json.loads(dict_path.read_text(encoding="utf-8"))
opcode_build_catalog = dictionary_command_opcode(dictionary, "OBCApp.dpCatalog.BUILD_CATALOG")
opcode_start_xmit_catalog = dictionary_command_opcode(dictionary, "OBCApp.dpCatalog.START_XMIT_CATALOG")
opcode_hk_trend_flush = dictionary_command_opcode(dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH")

os.environ["DICT_PATH"] = str(dict_path)
os.environ["COMM_RT_OUTPUT_DIR"] = str(rt_output_dir)
os.environ["COMM_RT_ACK_TIMEOUT_POLLS"] = ack_timeout_polls
os.environ["OBC_DATA_PRODUCTS_QUOTA_BYTES"] = "1048576"

processes: list[ManagedProcess] = []
stack: HostedPerBandStockStacks | None = None


def cleanup() -> None:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)
    ensure_no_new_legacy_aliases(root_dir, legacy_alias_baseline)


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
        mode_name="comm-rt",
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
        raise RuntimeError("hosted S-band COMM node did not start")
    if not wait_for_log(stack.sband_process_log, "node=5", 2):
        raise RuntimeError("hosted S-band COMM node did not use node 5")
    if not wait_for_log(stack.sband_gateway_log, "gds-connected", 20):
        raise RuntimeError("ground TTC gateway did not start on the hosted S-band path")
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
        expected_open_pattern=r"Command session opened ingress 0 identity 1 role 1 session .*replaced 0",
        auth_timeout=10.0,
        establish_timeout=30.0,
    )
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=opcode_hk_trend_flush,
        log_waits=[(obc_log, "HK_TREND_PRODUCT_WRITTEN")],
        attempts=6,
        timeout=8.0,
        label="hk-trend-flush",
    )
    if not wait_for_log(obc_log, "FileWritten", 10):
        raise RuntimeError("OBC log did not contain DpWriter FileWritten evidence after HK_TREND_FLUSH")

    source_files = select_source_files()
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=opcode_build_catalog,
        log_waits=[(obc_log, "CatalogBuildComplete")],
        attempts=4,
        timeout=8.0,
        label="dp-build-catalog",
    )
    send_secure_command_with_retry(
        raw_command_log=sband.raw_command_log,
        gds_tts_port=sband.gds_tts_port,
        session=session,
        opcode=opcode_start_xmit_catalog,
        log_waits=[(obc_log, "SendingProduct")],
        args=struct.pack(">B", FW_WAIT_NO_WAIT),
        attempts=4,
        timeout=8.0,
        label="dp-start-xmit-catalog",
    )

    if not wait_for_log(obc_log, "COMM_RT_TRANSFER_STARTED", 12):
        raise RuntimeError("CommController did not emit COMM_RT_TRANSFER_STARTED")

    if probe_mode == "retry-exhausted":
        if not wait_for_log(obc_log, "COMM_RT_RETRY_EXHAUSTED", 20):
            raise RuntimeError("reliable-transfer retry-exhausted proof did not emit COMM_RT_RETRY_EXHAUSTED")
        time.sleep(1.0)
        received = sorted(rt_output_dir.glob("*.fdp"))
        if received:
            raise RuntimeError(f"retry-exhausted proof unexpectedly promoted a final file: {received[0]}")
        if gds_received_fdp_files(sband.file_storage):
            raise RuntimeError("retry-exhausted proof unexpectedly used stock GDS file storage")
        print("comm-reliable-transfer-hosted-probe: PASS")
        print("formal-verdict=reliable-transfer")
        print(f"probe-mode={probe_mode}")
        print("result=retry-exhausted")
        print("transfer-started=PASS")
        print("retry-exhausted-event=PASS")
        print("rt-output-final-file=ABSENT")
        print("legacy-gds-file-storage=ABSENT")
        print("comm-node=5")
        print(f"runtime-root={active_runtime_root}")
        print(f"rt-output-dir={rt_output_dir}")
        print(f"gds-file-storage-dir={gds_file_storage_dir}")
        print(f"logs={probe_tmp_dir}")
    else:
        matched_source, received_path, received_hash, received_size = wait_for_matching_received_file(source_files, 30)
        if gds_received_fdp_files(sband.file_storage):
            raise RuntimeError("hosted reliable-transfer proof unexpectedly used stock GDS file storage")
        if probe_mode == "ack-loss" and not wait_for_log(obc_log, "COMM_RT_RESEND", 12):
            raise RuntimeError("ack-loss proof did not emit COMM_RT_RESEND")
        if not wait_for_log(obc_log, "COMM_RT_FINAL_RESULT", 12):
            raise RuntimeError("successful reliable-transfer proof did not emit COMM_RT_FINAL_RESULT")
        print("comm-reliable-transfer-hosted-probe: PASS")
        print("formal-verdict=reliable-transfer")
        print(f"probe-mode={probe_mode}")
        print("result=success")
        print("transfer-started=PASS")
        print(f"resend-observed={'PASS' if probe_mode == 'ack-loss' else 'N/A'}")
        print("legacy-gds-file-storage=ABSENT")
        print(f"matched-source={matched_source}")
        print(f"received-path={received_path}")
        print(f"received-size={received_size}")
        print(f"received-sha256={received_hash}")
        print("comm-node=5")
        print(f"runtime-root={active_runtime_root}")
        print(f"rt-output-dir={rt_output_dir}")
        print(f"gds-file-storage-dir={gds_file_storage_dir}")
        print(f"logs={probe_tmp_dir}")
finally:
    cleanup()
PY
