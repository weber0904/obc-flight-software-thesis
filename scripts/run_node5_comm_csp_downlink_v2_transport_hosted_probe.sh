#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

COMM_DOWNLINK_V2_SENDER_BIN="${BIN_DIR}/comm_downlink_v2_sender"
COMM_DOWNLINK_V2_STATUS_PROBE_BIN="${BIN_DIR}/comm_downlink_v2_status_probe"
CSP_ZMQPROXY_BIN="${BIN_DIR}/csp_zmqproxy"
SBAND_COMM_NODE_BIN="${BIN_DIR}/sband_comm_csp_node"
for required_bin in \
  "${COMM_DOWNLINK_V2_SENDER_BIN}" \
  "${COMM_DOWNLINK_V2_STATUS_PROBE_BIN}" \
  "${CSP_ZMQPROXY_BIN}" \
  "${SBAND_COMM_NODE_BIN}"; do
  if [[ ! -x "${required_bin}" ]]; then
    echo "Required binary not found: ${required_bin}" >&2
    exit 1
  fi
done

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/node5-comm-csp-downlink-v2-transport-hosted.XXXXXX")}"
SOURCE_FDP_PATH="${SOURCE_FDP_PATH:-}"
SINK_INITIAL_PAUSE_MS="${SINK_INITIAL_PAUSE_MS:-1000}"
SINK_READ_DELAY_MS="${SINK_READ_DELAY_MS:-0}"
SINK_RECV_BUFFER_BYTES="${SINK_RECV_BUFFER_BYTES:-1024}"
SEND_CHUNK_BYTES="${SEND_CHUNK_BYTES:-4096}"
STATUS_POLL_INTERVAL_SEC="${STATUS_POLL_INTERVAL_SEC:-0.002}"
WAIT_FLUSH_MS="${WAIT_FLUSH_MS:-15000}"
SOURCE_REPEAT_COUNT="${SOURCE_REPEAT_COUNT:-1}"
COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS="${COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS:-10}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
SOURCE_FDP_PATH="${SOURCE_FDP_PATH}" \
SINK_INITIAL_PAUSE_MS="${SINK_INITIAL_PAUSE_MS}" \
SINK_READ_DELAY_MS="${SINK_READ_DELAY_MS}" \
SINK_RECV_BUFFER_BYTES="${SINK_RECV_BUFFER_BYTES}" \
SEND_CHUNK_BYTES="${SEND_CHUNK_BYTES}" \
STATUS_POLL_INTERVAL_SEC="${STATUS_POLL_INTERVAL_SEC}" \
WAIT_FLUSH_MS="${WAIT_FLUSH_MS}" \
SOURCE_REPEAT_COUNT="${SOURCE_REPEAT_COUNT}" \
COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS="${COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS}" \
python3 - <<'PY'
from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shutil
import socket
import subprocess
import threading
import time

root_dir = pathlib.Path(os.environ["ROOT_DIR"])
bin_dir = pathlib.Path(os.environ["BIN_DIR"])
probe_tmp_dir = pathlib.Path(os.environ["PROBE_TMP_DIR"])
source_fdp_override = os.environ.get("SOURCE_FDP_PATH", "").strip()
sink_initial_pause_ms = int(os.environ["SINK_INITIAL_PAUSE_MS"])
sink_read_delay_ms = int(os.environ["SINK_READ_DELAY_MS"])
sink_recv_buffer_bytes = int(os.environ["SINK_RECV_BUFFER_BYTES"])
send_chunk_bytes = int(os.environ["SEND_CHUNK_BYTES"])
status_poll_interval_sec = float(os.environ["STATUS_POLL_INTERVAL_SEC"])
wait_flush_ms = int(os.environ["WAIT_FLUSH_MS"])
source_repeat_count = int(os.environ["SOURCE_REPEAT_COUNT"])
comm_node_downlink_v2_drain_delay_ms = int(os.environ["COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS"])

import sys

sys.path.insert(0, str(root_dir / "scripts"))

from probe_process_utils import cleanup_managed_processes, start_managed_process


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"Refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def file_sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def reserve_tcp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        sock.listen(1)
        return int(sock.getsockname()[1])


def resolve_fixture_fdp() -> pathlib.Path:
    candidates = []
    if source_fdp_override:
        candidates.append(pathlib.Path(source_fdp_override))
    candidates.extend(
        [
            root_dir
            / "evidence/records/payload-target-capture-sanity-v1/artifacts/2026-07-07-target-formal-rerun/probe-root/source-artifacts/deterministic/data-products/Dp_268673025_1783382245_00075615.fdp",
            root_dir
            / "evidence/records/chapter5-integrated-route-closure-v1/artifacts/2026-06-28-formal-rerun/route1/target/external-roots/payload-raw-preview-dual-artifact-v1-target.xw9FPi/case/source-artifacts/vga/data-products/Dp_268673025_1782664108_00550576.fdp",
        ]
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    for candidate in sorted(root_dir.glob("evidence/records/**/source-artifacts/**/data-products/Dp_*.fdp")):
        if candidate.is_file():
            return candidate.resolve()
    raise RuntimeError("Could not locate a committed official .fdp fixture under docs/test-records")


def status_probe_env(csp_sub_port: int, csp_pub_port: int) -> dict[str, str]:
    env = os.environ.copy()
    env["CSP_TRANSPORT"] = "zmqhub"
    env["CSP_HUB_HOST"] = "127.0.0.1"
    env["CSP_HUB_SUB_PORT"] = str(csp_sub_port)
    env["CSP_HUB_PUB_PORT"] = str(csp_pub_port)
    return env


def sample_downlink_status(csp_sub_port: int, csp_pub_port: int) -> dict[str, object]:
    result = subprocess.run(
        [
            str(bin_dir / "comm_downlink_v2_status_probe"),
            "--local-node",
            "7",
            "--target-node",
            "5",
            "--timeout-ms",
            "1000",
            "--interface-name",
            "DLV2TPR",
        ],
        env=status_probe_env(csp_sub_port, csp_pub_port),
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


class StatusCollector:
    def __init__(self, csp_sub_port: int, csp_pub_port: int, log_path: pathlib.Path) -> None:
        self.csp_sub_port = csp_sub_port
        self.csp_pub_port = csp_pub_port
        self.log_path = log_path
        self.samples: list[dict[str, object]] = []
        self.errors: list[str] = []
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, name="node5-downlink-v2-transport-status", daemon=True)

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
                sample = sample_downlink_status(self.csp_sub_port, self.csp_pub_port)
                sample["timestampMonotonic"] = time.monotonic()
                self.samples.append(sample)
            except Exception as exc:  # noqa: BLE001
                self.errors.append(str(exc))
            time.sleep(status_poll_interval_sec)


class SlowSink:
    def __init__(self,
                 host: str,
                 port: int,
                 output_path: pathlib.Path,
                 expected_bytes: int,
                 initial_pause_ms: int,
                 read_delay_ms: int,
                 recv_buffer_bytes: int) -> None:
        self.host = host
        self.port = port
        self.output_path = output_path
        self.expected_bytes = expected_bytes
        self.initial_pause_ms = initial_pause_ms
        self.read_delay_ms = read_delay_ms
        self.recv_buffer_bytes = recv_buffer_bytes
        self.received_bytes = 0
        self._ready = threading.Event()
        self._done = threading.Event()
        self._error: Exception | None = None
        self._thread = threading.Thread(target=self._run, name="node5-downlink-v2-slow-sink", daemon=True)

    def start(self) -> None:
        self._thread.start()

    def wait_ready(self, timeout_sec: float) -> None:
        if not self._ready.wait(timeout_sec):
            raise RuntimeError("Timed out waiting for slow sink to connect")

    def wait_done(self, timeout_sec: float) -> None:
        if not self._done.wait(timeout_sec):
            raise RuntimeError("Timed out waiting for slow sink completion")
        if self._error is not None:
            raise self._error

    def _run(self) -> None:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            if self.recv_buffer_bytes > 0:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, self.recv_buffer_bytes)
            deadline = time.monotonic() + 10.0
            while True:
                try:
                    sock.connect((self.host, self.port))
                    break
                except OSError:
                    if time.monotonic() >= deadline:
                        raise
                    time.sleep(0.1)
            self._ready.set()
            if self.initial_pause_ms > 0:
                time.sleep(self.initial_pause_ms / 1000.0)
            with self.output_path.open("wb") as handle:
                while self.received_bytes < self.expected_bytes:
                    chunk = sock.recv(256)
                    if not chunk:
                        raise RuntimeError(
                            f"slow sink connection closed early after {self.received_bytes} / {self.expected_bytes} bytes"
                        )
                    handle.write(chunk)
                    handle.flush()
                    self.received_bytes += len(chunk)
                    if self.read_delay_ms > 0:
                        time.sleep(self.read_delay_ms / 1000.0)
        except Exception as exc:  # noqa: BLE001
            self._error = exc
        finally:
            sock.close()
            self._done.set()


clean_dir(probe_tmp_dir)
logs_dir = probe_tmp_dir / "logs"
logs_dir.mkdir(parents=True, exist_ok=True)
received_dir = probe_tmp_dir / "received"
received_dir.mkdir(parents=True, exist_ok=True)
source_dir = probe_tmp_dir / "source"
source_dir.mkdir(parents=True, exist_ok=True)
sender_stdout_log = logs_dir / "sender.stdout.json"
sender_stderr_log = logs_dir / "sender.stderr.log"

fixture_fdp = resolve_fixture_fdp()
expanded_source_path = source_dir / fixture_fdp.name
fixture_bytes = fixture_fdp.read_bytes()
if source_repeat_count <= 0:
    raise RuntimeError("SOURCE_REPEAT_COUNT must be > 0")
with expanded_source_path.open("wb") as handle:
    for _ in range(source_repeat_count):
        handle.write(fixture_bytes)
source_size = expanded_source_path.stat().st_size
source_hash = file_sha256(expanded_source_path)
received_path = received_dir / expanded_source_path.name

csp_sub_port = reserve_tcp_port()
csp_pub_port = reserve_tcp_port()
southbound_port = reserve_tcp_port()

runtime_env = os.environ.copy()
runtime_env["CSP_TRANSPORT"] = "zmqhub"
runtime_env["CSP_HUB_HOST"] = "127.0.0.1"
runtime_env["CSP_HUB_SUB_PORT"] = str(csp_sub_port)
runtime_env["CSP_HUB_PUB_PORT"] = str(csp_pub_port)

processes = []
collector: StatusCollector | None = None
sink: SlowSink | None = None

try:
    processes.append(
        start_managed_process(
            "csp_zmqproxy",
            [
                str(bin_dir / "csp_zmqproxy"),
                "-s",
                f"tcp://127.0.0.1:{csp_sub_port}",
                "-p",
                f"tcp://127.0.0.1:{csp_pub_port}",
            ],
            logs_dir / "csp-zmqproxy.log",
            env=runtime_env.copy(),
            cwd=root_dir,
            stale_match_groups=((f"tcp://127.0.0.1:{csp_sub_port}", f"tcp://127.0.0.1:{csp_pub_port}"),),
            stale_match_markers=("csp_zmqproxy",),
        )
    )
    processes.append(
        start_managed_process(
            "sband_comm_csp_node",
            [
                str(bin_dir / "sband_comm_csp_node"),
                "--tcp-listen-host",
                "127.0.0.1",
                "--tcp-listen-port",
                str(southbound_port),
                "--node-id",
                "5",
            ],
            logs_dir / "sband-comm.log",
            env={**runtime_env.copy(), "COMM_NODE_DOWNLINK_V2_DRAIN_DELAY_MS": str(comm_node_downlink_v2_drain_delay_ms)},
            cwd=root_dir,
            stale_match_groups=((f"--tcp-listen-port {southbound_port}", "--node-id 5"),),
            stale_match_markers=("sband_comm_csp_node",),
        )
    )

    deadline = time.monotonic() + 12.0
    initial_status = None
    while time.monotonic() < deadline:
        try:
            initial_status = sample_downlink_status(csp_sub_port, csp_pub_port)
            break
        except Exception:
            time.sleep(0.2)
    if initial_status is None:
        raise RuntimeError("Timed out waiting for node-5 DOWNLINK_STATUS_V2 availability")

    sink = SlowSink(
        "127.0.0.1",
        southbound_port,
        received_path,
        source_size,
        sink_initial_pause_ms,
        sink_read_delay_ms,
        sink_recv_buffer_bytes,
    )
    sink.start()
    sink.wait_ready(12.0)

    collector = StatusCollector(csp_sub_port, csp_pub_port, probe_tmp_dir / "status-samples.json")
    collector.start()
    try:
        sender = subprocess.run(
            [
                str(bin_dir / "comm_downlink_v2_sender"),
                "--local-node",
                "1",
                "--target-node",
                "5",
                "--source-file",
                str(expanded_source_path),
                "--send-chunk-bytes",
                str(send_chunk_bytes),
                "--wait-flush-ms",
                str(wait_flush_ms),
                "--wait-flush-poll-ms",
                "5",
                "--interface-name",
                "DLV2SND",
            ],
            env=runtime_env.copy(),
            cwd=root_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    finally:
        collector.stop()

    if sender.returncode != 0:
        sender_stdout_log.write_text(sender.stdout, encoding="utf-8")
        sender_stderr_log.write_text(sender.stderr, encoding="utf-8")
        raise RuntimeError(
            "comm_downlink_v2_sender failed "
            f"returncode={sender.returncode} stderr={sender.stderr.strip()}"
        )
    sender_stdout_log.write_text(sender.stdout, encoding="utf-8")
    sender_stderr_log.write_text(sender.stderr, encoding="utf-8")
    sender_stats = json.loads(sender.stdout.strip())

    sink.wait_done((wait_flush_ms / 1000.0) + 10.0)

    received_hash = file_sha256(received_path)
    if sink.received_bytes != source_size:
        raise RuntimeError(
            f"slow sink byte count mismatch expected={source_size} actual={sink.received_bytes}"
        )
    if received_hash != source_hash:
        raise RuntimeError(
            f"slow sink sha256 mismatch expected={source_hash} actual={received_hash}"
        )

    final_status = sample_downlink_status(csp_sub_port, csp_pub_port)
    if not collector.observed_gap() and int(sender_stats["maxGapBytes"]) <= 0:
        raise RuntimeError(
            "Did not observe acceptedBytes > flushedBytes during transport-only hosted probe; "
            f"statusMaxGap={collector.max_gap()} senderMaxGap={int(sender_stats['maxGapBytes'])} "
            f"maxQueueSlots={collector.max_queue_slots()} samples={len(collector.samples)}"
        )

    print("node5-comm-csp-downlink-v2-transport-hosted-probe: PASS")
    print("formal-verdict=node5-comm-csp-downlink-v2 transport-hosted")
    print(f"fixture-fdp={fixture_fdp}")
    print(f"expanded-source={expanded_source_path}")
    print(f"received-fdp={received_path}")
    print(f"fdp-byte-match=PASS size={source_size} sha256={source_hash} repeat-count={source_repeat_count}")
    print(
        "downlink-v2-status=PASS "
        f"accepted={int(final_status['acceptedBytes'])} "
        f"flushed={int(final_status['flushedBytes'])} "
        f"dropped={int(final_status['droppedCommittedBytes'])}"
    )
    print(
        "pipeline-gap=PASS "
        f"status-max-gap-bytes={collector.max_gap()} "
        f"status-max-queued-slots={collector.max_queue_slots()} "
        f"sender-max-gap-bytes={int(sender_stats['maxGapBytes'])}"
    )
    print(f"logs={probe_tmp_dir}")
finally:
    cleanup_managed_processes(processes, timeout_sec=5.0)
PY
