from __future__ import annotations

import hashlib
import hmac
import json
import os
import pathlib
import shutil
import socket
import struct
import sys
import time
from dataclasses import dataclass

COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32
FW_WAIT_NO_WAIT = 1

SBAND_SOURCE_ID = 1
SBAND_KEY_SLOT = 1
SBAND_KEY_BYTES = bytes.fromhex("101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F")
UHF_SOURCE_ID = 2
UHF_KEY_SLOT = 2
UHF_KEY_BYTES = bytes.fromhex("303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F")


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                return
        except OSError:
            time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


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


def wait_log(path: pathlib.Path, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path}")


def wait_log_optional(path: pathlib.Path, fragment: str, timeout: float) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return True
        time.sleep(0.2)
    return False


def dictionary_command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"command {name!r} not found in dictionary")


def inner_command(opcode: int, args: bytes = b"") -> bytes:
    return struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args


def gds_command_packet(opcode: int, args: bytes = b"") -> bytes:
    packet = inner_command(opcode, args)
    return struct.pack(">II", COMMAND_DESCRIPTOR, len(packet)) + packet


def authenticated_envelope(inner: bytes, source_id: int, key_slot: int, key_bytes: bytes, session_id: int, sequence_number: int) -> bytes:
    header = struct.pack(
        ">IBBHIIIHHHH",
        COMMAND_ENVELOPE_V1_MAGIC,
        COMMAND_ENVELOPE_V1_VERSION,
        0,
        COMMAND_ENVELOPE_V1_HEADER_LENGTH,
        source_id,
        session_id,
        sequence_number,
        key_slot,
        len(inner),
        COMMAND_ENVELOPE_V1_MAC_LENGTH,
        0,
    )
    auth_tag = hmac.new(key_bytes, header + inner, hashlib.sha256).digest()
    return gds_command_packet(OBC_COMMAND_ENVELOPE_V1_OPCODE, header + inner + auth_tag)


@dataclass
class AuthProfile:
    source_id: int
    key_slot: int
    key_bytes: bytes


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
PROBE_MODE = require_env("PROBE_MODE")
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = pathlib.Path(require_env("RUNTIME_ROOT"))
OBSERVE_TIMEOUT_SEC = int(require_env("OBSERVE_TIMEOUT_SEC"))

if PROBE_MODE not in {"happy", "ack-loss", "retry-exhausted"}:
    raise RuntimeError("PROBE_MODE must be one of: happy, ack-loss, retry-exhausted")

receiver_ack_timeout_polls = {
    "happy": "0",
    "ack-loss": "1",
    "retry-exhausted": "4",
}[PROBE_MODE]

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from per_band_stock_ground_stacks import GroundPath, HostedPerBandStockStacks, find_fprime_cli  # noqa: E402
from probe_process_utils import install_signal_cleanup  # noqa: E402


class HostedUhfReliableTransferProbe:
    def __init__(self) -> None:
        self.probe_root = PROBE_TMP_DIR
        self.runtime_root = RUNTIME_ROOT
        self.stack_root = self.probe_root / "combined-stack"
        self.stack: HostedPerBandStockStacks | None = None
        self.stack_env_keys = (
            "COMM_RT_OUTPUT_DIR",
            "COMM_RT_ACK_TIMEOUT_POLLS",
            "COMM_NODE_INGRESS_DIAGNOSTICS",
            "COMM_NODE_STRIP_TC_FILL_PATTERN",
        )
        self.saved_stack_env: dict[str, str | None] = {}
        self.dictionary = json.loads(DICT_PATH.read_text(encoding="utf-8"))
        self.sband = None
        self.uhf = None
        self.rt_output_dir = self.probe_root / "rt-output"
        self.obc_log = self.probe_root / "combined-stack" / "logs" / "obc.log"
        self.sband_comm_log = self.probe_root / "combined-stack" / "logs" / "sband-comm.log"
        self.uhf_comm_log = self.probe_root / "combined-stack" / "logs" / "uhf-comm.log"
        self.command_log = self.probe_root / "command-send.log"
        self.opcode_session_open = dictionary_command_opcode(self.dictionary, "OBCApp.commandIngressAuthority.SESSION_OPEN")
        self.opcode_hk_trend_flush = dictionary_command_opcode(self.dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH")
        self.opcode_comm_set_active = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_SET_ACTIVE")
        self.opcode_build_catalog = dictionary_command_opcode(self.dictionary, "OBCApp.dpCatalog.BUILD_CATALOG")
        self.opcode_start_xmit_catalog = dictionary_command_opcode(self.dictionary, "OBCApp.dpCatalog.START_XMIT_CATALOG")
        self.sband_auth = AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES)
        self.uhf_auth = AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES)

    def start(self) -> None:
        clean_dir(self.probe_root)
        clean_dir(self.runtime_root)
        clean_dir(self.rt_output_dir)
        self.push_stack_env()
        try:
            self.stack = HostedPerBandStockStacks(
                mode_name="combined",
                root_dir=ROOT_DIR,
                bin_dir=BIN_DIR,
                dictionary_path=DICT_PATH,
                cli_path=find_fprime_cli(ROOT_DIR),
                stack_root=self.stack_root,
                runtime_root=self.runtime_root,
                expose_sband_surface=True,
                expose_uhf_surface=True,
                preserve_sband_primary=True,
                enable_uhf_beacon_side_channel=False,
                auto_ports=True,
                command_authority_profile="sband-primary",
            )
            self.stack.start()
        finally:
            self.restore_stack_env()
        if self.stack is None or self.stack.sband is None or self.stack.uhf is None:
            raise RuntimeError("maintained combined hosted stack did not expose both S-band and UHF surfaces")
        self.sband = self.stack.sband
        self.uhf = self.stack.uhf
        self.runtime_root = self.stack.runtime_root
        self.obc_log = self.stack.obc_log
        self.sband_comm_log = self.stack.sband_process_log
        self.uhf_comm_log = self.stack.uhf_process_log

    def stop(self) -> None:
        if self.stack is not None:
            self.stack.stop()
            self.stack = None

    def push_stack_env(self) -> None:
        self.saved_stack_env = {key: os.environ.get(key) for key in self.stack_env_keys}
        os.environ["COMM_RT_OUTPUT_DIR"] = str(self.rt_output_dir)
        os.environ["COMM_RT_ACK_TIMEOUT_POLLS"] = receiver_ack_timeout_polls
        os.environ["COMM_NODE_INGRESS_DIAGNOSTICS"] = os.environ.get("COMM_NODE_INGRESS_DIAGNOSTICS", "0")
        os.environ["COMM_NODE_STRIP_TC_FILL_PATTERN"] = os.environ.get("COMM_NODE_STRIP_TC_FILL_PATTERN", "0")

    def restore_stack_env(self) -> None:
        for key, previous in self.saved_stack_env.items():
            if previous is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = previous
        self.saved_stack_env.clear()

    def send_envelope(self, ground: GroundPath, label: str, profile: AuthProfile, session_id: int, sequence_number: int, opcode: int, args: bytes = b"") -> None:
        payload = authenticated_envelope(inner_command(opcode, args), profile.source_id, profile.key_slot, profile.key_bytes, session_id, sequence_number)
        with self.command_log.open("a", encoding="utf-8") as handle:
            handle.write(f"{label}: tts=127.0.0.1:{ground.gds_tts_port} opcode=0x{opcode:x} session={session_id} seq={sequence_number}\n")
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.3)

    def send_until_log(
        self,
        ground: GroundPath,
        label: str,
        profile: AuthProfile,
        session_id: int,
        sequence_number: int,
        opcode: int,
        expected_fragment: str,
        args: bytes = b"",
        attempts: int = 4,
        per_attempt_timeout: float = 8.0,
        resend_with_incremented_sequence: bool = False,
    ) -> int:
        current_sequence = sequence_number
        for _ in range(attempts):
            self.send_envelope(ground, label, profile, session_id, current_sequence, opcode, args)
            if wait_log_optional(self.obc_log, expected_fragment, per_attempt_timeout):
                return current_sequence
            if resend_with_incremented_sequence:
                current_sequence += 1
            time.sleep(0.8)
        raise RuntimeError(f"timed out waiting for {expected_fragment!r}")

    def select_source_files(self) -> list[pathlib.Path]:
        deadline = time.time() + OBSERVE_TIMEOUT_SEC
        while time.time() < deadline:
            files = sorted((self.runtime_root / "data-products").glob("Dp_*.fdp"))
            if files:
                return files[:2]
            time.sleep(0.5)
        raise RuntimeError("no official .fdp files appeared under runtime-root/data-products")

    def wait_for_matching_received_file(self, expected_sources: list[pathlib.Path], timeout: float) -> tuple[pathlib.Path, pathlib.Path, str, int]:
        expected_map = {source.resolve(): source.read_bytes() for source in expected_sources}
        deadline = time.time() + timeout
        while time.time() < deadline:
            for received_path in sorted(self.rt_output_dir.glob("*.fdp")):
                payload = received_path.read_bytes()
                for source_path, expected in expected_map.items():
                    if payload == expected:
                        digest = hashlib.sha256(payload).hexdigest()
                        return pathlib.Path(source_path), received_path, digest, len(payload)
            time.sleep(0.2)
        source_names = ", ".join(path.name for path in expected_sources)
        raise RuntimeError(f"timed out waiting for any byte-matching RT output among [{source_names}]")

    def gds_received_fdp_files(self) -> list[pathlib.Path]:
        if self.sband is None or self.uhf is None:
            return []
        return sorted(self.sband.file_storage.glob("**/*.fdp")) + sorted(self.uhf.file_storage.glob("**/*.fdp"))

    def receiver_log_offsets(self) -> tuple[int, int]:
        sband_offset = self.sband_comm_log.stat().st_size if self.sband_comm_log.exists() else 0
        uhf_offset = self.uhf_comm_log.stat().st_size if self.uhf_comm_log.exists() else 0
        return sband_offset, uhf_offset

    def wait_for_receiver_route(self, sband_offset: int, uhf_offset: int, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            sband_text = read_text_since(self.sband_comm_log, sband_offset)
            if "COMM reliable transfer begin accepted:" in sband_text:
                raise RuntimeError("hosted UHF reliable-transfer proof unexpectedly routed BEGIN to S-band node 5")
            uhf_text = read_text_since(self.uhf_comm_log, uhf_offset)
            if "COMM reliable transfer begin accepted:" in uhf_text:
                return
            time.sleep(0.2)
        raise RuntimeError("timed out waiting for hosted UHF node 6 receiver BEGIN acceptance")

    def wait_for_receiver_complete(self, sband_offset: int, uhf_offset: int, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            sband_text = read_text_since(self.sband_comm_log, sband_offset)
            if "COMM reliable transfer complete:" in sband_text:
                raise RuntimeError("hosted UHF reliable-transfer proof unexpectedly completed on S-band node 5")
            uhf_text = read_text_since(self.uhf_comm_log, uhf_offset)
            if "COMM reliable transfer complete:" in uhf_text:
                return
            time.sleep(0.2)
        raise RuntimeError("timed out waiting for hosted UHF node 6 receiver COMPLETE")

    def run(self) -> list[str]:
        sband_session = 0x73000000
        self.send_until_log(
            self.sband,
            "sband session open",
            self.sband_auth,
            sband_session,
            0,
            self.opcode_session_open,
            f"Command session opened ingress 0 identity 1 role 1 session {sband_session} replaced 0",
        )
        flush_sequence = self.send_until_log(
            self.sband,
            "sband hk flush",
            self.sband_auth,
            sband_session,
            1,
            self.opcode_hk_trend_flush,
            "HK_TREND_PRODUCT_WRITTEN",
            attempts=6,
            resend_with_incremented_sequence=True,
        )
        wait_log(self.obc_log, "FileWritten", 10.0)
        source_files = self.select_source_files()
        switch_sequence = self.send_until_log(
            self.sband,
            "switch to uhf primary",
            self.sband_auth,
            sband_session,
            flush_sequence + 1,
            self.opcode_comm_set_active,
            "Comm primary links command UHF (1) telemetry UHF (1) file UHF (1) reason 1",
            struct.pack(">B", 1),
            attempts=6,
            resend_with_incremented_sequence=True,
        )
        sband_backup_session = 0x73000001
        self.send_until_log(
            self.sband,
            "sband backup session after uhf switch",
            self.sband_auth,
            sband_backup_session,
            0,
            self.opcode_session_open,
            f"Command session opened ingress 0 identity 1 role 2 session {sband_backup_session} replaced 0",
        )
        time.sleep(0.5)
        uhf_session = 0
        for attempt in range(3):
            candidate = 0x74000000 + attempt
            self.send_envelope(self.uhf, f"uhf session open attempt {attempt + 1}", self.uhf_auth, candidate, 0, self.opcode_session_open)
            if wait_log_optional(self.obc_log, f"Command session opened ingress 1 identity 2 role 3 session {candidate} replaced 0", 8.0):
                uhf_session = candidate
                break
            time.sleep(0.5)
        if uhf_session == 0:
            raise RuntimeError("failed to establish hosted UHF primary session-open after bounded retries")

        sband_log_offset, uhf_log_offset = self.receiver_log_offsets()
        self.send_until_log(
            self.uhf,
            "uhf build dp catalog",
            self.uhf_auth,
            uhf_session,
            1,
            self.opcode_build_catalog,
            "CatalogBuildComplete",
        )
        self.send_until_log(
            self.uhf,
            "uhf start xmit catalog",
            self.uhf_auth,
            uhf_session,
            2,
            self.opcode_start_xmit_catalog,
            "COMM_RT_TRANSFER_STARTED",
            struct.pack(">B", FW_WAIT_NO_WAIT),
        )
        self.wait_for_receiver_route(sband_log_offset, uhf_log_offset, 12.0)

        if PROBE_MODE == "retry-exhausted":
            wait_log(self.obc_log, "COMM_RT_RETRY_EXHAUSTED", 20.0)
            time.sleep(1.0)
            if sorted(self.rt_output_dir.glob("*.fdp")):
                raise RuntimeError("retry-exhausted proof unexpectedly promoted a final file")
            if self.gds_received_fdp_files():
                raise RuntimeError("retry-exhausted proof unexpectedly used stock GDS file storage")
            return [
                "comm-uhf-reliable-transfer-hosted-probe: PASS",
                "formal-verdict=uhf-reliable-transfer",
                f"probe-mode={PROBE_MODE}",
                "result=retry-exhausted",
                "transfer-started=PASS",
                "receiver-route=uhf-node-6-only",
                "retry-exhausted-event=PASS",
                "rt-output-final-file=ABSENT",
                "legacy-gds-file-storage=ABSENT",
                "comm-node=6",
                "uhf-path=uhf-primary-after-failover",
                f"switch-sequence={switch_sequence}",
                f"runtime-root={self.runtime_root}",
                f"rt-output-dir={self.rt_output_dir}",
                f"sband-gds-file-storage-dir={self.sband.file_storage}",
                f"uhf-gds-file-storage-dir={self.uhf.file_storage}",
                f"logs={self.probe_root}",
            ]

        matched_source, received_path, received_hash, received_size = self.wait_for_matching_received_file(source_files, 30.0)
        if self.gds_received_fdp_files():
            raise RuntimeError("hosted UHF reliable-transfer proof unexpectedly used stock GDS file storage")
        if PROBE_MODE == "ack-loss":
            wait_log(self.obc_log, "COMM_RT_RESEND", 12.0)
        wait_log(self.obc_log, "COMM_RT_FINAL_RESULT", 12.0)
        self.wait_for_receiver_complete(sband_log_offset, uhf_log_offset, 12.0)
        return [
            "comm-uhf-reliable-transfer-hosted-probe: PASS",
            "formal-verdict=uhf-reliable-transfer",
            f"probe-mode={PROBE_MODE}",
            "result=success",
            "transfer-started=PASS",
            "receiver-route=uhf-node-6-only",
            f"resend-observed={'PASS' if PROBE_MODE == 'ack-loss' else 'N/A'}",
            "legacy-gds-file-storage=ABSENT",
            "comm-node=6",
            "uhf-path=uhf-primary-after-failover",
            f"switch-sequence={switch_sequence}",
            f"matched-source={matched_source}",
            f"received-path={received_path}",
            f"received-size={received_size}",
            f"received-sha256={received_hash}",
            f"runtime-root={self.runtime_root}",
            f"rt-output-dir={self.rt_output_dir}",
            f"sband-gds-file-storage-dir={self.sband.file_storage}",
            f"uhf-gds-file-storage-dir={self.uhf.file_storage}",
            f"logs={self.probe_root}",
        ]


def main() -> int:
    probe = HostedUhfReliableTransferProbe()
    install_signal_cleanup(probe.stop)
    lines: list[str]
    try:
        probe.start()
        lines = probe.run()
        sys.stdout.write("\n".join(lines) + "\n")
        return 0
    finally:
        probe.stop()


if __name__ == "__main__":
    raise SystemExit(main())
