#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
import socket
import struct
import subprocess
import sys
import time
from dataclasses import dataclass

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.data_types.file_data import DataPacketData, EndPacketData, StartPacketData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.encoders.file_encoder import FileEncoder
from fprime_gds.common.files.helpers import CFDPChecksum
from fprime_gds.common.models.dictionaries import Dictionaries
from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
    SERVICE_ID_SBAND,
    SERVICE_ID_UHF,
    SECURE_LINK_HANDSHAKE_MAGIC,
    build_inner_command,
    build_req_auth_packet,
    build_response_packet,
    build_secure_command_v2_packet,
    compute_auth_response,
    load_handshake_messages,
    request_session_key,
    send_tts_raw_packet,
    wait_for_handshake_message,
)


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_bytes().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def file_size(path: pathlib.Path) -> int:
    try:
        return path.stat().st_size
    except FileNotFoundError:
        return 0


def read_text_since(path: pathlib.Path, offset: int) -> str:
    try:
        payload = path.read_bytes()
    except FileNotFoundError:
        return ""
    bounded_offset = max(0, min(offset, len(payload)))
    return payload[bounded_offset:].replace(b"\0", b"\n").decode("utf-8", errors="replace")


def wait_text_since(path: pathlib.Path, offset: int, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text_since(path, offset)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path} from offset {offset}")


def wait_regex_since(path: pathlib.Path, offset: int, pattern: str, timeout: float) -> str:
    compiled = re.compile(pattern)
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text_since(path, offset)
        if compiled.search(text) is not None:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for regex {pattern!r} in {path} from offset {offset}")


def assert_no_text_since(path: pathlib.Path, offset: int, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text_since(path, offset):
            raise RuntimeError(f"unexpected {fragment!r} observed in {path}")
        time.sleep(0.2)


def wait_for_path(path: pathlib.Path, timeout: float) -> pathlib.Path:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if path.exists():
            return path
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for path {path}")


def wait_for_glob(root: pathlib.Path, pattern: str, timeout: float) -> pathlib.Path:
    deadline = time.time() + timeout
    while time.time() < deadline:
        matches = sorted(root.glob(pattern))
        if matches:
            return matches[0]
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for glob {pattern!r} under {root}")


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def dictionary_command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"command {name!r} not found in dictionary")


@dataclass
class SecureSession:
    service_id: int
    session_key: bytes
    next_sequence: int = 1

    def accept_sequence(self) -> int:
        sequence = self.next_sequence
        self.next_sequence += 1
        return sequence


@dataclass(frozen=True)
class SequenceArtifact:
    name: str
    source: pathlib.Path
    binary: pathlib.Path
    destination: str


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = pathlib.Path(require_env("RUNTIME_ROOT"))
PYTHON_BIN = require_env("PYTHON_BIN")
SEQGEN_BIN = pathlib.Path(require_env("SEQGEN_BIN"))

SECURITY_SERVER_SOCKET = PROBE_TMP_DIR / "security-server" / "secure-server.sock"
SECURITY_SERVER_LOG = PROBE_TMP_DIR / "security-server.log"
HOSTED_TIMEOUT_SEC = 30.0
AUTH_TIMEOUT_SEC = 10.0
AUTH_CHALLENGE_WAIT_SEC = 8.0
AUTH_RESPONSE_WAIT_SEC = 8.0
AUTH_ESTABLISH_TIMEOUT_SEC = 30.0
AUTH_RESPONSE_RETRY_LIMIT = 3
AUTH_RETRY_BACKOFF_SEC = 0.5
UPLOAD_TIMEOUT_SEC = 25.0
UHF_BACKUP_PASS_SECONDS = 30
COMM_BAND_UHF = 1

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from per_band_stock_ground_stacks import GroundPath, HostedPerBandStockStacks, find_fprime_cli  # noqa: E402
from probe_process_utils import cleanup_managed_processes, start_managed_process  # noqa: E402


class UplinkAuthorityAndKeyHardeningHostedProbe:
    def __init__(self) -> None:
        self.probe_root = PROBE_TMP_DIR
        self.stack_root = self.probe_root / "combined-stack"
        self.runtime_root = RUNTIME_ROOT
        self.dictionary = json.loads(DICT_PATH.read_text(encoding="utf-8"))
        self.command_dictionaries = Dictionaries()
        self.command_dictionaries.load_dictionaries(str(DICT_PATH), None, None)
        self.command_encoder = CmdEncoder()
        self.processes = []
        self.stack: HostedPerBandStockStacks | None = None
        self.sband: GroundPath | None = None
        self.uhf: GroundPath | None = None
        self.obc_log = self.stack_root / "logs" / "obc.log"
        self.summary: list[str] = []
        self.seqgen_path = SEQGEN_BIN
        self.sequence_root = self.probe_root / "sequences"
        self.file_encoder = FileEncoder()

        self.opcode_eps_get_status = dictionary_command_opcode(self.dictionary, "OBCApp.epsBridge.EPS_GET_STATUS")
        self.opcode_comm_set_active = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_SET_ACTIVE")
        self.opcode_comm_start_pass = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_START_PASS")
        self.opcode_seq_validate = dictionary_command_opcode(self.dictionary, "OBCApp.sequenceAdmissionController.SEQ_VALIDATE")
        self.opcode_seq_prepare_manual = dictionary_command_opcode(
            self.dictionary, "OBCApp.sequenceAdmissionController.SEQ_PREPARE_MANUAL"
        )

    def start(self) -> None:
        clean_dir(self.probe_root)
        clean_dir(self.runtime_root)
        self.sequence_root.mkdir(parents=True, exist_ok=True)
        SECURITY_SERVER_SOCKET.parent.mkdir(parents=True, exist_ok=True)
        self.processes.append(
            start_managed_process(
                "security_server_sim",
                [
                    PYTHON_BIN,
                    str(ROOT_DIR / "scripts/security_server_sim.py"),
                    "--socket-path",
                    str(SECURITY_SERVER_SOCKET),
                ],
                SECURITY_SERVER_LOG,
                cwd=ROOT_DIR,
                stale_match_groups=((str(SECURITY_SERVER_SOCKET),),),
                stale_match_markers=("security_server_sim.py",),
            )
        )
        self.wait_for_socket(SECURITY_SERVER_SOCKET, HOSTED_TIMEOUT_SEC)
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
            enable_uhf_beacon_side_channel=True,
            auto_ports=True,
            command_authority_profile="sband-primary",
        )
        self.stack.start()
        if self.stack.sband is None or self.stack.uhf is None:
            raise RuntimeError("combined hosted stack did not expose both S-band and UHF surfaces")
        self.sband = self.stack.sband
        self.uhf = self.stack.uhf
        self.runtime_root = self.stack.runtime_root
        self.obc_log = self.stack.obc_log
        wait_text_since(self.obc_log, 0, "BEACON_PACKET_EMITTED", HOSTED_TIMEOUT_SEC)

    def stop(self) -> None:
        if self.stack is not None:
            self.stack.stop()
            self.stack = None
        cleanup_managed_processes(self.processes, timeout_sec=5.0)

    @staticmethod
    def wait_for_socket(socket_path: pathlib.Path, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if socket_path.exists():
                return
            time.sleep(0.1)
        raise RuntimeError(f"timed out waiting for socket {socket_path}")

    def capture_path(self, ground: GroundPath, direction: str) -> pathlib.Path:
        assert self.stack is not None
        return self.stack.captures_root / f"{ground.band}-{direction}.bin"

    def send_logged_raw_packet(self, ground: GroundPath, label: str, payload: bytes) -> None:
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(f"{label}: tts=127.0.0.1:{ground.gds_tts_port} bytes={len(payload)} payload={payload.hex()}\n")
        send_tts_raw_packet(ground.gds_tts_port, payload)

    def fresh_handshake_count(self, ground: GroundPath, service_id: int, message_type: HandshakeMessageType) -> int:
        return len(
            load_handshake_messages(
                self.capture_path(ground, "southbound-to-gds"),
                scid=ground.scid,
                vcid=ground.vcid,
                frame_size=ground.frame_size,
                service_id=service_id,
                message_type=message_type,
            )
        )

    def wait_for_handshake(self, ground: GroundPath, service_id: int, message_type: HandshakeMessageType, seen_count: int, *, timeout: float = AUTH_TIMEOUT_SEC):
        return wait_for_handshake_message(
            self.capture_path(ground, "southbound-to-gds"),
            scid=ground.scid,
            vcid=ground.vcid,
            frame_size=ground.frame_size,
            service_id=service_id,
            message_type=message_type,
            seen_count=seen_count,
            timeout=timeout,
        )

    def authenticate_service(self, ground: GroundPath, *, service_id: int, ingress_port: int, expected_open_fragment: str, expected_replaced: int = 0) -> SecureSession:
        challenge_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.CHALLENGE)
        status_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.AUTH_STATUS)
        deadline = time.time() + AUTH_ESTABLISH_TIMEOUT_SEC
        attempt = 0
        while time.time() < deadline:
            attempt += 1
            auth_offset = file_size(self.obc_log)
            self.send_logged_raw_packet(ground, f"{ground.name}-req-auth-service-{service_id}-attempt-{attempt}", build_req_auth_packet(service_id))
            try:
                challenge = self.wait_for_handshake(ground, service_id, HandshakeMessageType.CHALLENGE, challenge_count, timeout=AUTH_CHALLENGE_WAIT_SEC)
            except TimeoutError:
                time.sleep(AUTH_RETRY_BACKOFF_SEC)
                continue
            challenge_count += 1
            session_key = request_session_key(SECURITY_SERVER_SOCKET, service_id, challenge.challenge)
            response = compute_auth_response(session_key)
            for response_attempt in range(AUTH_RESPONSE_RETRY_LIMIT):
                self.send_logged_raw_packet(
                    ground,
                    f"{ground.name}-auth-response-service-{service_id}-attempt-{attempt}-response-{response_attempt + 1}",
                    build_response_packet(service_id, response),
                )
                try:
                    auth_status = self.wait_for_handshake(ground, service_id, HandshakeMessageType.AUTH_STATUS, status_count, timeout=AUTH_RESPONSE_WAIT_SEC)
                except TimeoutError:
                    time.sleep(AUTH_RETRY_BACKOFF_SEC)
                    continue
                status_count += 1
                if auth_status.status_code == AuthStatusCode.AUTHENTICATED:
                    wait_text_since(self.obc_log, auth_offset, f"Secure auth established ingress {ingress_port} service {service_id}", AUTH_TIMEOUT_SEC)
                    wait_regex_since(self.obc_log, auth_offset, re.escape(expected_open_fragment) + r".*replaced " + str(expected_replaced), AUTH_TIMEOUT_SEC)
                    return SecureSession(service_id=service_id, session_key=session_key)
                if auth_status.status_code != AuthStatusCode.NOT_AUTHENTICATED:
                    raise RuntimeError(f"{ground.name}: unexpected auth status {auth_status.status_code} for service {service_id}")
                time.sleep(AUTH_RETRY_BACKOFF_SEC)
                break
        raise RuntimeError(f"{ground.name}: failed to authenticate service {service_id} after {attempt} bounded request attempts")

    def send_secure_command(
        self,
        ground: GroundPath,
        session: SecureSession,
        opcode: int,
        *,
        args: bytes = b"",
        log_waits: list[tuple[pathlib.Path, str]] = (),
        regex_waits: list[tuple[pathlib.Path, str]] = (),
        timeout: float = AUTH_TIMEOUT_SEC,
    ) -> int:
        offsets = {path: file_size(path) for path, _ in [*log_waits, *regex_waits]}
        sequence_number = session.accept_sequence()
        payload = build_secure_command_v2_packet(build_inner_command(opcode, args=args), session.session_key, sequence_number)
        self.send_logged_raw_packet(ground, f"{ground.name}-secure-command-seq-{sequence_number}", payload)
        for path, fragment in log_waits:
            wait_text_since(path, offsets[path], fragment, timeout)
        for path, pattern in regex_waits:
            wait_regex_since(path, offsets[path], pattern, timeout)
        return sequence_number

    def encode_inner_command(self, command_name: str, *args: str) -> bytes:
        template = self.command_dictionaries.command_name[command_name]
        encoded = self.command_encoder.encode_api(CmdData(tuple(args), template))
        return encoded[8:]

    def send_file_packet(self, ground: GroundPath, packet: StartPacketData | DataPacketData | EndPacketData) -> None:
        payload = self.file_encoder.encode_api(packet)
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)

    def upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> int:
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        event_offset = file_size(ground.events_log)
        self.send_file_packet(ground, StartPacketData(0, len(payload), str(artifact.binary), artifact.destination))
        self.send_file_packet(ground, DataPacketData(1, 0, payload))
        self.send_file_packet(ground, EndPacketData(2, checksum.value))
        return event_offset

    def build_sequence_artifact(self, name: str) -> SequenceArtifact:
        source = self.sequence_root / f"{name}.seq"
        source.write_text("R00:00:00 OBCApp.epsBridge.EPS_GET_STATUS\n", encoding="utf-8")
        binary = self.sequence_root / f"{name}.bin"
        subprocess.run([str(self.seqgen_path), "--dictionary", str(DICT_PATH), str(source), str(binary)], check=True, cwd=str(ROOT_DIR), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return SequenceArtifact(name=name, source=source, binary=binary, destination=f".sequence-staging/{name}.bin")

    def validate_sequence(self, ground: GroundPath, session: SecureSession, artifact: SequenceArtifact) -> None:
        event_offset = file_size(self.obc_log)
        sequence_number = session.accept_sequence()
        inner = self.encode_inner_command("OBCApp.sequenceAdmissionController.SEQ_PREPARE_MANUAL", artifact.destination)
        payload = build_secure_command_v2_packet(inner, session.session_key, sequence_number)
        self.send_logged_raw_packet(ground, f"{ground.name}-file-packet", payload)
        wait_text_since(self.obc_log, event_offset, "SEQUENCE_CONTEXT_UPDATED", AUTH_TIMEOUT_SEC)
        wait_for_glob(self.runtime_root / "sequences" / "admitted", f"ctx-*-{artifact.binary.name}", AUTH_TIMEOUT_SEC)
        assert_no_text_since(self.obc_log, event_offset, "SEQUENCE_CONTROL_REJECTED", 2.0)
        assert_no_text_since(self.obc_log, event_offset, "COMMAND_AUTHORITY_REJECTED", 2.0)

    def verify_malformed_handshake_fail_closed(self) -> None:
        assert self.sband is not None
        challenge_count = self.fresh_handshake_count(self.sband, SERVICE_ID_SBAND, HandshakeMessageType.CHALLENGE)
        status_count = self.fresh_handshake_count(self.sband, SERVICE_ID_SBAND, HandshakeMessageType.AUTH_STATUS)
        obc_offset = file_size(self.obc_log)
        malformed = bytearray(build_req_auth_packet(SERVICE_ID_SBAND))
        malformed[2] ^= 0x01
        self.send_logged_raw_packet(self.sband, "sband-malformed-req-auth", bytes(malformed))
        wait_text_since(self.obc_log, obc_offset, "Secure auth packet rejected ingress 0 service 0 reason", AUTH_TIMEOUT_SEC)
        time.sleep(1.0)
        if self.fresh_handshake_count(self.sband, SERVICE_ID_SBAND, HandshakeMessageType.CHALLENGE) != challenge_count:
            raise RuntimeError("malformed handshake unexpectedly produced a challenge")
        if self.fresh_handshake_count(self.sband, SERVICE_ID_SBAND, HandshakeMessageType.AUTH_STATUS) != status_count:
            raise RuntimeError("malformed handshake unexpectedly produced an auth status packet")
        self.summary.append("case-malformed-handshake-fail-closed=PASS")

    def verify_sband_upload_and_validate(self) -> SecureSession:
        assert self.sband is not None
        session = self.authenticate_service(
            self.sband,
            service_id=SERVICE_ID_SBAND,
            ingress_port=0,
            expected_open_fragment="Command session opened ingress 0 identity 1 role 1 session ",
        )
        self.send_secure_command(
            self.sband,
            session,
            self.opcode_comm_start_pass,
            args=struct.pack(">I", UHF_BACKUP_PASS_SECONDS),
            log_waits=[(self.sband.events_log, f"Comm pass started for {UHF_BACKUP_PASS_SECONDS} sec")],
        )
        artifact = self.build_sequence_artifact("sband-allowed")
        event_offset = file_size(self.obc_log)
        self.upload_sequence(self.sband, artifact)
        wait_text_since(self.obc_log, event_offset, "FILE_INGRESS_START_ACCEPTED", UPLOAD_TIMEOUT_SEC)
        wait_text_since(self.obc_log, event_offset, "FileReceived", UPLOAD_TIMEOUT_SEC)
        self.validate_sequence(self.sband, session, artifact)
        self.summary.append("case-sband-secure-auth-staged-upload-and-sequence-validate=PASS")
        return session

    def verify_uhf_backup_deny(self) -> SecureSession:
        assert self.uhf is not None
        session = self.authenticate_service(
            self.uhf,
            service_id=SERVICE_ID_UHF,
            ingress_port=1,
            expected_open_fragment="Command session opened ingress 1 identity 2 role 2 session ",
        )
        artifact = self.build_sequence_artifact("uhf-backup-denied")
        event_offset = file_size(self.obc_log)
        self.upload_sequence(self.uhf, artifact)
        wait_text_since(self.obc_log, event_offset, "FILE_INGRESS_START_REJECTED", UPLOAD_TIMEOUT_SEC)
        assert_no_text_since(self.obc_log, event_offset, "FileReceived", 3.0)
        self.summary.append("case-uhf-backup-staged-upload-denied=PASS")
        return session

    def verify_uhf_primary_after_failover(self, sband_session: SecureSession, uhf_backup_session: SecureSession) -> None:
        assert self.sband is not None and self.uhf is not None
        switch_offset = file_size(self.obc_log)
        self.send_secure_command(
            self.sband,
            sband_session,
            self.opcode_comm_set_active,
            args=struct.pack(">B", COMM_BAND_UHF),
            log_waits=[(self.obc_log, "Comm primary links command UHF (1) telemetry UHF (1) file UHF (1) reason 1")],
            timeout=20.0,
        )
        wait_text_since(self.obc_log, switch_offset, "Secure auth revoked ingress 0 service 1 reason 2", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_offset, "Secure auth revoked ingress 1 service 2 reason 2", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_offset, "Command session revoked ingress 1 identity 2 role 2", AUTH_TIMEOUT_SEC)

        reject_offset = file_size(self.obc_log)
        payload = build_secure_command_v2_packet(build_inner_command(self.opcode_eps_get_status), uhf_backup_session.session_key, uhf_backup_session.next_sequence)
        self.send_logged_raw_packet(self.uhf, "uhf-stale-secure-command-post-role-switch", payload)
        wait_text_since(self.obc_log, reject_offset, "Secure command rejected ingress 1 identity 2 role 3 session 0", AUTH_TIMEOUT_SEC)

        uhf_primary_session = self.authenticate_service(
            self.uhf,
            service_id=SERVICE_ID_UHF,
            ingress_port=1,
            expected_open_fragment="Command session opened ingress 1 identity 2 role 3 session ",
        )
        artifact = self.build_sequence_artifact("uhf-primary-allowed")
        event_offset = file_size(self.obc_log)
        self.upload_sequence(self.uhf, artifact)
        wait_text_since(self.obc_log, event_offset, "FILE_INGRESS_START_ACCEPTED", UPLOAD_TIMEOUT_SEC)
        wait_text_since(self.obc_log, event_offset, "FileReceived", UPLOAD_TIMEOUT_SEC)
        self.validate_sequence(self.uhf, uhf_primary_session, artifact)
        self.summary.append("case-uhf-failover-primary-reauth-staged-upload-and-sequence-validate=PASS")

    def run(self) -> list[str]:
        assert self.sband is not None and self.uhf is not None
        self.verify_malformed_handshake_fail_closed()
        sband_session = self.verify_sband_upload_and_validate()
        uhf_backup_session = self.verify_uhf_backup_deny()
        self.verify_uhf_primary_after_failover(sband_session, uhf_backup_session)
        return [
            "uplink-authority-and-key-hardening-hosted-probe: PASS",
            "formal-verdict=uplink-authority-and-key-hardening-hosted",
            *self.summary,
            f"security-server-log={SECURITY_SERVER_LOG}",
            f"stack-root={self.stack_root}",
            f"runtime-root={self.runtime_root}",
            f"sband-capture={self.capture_path(self.sband, 'southbound-to-gds')}",
            f"uhf-capture={self.capture_path(self.uhf, 'southbound-to-gds')}",
            f"obc-log={self.obc_log}",
            f"sband-events-log={self.sband.events_log}",
            f"uhf-events-log={self.uhf.events_log}",
        ]


def main() -> int:
    probe = UplinkAuthorityAndKeyHardeningHostedProbe()
    try:
        probe.start()
        for line in probe.run():
            print(line)
        return 0
    finally:
        probe.stop()


if __name__ == "__main__":
    sys.exit(main())
