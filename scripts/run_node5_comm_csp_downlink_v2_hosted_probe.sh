#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
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

FPRIME_DP_BIN="$(find_local_tool fprime-dp || true)"
FPRIME_DP_WRITE_BIN="$(find_local_tool fprime-dp-write || true)"
if [[ -z "${FPRIME_DP_BIN}" && -z "${FPRIME_DP_WRITE_BIN}" ]]; then
  echo "Neither fprime-dp nor fprime-dp-write was found. Install one in fprime-venv or PATH." >&2
  exit 1
fi

COMM_DOWNLINK_V2_STATUS_PROBE_BIN="${BIN_DIR}/comm_downlink_v2_status_probe"
if [[ ! -x "${COMM_DOWNLINK_V2_STATUS_PROBE_BIN}" ]]; then
  echo "comm_downlink_v2_status_probe not found at ${COMM_DOWNLINK_V2_STATUS_PROBE_BIN}. Build native tools first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/node5-comm-csp-downlink-v2-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/n5v2rt}"
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC:-120}"
STATUS_POLL_INTERVAL_SEC="${STATUS_POLL_INTERVAL_SEC:-0.02}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_DP_BIN="${FPRIME_DP_BIN}" \
FPRIME_DP_WRITE_BIN="${FPRIME_DP_WRITE_BIN}" \
COMM_DOWNLINK_V2_STATUS_PROBE_BIN="${COMM_DOWNLINK_V2_STATUS_PROBE_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
OBSERVE_TIMEOUT_SEC="${OBSERVE_TIMEOUT_SEC}" \
STATUS_POLL_INTERVAL_SEC="${STATUS_POLL_INTERVAL_SEC}" \
python3 - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import struct
import subprocess
import threading
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
status_probe_bin = pathlib.Path(require_env("COMM_DOWNLINK_V2_STATUS_PROBE_BIN"))
probe_tmp_dir = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = pathlib.Path(require_env("RUNTIME_ROOT"))
observe_timeout_sec = float(require_env("OBSERVE_TIMEOUT_SEC"))
status_poll_interval_sec = float(require_env("STATUS_POLL_INTERVAL_SEC"))
security_server_socket = probe_tmp_dir / "security-server" / "secure-server.sock"
security_server_log = probe_tmp_dir / "security-server.log"
fdp_decode_log = probe_tmp_dir / "fdp-decode.log"
stack_root = pathlib.Path("/tmp") / f"node5v2-{probe_tmp_dir.name[-6:]}"
file_downlink_path_max = 100

os.environ["DICT_PATH"] = str(dict_path)
os.environ["OBC_DATA_PRODUCTS_QUOTA_BYTES"] = "1048576"

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


def wait_for_log(path: pathlib.Path, fragment: str, timeout_sec: float) -> None:
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        if fragment in read_text(path):
            return
        time.sleep(0.2)
    raise RuntimeError(f"Timed out waiting for {fragment!r} in {path}")


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
    return decoded


def resolve_fixture_fdp() -> pathlib.Path:
    candidates = [
        root_dir
        / "docs/test-records/payload-target-capture-sanity-v1/artifacts/2026-07-07-target-formal-rerun/probe-root/source-artifacts/deterministic/data-products/Dp_268673025_1783382245_00075615.fdp",
        root_dir
        / "docs/test-records/chapter5-integrated-route-closure-v1/artifacts/2026-06-28-formal-rerun/route1/target/external-roots/payload-raw-preview-dual-artifact-v1-target.xw9FPi/case/source-artifacts/vga/data-products/Dp_268673025_1782664108_00550576.fdp",
    ]
    override = os.environ.get("SOURCE_FDP_PATH", "").strip()
    if override:
        candidates.insert(0, pathlib.Path(override))
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    for candidate in sorted(root_dir.glob("docs/test-records/**/source-artifacts/**/data-products/Dp_*.fdp")):
        if candidate.is_file():
            return candidate.resolve()
    raise RuntimeError("Could not locate a committed official .fdp fixture under docs/test-records")


def find_matching_received_fdp(source_path: pathlib.Path, received_root: pathlib.Path, timeout_sec: float) -> pathlib.Path:
    source_hash = file_sha256(source_path)
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        for received in sorted(received_root.rglob("*.fdp")):
            if received.is_file() and file_sha256(received) == source_hash:
                return received
        time.sleep(0.2)
    received_files = ", ".join(str(path) for path in sorted(received_root.rglob("*.fdp")))
    raise RuntimeError(f"Did not find a byte-matching GDS-received .fdp file; received=[{received_files}]")


def validate_runtime_root_path_budget(active_runtime_root: pathlib.Path, source_name: str) -> None:
    sample_path = active_runtime_root / "data-products" / source_name
    if len(str(sample_path)) >= file_downlink_path_max:
        raise RuntimeError(
            f"source path would exceed FileDownlink's {file_downlink_path_max}-byte limit: {sample_path}"
        )


def status_probe_env(stack: HostedPerBandStockStacks) -> dict[str, str]:
    env = os.environ.copy()
    env["CSP_TRANSPORT"] = "zmqhub"
    env["CSP_HUB_HOST"] = "127.0.0.1"
    env["CSP_HUB_SUB_PORT"] = str(stack.csp_sub_port)
    env["CSP_HUB_PUB_PORT"] = str(stack.csp_pub_port)
    return env


def sample_downlink_status(stack: HostedPerBandStockStacks) -> dict[str, object]:
    result = subprocess.run(
        [
            str(status_probe_bin),
            "--local-node",
            "7",
            "--target-node",
            "5",
            "--timeout-ms",
            "2000",
            "--interface-name",
            "DLV2PRB",
        ],
        env=status_probe_env(stack),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "comm_downlink_v2_status_probe failed "
            f"returncode={result.returncode} stderr={result.stderr.strip()}"
    )
    return json.loads(result.stdout)


def wait_for_downlink_status(stack: HostedPerBandStockStacks, timeout_sec: float) -> dict[str, object]:
    deadline = time.monotonic() + timeout_sec
    last_error = "status probe did not run"
    while time.monotonic() < deadline:
        try:
            return sample_downlink_status(stack)
        except Exception as exc:  # noqa: BLE001
            last_error = str(exc)
            time.sleep(0.25)
    raise RuntimeError(f"Timed out waiting for node-5 DOWNLINK_STATUS_V2 availability: {last_error}")


class StatusCollector:
    def __init__(self, stack: HostedPerBandStockStacks, log_path: pathlib.Path) -> None:
        self.stack = stack
        self.log_path = log_path
        self.samples: list[dict[str, object]] = []
        self.errors: list[str] = []
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, name="node5-downlink-v2-status", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=5.0)
        with self.log_path.open("w", encoding="utf-8") as handle:
            json.dump({"samples": self.samples, "errors": self.errors}, handle, indent=2, sort_keys=True)

    def observed_gap(self) -> bool:
        return any(
            int(sample["acceptedBytes"]) > int(sample["flushedBytes"])
            for sample in self.samples
            if "acceptedBytes" in sample and "flushedBytes" in sample
        )

    def max_gap(self) -> int:
        return max(
            [int(sample["acceptedBytes"]) - int(sample["flushedBytes"]) for sample in self.samples] or [0]
        )

    def max_queue_slots(self) -> int:
        return max([int(sample["drainQueuedSlots"]) for sample in self.samples] or [0])

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                sample = sample_downlink_status(self.stack)
                sample["timestampMonotonic"] = time.monotonic()
                self.samples.append(sample)
            except Exception as exc:  # noqa: BLE001
                self.errors.append(str(exc))
            time.sleep(status_poll_interval_sec)


clean_dir(probe_tmp_dir)
clean_dir(runtime_root)
processes = []
stack: HostedPerBandStockStacks | None = None

try:
    fixture_fdp = resolve_fixture_fdp()
    security_server_socket.parent.mkdir(parents=True, exist_ok=True)
    processes.append(
        start_managed_process(
            "security_server_sim",
            [sys.executable, str(root_dir / "scripts/security_server_sim.py"), "--socket-path", str(security_server_socket)],
            security_server_log,
            env=os.environ.copy(),
            cwd=root_dir,
            stale_match_groups=((str(security_server_socket),),),
            stale_match_markers=("security_server_sim.py",),
        )
    )
    wait_for_socket(security_server_socket, 20.0)

    stack = HostedPerBandStockStacks(
        mode_name="node5-downlink-v2",
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
        raise RuntimeError("Hosted stack did not expose the S-band surface")
    if stack.uhf is None:
        raise RuntimeError("Hosted stack did not expose the UHF/node-6 surface")
    sband = stack.sband
    active_runtime_root = stack.runtime_root
    obc_log = stack.obc_log
    validate_runtime_root_path_budget(active_runtime_root, fixture_fdp.name)
    wait_for_log(obc_log, "Runtime mode: headless", 30.0)
    wait_for_log(stack.sband_process_log, "node=5", 20.0)
    wait_for_log(stack.uhf_process_log, "node=6", 20.0)
    wait_for_log(obc_log, "GROUND_LINK_UP", 20.0)

    initial_status = wait_for_downlink_status(stack, 12.0)
    if not bool(initial_status.get("connected")):
        raise RuntimeError(f"node-5 DOWNLINK_STATUS_V2 reported disconnected status: {initial_status}")

    data_products_root = active_runtime_root / "data-products"
    data_products_root.mkdir(parents=True, exist_ok=True)
    staged_fixture = data_products_root / fixture_fdp.name
    shutil.copy2(fixture_fdp, staged_fixture)

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
        expected_open_pattern=r"Command session opened ingress 0 identity 1 role 1 session .*replaced 0",
        auth_timeout=10.0,
        establish_timeout=30.0,
    )

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

    collector = StatusCollector(stack, probe_tmp_dir / "status-samples.json")
    collector.start()
    try:
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
        received_fdp = find_matching_received_fdp(staged_fixture, sband.file_storage, observe_timeout_sec)
        time.sleep(0.3)
    finally:
        collector.stop()

    decoded_json = decode_received_fdp(received_fdp)
    final_status = wait_for_downlink_status(stack, 5.0)

    print("node5-comm-csp-downlink-v2-hosted-probe: PASS")
    print("formal-verdict=node5-comm-csp-downlink-v2 hosted")
    print(f"fixture-fdp={fixture_fdp}")
    print(f"staged-fdp={staged_fixture}")
    print(f"received-fdp={received_fdp}")
    print(f"decoded-json={decoded_json}")
    print(
        "downlink-v2-status=PASS "
        f"connected={1 if bool(initial_status.get('connected')) else 0} "
        f"accepted={int(final_status['acceptedBytes'])} "
        f"flushed={int(final_status['flushedBytes'])} "
        f"dropped={int(final_status['droppedCommittedBytes'])}"
    )
    print(
        "pipeline-gap="
        + ("PASS" if collector.observed_gap() else "UNOBSERVED")
        + " "
        + f"max-gap-bytes={collector.max_gap()} "
        + f"max-queued-slots={collector.max_queue_slots()} "
        + f"samples={len(collector.samples)}"
    )
    print(
        "fdp-byte-match=PASS "
        f"source={staged_fixture} "
        f"received={received_fdp} "
        f"size={staged_fixture.stat().st_size} "
        f"sha256={file_sha256(staged_fixture)}"
    )
    print(f"logs={probe_tmp_dir}")
    print(f"runtime-root={active_runtime_root}")
finally:
    if stack is not None:
        stack.stop()
    cleanup_managed_processes(processes, timeout_sec=5.0)
PY
