from __future__ import annotations

import pathlib
import re
import time
from dataclasses import dataclass

from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
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


def wait_for_socket(socket_path: pathlib.Path, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if socket_path.exists():
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for socket {socket_path}")


def send_logged_raw_packet(raw_command_log: pathlib.Path, gds_tts_port: int, label: str, payload: bytes) -> None:
    with raw_command_log.open("a", encoding="utf-8") as handle:
        handle.write(f"{label}: tts=127.0.0.1:{gds_tts_port} bytes={len(payload)} payload={payload.hex()}\n")
    send_tts_raw_packet(gds_tts_port, payload)


def fresh_handshake_count(
    capture_path: pathlib.Path,
    *,
    scid: int,
    vcid: int,
    frame_size: int,
    service_id: int,
    message_type: HandshakeMessageType,
) -> int:
    return len(
        load_handshake_messages(
            capture_path,
            scid=scid,
            vcid=vcid,
            frame_size=frame_size,
            service_id=service_id,
            message_type=message_type,
        )
    )


def wait_for_handshake(
    capture_path: pathlib.Path,
    *,
    scid: int,
    vcid: int,
    frame_size: int,
    service_id: int,
    message_type: HandshakeMessageType,
    seen_count: int,
    timeout: float,
):
    return wait_for_handshake_message(
        capture_path,
        scid=scid,
        vcid=vcid,
        frame_size=frame_size,
        service_id=service_id,
        message_type=message_type,
        seen_count=seen_count,
        timeout=timeout,
    )


def authenticate_service(
    *,
    capture_path: pathlib.Path,
    raw_command_log: pathlib.Path,
    gds_tts_port: int,
    scid: int,
    vcid: int,
    frame_size: int,
    security_server_socket: pathlib.Path,
    obc_log: pathlib.Path,
    service_id: int,
    ingress_port: int,
    expected_open_pattern: str,
    auth_timeout: float,
    establish_timeout: float,
    challenge_timeout: float | None = None,
    response_timeout: float | None = None,
    retry_backoff_sec: float = 0.5,
    response_retry_limit: int = 3,
) -> SecureSession:
    effective_challenge_timeout = auth_timeout if challenge_timeout is None else challenge_timeout
    effective_response_timeout = auth_timeout if response_timeout is None else response_timeout
    challenge_count = fresh_handshake_count(
        capture_path,
        scid=scid,
        vcid=vcid,
        frame_size=frame_size,
        service_id=service_id,
        message_type=HandshakeMessageType.CHALLENGE,
    )
    status_count = fresh_handshake_count(
        capture_path,
        scid=scid,
        vcid=vcid,
        frame_size=frame_size,
        service_id=service_id,
        message_type=HandshakeMessageType.AUTH_STATUS,
    )
    deadline = time.time() + establish_timeout
    attempt = 0
    while time.time() < deadline:
        attempt += 1
        auth_offset = file_size(obc_log)
        send_logged_raw_packet(
            raw_command_log,
            gds_tts_port,
            f"req-auth-service-{service_id}-attempt-{attempt}",
            build_req_auth_packet(service_id),
        )
        try:
            challenge = wait_for_handshake(
                capture_path,
                scid=scid,
                vcid=vcid,
                frame_size=frame_size,
                service_id=service_id,
                message_type=HandshakeMessageType.CHALLENGE,
                seen_count=challenge_count,
                timeout=effective_challenge_timeout,
            )
        except TimeoutError:
            time.sleep(retry_backoff_sec)
            continue
        challenge_count += 1
        session_key = request_session_key(security_server_socket, service_id, challenge.challenge)
        response = compute_auth_response(session_key)
        for response_attempt in range(1, response_retry_limit + 1):
            send_logged_raw_packet(
                raw_command_log,
                gds_tts_port,
                f"auth-response-service-{service_id}-attempt-{attempt}-response-{response_attempt}",
                build_response_packet(service_id, response),
            )
            try:
                auth_status = wait_for_handshake(
                    capture_path,
                    scid=scid,
                    vcid=vcid,
                    frame_size=frame_size,
                    service_id=service_id,
                    message_type=HandshakeMessageType.AUTH_STATUS,
                    seen_count=status_count,
                    timeout=effective_response_timeout,
                )
            except TimeoutError:
                time.sleep(retry_backoff_sec)
                continue
            status_count += 1
            if auth_status.status_code == AuthStatusCode.AUTHENTICATED:
                wait_text_since(
                    obc_log,
                    auth_offset,
                    f"Secure auth established ingress {ingress_port} service {service_id}",
                    auth_timeout,
                )
                wait_regex_since(obc_log, auth_offset, expected_open_pattern, auth_timeout)
                return SecureSession(service_id=service_id, session_key=session_key)
            if auth_status.status_code != AuthStatusCode.NOT_AUTHENTICATED:
                raise RuntimeError(f"unexpected auth status {auth_status.status_code} for service {service_id}")
            time.sleep(retry_backoff_sec)
            break
    raise RuntimeError(f"failed to authenticate service {service_id} after {attempt} bounded request attempts")


def send_secure_command(
    *,
    raw_command_log: pathlib.Path,
    gds_tts_port: int,
    session: SecureSession,
    opcode: int,
    args: bytes = b"",
    accept_sequence: bool = True,
    log_waits: list[tuple[pathlib.Path, str]] | tuple[tuple[pathlib.Path, str], ...] = (),
    regex_waits: list[tuple[pathlib.Path, str]] | tuple[tuple[pathlib.Path, str], ...] = (),
    timeout: float = 10.0,
    label: str | None = None,
) -> int:
    offsets = {path: file_size(path) for path, _ in [*log_waits, *regex_waits]}
    sequence_number = session.accept_sequence() if accept_sequence else session.claim_sequence()
    payload = build_secure_command_v2_packet(build_inner_command(opcode, args=args), session.session_key, sequence_number)
    send_logged_raw_packet(raw_command_log, gds_tts_port, label or f"secure-command-seq-{sequence_number}", payload)
    for path, fragment in log_waits:
        wait_text_since(path, offsets[path], fragment, timeout)
    for path, pattern in regex_waits:
        wait_regex_since(path, offsets[path], pattern, timeout)
    return sequence_number


def send_secure_command_with_retry(
    *,
    raw_command_log: pathlib.Path,
    gds_tts_port: int,
    session: SecureSession,
    opcode: int,
    args: bytes = b"",
    log_waits: list[tuple[pathlib.Path, str]] | tuple[tuple[pathlib.Path, str], ...] = (),
    regex_waits: list[tuple[pathlib.Path, str]] | tuple[tuple[pathlib.Path, str], ...] = (),
    attempts: int = 3,
    timeout: float = 10.0,
    retry_delay: float = 0.8,
    label: str | None = None,
) -> int:
    last_error: RuntimeError | None = None
    for _ in range(attempts):
        try:
            return send_secure_command(
                raw_command_log=raw_command_log,
                gds_tts_port=gds_tts_port,
                session=session,
                opcode=opcode,
                args=args,
                accept_sequence=True,
                log_waits=log_waits,
                regex_waits=regex_waits,
                timeout=timeout,
                label=label,
            )
        except RuntimeError as exc:
            last_error = exc
            time.sleep(retry_delay)
    assert last_error is not None
    raise last_error
