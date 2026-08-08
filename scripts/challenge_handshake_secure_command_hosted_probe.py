#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
import struct
import sys
import time
from dataclasses import dataclass

from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
    SERVICE_ID_SBAND,
    SERVICE_ID_UHF,
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

    def claim_sequence(self) -> int:
        return self.next_sequence

    def accept_sequence(self) -> int:
        sequence = self.next_sequence
        self.next_sequence += 1
        return sequence


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = pathlib.Path(require_env("RUNTIME_ROOT"))
PYTHON_BIN = require_env("PYTHON_BIN")

SECURITY_SERVER_SOCKET = PROBE_TMP_DIR / "security-server" / "secure-server.sock"
SECURITY_SERVER_LOG = PROBE_TMP_DIR / "security-server.log"
HOSTED_TIMEOUT_SEC = 30.0
AUTH_TIMEOUT_SEC = 10.0
AUTH_CHALLENGE_WAIT_SEC = 8.0
AUTH_RESPONSE_WAIT_SEC = 8.0
AUTH_ESTABLISH_TIMEOUT_SEC = 30.0
AUTH_RESPONSE_RETRY_LIMIT = 3
AUTH_RETRY_BACKOFF_SEC = 0.5
TIMEOUT_WAIT_SEC = 210.0
UHF_BACKUP_PASS_SECONDS = 30
SAT_MODE_IDLE = 1
COMM_BAND_SBAND = 0
COMM_BAND_UHF = 1

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from per_band_stock_ground_stacks import GroundPath, HostedPerBandStockStacks, find_fprime_cli  # noqa: E402
from probe_process_utils import cleanup_managed_processes, install_signal_cleanup, start_managed_process  # noqa: E402


class ChallengeHandshakeHostedProbe:
    def __init__(self) -> None:
        self.probe_root = PROBE_TMP_DIR
        self.stack_root = self.probe_root / "combined-stack"
        self.runtime_root = RUNTIME_ROOT
        self.dictionary = json.loads(DICT_PATH.read_text(encoding="utf-8"))
        self.processes = []
        self.stack: HostedPerBandStockStacks | None = None
        self.sband: GroundPath | None = None
        self.uhf: GroundPath | None = None
        self.obc_log = self.stack_root / "logs" / "obc.log"
        self.summary: list[str] = []

        self.opcode_eps_get_status = dictionary_command_opcode(self.dictionary, "OBCApp.epsBridge.EPS_GET_STATUS")
        self.opcode_mode_set = dictionary_command_opcode(self.dictionary, "OBCApp.modeManager.MODE_SET")
        self.opcode_comm_set_active = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_SET_ACTIVE")
        self.opcode_comm_start_pass = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_START_PASS")

    def start(self) -> None:
        clean_dir(self.probe_root)
        clean_dir(self.runtime_root)
        security_server_dir = SECURITY_SERVER_SOCKET.parent
        security_server_dir.mkdir(parents=True, exist_ok=True)
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

    def wait_for_handshake(
        self,
        ground: GroundPath,
        service_id: int,
        message_type: HandshakeMessageType,
        seen_count: int,
        *,
        timeout: float = AUTH_TIMEOUT_SEC,
    ):
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

    def authenticate_service(
        self,
        ground: GroundPath,
        *,
        service_id: int,
        ingress_port: int,
        expected_open_fragment: str,
        expected_replaced: int = 0,
    ) -> SecureSession:
        challenge_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.CHALLENGE)
        status_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.AUTH_STATUS)
        deadline = time.time() + AUTH_ESTABLISH_TIMEOUT_SEC
        attempt = 0
        while time.time() < deadline:
            attempt += 1
            auth_offset = file_size(self.obc_log)
            self.send_logged_raw_packet(ground, f"{ground.name}-req-auth-service-{service_id}-attempt-{attempt}", build_req_auth_packet(service_id))
            try:
                challenge = self.wait_for_handshake(
                    ground,
                    service_id,
                    HandshakeMessageType.CHALLENGE,
                    challenge_count,
                    timeout=AUTH_CHALLENGE_WAIT_SEC,
                )
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
                    auth_status = self.wait_for_handshake(
                        ground,
                        service_id,
                        HandshakeMessageType.AUTH_STATUS,
                        status_count,
                        timeout=AUTH_RESPONSE_WAIT_SEC,
                    )
                except TimeoutError:
                    time.sleep(AUTH_RETRY_BACKOFF_SEC)
                    continue
                status_count += 1
                if auth_status.status_code == AuthStatusCode.AUTHENTICATED:
                    wait_text_since(
                        self.obc_log,
                        auth_offset,
                        f"Secure auth established ingress {ingress_port} service {service_id}",
                        AUTH_TIMEOUT_SEC,
                    )
                    wait_regex_since(
                        self.obc_log,
                        auth_offset,
                        re.escape(expected_open_fragment) + r".*replaced " + str(expected_replaced),
                        AUTH_TIMEOUT_SEC,
                    )
                    return SecureSession(service_id=service_id, session_key=session_key)
                if auth_status.status_code != AuthStatusCode.NOT_AUTHENTICATED:
                    raise RuntimeError(f"{ground.name}: unexpected auth status {auth_status.status_code} for service {service_id}")
                time.sleep(AUTH_RETRY_BACKOFF_SEC)
                break
        raise RuntimeError(
            f"{ground.name}: failed to authenticate service {service_id} after {attempt} bounded request attempts"
        )

    def send_secure_command(
        self,
        ground: GroundPath,
        session: SecureSession,
        opcode: int,
        *,
        args: bytes = b"",
        accept_sequence: bool,
        log_waits: list[tuple[pathlib.Path, str]] = (),
        regex_waits: list[tuple[pathlib.Path, str]] = (),
        timeout: float = AUTH_TIMEOUT_SEC,
    ) -> int:
        offsets = {path: file_size(path) for path, _ in [*log_waits, *regex_waits]}
        sequence_number = session.accept_sequence() if accept_sequence else session.claim_sequence()
        payload = build_secure_command_v2_packet(
            build_inner_command(opcode, args=args),
            session.session_key,
            sequence_number,
        )
        self.send_logged_raw_packet(ground, f"{ground.name}-secure-command-seq-{sequence_number}", payload)
        for path, fragment in log_waits:
            wait_text_since(path, offsets[path], fragment, timeout)
        for path, pattern in regex_waits:
            wait_regex_since(path, offsets[path], pattern, timeout)
        return sequence_number

    def send_secure_command_with_retry(
        self,
        ground: GroundPath,
        session: SecureSession,
        opcode: int,
        *,
        args: bytes = b"",
        log_waits: list[tuple[pathlib.Path, str]] = (),
        regex_waits: list[tuple[pathlib.Path, str]] = (),
        attempts: int = 3,
        timeout: float = AUTH_TIMEOUT_SEC,
        retry_delay: float = 0.8,
    ) -> int:
        last_error: RuntimeError | None = None
        for attempt in range(1, attempts + 1):
            try:
                return self.send_secure_command(
                    ground,
                    session,
                    opcode,
                    args=args,
                    accept_sequence=True,
                    log_waits=log_waits,
                    regex_waits=regex_waits,
                    timeout=timeout,
                )
            except RuntimeError as exc:
                last_error = exc
                if attempt == attempts:
                    break
                time.sleep(retry_delay)
        assert last_error is not None
        raise last_error

    def verify_sband_flow(self) -> SecureSession:
        assert self.sband is not None
        sband_session = self.authenticate_service(
            self.sband,
            service_id=SERVICE_ID_SBAND,
            ingress_port=0,
            expected_open_fragment="Command session opened ingress 0 identity 1 role 1 session ",
        )
        self.send_secure_command(
            self.sband,
            sband_session,
            self.opcode_eps_get_status,
            accept_sequence=True,
            log_waits=[(self.sband.events_log, "EPS status updated")],
        )
        self.send_secure_command(
            self.sband,
            sband_session,
            self.opcode_comm_start_pass,
            args=struct.pack(">I", UHF_BACKUP_PASS_SECONDS),
            accept_sequence=True,
            log_waits=[(self.sband.events_log, f"Comm pass started for {UHF_BACKUP_PASS_SECONDS} sec")],
        )
        self.summary.append("case-sband-secure-auth-eps-status=PASS")
        return sband_session

    def verify_uhf_backup_flow(self) -> SecureSession:
        assert self.sband is not None and self.uhf is not None
        uhf_session = self.authenticate_service(
            self.uhf,
            service_id=SERVICE_ID_UHF,
            ingress_port=1,
            expected_open_fragment="Command session opened ingress 1 identity 2 role 2 session ",
        )
        self.send_secure_command(
            self.uhf,
            uhf_session,
            self.opcode_eps_get_status,
            accept_sequence=True,
            log_waits=[(self.sband.events_log, "EPS status updated")],
        )
        self.send_secure_command(
            self.uhf,
            uhf_session,
            self.opcode_mode_set,
            args=struct.pack(">B", SAT_MODE_IDLE),
            accept_sequence=False,
            log_waits=[
                (
                    self.sband.events_log,
                    f"Command authority rejected opcode 0x{self.opcode_mode_set:x} ingress 1 identity 2 role 2",
                )
            ],
        )
        self.send_secure_command(
            self.uhf,
            uhf_session,
            self.opcode_eps_get_status,
            accept_sequence=True,
            log_waits=[(self.sband.events_log, "EPS status updated")],
        )
        self.summary.append("case-uhf-backup-secure-auth-read-continuity-and-high-authority-deny=PASS")
        return uhf_session

    def verify_uhf_failover_and_reauth(self, sband_session: SecureSession, uhf_session: SecureSession) -> None:
        assert self.sband is not None and self.uhf is not None
        switch_offset = file_size(self.obc_log)
        self.send_secure_command_with_retry(
            self.sband,
            sband_session,
            self.opcode_comm_set_active,
            args=struct.pack(">B", COMM_BAND_UHF),
            log_waits=[(self.obc_log, "Comm primary links command UHF (1) telemetry UHF (1) file UHF (1) reason 1")],
            attempts=4,
            timeout=20.0,
        )
        wait_text_since(self.obc_log, switch_offset, "Secure auth revoked ingress 0 service 1 reason 2", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_offset, "Command session revoked ingress 0 identity 1 role 1", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_offset, "Secure auth revoked ingress 1 service 2 reason 2", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_offset, "Command session revoked ingress 1 identity 2 role 2", AUTH_TIMEOUT_SEC)

        self.send_secure_command(
            self.uhf,
            uhf_session,
            self.opcode_eps_get_status,
            accept_sequence=False,
            log_waits=[(self.obc_log, "Secure command rejected ingress 1 identity 2 role 3 session 0")],
        )

        uhf_primary_session = self.authenticate_service(
            self.uhf,
            service_id=SERVICE_ID_UHF,
            ingress_port=1,
            expected_open_fragment="Command session opened ingress 1 identity 2 role 3 session ",
        )
        switch_back_offset = file_size(self.obc_log)
        self.send_secure_command_with_retry(
            self.uhf,
            uhf_primary_session,
            self.opcode_comm_set_active,
            args=struct.pack(">B", COMM_BAND_SBAND),
            log_waits=[(self.obc_log, "Comm primary links command SBAND (0) telemetry SBAND (0) file SBAND (0) reason 1")],
            attempts=4,
            timeout=20.0,
        )
        wait_text_since(self.obc_log, switch_back_offset, "Secure auth revoked ingress 1 service 2 reason 2", AUTH_TIMEOUT_SEC)
        wait_text_since(self.obc_log, switch_back_offset, "Command session revoked ingress 1 identity 2 role 3", AUTH_TIMEOUT_SEC)
        self.summary.append("case-uhf-primary-reauth-required-and-high-authority-after-reauth=PASS")

    def verify_timeout(self) -> SecureSession:
        assert self.sband is not None
        timeout_session = self.authenticate_service(
            self.sband,
            service_id=SERVICE_ID_SBAND,
            ingress_port=0,
            expected_open_fragment="Command session opened ingress 0 identity 1 role 1 session ",
        )
        self.send_secure_command(
            self.sband,
            timeout_session,
            self.opcode_eps_get_status,
            accept_sequence=True,
            log_waits=[(self.sband.events_log, "EPS status updated")],
        )
        timeout_offset = file_size(self.obc_log)
        wait_text_since(self.obc_log, timeout_offset, "Secure auth revoked ingress 0 service 1 reason 1", TIMEOUT_WAIT_SEC)
        wait_text_since(self.obc_log, timeout_offset, "Command session revoked ingress 0 identity 1 role 1", AUTH_TIMEOUT_SEC)
        self.send_secure_command(
            self.sband,
            timeout_session,
            self.opcode_eps_get_status,
            accept_sequence=False,
            log_waits=[(self.obc_log, "Secure command rejected ingress 0 identity 1 role 1 session 0")],
        )
        self.summary.append("case-secure-auth-inactivity-timeout-clears-session=PASS")
        return timeout_session

    def run(self) -> list[str]:
        assert self.sband is not None and self.uhf is not None
        sband_session = self.verify_sband_flow()
        uhf_session = self.verify_uhf_backup_flow()
        self.verify_uhf_failover_and_reauth(sband_session, uhf_session)
        self.verify_timeout()
        return [
            "challenge-handshake-secure-command-hosted-probe: PASS",
            "formal-verdict=challenge-handshake-secure-command-hosted",
            "secure-auth-service-1=sband",
            "secure-auth-service-2=uhf",
            "handshake-apid=0x00FE",
            "secure-command-apid=0x0000",
            *self.summary,
            f"security-server-log={SECURITY_SERVER_LOG}",
            f"stack-root={self.stack_root}",
            f"runtime-root={self.runtime_root}",
            f"sband-capture={self.capture_path(self.sband, 'southbound-to-gds')}",
            f"uhf-capture={self.capture_path(self.uhf, 'southbound-to-gds')}",
            f"obc-log={self.obc_log}",
            f"sband-events-log={self.sband.events_log}",
            f"uhf-events-log={self.uhf.events_log}",
            f"logs={self.probe_root}",
        ]


def main() -> int:
    probe = ChallengeHandshakeHostedProbe()
    install_signal_cleanup(probe.stop)
    try:
        probe.start()
        lines = probe.run()
        sys.stdout.write("\n".join(lines) + "\n")
        return 0
    finally:
        probe.stop()


if __name__ == "__main__":
    raise SystemExit(main())
