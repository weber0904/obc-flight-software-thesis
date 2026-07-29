from __future__ import annotations

import hashlib
import hmac
import json
import pathlib
import socket
import struct
import time
import configparser
from dataclasses import dataclass
from enum import IntEnum


COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
FW_PACKET_HAND = 0x00FE

SERVICE_ID_SBAND = 1
SERVICE_ID_UHF = 2

SECURE_LINK_HANDSHAKE_MAGIC = 0x0BC0A701
SECURE_LINK_HANDSHAKE_VERSION = 1
SECURE_LINK_HANDSHAKE_HEADER_SIZE = 8
SECURE_LINK_MODULE_SERIAL_SIZE = 16
SECURE_LINK_RANDOM_NONCE_SIZE = 15
SECURE_LINK_CHALLENGE_SIZE = SECURE_LINK_MODULE_SERIAL_SIZE + SECURE_LINK_RANDOM_NONCE_SIZE
SECURE_LINK_AUTH_RESPONSE_SIZE = 32
SECURE_LINK_SESSION_KEY_SIZE = 32

SECURE_COMMAND_V2_OPCODE = 0x0BC20001
SECURE_COMMAND_V2_MAGIC = 0x0BC0DE02
SECURE_COMMAND_V2_VERSION = 2
SECURE_COMMAND_V2_HEADER_LENGTH = 20
SECURE_COMMAND_V2_MAC_LENGTH = 32

COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32

CCSDS_SPACE_PACKET_HEADER_SIZE = 6
CCSDS_TM_HEADER_SIZE = 6
CCSDS_TM_TRAILER_SIZE = 2
CCSDS_IDLE_APID = 0x07FF
CRC16_CCITT_POLY = 0x1021
CRC16_CCITT_INIT = 0xFFFF

DEFAULT_MODULE_SERIAL = b"FPOBCSAT00000001"
DEFAULT_SBAND_ROOT_KEY = bytes.fromhex("101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F")
DEFAULT_UHF_ROOT_KEY = bytes.fromhex("303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F")
AUTH_KDF_LABEL = b"AUTH-SKEY-V1"
SECURE_COMMAND_V2_AUTH_LABEL = b"CMD-V2"
AUTH_KNOWN_MESSAGE = bytes([0xFF] * 32)


@dataclass(frozen=True)
class CommandAuthKeystoreEntry:
    key_bytes: bytes


@dataclass(frozen=True)
class CommandAuthKeystore:
    module_serial: bytes
    sband: CommandAuthKeystoreEntry
    uhf: CommandAuthKeystoreEntry


class HandshakeMessageType(IntEnum):
    REQ_AUTH = 1
    CHALLENGE = 2
    RESPONSE = 3
    AUTH_STATUS = 4


class AuthStatusCode(IntEnum):
    AUTHENTICATED = 1
    NOT_AUTHENTICATED = 2
    MALFORMED = 3
    UNSUPPORTED_SERVICE = 4
    INTERNAL_ERROR = 5


@dataclass(frozen=True)
class CapturedPacket:
    apid: int
    sequence_count: int
    payload: bytes
    offset: int


@dataclass(frozen=True)
class HandshakeMessage:
    message_type: HandshakeMessageType
    service_id: int
    challenge: bytes = b""
    response: bytes = b""
    status_code: int = 0
    packet_offset: int = 0


def default_command_auth_keystore_path(root_dir: pathlib.Path | None = None) -> pathlib.Path:
    base = pathlib.Path(__file__).resolve().parent.parent if root_dir is None else root_dir
    return base / "config/security/command-auth.ini"


def load_command_auth_keystore(path: pathlib.Path | str) -> CommandAuthKeystore:
    config_path = pathlib.Path(path)
    raw_text = config_path.read_text(encoding="utf-8")
    parser = configparser.ConfigParser()
    parser.read_string("[_root_]\n" + raw_text)

    module_serial = parser.get("_root_", "module_serial", fallback="").encode("utf-8")
    if len(module_serial) != SECURE_LINK_MODULE_SERIAL_SIZE:
        raise RuntimeError("command auth keystore module_serial must be 16 bytes")

    def parse_entry(section: str) -> CommandAuthKeystoreEntry:
        if not parser.has_section(section):
            raise RuntimeError(f"command auth keystore missing section [{section}]")
        if parser.has_option(section, "source_id") or parser.has_option(section, "key_slot"):
            raise RuntimeError(
                f"command auth keystore section [{section}] still declares retired legacy tuple keys"
            )
        key_hex = parser.get(section, "key_hex").strip()
        key_bytes = bytes.fromhex(key_hex)
        if len(key_bytes) != SECURE_LINK_SESSION_KEY_SIZE:
            raise RuntimeError(f"command auth keystore section [{section}] must use 32-byte key_hex")
        return CommandAuthKeystoreEntry(key_bytes=key_bytes)

    return CommandAuthKeystore(
        module_serial=module_serial,
        sband=parse_entry("sband"),
        uhf=parse_entry("uhf"),
    )


def keystore_entry_for_service_id(keystore: CommandAuthKeystore, service_id: int) -> CommandAuthKeystoreEntry:
    if service_id == SERVICE_ID_SBAND:
        return keystore.sband
    if service_id == SERVICE_ID_UHF:
        return keystore.uhf
    raise RuntimeError(f"unknown secure auth service id: {service_id}")


def legacy_auth_material_for_profile(keystore: CommandAuthKeystore, profile: str) -> CommandAuthKeystoreEntry:
    del keystore, profile
    raise RuntimeError(
        "legacy command-envelope v1 tuple material is retired from the tracked command-auth keystore contract"
    )


def hmac_sha256(key: bytes, payload: bytes) -> bytes:
    return hmac.new(key, payload, hashlib.sha256).digest()


def derive_session_key(root_key: bytes, service_id: int, challenge: bytes) -> bytes:
    if len(challenge) != SECURE_LINK_CHALLENGE_SIZE:
        raise ValueError(f"challenge must be {SECURE_LINK_CHALLENGE_SIZE} bytes")
    material = AUTH_KDF_LABEL + bytes([service_id]) + challenge
    return hmac_sha256(root_key, material)


def compute_auth_response(session_key: bytes) -> bytes:
    return hmac_sha256(session_key, AUTH_KNOWN_MESSAGE)


def build_inner_command(opcode: int, args: bytes = b"") -> bytes:
    return struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args


def build_secure_command_v2_packet(inner_command: bytes, session_key: bytes, sequence_number: int) -> bytes:
    auth_material = SECURE_COMMAND_V2_AUTH_LABEL + struct.pack(">I", sequence_number) + inner_command
    auth_tag = hmac_sha256(session_key, auth_material)
    return (
        struct.pack(
            ">HIIBBHIHHI",
            FW_PACKET_COMMAND,
            SECURE_COMMAND_V2_OPCODE,
            SECURE_COMMAND_V2_MAGIC,
            SECURE_COMMAND_V2_VERSION,
            0,
            SECURE_COMMAND_V2_HEADER_LENGTH,
            sequence_number,
            len(inner_command),
            SECURE_COMMAND_V2_MAC_LENGTH,
            0,
        )
        + inner_command
        + auth_tag
    )


def build_legacy_envelope_packet(
    inner_command: bytes,
    *,
    source_id: int,
    key_slot: int,
    key_bytes: bytes,
    session_id: int,
    sequence_number: int,
) -> bytes:
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
        len(inner_command),
        COMMAND_ENVELOPE_V1_MAC_LENGTH,
        0,
    )
    auth_tag = hmac_sha256(key_bytes, header + inner_command)
    return struct.pack(">HI", FW_PACKET_COMMAND, COMMAND_ENVELOPE_V1_OPCODE) + header + inner_command + auth_tag


def build_req_auth_packet(service_id: int) -> bytes:
    return struct.pack(">H", FW_PACKET_HAND) + struct.pack(
        ">IBBBB",
        SECURE_LINK_HANDSHAKE_MAGIC,
        SECURE_LINK_HANDSHAKE_VERSION,
        HandshakeMessageType.REQ_AUTH,
        service_id,
        0,
    )


def build_response_packet(service_id: int, response: bytes) -> bytes:
    if len(response) != SECURE_LINK_AUTH_RESPONSE_SIZE:
        raise ValueError(f"response must be {SECURE_LINK_AUTH_RESPONSE_SIZE} bytes")
    return (
        struct.pack(">H", FW_PACKET_HAND)
        + struct.pack(
            ">IBBBB",
            SECURE_LINK_HANDSHAKE_MAGIC,
            SECURE_LINK_HANDSHAKE_VERSION,
            HandshakeMessageType.RESPONSE,
            service_id,
            0,
        )
        + response
    )


def build_gds_transport_packet(fw_packet_payload: bytes) -> bytes:
    return struct.pack(">II", COMMAND_DESCRIPTOR, len(fw_packet_payload)) + fw_packet_payload


def send_tts_raw_packet(tts_port: int, fw_packet_payload: bytes, *, settle_seconds: float = 0.3) -> None:
    transport_payload = build_gds_transport_packet(fw_packet_payload)
    with socket.create_connection(("127.0.0.1", tts_port), timeout=5.0) as sock:
        sock.sendall(b"Register GUI\n")
        time.sleep(0.1)
        sock.sendall(b"A5A5 FSW " + transport_payload)
        time.sleep(settle_seconds)


def request_session_key(socket_path: pathlib.Path, service_id: int, challenge: bytes, timeout: float = 5.0) -> bytes:
    if len(challenge) != SECURE_LINK_CHALLENGE_SIZE:
        raise ValueError(f"challenge must be {SECURE_LINK_CHALLENGE_SIZE} bytes")
    request = json.dumps(
        {
            "action": "GetSessionKey",
            "serviceId": service_id,
            "challengeHex": challenge.hex(),
        }
    ).encode("utf-8") + b"\n"
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.settimeout(timeout)
        sock.connect(str(socket_path))
        sock.sendall(request)
        response = b""
        while not response.endswith(b"\n"):
            chunk = sock.recv(4096)
            if not chunk:
                raise RuntimeError("security-server-sim closed connection before replying")
            response += chunk
    payload = json.loads(response.decode("utf-8"))
    if not payload.get("ok"):
        raise RuntimeError(payload.get("error", "security-server-sim request failed"))
    session_key_hex = payload.get("sessionKeyHex")
    if not isinstance(session_key_hex, str):
        raise RuntimeError("security-server-sim returned no sessionKeyHex")
    session_key = bytes.fromhex(session_key_hex)
    if len(session_key) != SECURE_LINK_SESSION_KEY_SIZE:
        raise RuntimeError("security-server-sim returned invalid session key size")
    return session_key


def u16be(data: bytes, offset: int) -> int:
    return (data[offset] << 8) | data[offset + 1]


def crc16_ccitt(data: bytes) -> int:
    value = CRC16_CCITT_INIT
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            if value & 0x8000:
                value = ((value << 1) ^ CRC16_CCITT_POLY) & 0xFFFF
            else:
                value = (value << 1) & 0xFFFF
    return value


def parse_space_packets(payload: bytes, *, stop_at_idle: bool = False) -> list[CapturedPacket]:
    packets: list[CapturedPacket] = []
    offset = 0
    while offset + CCSDS_SPACE_PACKET_HEADER_SIZE <= len(payload):
        packet_id = u16be(payload, offset)
        sequence_control = u16be(payload, offset + 2)
        data_length_token = u16be(payload, offset + 4)
        apid = packet_id & 0x07FF
        sequence_count = sequence_control & 0x3FFF
        total_size = CCSDS_SPACE_PACKET_HEADER_SIZE + data_length_token + 1
        if offset + total_size > len(payload):
            break
        if apid == CCSDS_IDLE_APID:
            if stop_at_idle:
                break
            offset += total_size
            continue
        packets.append(
            CapturedPacket(
                apid=apid,
                sequence_count=sequence_count,
                payload=payload[offset + CCSDS_SPACE_PACKET_HEADER_SIZE : offset + total_size],
                offset=offset,
            )
        )
        offset += total_size
    return packets


def parse_tm_packets(data: bytes, expected_scid: int, expected_vcid: int, frame_size: int) -> list[CapturedPacket]:
    packets: list[CapturedPacket] = []
    offset = 0
    while offset + frame_size <= len(data):
        frame = data[offset : offset + frame_size]
        global_vcid = u16be(frame, 0)
        scid = (global_vcid >> 4) & 0x03FF
        vcid = (global_vcid >> 1) & 0x07
        transmitted_crc = u16be(frame, frame_size - CCSDS_TM_TRAILER_SIZE)
        computed_crc = crc16_ccitt(frame[: frame_size - CCSDS_TM_TRAILER_SIZE])
        if scid != expected_scid or vcid != expected_vcid or transmitted_crc != computed_crc:
            offset += 1
            continue
        frame_packets = parse_space_packets(frame[CCSDS_TM_HEADER_SIZE : frame_size - CCSDS_TM_TRAILER_SIZE], stop_at_idle=True)
        packets.extend(
            CapturedPacket(
                apid=packet.apid,
                sequence_count=packet.sequence_count,
                payload=packet.payload,
                offset=offset + CCSDS_TM_HEADER_SIZE + packet.offset,
            )
            for packet in frame_packets
        )
        offset += frame_size
    return packets


def parse_handshake_message(packet_payload: bytes) -> HandshakeMessage | None:
    if len(packet_payload) < 2 + SECURE_LINK_HANDSHAKE_HEADER_SIZE:
        return None
    descriptor = struct.unpack_from(">H", packet_payload, 0)[0]
    if descriptor != FW_PACKET_HAND:
        return None
    magic, version, message_type, service_id, reserved = struct.unpack_from(">IBBBB", packet_payload, 2)
    if magic != SECURE_LINK_HANDSHAKE_MAGIC or version != SECURE_LINK_HANDSHAKE_VERSION or reserved != 0:
        return None
    message_enum = HandshakeMessageType(message_type)
    body = packet_payload[2 + SECURE_LINK_HANDSHAKE_HEADER_SIZE :]
    if message_enum == HandshakeMessageType.CHALLENGE:
        if len(body) != SECURE_LINK_CHALLENGE_SIZE:
            return None
        return HandshakeMessage(message_enum, service_id, challenge=body)
    if message_enum == HandshakeMessageType.RESPONSE:
        if len(body) != SECURE_LINK_AUTH_RESPONSE_SIZE:
            return None
        return HandshakeMessage(message_enum, service_id, response=body)
    if message_enum == HandshakeMessageType.AUTH_STATUS:
        if len(body) != 4:
            return None
        (status_code,) = struct.unpack(">I", body)
        return HandshakeMessage(message_enum, service_id, status_code=status_code)
    if message_enum == HandshakeMessageType.REQ_AUTH:
        if body:
            return None
        return HandshakeMessage(message_enum, service_id)
    return None


def load_handshake_messages(
    capture_path: pathlib.Path,
    *,
    scid: int,
    vcid: int,
    frame_size: int,
    service_id: int,
    message_type: HandshakeMessageType,
) -> list[HandshakeMessage]:
    if not capture_path.exists():
        return []
    packets = parse_tm_packets(capture_path.read_bytes(), scid, vcid, frame_size)
    messages: list[HandshakeMessage] = []
    for packet in packets:
        if packet.apid != FW_PACKET_HAND:
            continue
        message = parse_handshake_message(packet.payload)
        if message is None:
            continue
        if message.service_id != service_id or message.message_type != message_type:
            continue
        messages.append(
            HandshakeMessage(
                message_type=message.message_type,
                service_id=message.service_id,
                challenge=message.challenge,
                response=message.response,
                status_code=message.status_code,
                packet_offset=packet.offset,
            )
        )
    return messages


def load_handshake_messages_from_native_packet_log(
    packet_log_path: pathlib.Path,
    *,
    service_id: int,
    message_type: HandshakeMessageType,
) -> list[HandshakeMessage]:
    if not packet_log_path.exists():
        return []
    data = packet_log_path.read_bytes()
    offset = 0
    messages: list[HandshakeMessage] = []
    while offset + 4 <= len(data):
        packet_size = int.from_bytes(data[offset : offset + 4], "big")
        offset += 4
        if packet_size < 0 or offset + packet_size > len(data):
            break
        packet_payload = data[offset : offset + packet_size]
        packet_offset = offset - 4
        offset += packet_size
        message = parse_handshake_message(packet_payload)
        if message is None:
            continue
        if message.service_id != service_id or message.message_type != message_type:
            continue
        messages.append(
            HandshakeMessage(
                message_type=message.message_type,
                service_id=message.service_id,
                challenge=message.challenge,
                response=message.response,
                status_code=message.status_code,
                packet_offset=packet_offset,
            )
        )
    return messages


def wait_for_handshake_message(
    capture_path: pathlib.Path,
    *,
    scid: int,
    vcid: int,
    frame_size: int,
    service_id: int,
    message_type: HandshakeMessageType,
    seen_count: int,
    timeout: float,
) -> HandshakeMessage:
    deadline = time.time() + timeout
    while time.time() < deadline:
        messages = load_handshake_messages(
            capture_path,
            scid=scid,
            vcid=vcid,
            frame_size=frame_size,
            service_id=service_id,
            message_type=message_type,
        )
        if len(messages) > seen_count:
            return messages[-1]
        time.sleep(0.2)
    raise TimeoutError(f"timed out waiting for {message_type.name} on {capture_path}")
