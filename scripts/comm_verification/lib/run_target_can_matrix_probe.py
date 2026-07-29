#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import pathlib
import re
import shlex
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time
import traceback
from dataclasses import dataclass
from typing import Callable

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.data_types.file_data import DataPacketData, EndPacketData, StartPacketData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.encoders.file_encoder import FileEncoder
from fprime_gds.common.files.helpers import CFDPChecksum
from fprime_gds.common.models.dictionaries import Dictionaries
from fprime_gds.common.pipeline.standard import StandardPipeline
from fprime_gds.common.testing_fw.api import IntegrationTestAPI
from fprime_gds.common.utils.config_manager import ConfigManager
from decode_beacon_v1 import decode_beacon_v1
from probe_process_utils import (
    ManagedProcess as ProbeManagedProcess,
    build_fragment_group,
    build_gds_stale_match_groups,
    cleanup_managed_processes,
    install_signal_cleanup,
    start_managed_process,
)
from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
    SERVICE_ID_SBAND,
    SERVICE_ID_UHF,
    build_req_auth_packet,
    build_response_packet,
    build_secure_command_v2_packet,
    compute_auth_response,
    default_command_auth_keystore_path,
    load_command_auth_keystore,
    load_handshake_messages,
    load_handshake_messages_from_native_packet_log,
    request_session_key,
    send_tts_raw_packet,
)

ManagedProcess = ProbeManagedProcess


COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32

SBAND_SOURCE_ID = 1
SBAND_KEY_SLOT = 1
SBAND_KEY_BYTES = bytes.fromhex("101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F")
UHF_SOURCE_ID = 2
UHF_KEY_SLOT = 2
UHF_KEY_BYTES = bytes.fromhex("303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F")

PROCESS_TERM_TIMEOUT = 1.0
PROCESS_KILL_TIMEOUT = 1.0
BEACON_FRAME_SIZE = 108
TC_FILL_PATTERN = b"sitting well"
SBAND_CORROBORATION_SETTLE_TIMEOUT = 20.0
FORBIDDEN_COMMAND_AUTH_CLI_RE = re.compile(r"(?<!\S)--command-auth(?:\b|-)")


class ProbeFailure(RuntimeError):
    pass


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise ProbeFailure(f"missing required environment variable: {name}")
    return value


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for port {port}")


def read_text(path: pathlib.Path) -> str:
    try:
        if path.is_file():
            return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""
    return ""


def read_text_range(path: pathlib.Path, start_byte: int = 0, end_byte: int | None = None) -> str:
    try:
        if path.is_file():
            with path.open("rb") as handle:
                handle.seek(max(0, start_byte))
                data = handle.read() if end_byte is None else handle.read(max(0, end_byte - start_byte))
            return data.decode("utf-8", errors="replace")
    except OSError:
        return ""
    return ""


def file_size(path: pathlib.Path) -> int:
    try:
        if path.is_file():
            return path.stat().st_size
    except OSError:
        return 0
    return 0


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text(path)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for {fragment!r} in {path}")


def command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise ProbeFailure(f"command {name!r} not found in dictionary")


def command_opcode_optional(dictionary: dict[str, object], name: str) -> int | None:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    return None


def gds_command_packet(opcode: int, args: bytes = b"") -> bytes:
    packet = struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args
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


def ensure_no_legacy_aliases(root_dir: pathlib.Path) -> None:
    for alias in (root_dir / ".sequence-staging", root_dir / ".sequence-admitted"):
        if alias.exists() or alias.is_symlink():
            raise ProbeFailure(f"legacy checkout-level alias should not be created: {alias.name}")


def reap_local_process_pattern(pattern: str, *, term_wait_sec: float = PROCESS_TERM_TIMEOUT) -> list[int]:
    compiled = re.compile(pattern)
    matched_pids: list[int] = []
    this_pid = os.getpid()
    try:
        result = subprocess.run(
            ["ps", "-ax", "-o", "pid=", "-o", "command="],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, PermissionError, subprocess.CalledProcessError):
        return matched_pids

    for line in result.stdout.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        parts = stripped.split(None, 1)
        if len(parts) != 2:
            continue
        pid_text, command = parts
        try:
            pid = int(pid_text)
        except ValueError:
            continue
        if pid == this_pid:
            continue
        if compiled.search(command):
            matched_pids.append(pid)

    for pid in matched_pids:
        try:
            os.kill(pid, signal.SIGTERM)
        except (ProcessLookupError, PermissionError):
            pass

    deadline = time.monotonic() + term_wait_sec
    survivors = set(matched_pids)
    while survivors and time.monotonic() < deadline:
        next_survivors: set[int] = set()
        for pid in survivors:
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                continue
            except PermissionError:
                next_survivors.add(pid)
            else:
                next_survivors.add(pid)
        survivors = next_survivors
        if survivors:
            time.sleep(0.1)

    for pid in survivors:
        try:
            os.kill(pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass

    return matched_pids


def shq(value: str) -> str:
    import shlex

    return shlex.quote(value)


def ssh_command(target: str, script: str) -> list[str]:
    return ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", target, script]


def ssh_capture(target: str, script: str, check: bool = True) -> str:
    result = subprocess.run(
        ssh_command(target, script),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if check and result.returncode != 0:
        raise ProbeFailure(
            f"ssh command failed target={target} rc={result.returncode}\ncmd={script}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    return result.stdout


def ssh_capture_bytes(target: str, script: str, check: bool = True) -> bytes:
    result = subprocess.run(
        ssh_command(target, script),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and result.returncode != 0:
        raise ProbeFailure(
            f"ssh command failed target={target} rc={result.returncode}\ncmd={script}\nstdout={result.stdout.decode('utf-8', errors='replace')}\nstderr={result.stderr.decode('utf-8', errors='replace')}"
        )
    return result.stdout


def service_environment(target: str, service: str) -> dict[str, str]:
    import shlex

    text = ssh_capture(
        target,
        f"systemctl show {shq(service)} --property=Environment --value",
        check=False,
    ).strip()
    env: dict[str, str] = {}
    for token in shlex.split(text):
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        env[key] = value
    return env


def service_process_environment(target: str, service: str) -> dict[str, str]:
    show = systemctl_show(target, service, ("MainPID",))
    main_pid = show.get("MainPID", "").strip()
    if not main_pid.isdigit() or main_pid == "0":
        return {}
    text = ssh_capture(
        target,
        f"if [[ -r /proc/{main_pid}/environ ]]; then tr '\\0' '\\n' < /proc/{main_pid}/environ; fi",
        check=False,
    )
    env: dict[str, str] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        env[key] = value
    return env


def service_process_cmdlines(target: str, service: str) -> list[dict[str, str]]:
    show = systemctl_show(target, service, ("MainPID",))
    main_pid = show.get("MainPID", "").strip()
    if not main_pid.isdigit() or main_pid == "0":
        return []
    script = r"""
import json
import os
import sys

root = sys.argv[1]
parents = {}
commands = {}

for pid in [entry for entry in os.listdir("/proc") if entry.isdigit()]:
    try:
        stat_text = open(f"/proc/{pid}/stat", encoding="utf-8", errors="replace").read()
        ppid = stat_text.rsplit(") ", 1)[1].split()[1]
        data = open(f"/proc/{pid}/cmdline", "rb").read()
    except (OSError, IndexError, ValueError):
        continue
    parents.setdefault(ppid, []).append(pid)
    commands[pid] = data.replace(b"\0", b" ").decode("utf-8", errors="replace").strip()

result = []
stack = [root]
seen = set()
while stack:
    pid = stack.pop()
    if pid in seen:
        continue
    seen.add(pid)
    result.append({"pid": pid, "command": commands.get(pid, "")})
    stack.extend(parents.get(pid, []))

print(json.dumps(result, sort_keys=True))
"""
    text = ssh_capture(target, f"python3 -c {shq(script)} {shq(main_pid)}", check=False).strip()
    try:
        parsed = json.loads(text) if text else []
    except json.JSONDecodeError:
        return [{"pid": main_pid, "command": ""}]
    if not isinstance(parsed, list):
        return []
    return [
        {"pid": str(entry.get("pid", "")), "command": str(entry.get("command", ""))}
        for entry in parsed
        if isinstance(entry, dict)
    ]


def systemctl_show(target: str, unit: str, fields: tuple[str, ...]) -> dict[str, str]:
    props = " ".join(f"-p {shq(field)}" for field in fields)
    output = ssh_capture(target, f"systemctl show {props} {shq(unit)}")
    values: dict[str, str] = {}
    for line in output.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def service_invocation_id(target: str, unit: str) -> str:
    return systemctl_show(target, unit, ("InvocationID",)).get("InvocationID", "").strip()


def current_service_journal(
    target: str,
    unit: str,
    *,
    fallback_lines: int = 400,
    since: str | None = None,
) -> str:
    invocation_id = service_invocation_id(target, unit)
    if invocation_id:
        since_clause = f" --since {shq(since)}" if since else ""
        return ssh_capture(
            target,
            f"journalctl _SYSTEMD_INVOCATION_ID={shq(invocation_id)}{since_clause} --no-pager || true",
            check=False,
        )
    since_clause = f" --since {shq(since)}" if since else ""
    return ssh_capture(
        target,
        f"journalctl -u {shq(unit)}{since_clause} -n {fallback_lines} --no-pager || true",
        check=False,
    )


def command_auth_cli_hits(label: str, text: str) -> list[dict[str, str]]:
    if not text:
        return []
    hits: list[dict[str, str]] = []
    for line_number, line in enumerate(text.splitlines() or [text], start=1):
        if FORBIDDEN_COMMAND_AUTH_CLI_RE.search(line):
            hits.append({"source": label, "line": str(line_number), "text": line.strip()})
    return hits


def wait_service_active(target: str, unit: str, timeout: int) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = ssh_capture(target, f"systemctl is-active {shq(unit)} || true", check=False).strip()
        if state == "active":
            return
        time.sleep(1.0)
    raise ProbeFailure(f"{unit} on {target} did not become active")


def quiet_override_path(service: str, dropin_name: str) -> str:
    return f"/etc/systemd/system/{service}.d/{dropin_name}"


def render_service_override(env: dict[str, str]) -> str:
    lines = ["[Service]"]
    for key, value in env.items():
        lines.append(f"Environment={key}={value}")
    return "\n".join(lines) + "\n"


def apply_service_override(target: str, service: str, dropin_name: str, env: dict[str, str]) -> None:
    override_path = quiet_override_path(service, dropin_name)
    payload = render_service_override(env)
    remote_tmp = f"/tmp/{dropin_name}.tmp"
    write_result = subprocess.run(
        ssh_command(target, f"cat > {shq(remote_tmp)}"),
        input=payload,
        check=False,
        capture_output=True,
        text=True,
    )
    if write_result.returncode != 0:
        raise ProbeFailure(
            f"failed to stage override {dropin_name} for {service} on {target}: "
            f"rc={write_result.returncode}\nstdout={write_result.stdout}\nstderr={write_result.stderr}"
        )
    command = (
        f"sudo mkdir -p {shq(f'/etc/systemd/system/{service}.d')} && "
        f"sudo install -m 0644 {shq(remote_tmp)} {shq(override_path)} && "
        f"rm -f {shq(remote_tmp)} && "
        "sudo systemctl daemon-reload && "
        f"sudo systemctl restart {shq(service)}"
    )
    ssh_capture(target, command)


def remove_service_override(target: str, service: str, dropin_name: str) -> None:
    override_path = quiet_override_path(service, dropin_name)
    command = (
        f"sudo rm -f {shq(override_path)} && "
        "sudo systemctl daemon-reload && "
        f"sudo systemctl restart {shq(service)}"
    )
    ssh_capture(target, command, check=False)


def service_override_exists(target: str, service: str, dropin_name: str) -> bool:
    override_path = quiet_override_path(service, dropin_name)
    result = ssh_capture(
        target,
        f"sudo -n test -f {shq(override_path)} && echo yes || true",
        check=False,
    ).strip()
    return result == "yes"


def authority_identity_role(profile: str) -> tuple[int, int]:
    mapping = {
        "sband-primary": (1, 1),
        "uhf-primary": (2, 3),
        "uhf-backup": (2, 2),
        "dev-direct": (3, 4),
        "internal": (4, 5),
    }
    try:
        return mapping[profile]
    except KeyError as exc:
        raise ProbeFailure(f"unsupported COMMAND_AUTHORITY_PROFILE for session-floor lookup: {profile}") from exc


def next_session_id(session_minimum: int, persisted_floor: int | None) -> int:
    lower_bound = session_minimum
    if persisted_floor is not None:
        if persisted_floor >= 0xFFFFFFFF:
            raise ProbeFailure(f"persisted session floor exhausted U32 range: {persisted_floor}")
        lower_bound = max(lower_bound, persisted_floor + 1)
    candidate = max(lower_bound, int(time.time()))
    candidate += int.from_bytes(os.urandom(2), "big") % 1024
    candidate &= 0xFFFFFFFF
    if candidate == 0:
        candidate = 1
    if persisted_floor is not None and candidate <= persisted_floor:
        candidate = persisted_floor + 1
    if candidate < session_minimum:
        candidate = session_minimum
    return candidate


def read_persisted_session_floor(
    target: str,
    runtime_root: str,
    ingress_port: int,
    link_identity: int,
    link_role: int,
) -> int | None:
    script = """
import pathlib
import struct
import sys
import zlib

SNAPSHOT_MAGIC = 0x31465343
SNAPSHOT_VERSION = 1
SNAPSHOT_BYTES = 28 + (20 * 16)
ENTRY_BYTES = 20

root = pathlib.Path(sys.argv[1]) / "persistent-data" / "command-ingress"
ingress_port = int(sys.argv[2])
link_identity = int(sys.argv[3])
link_role = int(sys.argv[4])
best = None

for name in ("session-floor-a.bin", "session-floor-b.bin"):
    path = root / name
    if not path.exists():
        continue
    data = path.read_bytes()
    if len(data) != SNAPSHOT_BYTES:
        continue
    magic, version, reserved, generation, capacity, count, record_bytes, crc32 = struct.unpack_from("<IHHIIIII", data, 0)
    if magic != SNAPSHOT_MAGIC or version != SNAPSHOT_VERSION or reserved != 0 or capacity != 16 or count > 16 or record_bytes != ENTRY_BYTES:
        continue
    crc_data = bytearray(data)
    crc_data[24:28] = b"\\x00\\x00\\x00\\x00"
    if (zlib.crc32(crc_data) & 0xFFFFFFFF) != crc32:
        continue
    floor = None
    valid_count = 0
    for index in range(16):
        offset = 28 + (index * ENTRY_BYTES)
        valid, port, identity, role, session_floor = struct.unpack_from("<IIIII", data, offset)
        if valid:
            valid_count += 1
            if port == ingress_port and identity == link_identity and role == link_role:
                floor = session_floor
    if valid_count != count:
        continue
    if best is None or generation >= best[0]:
        best = (generation, floor)

print("" if best is None or best[1] is None else str(best[1]))
""".strip()
    output = ssh_capture(
        target,
        f"python3 -c {shq(script)} {shq(runtime_root)} {ingress_port} {link_identity} {link_role}",
    ).strip()
    if not output:
        return None
    return int(output)


@dataclass(frozen=True)
class AuthProfile:
    source_id: int
    key_slot: int
    key_bytes: bytes
    authority_profile: str


@dataclass
class SecureAuthSession:
    service_id: int
    ingress_port: int
    role_fragment: str
    session_key: bytes
    next_sequence: int = 41

    def accept_sequence(self) -> int:
        sequence = self.next_sequence
        self.next_sequence += 1
        return sequence

    def claim_sequence(self) -> int:
        return self.next_sequence


@dataclass(frozen=True)
class SequenceArtifact:
    source: pathlib.Path
    binary: pathlib.Path
    destination: str


@dataclass(frozen=True)
class GroundWindowMarker:
    event_line_count: int
    channels_size: int
    gateway_size: int
    gds_log_size: int
    raw_command_size: int


@dataclass(frozen=True)
class CaptureWindowMarker:
    gds_to_southbound_size: int
    southbound_to_gds_size: int


@dataclass(frozen=True)
class PhaseWindowMarker:
    sband: GroundWindowMarker | None
    uhf: GroundWindowMarker | None
    capture: CaptureWindowMarker


@dataclass(frozen=True)
class PhaseWindow:
    start: PhaseWindowMarker
    end: PhaseWindowMarker


@dataclass(frozen=True)
class HandshakeProgress:
    wire_count: int = 0
    native_count: int = 0


@dataclass(frozen=True)
class SecureCommandReadbackAttemptContext:
    attempt: int
    sequence: int
    sequence_policy: str
    command_name: str
    command_args: tuple[str, ...]
    ground: "GroundPath"
    journal_since: str
    events_log_line_count: int
    events_log_offset: int
    native_event_log_offset: int
    native_channel_log_offset: int
    capture_offset: int
    session_events_log_offset: int
    session_native_event_log_offset: int
    session_native_channel_log_offset: int
    session_capture_offset: int


@dataclass(frozen=True)
class SecureCommandReadbackJournalState:
    accepted: bool
    duplicate_reject: bool
    secure_reject: bool
    session_reject: bool
    journal_tail: str


class GroundPath:
    def __init__(self, name: str, root_dir: pathlib.Path, cli_path: pathlib.Path, dictionary_path: pathlib.Path, probe_root: pathlib.Path, vcid: int) -> None:
        self.name = name
        self.root_dir = root_dir
        self.cli_path = cli_path
        self.dictionary_path = dictionary_path
        self.gds_port = free_port()
        self.gds_tts_port = free_port()
        self.set_runtime_root(probe_root / name)
        self.processes: list[ProbeManagedProcess] = []
        self.pipeline: StandardPipeline | None = None
        self.api: IntegrationTestAPI | None = None
        self.vcid = vcid
        self.scid = int(os.environ.get("COMMV_GDS_SCID", "68"))
        self.frame_size = int(os.environ.get("COMMV_GDS_FRAME_SIZE", "4096"))
        if vcid == 1:
            self.link_up_fragment = "OBCApp.groundLinkDriver.GROUND_LINK_UP"
            self.link_down_fragment = "OBCApp.groundLinkDriver.GROUND_LINK_DOWN"
        else:
            self.link_up_fragment = "OBCApp.uhfGroundLinkDriver.GROUND_LINK_UP"
            self.link_down_fragment = "OBCApp.uhfGroundLinkDriver.GROUND_LINK_DOWN"

    def set_runtime_root(self, root: pathlib.Path) -> None:
        self.root = root
        self.gds_runtime_dir = self.root
        self.file_storage = self.root / "gds-files"
        self.gds_log = self.root / "gds.log"
        self.gateway_log = self.root / "gateway.log"
        self.events_log = self.root / "events.log"
        self.channels_log = self.root / "channels.log"
        self.process_control_dir = self.root
        self.passive_cli_control_log = self.events_log
        self.control_query_log = self.channels_log
        self.cli_log_dir = self.root / "cli-logs"
        self.passive_cli_log_dir = self.cli_log_dir / "events"
        self.raw_command_log = self.root / "raw-command.log"

    def gds_stale_match_groups(self) -> tuple[tuple[str, ...], ...]:
        return build_gds_stale_match_groups(
            ip_port=self.gds_port,
            tts_port=self.gds_tts_port,
            file_storage_dir=self.file_storage,
        )

    def gateway_stale_match_groups(self) -> tuple[tuple[str, ...], ...]:
        return (
            build_fragment_group("ground_ttc_gateway", f"--gds-port {self.gds_port}"),
        )

    @property
    def native_event_log(self) -> pathlib.Path:
        return self.passive_cli_log_dir / "event.log"

    @property
    def native_channel_log(self) -> pathlib.Path:
        return self.passive_cli_log_dir / "channel.log"

    @property
    def native_command_log(self) -> pathlib.Path:
        return self.passive_cli_log_dir / "command.log"

    @property
    def native_recv_bin(self) -> pathlib.Path:
        return self.passive_cli_log_dir / "recv.bin"

    @property
    def native_sent_bin(self) -> pathlib.Path:
        return self.passive_cli_log_dir / "sent.bin"

    def events_stale_match_groups(self) -> tuple[tuple[str, ...], ...]:
        return (
            build_fragment_group("fprime-cli events", f"--tts-port {self.gds_tts_port}"),
        )

    def start_gds(self) -> None:
        self.root.mkdir(parents=True, exist_ok=True)
        self.gds_runtime_dir.mkdir(parents=True, exist_ok=True)
        self.process_control_dir.mkdir(parents=True, exist_ok=True)
        self.file_storage.mkdir(parents=True, exist_ok=True)
        self.passive_cli_log_dir.mkdir(parents=True, exist_ok=True)
        env = os.environ.copy()
        env["DICT_PATH"] = str(self.dictionary_path)
        env["GDS_PORT"] = str(self.gds_port)
        env["GDS_TTS_PORT"] = str(self.gds_tts_port)
        env["GDS_FILE_STORAGE_DIR"] = str(self.file_storage)
        env["GDS_FRAMING_SELECTION"] = os.environ.get("COMMV_GDS_FRAMING_SELECTION", "space-packet-space-data-link")
        env["GDS_SCID"] = os.environ.get("COMMV_GDS_SCID", "68")
        env["GDS_VCID"] = os.environ.get("COMMV_GDS_VCID", str(self.vcid))
        env["GDS_FRAME_SIZE"] = os.environ.get("COMMV_GDS_FRAME_SIZE", "4096")
        env["GDS_KEEPALIVE_INTERVAL"] = os.environ.get("COMMV_GDS_KEEPALIVE_INTERVAL", "0")
        self._start(
            "ground_gds",
            ["bash", str(self.root_dir / "scripts/run_ground_gds_only_stack.sh")],
            self.gds_log,
            env,
            stale_match_groups=self.gds_stale_match_groups(),
        )
        wait_port(self.gds_port, 20.0)
        wait_port(self.gds_tts_port, 20.0)
        self._start(
            "events",
            [
                str(self.cli_path),
                "events",
                "--dictionary",
                str(self.dictionary_path),
                "--no-zmq",
                "-l",
                str(self.passive_cli_log_dir),
                "--log-directly",
                "--tts-port",
                str(self.gds_tts_port),
            ],
            self.passive_cli_control_log,
            stale_match_groups=self.events_stale_match_groups(),
        )
        time.sleep(1.0)

    def start_gateway_tcp(
        self,
        gateway_bin: pathlib.Path,
        rf_tcp_host: str,
        rf_tcp_port: int,
        link_identity: str = "sband",
        capture_dir: pathlib.Path | None = None,
    ) -> None:
        args = [
            str(gateway_bin),
            "--rf-tcp-host",
            rf_tcp_host,
            "--rf-tcp-port",
            str(rf_tcp_port),
            "--link-identity",
            link_identity,
            "--gds-host",
            "127.0.0.1",
            "--gds-port",
            str(self.gds_port),
        ]
        if capture_dir is not None:
            capture_dir.mkdir(parents=True, exist_ok=True)
            args.extend(
                [
                    "--capture-gds-to-southbound",
                    str(capture_dir / "gds-to-southbound.bin"),
                    "--capture-southbound-to-gds",
                    str(capture_dir / "southbound-to-gds.bin"),
                ]
            )
        self._start("ground_ttc_gateway", args, self.gateway_log, stale_match_groups=self.gateway_stale_match_groups())

    def start_gateway_serial(
        self,
        gateway_bin: pathlib.Path,
        serial_device: str,
        baudrate: int,
        capture_dir: pathlib.Path | None = None,
        *,
        preamble_lines: int = 0,
        preamble_delay_ms: int = 0,
    ) -> None:
        args = [
            str(gateway_bin),
            "--serial-device",
            serial_device,
            "--baudrate",
            str(baudrate),
            "--link-identity",
            "uhf",
            "--serial-tx-preamble-lines",
            str(preamble_lines),
            "--serial-tx-preamble-delay-ms",
            str(preamble_delay_ms),
            "--gds-host",
            "127.0.0.1",
            "--gds-port",
            str(self.gds_port),
        ]
        if capture_dir is not None:
            capture_dir.mkdir(parents=True, exist_ok=True)
            args.extend(
                [
                    "--capture-gds-to-southbound",
                    str(capture_dir / "gds-to-southbound.bin"),
                    "--capture-southbound-to-gds",
                    str(capture_dir / "southbound-to-gds.bin"),
                ]
            )
        self._start("ground_ttc_gateway", args, self.gateway_log, stale_match_groups=self.gateway_stale_match_groups())

    def _start(
        self,
        name: str,
        args: list[str],
        log_path: pathlib.Path,
        env: dict[str, str] | None = None,
        cwd: pathlib.Path | None = None,
        stale_match_groups: tuple[tuple[str, ...], ...] = (),
    ) -> None:
        managed = start_managed_process(
            name,
            args,
            log_path,
            env=env,
            cwd=str(cwd or self.root),
            stale_match_groups=stale_match_groups,
        )
        self.processes.append(managed)

    def connect_api(self) -> None:
        if self.pipeline is not None and self.api is not None:
            return
        (self.root / "pipeline-store").mkdir(parents=True, exist_ok=True)
        (self.root / "pipeline-logs").mkdir(parents=True, exist_ok=True)
        (self.root / "test-api").mkdir(parents=True, exist_ok=True)
        dictionaries = Dictionaries()
        dictionaries.load_dictionaries(str(self.dictionary_path), None, None)
        pipeline = StandardPipeline()
        pipeline.setup(
            ConfigManager.get_instance(),
            dictionaries,
            str(self.root / "pipeline-store"),
            logging_prefix=str(self.root / "pipeline-logs"),
        )
        recv_thread = getattr(pipeline.client_socket, "_ThreadedTransportClient__data_recv_thread", None)
        if recv_thread is not None:
            recv_thread.daemon = True
        if hasattr(pipeline.client_socket, "timeout"):
            pipeline.client_socket.timeout = float(os.environ.get("COMMV_PIPELINE_DISCONNECT_TIMEOUT_SEC", "0.2"))
        pipeline.connect(f"127.0.0.1:{self.gds_tts_port}")
        time.sleep(3.0)
        self.pipeline = pipeline
        self.api = IntegrationTestAPI(pipeline, logpath=str(self.root / "test-api"))

    def upload_file(self, local_path: pathlib.Path, destination: str) -> int:
        self.connect_api()
        if self.api is None:
            raise ProbeFailure(f"{self.name}: API connection missing")
        event_start = self.event_count()
        self.api.uplink_file(str(local_path), destination)
        return event_start

    def start_file_only(self, local_path: pathlib.Path, destination: str) -> int:
        self.connect_api()
        if self.api is None or self.pipeline is None:
            raise ProbeFailure(f"{self.name}: API connection missing")
        event_start = self.event_count()
        uplinker = self.pipeline.files.uplinker
        packet = StartPacketData(
            uplinker.get_next_sequence(),
            local_path.stat().st_size,
            str(local_path),
            destination,
        )
        uplinker.file_encoder.data_callback(packet)
        return event_start

    def send_file_packet_via_pipeline(self, packet: StartPacketData | DataPacketData | EndPacketData) -> int:
        self.connect_api()
        if self.pipeline is None:
            raise ProbeFailure(f"{self.name}: API connection missing")
        event_start = self.event_count()
        self.pipeline.files.uplinker.file_encoder.data_callback(packet)
        return event_start

    def event_count(self) -> int:
        return len(read_text(self.native_event_log).splitlines())

    def await_event(self, fragment: str, timeout: float = 10.0, start: int | None = None) -> str:
        deadline = time.time() + timeout
        start_index = 0 if start is None else start
        while time.time() < deadline:
            lines = read_text(self.native_event_log).splitlines()
            for line in lines[start_index:]:
                if fragment in line:
                    return line
            time.sleep(0.2)
        raise ProbeFailure(f"{self.name}: timed out waiting for {fragment!r}")

    def assert_no_event(self, fragment: str, timeout: float = 2.0, start: int | None = None) -> None:
        deadline = time.time() + timeout
        start_index = 0 if start is None else start
        while time.time() < deadline:
            lines = read_text(self.native_event_log).splitlines()
            for line in lines[start_index:]:
                if fragment in line:
                    raise ProbeFailure(f"{self.name}: unexpected event {fragment!r}")
            time.sleep(0.2)

    def channel_search(self, label: str, search: str) -> str:
        cmd = [
            str(self.cli_path),
            "channels",
            "--dictionary",
            str(self.dictionary_path),
            "--no-zmq",
            "-l",
            str(self.root / "control-query-logs"),
            "--log-directly",
            "--disable-data-logging",
            "--tts-port",
            str(self.gds_tts_port),
            "--search",
            search,
            "--timeout",
            "8",
        ]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False, cwd=str(self.root_dir))
        text = result.stdout
        self.control_query_log.parent.mkdir(parents=True, exist_ok=True)
        with self.control_query_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ {' '.join(cmd)} # {label}\n")
            handle.write(text)
            if text and not text.endswith("\n"):
                handle.write("\n")
            handle.write(f"returncode={result.returncode}\n")
        if result.returncode != 0 and search not in text:
            raise ProbeFailure(f"{self.name}: channel search failed for {label}")
        return text

    def latest_link_state(self) -> str | None:
        lines = read_text(self.native_event_log).splitlines()
        link_lines = [line for line in lines if self.link_up_fragment in line or self.link_down_fragment in line]
        for line in reversed(link_lines):
            if self.link_up_fragment in line:
                return "UP"
            if self.link_down_fragment in line:
                return "DOWN"
        return None

    def await_link_up_if_needed(self, timeout: float = 10.0) -> None:
        if self.latest_link_state() == "UP":
            return
        self.await_event(self.link_up_fragment, timeout=timeout, start=0)

    def wait_link_quiet(self, quiet_sec: float = 0.3, timeout: float = 10.0) -> None:
        deadline = time.time() + timeout
        last_link_event_count = -1
        quiet_start: float | None = None
        while time.time() < deadline:
            lines = read_text(self.native_event_log).splitlines()
            link_lines = [line for line in lines if self.link_up_fragment in line or self.link_down_fragment in line]
            current_count = len(link_lines)
            current_state = "UP" if link_lines and self.link_up_fragment in link_lines[-1] else "DOWN"
            if current_count != last_link_event_count:
                last_link_event_count = current_count
                quiet_start = time.time() if current_state == "UP" else None
            if current_state == "UP" and quiet_start is not None and (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.1)
        raise ProbeFailure(f"{self.name}: timed out waiting for a quiet ground-link UP window")

    def disconnect_api(self) -> None:
        if self.pipeline is not None:
            client_socket = getattr(self.pipeline, "client_socket", None)
            try:
                self.pipeline.disconnect()
            except Exception:
                try:
                    if client_socket is not None and hasattr(client_socket, "stop"):
                        client_socket.stop()
                except Exception:
                    pass
                try:
                    if client_socket is not None and hasattr(client_socket, "sock"):
                        client_socket.sock.close()
                except Exception:
                    pass
                try:
                    if self.pipeline.files is not None and self.pipeline.files.uplinker is not None:
                        self.pipeline.files.uplinker.exit()
                except Exception:
                    pass
        self.api = None
        self.pipeline = None

    def stop(self) -> None:
        self.disconnect_api()
        cleanup_managed_processes(
            self.processes,
            timeout_sec=max(PROCESS_TERM_TIMEOUT, PROCESS_KILL_TIMEOUT, 5.0),
        )

    def force_stop(self) -> None:
        self.disconnect_api()
        cleanup_managed_processes(self.processes, timeout_sec=0.5)


class TargetCanScenario:
    def __init__(self, mode: str, profile: str, probe_root: pathlib.Path) -> None:
        self.mode = mode
        self.profile = profile
        self.probe_root = probe_root
        self.root_dir = ROOT_DIR
        self.bin_dir = self.root_dir / "build-fprime-automatic-native" / "bin" / "Darwin"
        self.dictionary_path = self.root_dir / "build-fprime-automatic-native" / "OBC" / "TopCcsds" / "AppTopologyDictionary.json"
        self.cli_path = self.root_dir / "fprime-venv" / "bin" / "fprime-cli"
        self.seqgen_path = self.root_dir / "fprime-venv" / "bin" / "fprime-seqgen"
        self.gateway_bin = pathlib.Path(os.environ.get("GROUND_GATEWAY_BIN", str(self.bin_dir / "ground_ttc_gateway")))
        self.obc_target = require_env("OBC_SSH_TARGET")
        self.subsystem_target = require_env("SUBSYSTEM_SIM_SSH_TARGET")
        self.host_serial_device = require_env("HOST_SERIAL_DEVICE")
        self.comm_baudrate = int(require_env("COMM_BAUDRATE"))
        self.sband_tcp_host = require_env("SBAND_TCP_HOST")
        self.sband_tcp_port = int(require_env("SBAND_TCP_PORT"))
        self.restart_timeout = int(require_env("RESTART_TIMEOUT_SEC"))
        self.adcs_restore_timeout = int(require_env("ADCS_RESTORE_TIMEOUT_SEC"))
        self.obc_service = require_env("OBC_COMM_CSP_SERVICE_NAME")
        self.eps_service = require_env("EPS_SERVICE_NAME")
        self.adcs_service = require_env("ADCS_SERVICE_NAME")
        self.sband_stack_target = require_env("SBAND_STACK_TARGET_NAME")
        self.uhf_stack_target = require_env("UHF_STACK_TARGET_NAME")
        self.sband_comm_service = require_env("SBAND_COMM_SERVICE_NAME")
        self.uhf_comm_service = require_env("UHF_COMM_SERVICE_NAME")
        self.quiet_override_dropin_name = require_env("QUIET_OVERRIDE_DROPIN_NAME")
        self.profile_override_dropin_name = require_env("PROFILE_OVERRIDE_DROPIN_NAME")
        self.uhf_beacon_target_override_dropin_name = os.getenv("UHF_BEACON_OVERRIDE_DROPIN_NAME", "52-uhf-beacon-csp-node.conf")
        self.obc_groundlink_diagnostics_override_dropin_name = os.getenv(
            "OBC_GROUNDLINK_DIAGNOSTICS_OVERRIDE_DROPIN_NAME",
            "55-obc-groundlink-diagnostics.conf",
        )
        self.obc_groundlink_timeout_override_dropin_name = os.getenv(
            "OBC_GROUNDLINK_TIMEOUT_OVERRIDE_DROPIN_NAME",
            "56-obc-groundlink-timeouts.conf",
        )
        self.csp_socketcan_canfd_override_dropin_name = os.getenv(
            "CSP_SOCKETCAN_CANFD_OVERRIDE_DROPIN_NAME",
            "57-csp-socketcan-canfd.conf",
        )
        self.uhf_ingress_diagnostics_override_dropin_name = os.getenv(
            "UHF_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME",
            "54-uhf-ingress-diagnostics.conf",
        )
        self.sband_ingress_diagnostics_override_dropin_name = os.getenv(
            "SBAND_INGRESS_DIAGNOSTICS_OVERRIDE_DROPIN_NAME",
            "58-sband-ingress-diagnostics.conf",
        )
        self.reliable_transfer_receiver_override_dropin_name = os.getenv(
            "RELIABLE_TRANSFER_RECEIVER_OVERRIDE_DROPIN_NAME",
            "52-reliable-transfer-output.conf",
        )
        self.reliable_transfer_admission_override_dropin_name = os.getenv(
            "RELIABLE_TRANSFER_ADMISSION_OVERRIDE_DROPIN_NAME",
            "53-obc-reliable-transfer-admission.conf",
        )
        self.target_service_profile = require_env("TARGET_SERVICE_PROFILE")
        self.target_service_command_authority_profile = require_env("TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE")
        self.target_service_initial_comm_band = require_env("TARGET_SERVICE_INITIAL_COMM_BAND")
        self.target_service_enable_primary_ground_link_driver = require_env("TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER")
        self.target_command_authority_profile = require_env("TARGET_COMMAND_AUTHORITY_PROFILE")
        self.target_comm_csp_node = int(require_env("TARGET_COMM_CSP_NODE"))
        self.target_requires_uhf_primary_switch = require_env("TARGET_REQUIRES_UHF_PRIMARY_SWITCH") == "1"
        self.capture_dir = self.probe_root / "captures"
        self.diagnostics_dir = self.probe_root / "diagnostics"
        self.service_snapshot_dir = self.diagnostics_dir / "service-snapshots"
        self.journal_snapshot_dir = self.diagnostics_dir / "journal-snapshots"
        self.checkpoints_log = self.diagnostics_dir / "checkpoints.jsonl"
        self.status_log = self.probe_root / "status.log"
        self.sequence_src_dir = self.probe_root / "sequence-src"
        self.sequence_bin_dir = self.probe_root / "sequence-bin"
        self.packet_audit_dir = self.probe_root / "packet-audit"
        self.opcodes: dict[str, int] = {}
        self.command_dictionaries: Dictionaries | None = None
        self.command_encoder: CmdEncoder | None = None
        self.file_encoder = FileEncoder()
        self.baseline_target_comm_profile = "sband"
        self.baseline_managed_externally = os.getenv("TARGET_BASELINE_MANAGED_EXTERNALLY", "0") == "1"
        self.require_external_uhf_service = os.getenv("TARGET_BASELINE_REQUIRE_UHF_SERVICE", "1") == "1"
        self.secure_auth_proof_include_file_downlink = os.getenv("SECURE_AUTH_PROOF_INCLUDE_FILE_DOWNLINK", "0") == "1"
        self.profile_override_applied = False
        self.quiet_override_applied = False
        self.reliable_transfer_receiver_override_applied = False
        self.reliable_transfer_admission_override_applied = False
        self.beacon_capture_override_applied = False
        self.uhf_beacon_target_override_applied = False
        self.obc_groundlink_diagnostics_override_applied = False
        self.obc_groundlink_timeout_override_applied = False
        self.csp_socketcan_canfd_override_applied = False
        self.uhf_ingress_diagnostics_override_applied = False
        self.sband_ingress_diagnostics_override_applied = False
        self.obc_groundlink_diagnostics = os.getenv(
            "OBC_GROUNDLINK_DIAGNOSTICS",
            "1" if self.mode in ("nonquiet-diagnosis", "dual-link-proof") else "0",
        )
        self.obc_groundlink_uplink_poll_timeout_ms = os.getenv("COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS", "")
        self.obc_groundlink_downlink_write_timeout_ms = os.getenv("COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS", "")
        self.obc_groundlink_health_timeout_ms = os.getenv("COMM_GROUNDLINK_HEALTH_TIMEOUT_MS", "")
        self.obc_groundlink_downlink_v3_max_data_bytes = os.getenv("COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES", "")
        self.obc_groundlink_downlink_v3_window_frames_override = os.getenv(
            "COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE", ""
        )
        self.obc_groundlink_downlink_v3_send_probe_wait_ms = os.getenv(
            "COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS", ""
        )
        self.obc_groundlink_downlink_v3_send_probe_retry_sleep_ms = os.getenv(
            "COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS", ""
        )
        self.obc_groundlink_downlink_v3_interframe_delay_usec = os.getenv(
            "COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC", ""
        )
        self.obc_csp_socketcan_tx_frame_delay_usec = os.getenv(
            "COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC", ""
        )
        self.csp_socketcan_use_canfd = os.getenv("COMM_CSP_SOCKETCAN_USE_CANFD", "")
        self.csp_socketcan_canfd_dest_allowlist = os.getenv(
            "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST", ""
        )
        self.csp_socketcan_canfd_dport_allowlist = os.getenv(
            "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST", ""
        )
        self.uhf_comm_node_ingress_diagnostics = os.getenv(
            "UHF_COMM_NODE_INGRESS_DIAGNOSTICS",
            os.getenv("COMM_NODE_INGRESS_DIAGNOSTICS", "1" if self.mode in ("nonquiet-diagnosis", "dual-link-proof") else "0"),
        )
        self.sband_comm_node_ingress_diagnostics = os.getenv(
            "SBAND_COMM_NODE_INGRESS_DIAGNOSTICS",
            "1",
        )
        self.nonquiet_beacon_capture_enabled = os.getenv("TARGET_CAN_NONQUIET_BEACON_CAPTURE", "0") == "1"
        self.subsystem_probe_baseline: dict[str, str] | None = None
        self.runtime_root = ""
        self.reliable_transfer_receiver_service = self.sband_comm_service
        self.reliable_transfer_output_dir = "/tmp/comm-reliable-transfer-node5"
        self.remote_beacon_capture_path = ""
        self.remote_beacon_service_device = ""
        self.remote_beacon_peer_device = ""
        self.remote_beacon_working_directory = ""
        self.remote_beacon_bridge_process: subprocess.Popen[str] | None = None
        self.remote_beacon_capture_process: subprocess.Popen[str] | None = None
        self.security_server_socket = pathlib.Path(f"/tmp/target-secure-auth-{os.getpid()}.sock")
        self.security_server_log = self.diagnostics_dir / "security-server-sim.log"
        self.installed_root = os.getenv("RPI_INSTALL_ROOT", "/home/operator/obc-deploy")
        self.sband = GroundPath("sband-ground", self.root_dir, self.cli_path, self.dictionary_path, self.probe_root, vcid=1)
        self.uhf = GroundPath("uhf-ground", self.root_dir, self.cli_path, self.dictionary_path, self.probe_root, vcid=2)

    def note(self, message: str) -> None:
        self.probe_root.mkdir(parents=True, exist_ok=True)
        with self.status_log.open("a", encoding="utf-8") as handle:
            handle.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')} {message}\n")

    def checkpoint(self, name: str, status: str = "info", **details: object) -> None:
        self.diagnostics_dir.mkdir(parents=True, exist_ok=True)
        record: dict[str, object] = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "name": name,
            "status": status,
        }
        if details:
            record["details"] = details
        with self.checkpoints_log.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, sort_keys=True) + "\n")

    def write_json_artifact(self, path: pathlib.Path, payload: object) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def remote_sha256(self, target: str, path: str) -> str:
        script = "import hashlib,sys; data=open(sys.argv[1],'rb').read(); print(hashlib.sha256(data).hexdigest())"
        return ssh_capture(target, f"python3 -c {shq(script)} {shq(path)}").strip()

    def wait_unix_socket(self, path: pathlib.Path, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if path.exists():
                return
            time.sleep(0.1)
        raise ProbeFailure(f"timed out waiting for Unix socket {path}")

    def start_security_server(self) -> None:
        if self.security_server_socket.exists():
            self.security_server_socket.unlink()
        self.security_server_log.parent.mkdir(parents=True, exist_ok=True)
        managed = start_managed_process(
            "security_server_sim",
            [
                str(self.cli_path.parent / "python"),
                str(self.root_dir / "scripts/security_server_sim.py"),
                "--socket-path",
                str(self.security_server_socket),
            ],
            self.security_server_log,
            cwd=str(self.root_dir),
            stale_match_groups=(
                build_fragment_group("security_server_sim.py", str(self.security_server_socket)),
            ),
        )
        self.sband.processes.append(managed)
        self.wait_unix_socket(self.security_server_socket, 10.0)
        self.checkpoint("security-server-sim-started", "pass", socket=str(self.security_server_socket))

    def gateway_capture_dir_for_ground(self, ground: GroundPath) -> pathlib.Path | None:
        if self.mode == "secure-auth-proof":
            return self.capture_dir / ("sband" if ground is self.sband else "uhf")
        if ground is self.uhf:
            return self.capture_dir
        return None

    def capture_path(self, ground: GroundPath, direction: str) -> pathlib.Path:
        capture_dir = self.gateway_capture_dir_for_ground(ground)
        if capture_dir is None:
            capture_dir = self.capture_dir
        return capture_dir / f"{direction}.bin"

    def secure_capture_summary(self) -> dict[str, object]:
        return {
            "sband": {
                "gdsToSouthbound": self.summarize_capture_file(self.capture_path(self.sband, "gds-to-southbound")),
                "southboundToGds": self.summarize_capture_file(self.capture_path(self.sband, "southbound-to-gds")),
            },
            "uhf": {
                "gdsToSouthbound": self.summarize_capture_file(self.capture_path(self.uhf, "gds-to-southbound")),
                "southboundToGds": self.summarize_capture_file(self.capture_path(self.uhf, "southbound-to-gds")),
            },
        }

    def record_secure_auth_provenance(self) -> dict[str, object]:
        repo_keystore_path = default_command_auth_keystore_path(self.root_dir)
        load_command_auth_keystore(repo_keystore_path)
        repo_keystore_sha = self.sha256_file(repo_keystore_path)
        current_link = f"{self.installed_root}/current"
        current_is_symlink = ssh_capture(self.obc_target, f"test -L {shq(current_link)} && echo yes || echo no", check=False).strip()
        current_resolved = ssh_capture(self.obc_target, f"readlink -f {shq(current_link)}", check=False).strip()
        obc_show = systemctl_show(
            self.obc_target,
            self.obc_service,
            ("Id", "ActiveState", "SubState", "WorkingDirectory", "ExecStart", "Environment", "MainPID"),
        )
        working_directory = obc_show.get("WorkingDirectory", "").strip()
        working_directory_resolved = (
            ssh_capture(self.obc_target, f"readlink -f {shq(working_directory)}", check=False).strip()
            if working_directory
            else ""
        )
        remote_keystore_path = f"{current_link}/config/security/command-auth.ini"
        remote_manifest_path = f"{current_link}/manifest.json"
        remote_keystore_sha = self.remote_sha256(self.obc_target, remote_keystore_path)
        remote_manifest_sha = self.remote_sha256(self.obc_target, remote_manifest_path)
        manifest_text = ssh_capture(self.obc_target, f"cat {shq(remote_manifest_path)}")
        manifest = json.loads(manifest_text)
        manifest_keystore_sha = (
            manifest.get("files", {})
            .get("config/security/command-auth.ini", {})
            .get("sha256")
        )
        service_env = service_environment(self.obc_target, self.obc_service)
        process_env = service_process_environment(self.obc_target, self.obc_service)
        process_cmdlines = service_process_cmdlines(self.obc_target, self.obc_service)
        combined_env = {**service_env, **process_env}
        forbidden_env = sorted(key for key in combined_env if key.startswith("COMMAND_AUTH_"))
        launch_wrapper_path = f"{current_link}/launch/run_obc_comm_csp_stack.sh"
        launch_wrapper_text = ssh_capture(self.obc_target, f"cat {shq(launch_wrapper_path)}", check=False)
        forbidden_cli = command_auth_cli_hits("systemd-ExecStart", obc_show.get("ExecStart", ""))
        forbidden_cli.extend(command_auth_cli_hits(launch_wrapper_path, launch_wrapper_text))
        for entry in process_cmdlines:
            forbidden_cli.extend(command_auth_cli_hits(f"process:{entry.get('pid', '')}", entry.get("command", "")))
        expected_comm_node = "5" if self.target_service_profile == "sband" else str(self.target_comm_csp_node)
        expected_service_env = {
            "TARGET_COMM_PROFILE": self.target_service_profile,
            "COMM_CSP_NODE": expected_comm_node,
            "COMMAND_AUTHORITY_PROFILE": self.target_service_command_authority_profile,
        }
        service_env_mismatches = {
            key: {"expected": value, "observed": service_env.get(key, "")}
            for key, value in expected_service_env.items()
            if service_env.get(key, "") != value
        }
        subsystem_states = {
            "sbandStack": systemctl_show(self.subsystem_target, self.sband_stack_target, ("Id", "ActiveState", "SubState")),
            "sbandComm": systemctl_show(self.subsystem_target, self.sband_comm_service, ("Id", "ActiveState", "SubState")),
            "uhfStack": systemctl_show(self.subsystem_target, self.uhf_stack_target, ("Id", "ActiveState", "SubState")),
            "uhfComm": systemctl_show(self.subsystem_target, self.uhf_comm_service, ("Id", "ActiveState", "SubState")),
        }
        failures: list[str] = []
        if current_is_symlink != "yes":
            failures.append("installed-current-is-not-symlink")
        if not current_resolved:
            failures.append("installed-current-target-unresolved")
        if working_directory != current_link:
            failures.append("service-working-directory-not-current-symlink")
        if working_directory_resolved != current_resolved:
            failures.append("service-working-directory-resolved-target-mismatch")
        if obc_show.get("ActiveState") != "active":
            failures.append("obc-service-not-active")
        if remote_keystore_sha != repo_keystore_sha:
            failures.append("bundled-keystore-sha-does-not-match-repo-tracked-keystore")
        if manifest_keystore_sha != remote_keystore_sha:
            failures.append("manifest-keystore-sha-does-not-match-bundled-keystore")
        if forbidden_env:
            failures.append("forbidden-command-auth-runtime-env-present")
        if forbidden_cli:
            failures.append("forbidden-command-auth-runtime-cli-present")
        if service_env_mismatches:
            failures.append("target-service-env-does-not-match-proof-profile")
        if subsystem_states["sbandComm"].get("ActiveState") != "active":
            failures.append("sband-comm-service-not-active")

        payload: dict[str, object] = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "verdict": "PASS" if not failures else "FAIL",
            "failures": failures,
            "repoKeystore": {
                "path": str(repo_keystore_path),
                "sha256": repo_keystore_sha,
            },
            "installedRelease": {
                "installRoot": self.installed_root,
                "currentLink": current_link,
                "currentIsSymlink": current_is_symlink == "yes",
                "currentResolved": current_resolved,
                "manifestPath": remote_manifest_path,
                "manifestSha256": remote_manifest_sha,
                "manifestReleaseId": manifest.get("release_id"),
                "manifestKeystoreSha256": manifest_keystore_sha,
                "bundledKeystorePath": remote_keystore_path,
                "bundledKeystoreSha256": remote_keystore_sha,
            },
            "targetService": {
                "target": self.obc_target,
                "unit": self.obc_service,
                "show": obc_show,
                "workingDirectory": working_directory,
                "workingDirectoryResolved": working_directory_resolved,
                "environment": service_env,
                "processEnvironmentCommandAuthKeys": {
                    key: process_env[key]
                    for key in sorted(process_env)
                    if key.startswith("COMMAND_AUTH")
                },
                "processCommandLines": process_cmdlines,
                "expectedEnvironment": expected_service_env,
                "environmentMismatches": service_env_mismatches,
                "forbiddenCommandAuthEnvironment": forbidden_env,
                "forbiddenCommandAuthCli": forbidden_cli,
                "launchWrapperPath": launch_wrapper_path,
            },
            "subsystemServices": {
                "target": self.subsystem_target,
                "states": subsystem_states,
            },
            "acceptanceNote": "Existing installed release accepted only if this gate is PASS.",
        }
        path = self.diagnostics_dir / "secure-auth-keystore-provenance.json"
        self.write_json_artifact(path, payload)
        self.checkpoint("secure-auth-provenance-gate", "pass" if not failures else "fail", artifact=str(path), failures=failures)
        if failures:
            raise ProbeFailure("secure auth provenance gate failed: " + ", ".join(failures))
        return payload

    def snapshot_service(self, target: str, unit: str, label: str) -> None:
        fields = (
            "Id",
            "LoadState",
            "ActiveState",
            "SubState",
            "UnitFileState",
            "FragmentPath",
            "DropInPaths",
            "ExecStart",
            "Environment",
            "MainPID",
        )
        payload = {
            "target": target,
            "unit": unit,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "show": systemctl_show(target, unit, fields),
            "environment": service_environment(target, unit),
            "processEnvironment": service_process_environment(target, unit),
            "cat": ssh_capture(target, f"systemctl cat {shq(unit)}", check=False),
        }
        self.write_json_artifact(self.service_snapshot_dir / f"{label}.json", payload)

    def snapshot_service_set(self, stage: str) -> None:
        self.snapshot_service(self.obc_target, self.obc_service, f"{stage}-obc-service")
        self.snapshot_service(self.subsystem_target, self.sband_comm_service, f"{stage}-sband-comm")
        self.snapshot_service(self.subsystem_target, self.uhf_comm_service, f"{stage}-uhf-comm")
        self.snapshot_service(self.subsystem_target, self.sband_stack_target, f"{stage}-sband-stack")
        self.snapshot_service(self.subsystem_target, self.uhf_stack_target, f"{stage}-uhf-stack")

    def snapshot_journal(self, target: str, unit: str, label: str, lines: int = 120) -> None:
        text = ssh_capture(
            target,
            f"journalctl -u {shq(unit)} -n {lines} --no-pager || true",
            check=False,
        )
        path = self.journal_snapshot_dir / f"{label}.log"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def record_bootstrap_state(self, stage: str, require_uhf: bool) -> None:
        journal = ssh_capture(
            self.obc_target,
            f"journalctl -u {shq(self.obc_service)} -n 400 --no-pager || true",
            check=False,
        )
        checkpoint = {
            "require_uhf": require_uhf,
            "runtime_started": "OBC CCSDS S-band runtime started." in journal,
            "sband_ground_link_up": "groundLinkDriver) GROUND_LINK_UP" in journal,
            "sband_available": "COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available 1" in journal,
            "uhf_ground_link_up": "uhfGroundLinkDriver) GROUND_LINK_UP" in journal,
            "ground_link_up_count": len(re.findall(r"groundLinkDriver\) GROUND_LINK_UP", journal)),
            "ground_link_down_count": len(re.findall(r"groundLinkDriver\) GROUND_LINK_DOWN", journal)),
            "uhf_link_up_count": len(re.findall(r"uhfGroundLinkDriver\) GROUND_LINK_UP", journal)),
            "uhf_link_down_count": len(re.findall(r"uhfGroundLinkDriver\) GROUND_LINK_DOWN", journal)),
        }
        self.checkpoint(stage, "pass", **checkpoint)
        self.snapshot_journal(self.obc_target, self.obc_service, stage)

    def start(self) -> None:
        if self.probe_root.exists():
            shutil.rmtree(self.probe_root)
        self.probe_root.mkdir(parents=True, exist_ok=True)
        self.sequence_src_dir.mkdir(parents=True, exist_ok=True)
        self.sequence_bin_dir.mkdir(parents=True, exist_ok=True)
        self.diagnostics_dir.mkdir(parents=True, exist_ok=True)
        ensure_no_legacy_aliases(self.root_dir)
        if not self.bin_dir.exists() or not self.dictionary_path.exists() or not self.cli_path.exists() or not self.seqgen_path.exists():
            raise ProbeFailure("Required build outputs or F Prime tools are missing.")
        baseline_env = service_environment(self.obc_target, self.obc_service)
        self.baseline_target_comm_profile = baseline_env.get("TARGET_COMM_PROFILE", "sband")
        self.runtime_root = baseline_env.get("RUNTIME_ROOT", "/home/operator/obc-deploy/runtime/comm-csp-lab-obc")
        self.checkpoint(
            "probe-start",
            "pass",
            mode=self.mode,
            profile=self.profile,
            initial_comm_profile=self.baseline_target_comm_profile,
            runtime_root=self.runtime_root,
            requested_baudrate=self.comm_baudrate,
            host_serial_device=self.host_serial_device,
        )
        self.snapshot_service_set("probe-start")
        self.load_opcodes()

    def load_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        legacy_session_open_opcode = command_opcode_optional(dictionary, "OBCApp.commandIngressAuthority.SESSION_OPEN")
        self.legacy_session_open_supported = legacy_session_open_opcode is not None
        self.opcodes = {
            "OBCApp.commController.COMM_SET_ACTIVE": command_opcode(dictionary, "OBCApp.commController.COMM_SET_ACTIVE"),
            "OBCApp.modeManager.MODE_GET": command_opcode(dictionary, "OBCApp.modeManager.MODE_GET"),
            "OBCApp.modeManager.MODE_SET": command_opcode(dictionary, "OBCApp.modeManager.MODE_SET"),
            "OBCApp.bootManager.GET_RESET_CAUSE": command_opcode(dictionary, "OBCApp.bootManager.GET_RESET_CAUSE"),
            "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY": command_opcode(
                dictionary, "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY"
            ),
            "OBCApp.dpCatalog.BUILD_CATALOG": command_opcode(dictionary, "OBCApp.dpCatalog.BUILD_CATALOG"),
            "OBCApp.dpCatalog.START_XMIT_CATALOG": command_opcode(dictionary, "OBCApp.dpCatalog.START_XMIT_CATALOG"),
            "OBCApp.sequenceAdmissionController.SEQ_VALIDATE": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_VALIDATE"),
            "OBCApp.sequenceAdmissionController.SEQ_RUN": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_RUN"),
            "OBCApp.epsBridge.EPS_GET_STATUS": command_opcode(dictionary, "OBCApp.epsBridge.EPS_GET_STATUS"),
            "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": command_opcode(dictionary, "OBCApp.adcsBridge.ADCS_GET_ATTITUDE"),
            "OBCApp.gpsBridge.GPS_GET_STATE": command_opcode(dictionary, "OBCApp.gpsBridge.GPS_GET_STATE"),
            "OBCApp.gpsBridge.GPS_SET_SOURCE_MODE": command_opcode(dictionary, "OBCApp.gpsBridge.GPS_SET_SOURCE_MODE"),
            "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH": command_opcode(
                dictionary, "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH"
            ),
            "OBCApp.radioController.RADIO_GET_STATUS": command_opcode(dictionary, "OBCApp.radioController.RADIO_GET_STATUS"),
            "OBCApp.storageHealthBridge.STORAGE_GET_STATUS": command_opcode(
                dictionary, "OBCApp.storageHealthBridge.STORAGE_GET_STATUS"
            ),
        }
        if legacy_session_open_opcode is not None:
            self.opcodes["OBCApp.commandIngressAuthority.SESSION_OPEN"] = legacy_session_open_opcode
        dictionaries = Dictionaries()
        dictionaries.load_dictionaries(str(self.dictionary_path), None, None)
        self.command_dictionaries = dictionaries
        self.command_encoder = CmdEncoder()

    def encode_inner_command(self, command_name: str, *args: str) -> bytes:
        if self.command_dictionaries is None or self.command_encoder is None:
            raise ProbeFailure("command dictionary is not initialized")
        if command_name not in self.command_dictionaries.command_name:
            raise ProbeFailure(
                f"command {command_name} is not present in the current dictionary; "
                "this probe path still depends on a retired public command surface"
            )
        template = self.command_dictionaries.command_name[command_name]
        encoded = self.command_encoder.encode_api(CmdData(tuple(args), template))
        return encoded[8:]

    def require_legacy_session_open_surface(self, reason: str) -> None:
        if self.legacy_session_open_supported:
            return
        raise ProbeFailure(
            "this probe path still depends on retired public SESSION_OPEN ingress; "
            f"{reason}"
        )

    def authority_profile(self, name: str) -> AuthProfile:
        mapping = {
            "sband-primary": AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES, "sband-primary"),
            "uhf-primary": AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES, "uhf-primary"),
            "uhf-backup": AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES, "uhf-backup"),
        }
        try:
            return mapping[name]
        except KeyError as exc:
            raise ProbeFailure(f"unsupported authority profile {name}") from exc

    def prepare_subsystem_environment(self, requested_profile: str) -> dict[str, str]:
        baseline = {
            self.sband_stack_target: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.sband_stack_target)} || true", check=False).strip(),
            self.uhf_stack_target: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.uhf_stack_target)} || true", check=False).strip(),
            self.sband_comm_service: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.sband_comm_service)} || true", check=False).strip(),
            self.uhf_comm_service: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.uhf_comm_service)} || true", check=False).strip(),
            self.eps_service: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.eps_service)} || true", check=False).strip(),
            self.adcs_service: ssh_capture(self.subsystem_target, f"systemctl is-active {shq(self.adcs_service)} || true", check=False).strip(),
        }
        self.write_json_artifact(self.service_snapshot_dir / f"subsystem-baseline-{requested_profile}.json", baseline)
        self.checkpoint("subsystem-baseline-snapshot", "pass", requested_profile=requested_profile, states=baseline)
        if self.baseline_managed_externally:
            required_units = [
                self.sband_stack_target,
                self.sband_comm_service,
                self.eps_service,
                self.adcs_service,
            ]
            if self.require_external_uhf_service:
                required_units.extend([self.uhf_stack_target, self.uhf_comm_service])
            inactive = [unit for unit in required_units if baseline.get(unit, "").strip() != "active"]
            if inactive:
                raise ProbeFailure(
                    "externally managed target baseline is not ready; inactive units: " + ", ".join(inactive)
                )
            self.snapshot_service_set(f"subsystem-external-baseline-{requested_profile}")
            self.checkpoint(
                "subsystem-baseline-external-ready",
                "pass",
                requested_profile=requested_profile,
                require_uhf=self.require_external_uhf_service,
            )
            return baseline
        ssh_capture(
            self.subsystem_target,
            f"sudo systemctl stop {shq(self.uhf_stack_target)} >/dev/null 2>&1 || true ; "
            f"sudo systemctl start {shq(self.sband_stack_target)} ; "
            f"sudo systemctl restart {shq(self.sband_comm_service)} ; "
            f"sudo systemctl start {shq(self.eps_service)} ; "
            f"sudo systemctl start {shq(self.adcs_service)}",
        )
        wait_service_active(self.subsystem_target, self.sband_stack_target, self.adcs_restore_timeout)
        wait_service_active(self.subsystem_target, self.sband_comm_service, self.adcs_restore_timeout)
        wait_service_active(self.subsystem_target, self.eps_service, self.adcs_restore_timeout)
        wait_service_active(self.subsystem_target, self.adcs_service, self.adcs_restore_timeout)
        self.snapshot_service_set(f"subsystem-prepared-{requested_profile}")
        self.checkpoint("node5-bootstrap-service-ready", "pass", requested_profile=requested_profile)
        return baseline

    def ensure_uhf_service_ready(self) -> None:
        if self.baseline_managed_externally:
            wait_service_active(self.subsystem_target, self.uhf_comm_service, self.adcs_restore_timeout)
            self.wait_for_target_current_ready_state(
                require_uhf=True,
                timeout=20,
                label="current externally managed target node6 readiness",
            )
            self.snapshot_service_set("node6-service-ready-external")
            self.record_bootstrap_state("node6-ready", require_uhf=True)
            return
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        self.checkpoint("node6-service-restart-begin", "info", baudrate=self.comm_baudrate)
        ssh_capture(self.subsystem_target, f"sudo systemctl restart {shq(self.uhf_comm_service)}")
        wait_service_active(self.subsystem_target, self.uhf_comm_service, self.adcs_restore_timeout)
        self.wait_for_target_link_readiness(
            self.obc_service,
            journal_since,
            ("uhfGroundLinkDriver) GROUND_LINK_UP",),
            20,
            "target node 6 service journal output",
        )
        self.snapshot_service_set("node6-service-ready")
        self.record_bootstrap_state("node6-ready", require_uhf=True)

    def restart_uhf_service_for_probe_only(self) -> None:
        if self.baseline_managed_externally:
            wait_service_active(self.subsystem_target, self.uhf_comm_service, self.adcs_restore_timeout)
            self.snapshot_service_set("node6-service-active-external")
            self.record_bootstrap_state("node6-active", require_uhf=False)
            return
        self.checkpoint("node6-service-restart-begin", "info", baudrate=self.comm_baudrate)
        ssh_capture(self.subsystem_target, f"sudo systemctl restart {shq(self.uhf_comm_service)}")
        wait_service_active(self.subsystem_target, self.uhf_comm_service, self.adcs_restore_timeout)
        self.snapshot_service_set("node6-service-active")
        self.record_bootstrap_state("node6-active", require_uhf=False)

    def prepare_uhf_service_for_switch(self, *, require_pre_switch_ping: bool, boundary: str) -> None:
        if require_pre_switch_ping:
            self.ensure_uhf_service_ready()
        else:
            self.restart_uhf_service_for_probe_only()
        self.checkpoint(
            "node6-switch-boundary",
            "pass",
            boundary=boundary,
            require_pre_switch_ping=require_pre_switch_ping,
        )

    def remote_service_working_directory(self, target: str, unit: str) -> str:
        return ssh_capture(
            target,
            f"systemctl show {shq(unit)} --property=WorkingDirectory --value",
        ).strip()

    def start_remote_beacon_capture(self) -> None:
        if self.remote_beacon_capture_path:
            return
        working_directory = self.remote_service_working_directory(self.subsystem_target, self.uhf_comm_service)
        if not working_directory:
            raise ProbeFailure(f"unable to resolve WorkingDirectory for {self.uhf_comm_service}")
        self.remote_beacon_working_directory = working_directory
        remote_bridge_log = self.diagnostics_dir / "remote-beacon-pty-bridge.log"
        bridge_handle = remote_bridge_log.open("w", encoding="utf-8", buffering=1)
        bridge_command = (
            f"cd {shq(working_directory)} && "
            f"{shq(working_directory + '/build-fprime-automatic-native/bin/Linux/pty_pair_bridge')}"
        )
        bridge_process = subprocess.Popen(
            ssh_command(self.subsystem_target, bridge_command),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        self.remote_beacon_bridge_process = bridge_process
        self.uhf.processes.append(
            ProbeManagedProcess(
                name="remote_beacon_pty_bridge",
                process=bridge_process,
                handle=bridge_handle,
            )
        )
        if bridge_process.stdout is None:
            raise ProbeFailure("remote beacon PTY bridge did not expose stdout")
        pty_paths: dict[str, str] = {}
        for _ in range(2):
            line = bridge_process.stdout.readline()
            if not line:
                raise ProbeFailure("remote beacon PTY bridge did not report PTY paths")
            bridge_handle.write(line)
            key, value = line.strip().split("=", 1)
            pty_paths[key] = value
        self.remote_beacon_service_device = pty_paths["PTY_A"]
        self.remote_beacon_peer_device = pty_paths["PTY_B"]
        self.remote_beacon_capture_path = f"/tmp/{self.probe_root.name}-remote-uhf-beacon.bin"

        remote_capture_log = self.diagnostics_dir / "remote-beacon-capture.log"
        capture_handle = remote_capture_log.open("w", encoding="utf-8", buffering=1)
        capture_script = """
import os
import sys
import termios
import time

device = sys.argv[1]
output = sys.argv[2]
fd = os.open(device, os.O_RDONLY | os.O_NOCTTY)
attrs = termios.tcgetattr(fd)
attrs[3] = attrs[3] & ~(termios.ICANON | termios.ECHO)
attrs[6][termios.VMIN] = 0
attrs[6][termios.VTIME] = 0
termios.tcsetattr(fd, termios.TCSANOW, attrs)
os.set_blocking(fd, False)
with open(output, "wb") as handle:
    while True:
        try:
            chunk = os.read(fd, 4096)
            if chunk:
                handle.write(chunk)
                handle.flush()
            else:
                time.sleep(0.05)
        except BlockingIOError:
            time.sleep(0.05)
"""
        capture_command = (
            f"rm -f {shq(self.remote_beacon_capture_path)} && "
            f"python3 -c {shq(capture_script)} {shq(self.remote_beacon_peer_device)} {shq(self.remote_beacon_capture_path)}"
        )
        capture_process = subprocess.Popen(
            ssh_command(self.subsystem_target, capture_command),
            stdin=subprocess.DEVNULL,
            stdout=capture_handle,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        self.remote_beacon_capture_process = capture_process
        self.uhf.processes.append(
            ProbeManagedProcess(
                name="remote_beacon_capture",
                process=capture_process,
                handle=capture_handle,
            )
        )
        self.checkpoint(
            "remote-beacon-capture-started",
            "pass",
            working_directory=working_directory,
            service_device=self.remote_beacon_service_device,
            peer_device=self.remote_beacon_peer_device,
            capture_path=self.remote_beacon_capture_path,
        )

    def remote_beacon_frame_count(self) -> int:
        if not self.remote_beacon_capture_path:
            raise ProbeFailure("remote beacon capture is not configured")
        text = ssh_capture(
            self.subsystem_target,
            (
                "python3 -c "
                + shq(
                    "import os,sys; path=sys.argv[1]; size=os.path.getsize(path) if os.path.exists(path) else 0; print(size // %d)"
                    % BEACON_FRAME_SIZE
                )
                + f" {shq(self.remote_beacon_capture_path)}"
            ),
            check=False,
        ).strip()
        return 0 if not text else int(text)

    def wait_for_remote_beacon_frame(self, previous_count: int, timeout_sec: float) -> int:
        deadline = time.time() + timeout_sec
        while time.time() < deadline:
            current = self.remote_beacon_frame_count()
            if current > previous_count:
                return current
            time.sleep(0.25)
        raise ProbeFailure(f"timed out waiting for remote beacon frame after count {previous_count}")

    def assert_no_remote_beacon_growth(self, previous_count: int, quiet_sec: float) -> None:
        deadline = time.time() + quiet_sec
        while time.time() < deadline:
            current = self.remote_beacon_frame_count()
            if current > previous_count:
                raise ProbeFailure(
                    f"unexpected remote beacon growth during suppress window: previous={previous_count} current={current}"
                )
            time.sleep(0.25)

    def apply_uhf_beacon_capture_override(self) -> None:
        if self.beacon_capture_override_applied:
            return
        self.start_remote_beacon_capture()
        apply_service_override(
            self.subsystem_target,
            self.uhf_comm_service,
            "53-uhf-beacon-capture.conf",
            {
                "SUBSYSTEM_SIM_COMM_BEACON_DEVICE": self.remote_beacon_service_device,
                "COMM_BEACON_BAUDRATE": str(self.comm_baudrate),
            },
        )
        self.beacon_capture_override_applied = True
        self.checkpoint(
            "uhf-beacon-capture-override-applied",
            "pass",
            service=self.uhf_comm_service,
            service_device=self.remote_beacon_service_device,
            capture_path=self.remote_beacon_capture_path,
        )

    def fetch_remote_beacon_capture_artifacts(self) -> tuple[pathlib.Path, pathlib.Path, pathlib.Path]:
        if not self.remote_beacon_capture_path:
            raise ProbeFailure("remote beacon capture is not configured")
        local_capture = self.probe_root / "uhf-beacon-capture.bin"
        payload = ssh_capture_bytes(
            self.subsystem_target,
            f"set -euo pipefail; cat {shq(self.remote_beacon_capture_path)}",
            check=False,
        )
        local_capture.write_bytes(payload)
        if len(payload) < BEACON_FRAME_SIZE:
            raise ProbeFailure("remote beacon capture did not produce a full frame")
        first_path = self.probe_root / "uhf-beacon-first.json"
        resume_path = self.probe_root / "uhf-beacon-resume.json"
        first_path.write_text(json.dumps(decode_beacon_v1(payload[:BEACON_FRAME_SIZE]), indent=2, sort_keys=True) + "\n", encoding="utf-8")
        resume_path.write_text(json.dumps(decode_beacon_v1(payload[-BEACON_FRAME_SIZE:]), indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return local_capture, first_path, resume_path

    def stop_remote_beacon_capture(self) -> None:
        commands: list[str] = []
        if self.remote_beacon_capture_path:
            commands.append(f"pkill -f {shq(self.remote_beacon_capture_path)} >/dev/null 2>&1 || true")
            commands.append("pkill -f nonquiet-diagnosis-remote-uhf-beacon >/dev/null 2>&1 || true")
            commands.append(f"rm -f {shq(self.remote_beacon_capture_path)} >/dev/null 2>&1 || true")
        if self.remote_beacon_working_directory:
            bridge_bin = f"{self.remote_beacon_working_directory}/build-fprime-automatic-native/bin/Linux/pty_pair_bridge"
            commands.append(f"pkill -f {shq(bridge_bin)} >/dev/null 2>&1 || true")
        commands.append("pkill -f 'build-fprime-automatic-native/bin/Linux/pty_pair_bridge' >/dev/null 2>&1 || true")
        if commands:
            ssh_capture(self.subsystem_target, " ; ".join(commands), check=False)
        self.remote_beacon_capture_path = ""
        self.remote_beacon_service_device = ""
        self.remote_beacon_peer_device = ""
        self.remote_beacon_working_directory = ""
        self.remote_beacon_bridge_process = None
        self.remote_beacon_capture_process = None

    def restore_subsystem_environment(self) -> None:
        if self.baseline_managed_externally:
            self.checkpoint("subsystem-environment-restore-skipped", "pass", reason="externally-managed-baseline")
            return
        if self.subsystem_probe_baseline is None:
            return
        commands: list[str] = []
        for unit in (self.eps_service, self.adcs_service, self.sband_comm_service, self.uhf_comm_service, self.sband_stack_target, self.uhf_stack_target):
            state = self.subsystem_probe_baseline.get(unit, "inactive")
            if state == "active":
                commands.append(f"sudo systemctl start {shq(unit)} >/dev/null 2>&1 || true")
            else:
                commands.append(f"sudo systemctl stop {shq(unit)} >/dev/null 2>&1 || true")
        if commands:
            ssh_capture(self.subsystem_target, " ; ".join(commands), check=False)
        self.subsystem_probe_baseline = None

    def apply_quiet_override(self) -> None:
        baseline_env = service_environment(self.obc_target, self.obc_service)
        if baseline_env.get("DIAGNOSTIC_QUIET_PACKET_EGRESS", "0") not in ("", "0"):
            if service_override_exists(self.obc_target, self.obc_service, self.quiet_override_dropin_name):
                self.note("quiet-override-already-present removing-owned-dropin-before-probe")
                remove_service_override(self.obc_target, self.obc_service, self.quiet_override_dropin_name)
                wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                baseline_env = service_environment(self.obc_target, self.obc_service)
            if baseline_env.get("DIAGNOSTIC_QUIET_PACKET_EGRESS", "0") not in ("", "0"):
                raise ProbeFailure("target service already has DIAGNOSTIC_QUIET_PACKET_EGRESS enabled; refusing to run probe against a pre-mutated baseline")
        quiet_journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        apply_service_override(self.obc_target, self.obc_service, self.quiet_override_dropin_name, {"DIAGNOSTIC_QUIET_PACKET_EGRESS": "1"})
        self.quiet_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.wait_for_target_journal(self.obc_service, quiet_journal_since, ("Diagnostic quiet packet egress: enabled",), 20, "quiet egress enablement")
        self.snapshot_service(self.obc_target, self.obc_service, "quiet-override-applied")
        self.checkpoint("quiet-override-applied", "pass")

    def remove_quiet_override(self) -> None:
        if not self.quiet_override_applied:
            return
        nonquiet_journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        remove_service_override(self.obc_target, self.obc_service, self.quiet_override_dropin_name)
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        quiet_restore_source = "journal"
        try:
            self.wait_for_target_journal(
                self.obc_service,
                nonquiet_journal_since,
                ("Diagnostic quiet packet egress: disabled",),
                20,
                "quiet egress disablement",
            )
        except ProbeFailure:
            restored_env = service_environment(self.obc_target, self.obc_service)
            if restored_env.get("DIAGNOSTIC_QUIET_PACKET_EGRESS", "0") != "0":
                raise
            quiet_restore_source = "service-environment"
        self.snapshot_service(self.obc_target, self.obc_service, "quiet-override-removed")
        self.checkpoint("quiet-override-removed", "pass", source=quiet_restore_source)
        self.quiet_override_applied = False

    def restore_nonquiet_target_after_quiet_rescue(self) -> None:
        self.remove_quiet_override()
        self.wait_for_target_ready_for_comm(self.require_external_uhf_service)
        self.prepare_uhf_service_for_switch(
            require_pre_switch_ping=False,
            boundary="phase-c-nonquiet-restored-after-quiet-rescue",
        )

    def apply_profile_override_if_needed(self) -> None:
        baseline_env = service_environment(self.obc_target, self.obc_service)
        service_comm_csp_node = "5" if self.target_service_profile == "sband" else str(self.target_comm_csp_node)
        desired_env = {
            "TARGET_COMM_PROFILE": self.target_service_profile,
            "COMM_CSP_NODE": service_comm_csp_node,
            "COMMAND_AUTHORITY_PROFILE": self.target_service_command_authority_profile,
            "INITIAL_COMM_BAND": self.target_service_initial_comm_band,
            "ENABLE_PRIMARY_GROUND_LINK_DRIVER": self.target_service_enable_primary_ground_link_driver,
            "ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR": "1",
        }
        mismatches = [
            f"{key}:{baseline_env.get(key, '')}->{value}"
            for key, value in desired_env.items()
            if baseline_env.get(key, "") != value
        ]
        if not mismatches:
            if self.baseline_managed_externally:
                self.checkpoint(
                    "profile-baseline-verified",
                    "pass",
                    target_service_profile=self.target_service_profile,
                    target_comm_csp_node=self.target_comm_csp_node,
                    target_service_command_authority_profile=self.target_service_command_authority_profile,
                    target_service_initial_comm_band=self.target_service_initial_comm_band,
                    target_service_enable_primary_ground_link_driver=self.target_service_enable_primary_ground_link_driver,
                )
            return
        if self.baseline_managed_externally:
            self.checkpoint(
                "profile-baseline-mismatch",
                "fail",
                mismatches=mismatches,
            )
            raise ProbeFailure(
                "externally managed target baseline profile does not match "
                "the requested profile: " + ", ".join(mismatches)
            )
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.profile_override_dropin_name,
            desired_env,
        )
        self.profile_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "profile-override-applied")
        self.checkpoint(
            "profile-override-applied",
            "pass",
            target_service_profile=self.target_service_profile,
            target_comm_csp_node=self.target_comm_csp_node,
            target_service_command_authority_profile=self.target_service_command_authority_profile,
            target_service_initial_comm_band=self.target_service_initial_comm_band,
            target_service_enable_primary_ground_link_driver=self.target_service_enable_primary_ground_link_driver,
            baseline_target_comm_profile=baseline_env.get("TARGET_COMM_PROFILE", ""),
            baseline_comm_csp_node=baseline_env.get("COMM_CSP_NODE", ""),
        )

    def apply_uhf_beacon_target_override(self) -> None:
        if self.uhf_beacon_target_override_applied:
            return
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.uhf_beacon_target_override_dropin_name,
            {"UHF_BEACON_CSP_NODE": str(self.target_comm_csp_node)},
        )
        self.uhf_beacon_target_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "uhf-beacon-target-override-applied")
        self.checkpoint(
            "uhf-beacon-target-override-applied",
            "pass",
            target_service=self.obc_service,
            beacon_node=self.target_comm_csp_node,
            override_dropin=self.uhf_beacon_target_override_dropin_name,
        )

    def apply_obc_groundlink_diagnostics_override(self) -> None:
        if self.obc_groundlink_diagnostics_override_applied:
            return
        if self.obc_groundlink_diagnostics in ("", "0"):
            self.checkpoint(
                "obc-groundlink-diagnostics-override-skipped",
                "pass",
                reason="disabled",
                requested_value=self.obc_groundlink_diagnostics,
            )
            return
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.obc_groundlink_diagnostics_override_dropin_name,
            {"COMM_GROUNDLINK_DIAGNOSTICS": self.obc_groundlink_diagnostics},
        )
        self.obc_groundlink_diagnostics_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "obc-groundlink-diagnostics-override-applied")
        self.checkpoint(
            "obc-groundlink-diagnostics-override-applied",
            "pass",
            service=self.obc_service,
            value=self.obc_groundlink_diagnostics,
            override_dropin=self.obc_groundlink_diagnostics_override_dropin_name,
        )

    def apply_obc_groundlink_timeout_override(self) -> None:
        if self.obc_groundlink_timeout_override_applied:
            return
        desired_env: dict[str, str] = {}
        if self.obc_groundlink_uplink_poll_timeout_ms not in ("", "0"):
            desired_env["COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS"] = self.obc_groundlink_uplink_poll_timeout_ms
        if self.obc_groundlink_downlink_write_timeout_ms not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS"] = self.obc_groundlink_downlink_write_timeout_ms
        if self.obc_groundlink_health_timeout_ms not in ("", "0"):
            desired_env["COMM_GROUNDLINK_HEALTH_TIMEOUT_MS"] = self.obc_groundlink_health_timeout_ms
        if self.obc_groundlink_downlink_v3_max_data_bytes not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES"] = self.obc_groundlink_downlink_v3_max_data_bytes
        if self.obc_groundlink_downlink_v3_window_frames_override not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE"] = (
                self.obc_groundlink_downlink_v3_window_frames_override
            )
        if self.obc_groundlink_downlink_v3_send_probe_wait_ms not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS"] = (
                self.obc_groundlink_downlink_v3_send_probe_wait_ms
            )
        if self.obc_groundlink_downlink_v3_send_probe_retry_sleep_ms not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS"] = (
                self.obc_groundlink_downlink_v3_send_probe_retry_sleep_ms
            )
        if self.obc_groundlink_downlink_v3_interframe_delay_usec not in ("", "0"):
            desired_env["COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC"] = (
                self.obc_groundlink_downlink_v3_interframe_delay_usec
            )
        if self.obc_csp_socketcan_tx_frame_delay_usec not in ("", "0"):
            desired_env["COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC"] = (
                self.obc_csp_socketcan_tx_frame_delay_usec
            )
        if not desired_env:
            self.checkpoint(
                "obc-groundlink-timeout-override-skipped",
                "pass",
                reason="disabled",
            )
            return
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.obc_groundlink_timeout_override_dropin_name,
            desired_env,
        )
        self.obc_groundlink_timeout_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "obc-groundlink-timeout-override-applied")
        self.checkpoint(
            "obc-groundlink-timeout-override-applied",
            "pass",
            service=self.obc_service,
            override_dropin=self.obc_groundlink_timeout_override_dropin_name,
            uplink_poll_timeout_ms=desired_env.get("COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS", ""),
            downlink_write_timeout_ms=desired_env.get("COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS", ""),
            health_timeout_ms=desired_env.get("COMM_GROUNDLINK_HEALTH_TIMEOUT_MS", ""),
            downlink_v3_max_data_bytes=desired_env.get("COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES", ""),
            downlink_v3_window_frames_override=desired_env.get("COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE", ""),
            downlink_v3_send_probe_wait_ms=desired_env.get("COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS", ""),
            downlink_v3_send_probe_retry_sleep_ms=desired_env.get(
                "COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS", ""
            ),
            downlink_v3_interframe_delay_usec=desired_env.get("COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC", ""),
            csp_socketcan_tx_frame_delay_usec=desired_env.get("COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC", ""),
        )

    def apply_csp_socketcan_canfd_override(self) -> None:
        if self.csp_socketcan_canfd_override_applied:
            return
        if self.csp_socketcan_use_canfd in ("", "0"):
            self.checkpoint(
                "csp-socketcan-canfd-override-skipped",
                "pass",
                reason="disabled",
                requested_value=self.csp_socketcan_use_canfd,
            )
            return

        if self.baseline_managed_externally:
            desired_env = {
                "COMM_CSP_SOCKETCAN_USE_CANFD": self.csp_socketcan_use_canfd,
                "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST": self.csp_socketcan_canfd_dest_allowlist,
                "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST": self.csp_socketcan_canfd_dport_allowlist,
            }
            services = (
                (self.obc_target, self.obc_service, "obc"),
                (self.subsystem_target, self.sband_comm_service, "sband"),
                (self.subsystem_target, self.uhf_comm_service, "uhf"),
            )
            mismatches: list[str] = []
            for target, service_name, family in services:
                observed = service_environment(target, service_name)
                for key, expected in desired_env.items():
                    if observed.get(key, "") != expected:
                        mismatches.append(
                            f"{family}:{key}:{observed.get(key, '')}->{expected}"
                        )
            if mismatches:
                raise ProbeFailure(
                    "externally managed COMM CAN FD baseline does not match the requested profile: "
                    + ", ".join(mismatches)
                )
            self.checkpoint(
                "csp-socketcan-canfd-baseline-verified",
                "pass",
                owner="target-baseline-a",
                **{
                    key.lower(): value
                    for key, value in desired_env.items()
                },
            )
            return

        desired_env = {"COMM_CSP_SOCKETCAN_USE_CANFD": self.csp_socketcan_use_canfd}
        if self.csp_socketcan_canfd_dest_allowlist not in ("", "0"):
            desired_env["COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST"] = (
                self.csp_socketcan_canfd_dest_allowlist
            )
        if self.csp_socketcan_canfd_dport_allowlist not in ("", "0"):
            desired_env["COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST"] = (
                self.csp_socketcan_canfd_dport_allowlist
            )
        # This override exists to exercise COMM_CSP node-5/node-6 transport
        # behavior. Applying it to EPS/ADCS perturbs unrelated can0 traffic and
        # can trip FDIR before the proof even reaches the COMM path under test.
        subsystem_services = (
            self.sband_comm_service,
            self.uhf_comm_service,
        )

        for service_name in subsystem_services:
            apply_service_override(
                self.subsystem_target,
                service_name,
                self.csp_socketcan_canfd_override_dropin_name,
                desired_env,
            )
            wait_service_active(self.subsystem_target, service_name, self.restart_timeout)

        # Restart OBC last so probe-owned subsystem service churn does not trip
        # the health detector into a recovery reboot while the override rolls out.
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.csp_socketcan_canfd_override_dropin_name,
            desired_env,
        )
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)

        self.csp_socketcan_canfd_override_applied = True
        self.snapshot_service(self.obc_target, self.obc_service, "csp-socketcan-canfd-override-applied-obc")
        for service_name in subsystem_services:
            self.snapshot_service(
                self.subsystem_target,
                service_name,
                f"csp-socketcan-canfd-override-applied-{service_name}",
            )
        self.checkpoint(
            "csp-socketcan-canfd-override-applied",
            "pass",
            override_dropin=self.csp_socketcan_canfd_override_dropin_name,
            comm_csp_socketcan_use_canfd=self.csp_socketcan_use_canfd,
            comm_csp_socketcan_canfd_dest_allowlist=self.csp_socketcan_canfd_dest_allowlist,
            comm_csp_socketcan_canfd_dport_allowlist=self.csp_socketcan_canfd_dport_allowlist,
            obc_service=self.obc_service,
            subsystem_services=list(subsystem_services),
        )

    def apply_uhf_ingress_diagnostics_override(self) -> None:
        if self.uhf_ingress_diagnostics_override_applied:
            return
        if self.uhf_comm_node_ingress_diagnostics in ("", "0"):
            self.checkpoint(
                "uhf-ingress-diagnostics-override-skipped",
                "pass",
                reason="disabled",
                requested_value=self.uhf_comm_node_ingress_diagnostics,
            )
            return
        apply_service_override(
            self.subsystem_target,
            self.uhf_comm_service,
            self.uhf_ingress_diagnostics_override_dropin_name,
            {"COMM_NODE_INGRESS_DIAGNOSTICS": self.uhf_comm_node_ingress_diagnostics},
        )
        self.uhf_ingress_diagnostics_override_applied = True
        wait_service_active(self.subsystem_target, self.uhf_comm_service, self.restart_timeout)
        self.snapshot_service(self.subsystem_target, self.uhf_comm_service, "uhf-ingress-diagnostics-override-applied")
        self.checkpoint(
            "uhf-ingress-diagnostics-override-applied",
            "pass",
            service=self.uhf_comm_service,
            value=self.uhf_comm_node_ingress_diagnostics,
            override_dropin=self.uhf_ingress_diagnostics_override_dropin_name,
        )

    def apply_sband_ingress_diagnostics_override(self) -> None:
        if self.sband_ingress_diagnostics_override_applied:
            return
        if self.sband_comm_node_ingress_diagnostics in ("", "0"):
            self.checkpoint(
                "sband-ingress-diagnostics-override-skipped",
                "pass",
                reason="disabled",
                requested_value=self.sband_comm_node_ingress_diagnostics,
            )
            return
        apply_service_override(
            self.subsystem_target,
            self.sband_comm_service,
            self.sband_ingress_diagnostics_override_dropin_name,
            {"COMM_NODE_INGRESS_DIAGNOSTICS": self.sband_comm_node_ingress_diagnostics},
        )
        self.sband_ingress_diagnostics_override_applied = True
        wait_service_active(self.subsystem_target, self.sband_comm_service, self.restart_timeout)
        self.snapshot_service(self.subsystem_target, self.sband_comm_service, "sband-ingress-diagnostics-override-applied")
        self.wait_for_target_ready_for_comm(self.require_external_uhf_service)
        self.checkpoint(
            "sband-ingress-diagnostics-override-applied",
            "pass",
            service=self.sband_comm_service,
            value=self.sband_comm_node_ingress_diagnostics,
            override_dropin=self.sband_ingress_diagnostics_override_dropin_name,
        )

    def wait_for_target_journal(self, service: str, journal_since: str, fragments: tuple[str, ...], timeout: int, label: str) -> None:
        deadline = time.time() + timeout
        last_journal = ""
        while time.time() < deadline:
            last_journal = ssh_capture(
                self.obc_target,
                f"journalctl -u {shq(service)} --since {shq(journal_since)} --no-pager || true",
                check=False,
            )
            if all(fragment in last_journal for fragment in fragments):
                return
            time.sleep(0.5)
        raise ProbeFailure(f"timed out waiting for {label} in target journal; last={last_journal[-4000:]}")

    def wait_for_target_link_readiness(
        self,
        service: str,
        journal_since: str,
        fragments: tuple[str, ...],
        timeout: int,
        label: str,
    ) -> None:
        self.wait_for_target_journal(service, journal_since, fragments, timeout, label)

    def wait_for_target_current_ready_state(self, *, require_uhf: bool, timeout: int, label: str) -> None:
        deadline = time.time() + timeout
        last_journal = ""
        while time.time() < deadline:
            invocation_id = service_invocation_id(self.obc_target, self.obc_service)
            last_journal = current_service_journal(self.obc_target, self.obc_service, fallback_lines=400)
            sband_last = None
            uhf_last = None
            runtime_started_seen = False
            ground_link_configured_seen = False
            csp_init_seen = False
            for line in reversed(last_journal.splitlines()):
                if not runtime_started_seen and "OBC CCSDS S-band runtime started." in line:
                    runtime_started_seen = True
                if not invocation_id and "OBC CCSDS S-band runtime started." in line:
                    break
                if not ground_link_configured_seen and "Ground link via COMM CSP node: 5" in line:
                    ground_link_configured_seen = True
                if not csp_init_seen and "CSP initialized for node 1" in line:
                    csp_init_seen = True
                if sband_last is None:
                    if "groundLinkDriver) GROUND_LINK_UP" in line:
                        sband_last = "UP"
                    elif "groundLinkDriver) GROUND_LINK_DOWN" in line:
                        sband_last = "DOWN"
                if uhf_last is None:
                    if "uhfGroundLinkDriver) GROUND_LINK_UP" in line:
                        uhf_last = "UP"
                    elif "uhfGroundLinkDriver) GROUND_LINK_DOWN" in line:
                        uhf_last = "DOWN"
            if self.baseline_managed_externally:
                have_sband = runtime_started_seen and ground_link_configured_seen and csp_init_seen
            else:
                have_sband = sband_last == "UP"
            have_uhf = (not require_uhf) or uhf_last == "UP"
            if have_sband and have_uhf:
                return
            time.sleep(0.5)
        raise ProbeFailure(f"timed out waiting for {label} in target journal; last={last_journal[-4000:]}")

    def wait_for_target_ready_for_comm(self, require_uhf: bool) -> None:
        if self.baseline_managed_externally:
            self.wait_for_target_current_ready_state(require_uhf=require_uhf, timeout=20, label="current externally managed target readiness")
        else:
            self.wait_for_target_current_ready_state(
                require_uhf=require_uhf,
                timeout=20,
                label="current probe-owned target readiness",
            )
        time.sleep(2.0)
        self.record_bootstrap_state("node5-bootstrap-ready", require_uhf=require_uhf)

    def begin_profile(self) -> None:
        if self.profile != "sband":
            service_baudrate = service_environment(self.subsystem_target, self.uhf_comm_service).get("COMM_BAUDRATE", "")
            if service_baudrate and service_baudrate != str(self.comm_baudrate):
                self.note(
                    f"uhf-baudrate-follow-installed-service old={self.comm_baudrate} new={service_baudrate}"
                )
                self.comm_baudrate = int(service_baudrate)
        self.checkpoint("begin-profile", "info", profile=self.profile, mode=self.mode, comm_baudrate=self.comm_baudrate)
        baseline = self.prepare_subsystem_environment(self.profile)
        if self.subsystem_probe_baseline is None and not self.baseline_managed_externally:
            self.subsystem_probe_baseline = baseline
        self.apply_profile_override_if_needed()
        if self.profile != "sband":
            if self.mode in ("nonquiet-diagnosis", "dual-link-proof", "secure-auth-proof"):
                self.checkpoint(
                    "quiet-override-skipped",
                    "pass",
                    reason=self.mode,
                )
            else:
                self.apply_quiet_override()
            if not self.baseline_managed_externally:
                self.prepare_subsystem_environment(self.profile)
        self.apply_obc_groundlink_timeout_override()
        self.apply_obc_groundlink_diagnostics_override()
        self.apply_csp_socketcan_canfd_override()
        self.wait_for_target_ready_for_comm(self.require_external_uhf_service)

    def start_ground_paths(self, need_sband: bool, need_uhf: bool) -> None:
        self.checkpoint(
            "ground-paths-start",
            "info",
            need_sband=need_sband,
            need_uhf=need_uhf,
            sband_tcp_host=self.sband_tcp_host,
            sband_tcp_port=self.sband_tcp_port,
            host_serial_device=self.host_serial_device,
            comm_baudrate=self.comm_baudrate,
        )
        if need_sband:
            self.sband.start_gds()
            self.sband.start_gateway_tcp(
                self.gateway_bin,
                self.sband_tcp_host,
                self.sband_tcp_port,
                capture_dir=self.gateway_capture_dir_for_ground(self.sband),
            )
        if need_uhf:
            self.uhf.start_gds()
            self.uhf.start_gateway_serial(
                self.gateway_bin,
                self.host_serial_device,
                self.comm_baudrate,
                self.gateway_capture_dir_for_ground(self.uhf),
                preamble_lines=int(os.environ.get("TARGET_CAN_UHF_SERIAL_PREAMBLE_LINES", "0")),
                preamble_delay_ms=int(os.environ.get("TARGET_CAN_UHF_SERIAL_PREAMBLE_DELAY_MS", "0")),
            )
        self.checkpoint("ground-paths-start", "pass", need_sband=need_sband, need_uhf=need_uhf)

    def wait_for_sband_ground_readiness(self, timeout: float) -> str:
        deadline = time.time() + timeout
        downlink_capture = self.capture_path(self.sband, "southbound-to-gds")
        gateway_opened = False
        gateway_opened_since: float | None = None
        while time.time() < deadline:
            if self.sband.gateway_log.exists():
                gateway_text = self.sband.gateway_log.read_text(encoding="utf-8", errors="replace")
                if "southbound-opened mode=tcp-client" in gateway_text:
                    gateway_opened = True
                    if gateway_opened_since is None:
                        gateway_opened_since = time.time()
            if self.sband.latest_link_state() == "UP":
                return "link-up"
            if self.sband.events_log.exists():
                lines = self.sband.events_log.read_text(encoding="utf-8", errors="replace").splitlines()
                for line in lines[1:]:
                    if "OBCApp." in line or "CdhCore." in line:
                        return "event-stream"
            if downlink_capture.exists() and downlink_capture.stat().st_size > 0:
                return "downlink-bytes"
            if gateway_opened_since is not None and (time.time() - gateway_opened_since) >= 2.0:
                return "southbound-opened-stable"
            time.sleep(0.25)
        raise ProbeFailure(
            "S-band ground helper did not observe link-up, target event-stream, or downlink bytes before auth bootstrap"
            f" (southbound-opened={gateway_opened})"
        )

    def wait_for_sband_tcp_reachability(self, timeout: float) -> None:
        deadline = time.time() + timeout
        last_error = "unreached"
        while time.time() < deadline:
            try:
                with socket.create_connection((self.sband_tcp_host, self.sband_tcp_port), timeout=2.0):
                    return
            except OSError as exc:
                last_error = str(exc)
                time.sleep(0.5)
        raise ProbeFailure(
            f"S-band southbound listener {self.sband_tcp_host}:{self.sband_tcp_port} was not reachable from the ground host before auth bootstrap: {last_error}"
        )

    def ensure_sband_ground_ready(self) -> str:
        last_error: ProbeFailure | None = None
        for attempt in range(3):
            try:
                readiness = self.wait_for_sband_ground_readiness(20.0)
                self.checkpoint(
                    "sband-ground-readiness",
                    "pass",
                    attempt=attempt + 1,
                    readiness=readiness,
                )
                return readiness
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    "sband-ground-readiness",
                    "fail",
                    attempt=attempt + 1,
                    error=str(exc),
                )
                self.sband.force_stop()
                if attempt == 2:
                    break
                shutil.rmtree(self.sband.root, ignore_errors=True)
                self.sband.gds_port = free_port()
                self.sband.gds_tts_port = free_port()
                self.start_security_server()
                time.sleep(1.0)
                self.start_ground_paths(need_sband=True, need_uhf=False)
        assert last_error is not None
        raise last_error

    def send_envelope(self, ground: GroundPath, label: str, profile: AuthProfile, session_id: int, sequence_number: int, command_name: str, *args: str) -> int:
        inner = self.encode_inner_command(command_name, *args)
        opcode = self.opcodes[command_name]
        payload = authenticated_envelope(inner, profile.source_id, profile.key_slot, profile.key_bytes, session_id, sequence_number)
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: command={command_name} args={list(args)} opcode=0x{opcode:x} session={session_id} seq={sequence_number} outer={payload.hex()}\n"
            )
        start = ground.event_count()
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)
        return start

    def prepare_ground_window(self, ground: GroundPath, timeout: float = 8.0) -> None:
        try:
            ground.await_link_up_if_needed(timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-link-up-not-observed-within-{timeout:g}s")
        try:
            ground.wait_link_quiet(timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-quiet-window-not-observed-within-{timeout:g}s")

    def wait_ground_or_journal(
        self,
        ground: GroundPath,
        ground_start: int,
        journal_since: str,
        ground_fragments: tuple[str, ...],
        journal_fragments: tuple[str, ...],
        timeout: float,
        label: str,
        prefer_target_journal: bool = False,
    ) -> str:
        deadline = time.time() + timeout
        last_events = ""
        last_journal = ""
        while time.time() < deadline:
            def check_ground() -> str | None:
                nonlocal last_events
                lines = read_text(ground.events_log).splitlines()
                last_events = "\n".join(lines[ground_start:])
                if all(fragment in last_events for fragment in ground_fragments):
                    return "ground-events"
                return None

            def check_journal() -> str | None:
                nonlocal last_journal
                last_journal = ssh_capture(
                    self.obc_target,
                    f"journalctl -u {shq(self.obc_service)} --since {shq(journal_since)} --no-pager || true",
                    check=False,
                )
                if all(fragment in last_journal for fragment in journal_fragments):
                    return "target-journal"
                return None

            first = check_journal if prefer_target_journal else check_ground
            second = check_ground if prefer_target_journal else check_journal
            source = first()
            if source is not None:
                return source
            source = second()
            if source is not None:
                return source
            time.sleep(0.5)
        raise ProbeFailure(
            f"timed out waiting for {label}; ground_fragments={ground_fragments} journal_fragments={journal_fragments}\n"
            f"ground_tail={last_events[-4000:]}\njournal_tail={last_journal[-4000:]}"
        )

    @staticmethod
    def _text_has_all_fragments(text: str, fragments: tuple[str, ...]) -> bool:
        return all(fragment in text for fragment in fragments)

    def _analyze_secure_command_retry_journal(
        self,
        *,
        journal_since: str,
        journal_fragments: tuple[str, ...],
        session: SecureAuthSession,
        sequence: int,
        opcode: int,
    ) -> SecureCommandReadbackJournalState:
        journal_text = ssh_capture(
            self.obc_target,
            f"journalctl -u {shq(self.obc_service)} --since {shq(journal_since)} --no-pager || true",
            check=False,
        )
        accepted = bool(journal_fragments) and self._text_has_all_fragments(journal_text, journal_fragments)
        sequence_tokens = (
            "COMMAND_SEQUENCE_REJECTED",
            f"ingress {session.ingress_port}",
            f"sequence {sequence}",
            f"inner opcode 0x{opcode:x}",
        )
        secure_tokens = (
            "SECURE_COMMAND_REJECTED",
            f"ingress {session.ingress_port}",
            f"sequence {sequence}",
            f"inner opcode 0x{opcode:x}",
        )
        session_tokens = (
            "COMMAND_SESSION_REJECTED",
            f"ingress {session.ingress_port}",
            f"sequence {sequence}",
            f"inner opcode 0x{opcode:x}",
        )
        return SecureCommandReadbackJournalState(
            accepted=accepted,
            duplicate_reject=self._text_has_all_fragments(journal_text, sequence_tokens),
            secure_reject=self._text_has_all_fragments(journal_text, secure_tokens),
            session_reject=self._text_has_all_fragments(journal_text, session_tokens),
            journal_tail=journal_text[-4000:],
        )

    def _send_secure_command_packet_once(
        self,
        ground: GroundPath,
        session: SecureAuthSession,
        label: str,
        command_name: str,
        args: tuple[str, ...],
        *,
        sequence: int,
        attempt: int,
        sequence_policy: str,
        session_events_log_offset: int | None = None,
        session_native_event_log_offset: int | None = None,
        session_native_channel_log_offset: int | None = None,
        session_capture_offset: int | None = None,
    ) -> SecureCommandReadbackAttemptContext:
        inner = self.encode_inner_command(command_name, *args)
        opcode = self.opcodes[command_name]
        payload = build_secure_command_v2_packet(inner, session.session_key, sequence)
        capture_path = self.capture_path(ground, "southbound-to-gds")
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: attempt={attempt} sequencePolicy={sequence_policy} "
                f"secure-v2 service={session.service_id} ingress={session.ingress_port} "
                f"role={session.role_fragment} command={command_name} args={list(args)} "
                f"opcode=0x{opcode:x} seq={sequence} outer={payload.hex()}\n"
            )
        context = SecureCommandReadbackAttemptContext(
            attempt=attempt,
            sequence=sequence,
            sequence_policy=sequence_policy,
            command_name=command_name,
            command_args=args,
            ground=ground,
            journal_since=ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip(),
            events_log_line_count=len(read_text(ground.events_log).splitlines()),
            events_log_offset=file_size(ground.events_log),
            native_event_log_offset=file_size(ground.native_event_log),
            native_channel_log_offset=file_size(ground.native_channel_log),
            capture_offset=file_size(capture_path),
            session_events_log_offset=(
                file_size(ground.events_log)
                if session_events_log_offset is None
                else session_events_log_offset
            ),
            session_native_event_log_offset=(
                file_size(ground.native_event_log)
                if session_native_event_log_offset is None
                else session_native_event_log_offset
            ),
            session_native_channel_log_offset=(
                file_size(ground.native_channel_log)
                if session_native_channel_log_offset is None
                else session_native_channel_log_offset
            ),
            session_capture_offset=(
                file_size(capture_path)
                if session_capture_offset is None
                else session_capture_offset
            ),
        )
        send_tts_raw_packet(ground.gds_tts_port, payload)
        return context

    def build_ground_event_readback_oracle(
        self,
        *,
        event_log_path: pathlib.Path,
        event_offset_attr: str,
        ground_fragments: tuple[str, ...],
        capture_path: pathlib.Path | None = None,
        require_capture_growth: bool = False,
        source_name: str,
    ) -> Callable[[SecureCommandReadbackAttemptContext, float], dict[str, object] | None]:
        def oracle(attempt: SecureCommandReadbackAttemptContext, timeout: float) -> dict[str, object] | None:
            deadline = time.time() + timeout
            session_attr = f"session_{event_offset_attr}"
            start_offset = getattr(attempt, session_attr, getattr(attempt, event_offset_attr))
            while True:
                event_text = read_text_range(event_log_path, start_offset)
                capture_growth = 0
                if require_capture_growth and capture_path is not None:
                    capture_growth = max(0, file_size(capture_path) - attempt.session_capture_offset)
                capture_ready = True
                if require_capture_growth:
                    capture_ready = capture_growth > 0
                if self._text_has_all_fragments(event_text, ground_fragments) and capture_ready:
                    return {
                        "source": source_name,
                        "groundFragments": list(ground_fragments),
                        "eventLogGrowth": len(event_text),
                        "captureGrowth": capture_growth,
                    }
                if time.time() >= deadline:
                    return None
                time.sleep(0.1)

        return oracle

    def send_secure_command_until_ground_readback(
        self,
        ground: GroundPath,
        session: SecureAuthSession,
        label: str,
        command_name: str,
        *args: str,
        accept_sequence: bool,
        journal_fragments: tuple[str, ...],
        ground_success_oracle: Callable[[SecureCommandReadbackAttemptContext, float], dict[str, object] | None],
        attempt_limit: int = 30,
        per_attempt_timeout: float = 1.0,
        sequence_number: int | None = None,
    ) -> dict[str, object]:
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        opcode = self.opcodes[command_name]
        current_sequence = (
            sequence_number
            if sequence_number is not None
            else (session.accept_sequence() if accept_sequence else session.claim_sequence())
        )
        next_policy = "initial"
        attempts: list[dict[str, object]] = []
        accepted_seen = False
        duplicate_seen = False
        secure_reject_seen = False
        session_reject_seen = False
        session_events_log_offset: int | None = None
        session_native_event_log_offset: int | None = None
        session_native_channel_log_offset: int | None = None
        session_capture_offset: int | None = None
        for attempt_index in range(1, attempt_limit + 1):
            if attempt_index > 1:
                if next_policy == "new-seq-retry":
                    if accept_sequence:
                        current_sequence = session.accept_sequence()
                    else:
                        current_sequence += 1
                        if session.next_sequence <= current_sequence:
                            session.next_sequence = current_sequence + 1
                sequence_policy = next_policy
            else:
                sequence_policy = "initial"
            attempt_context = self._send_secure_command_packet_once(
                ground,
                session,
                label,
                command_name,
                args,
                sequence=current_sequence,
                attempt=attempt_index,
                sequence_policy=sequence_policy,
                session_events_log_offset=session_events_log_offset,
                session_native_event_log_offset=session_native_event_log_offset,
                session_native_channel_log_offset=session_native_channel_log_offset,
                session_capture_offset=session_capture_offset,
            )
            if session_events_log_offset is None:
                session_events_log_offset = attempt_context.session_events_log_offset
                session_native_event_log_offset = attempt_context.session_native_event_log_offset
                session_native_channel_log_offset = attempt_context.session_native_channel_log_offset
                session_capture_offset = attempt_context.session_capture_offset
            ground_result = ground_success_oracle(attempt_context, per_attempt_timeout)
            journal_state = self._analyze_secure_command_retry_journal(
                journal_since=attempt_context.journal_since,
                journal_fragments=journal_fragments,
                session=session,
                sequence=current_sequence,
                opcode=opcode,
            )
            accepted_seen = accepted_seen or journal_state.accepted
            duplicate_seen = duplicate_seen or journal_state.duplicate_reject
            secure_reject_seen = secure_reject_seen or journal_state.secure_reject
            session_reject_seen = session_reject_seen or journal_state.session_reject
            if ground_result is None:
                ground_result = ground_success_oracle(attempt_context, 0.0)
            attempt_payload = {
                "attempt": attempt_index,
                "sequence": current_sequence,
                "sequencePolicy": sequence_policy,
                "groundSuccess": ground_result is not None,
                "journalAccepted": journal_state.accepted,
                "journalDuplicateReject": journal_state.duplicate_reject,
                "journalSecureReject": journal_state.secure_reject,
                "journalSessionReject": journal_state.session_reject,
            }
            if ground_result is not None:
                attempt_payload.update(ground_result)
                attempts.append(attempt_payload)
                self.checkpoint(
                    f"{ground.name}-secure-command-ground-readback",
                    "pass",
                    label=label,
                    command=command_name,
                    attempt=attempt_index,
                    sequence=current_sequence,
                    source=ground_result.get("source", "ground-readback"),
                    attemptsUsed=attempt_index,
                    sequencePolicy=sequence_policy,
                )
                return {
                    "acceptedSequence": current_sequence,
                    "source": ground_result.get("source", "ground-readback"),
                    "attemptCount": attempt_index,
                    "attempts": attempts,
                    "journalAcceptedSeen": accepted_seen,
                    "duplicateRejectSeen": duplicate_seen,
                    "secureRejectSeen": secure_reject_seen,
                    "sessionRejectSeen": session_reject_seen,
                    **ground_result,
                }
            attempt_payload["journalTail"] = journal_state.journal_tail
            attempts.append(attempt_payload)
            next_policy = (
                "new-seq-retry"
                if (
                    journal_state.accepted
                    or journal_state.duplicate_reject
                    or journal_state.secure_reject
                    or journal_state.session_reject
                )
                else "same-seq-retry"
            )

        failure_class = "uplink-target-acceptance-missing"
        if duplicate_seen or secure_reject_seen or session_reject_seen:
            failure_class = "duplicate-sequence-churn-after-prior-acceptance"
        elif accepted_seen:
            failure_class = "target-accepted-but-ground-readback-missing"
        failure = ProbeFailure(
            f"{ground.name}: timed out waiting for ground readback for {label} after {attempt_limit} sends; "
            f"failureClass={failure_class}"
        )
        setattr(
            failure,
            "retry_details",
            {
                "command": command_name,
                "attemptLimit": attempt_limit,
                "perAttemptTimeoutSec": per_attempt_timeout,
                "failureClass": failure_class,
                "attempts": attempts,
                "journalAcceptedSeen": accepted_seen,
                "duplicateRejectSeen": duplicate_seen,
                "secureRejectSeen": secure_reject_seen,
                "sessionRejectSeen": session_reject_seen,
            },
        )
        raise failure

    def load_handshake_messages_for_ground(
        self,
        ground: GroundPath,
        service_id: int,
        message_type: HandshakeMessageType,
    ) -> dict[str, list]:
        capture_messages = load_handshake_messages(
            self.capture_path(ground, "southbound-to-gds"),
            scid=ground.scid,
            vcid=ground.vcid,
            frame_size=ground.frame_size,
            service_id=service_id,
            message_type=message_type,
        )
        native_messages = load_handshake_messages_from_native_packet_log(
            ground.native_recv_bin,
            service_id=service_id,
            message_type=message_type,
        )
        return {
            "wire-capture": capture_messages,
            "native-recv": native_messages,
        }

    @staticmethod
    def handshake_progress(messages_by_source: dict[str, list]) -> HandshakeProgress:
        return HandshakeProgress(
            wire_count=len(messages_by_source["wire-capture"]),
            native_count=len(messages_by_source["native-recv"]),
        )

    def fresh_handshake_progress(
        self,
        ground: GroundPath,
        service_id: int,
        message_type: HandshakeMessageType,
    ) -> HandshakeProgress:
        return self.handshake_progress(
            self.load_handshake_messages_for_ground(ground, service_id, message_type)
        )

    def wait_for_handshake(
        self,
        ground: GroundPath,
        service_id: int,
        message_type: HandshakeMessageType,
        seen_progress: HandshakeProgress,
        timeout: float,
    ):
        deadline = time.time() + timeout
        # Native packet logs represent the exact packets the local proof-side GDS
        # consumed. Prefer them first so stale or lagging wire-capture decode on
        # long-running target paths cannot pair a fresh response with an older
        # challenge.
        preferred_sources = ("native-recv", "wire-capture")
        while time.time() < deadline:
            messages_by_source = self.load_handshake_messages_for_ground(ground, service_id, message_type)
            progress = self.handshake_progress(messages_by_source)
            for source in preferred_sources:
                messages = messages_by_source[source]
                seen_count = seen_progress.wire_count if source == "wire-capture" else seen_progress.native_count
                if len(messages) > seen_count:
                    return messages[-1], source, progress
            time.sleep(0.2)
        raise TimeoutError(
            f"timed out waiting for {message_type.name} on "
            f"{self.capture_path(ground, 'southbound-to-gds')} or {ground.native_recv_bin}"
        )

    def assert_no_target_journal(self, journal_since: str, fragments: tuple[str, ...], quiet_sec: float, label: str) -> None:
        deadline = time.time() + quiet_sec
        last_journal = ""
        while time.time() < deadline:
            last_journal = ssh_capture(
                self.obc_target,
                f"journalctl -u {shq(self.obc_service)} --since {shq(journal_since)} --no-pager || true",
                check=False,
            )
            if all(fragment in last_journal for fragment in fragments):
                raise ProbeFailure(f"unexpected target journal fragments for {label}: {fragments}")
            time.sleep(0.5)

    def verify_malformed_secure_handshake_fail_closed(self, ground: GroundPath) -> None:
        challenge_progress = self.fresh_handshake_progress(ground, SERVICE_ID_SBAND, HandshakeMessageType.CHALLENGE)
        status_progress = self.fresh_handshake_progress(ground, SERVICE_ID_SBAND, HandshakeMessageType.AUTH_STATUS)
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        malformed = bytearray(build_req_auth_packet(SERVICE_ID_SBAND))
        malformed[2:6] = b"\x00\x00\x00\x00"
        send_tts_raw_packet(ground.gds_tts_port, bytes(malformed))
        self.wait_for_target_journal(
            self.obc_service,
            journal_since,
            ("Secure auth packet rejected ingress 0 service 0 reason",),
            12,
            "malformed secure auth reject",
        )
        time.sleep(1.0)
        if self.fresh_handshake_progress(ground, SERVICE_ID_SBAND, HandshakeMessageType.CHALLENGE) != challenge_progress:
            raise ProbeFailure("malformed S-band handshake unexpectedly produced a challenge")
        if self.fresh_handshake_progress(ground, SERVICE_ID_SBAND, HandshakeMessageType.AUTH_STATUS) != status_progress:
            raise ProbeFailure("malformed S-band handshake unexpectedly produced an auth status")
        self.assert_no_target_journal(
            journal_since,
            ("Secure auth established ingress 0 service 1",),
            2.0,
            "malformed handshake auth state mutation",
        )
        self.assert_no_target_journal(
            journal_since,
            ("Command session opened ingress 0 identity 1 role 1",),
            2.0,
            "malformed handshake session state mutation",
        )
        self.checkpoint(
            "secure-auth-malformed-handshake-fail-closed",
            "pass",
            service_id=SERVICE_ID_SBAND,
            challenge_progress=challenge_progress.__dict__,
            status_progress=status_progress.__dict__,
        )

    def authenticate_secure_service(
        self,
        ground: GroundPath,
        *,
        service_id: int,
        ingress_port: int,
        role_fragment: str,
        initial_sequence: int = 41,
    ) -> SecureAuthSession:
        expected_open_fragment = f"Command session opened ingress {ingress_port} {role_fragment} session "
        expected_journal_fragments = (
            f"Secure auth established ingress {ingress_port} service {service_id}",
            expected_open_fragment,
        )
        expected_challenge_fragment = f"Secure auth challenge issued ingress {ingress_port} service {service_id}"
        deadline = time.time() + 45.0
        attempt = 0
        last_error: Exception | None = None
        while time.time() < deadline:
            attempt += 1
            challenge_progress = self.fresh_handshake_progress(ground, service_id, HandshakeMessageType.CHALLENGE)
            status_progress = self.fresh_handshake_progress(ground, service_id, HandshakeMessageType.AUTH_STATUS)
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            response: bytes | None = None
            self.checkpoint(
                f"{ground.name}-secure-auth-attempt",
                "info",
                service_id=service_id,
                ingress_port=ingress_port,
                attempt=attempt,
            )
            send_tts_raw_packet(ground.gds_tts_port, build_req_auth_packet(service_id))
            try:
                challenge, challenge_source, challenge_progress = self.wait_for_handshake(
                    ground,
                    service_id,
                    HandshakeMessageType.CHALLENGE,
                    challenge_progress,
                    timeout=8.0,
                )
            except TimeoutError as exc:
                try:
                    self.wait_for_target_journal(
                        self.obc_service,
                        journal_since,
                        (expected_challenge_fragment,),
                        3,
                        f"{ground.name} secure auth challenge issued",
                    )
                    challenge_seen_in_journal = True
                    challenge, challenge_source, challenge_progress = self.wait_for_handshake(
                        ground,
                        service_id,
                        HandshakeMessageType.CHALLENGE,
                        challenge_progress,
                        timeout=6.0,
                    )
                except (ProbeFailure, TimeoutError):
                    last_error = exc
                    time.sleep(1.0)
                    continue
                else:
                    self.checkpoint(
                        f"{ground.name}-secure-auth-challenge-late-arrival",
                        "pass",
                        service_id=service_id,
                        ingress_port=ingress_port,
                        attempt=attempt,
                        source=challenge_source,
                    )
            if response is None:
                session_key = request_session_key(self.security_server_socket, service_id, challenge.challenge)
                response = compute_auth_response(session_key)

            def journal_confirms_auth(timeout: float) -> bool:
                try:
                    self.wait_for_target_journal(
                        self.obc_service,
                        journal_since,
                        expected_journal_fragments,
                        timeout,
                        f"{ground.name} secure auth established",
                    )
                    return True
                except ProbeFailure:
                    return False

            for response_attempt in range(3):
                send_tts_raw_packet(ground.gds_tts_port, build_response_packet(service_id, response))
                try:
                    auth_status, auth_status_source, status_progress = self.wait_for_handshake(
                        ground,
                        service_id,
                        HandshakeMessageType.AUTH_STATUS,
                        status_progress,
                        timeout=8.0,
                    )
                except TimeoutError as exc:
                    if journal_confirms_auth(4):
                        self.checkpoint(
                            f"{ground.name}-secure-auth-established",
                            "pass",
                            service_id=service_id,
                            ingress_port=ingress_port,
                            role_fragment=role_fragment,
                            challenge_offset=challenge.packet_offset,
                            status_offset=None,
                            attempts=attempt,
                            response_attempt=response_attempt + 1,
                            challenge_source=challenge_source,
                            confirmation_source="target-journal",
                            wire_status="missing",
                        )
                        return SecureAuthSession(
                            service_id=service_id,
                            ingress_port=ingress_port,
                            role_fragment=role_fragment,
                            session_key=session_key,
                            next_sequence=initial_sequence,
                        )
                    last_error = exc
                    time.sleep(1.0)
                    continue
                if auth_status.status_code == AuthStatusCode.AUTHENTICATED:
                    self.wait_for_target_journal(
                        self.obc_service,
                        journal_since,
                        expected_journal_fragments,
                        20,
                        f"{ground.name} secure auth established",
                    )
                    self.checkpoint(
                        f"{ground.name}-secure-auth-established",
                        "pass",
                        service_id=service_id,
                        ingress_port=ingress_port,
                        role_fragment=role_fragment,
                        challenge_offset=challenge.packet_offset,
                        status_offset=auth_status.packet_offset,
                        attempts=attempt,
                        response_attempt=response_attempt + 1,
                        challenge_source=challenge_source,
                        confirmation_source=(
                            "wire-auth-status" if auth_status_source == "wire-capture" else "native-auth-status"
                        ),
                    )
                    return SecureAuthSession(
                        service_id=service_id,
                        ingress_port=ingress_port,
                        role_fragment=role_fragment,
                        session_key=session_key,
                        next_sequence=initial_sequence,
                    )
                if auth_status.status_code != AuthStatusCode.NOT_AUTHENTICATED:
                    raise ProbeFailure(
                        f"{ground.name}: unexpected auth status {auth_status.status_code} for service {service_id}"
                    )
                if journal_confirms_auth(4):
                    self.checkpoint(
                        f"{ground.name}-secure-auth-established",
                        "pass",
                        service_id=service_id,
                        ingress_port=ingress_port,
                        role_fragment=role_fragment,
                        challenge_offset=challenge.packet_offset,
                        status_offset=auth_status.packet_offset,
                        attempts=attempt,
                        response_attempt=response_attempt + 1,
                        challenge_source=challenge_source,
                        confirmation_source="target-journal",
                        wire_status=auth_status.status_code,
                    )
                    return SecureAuthSession(
                        service_id=service_id,
                        ingress_port=ingress_port,
                        role_fragment=role_fragment,
                        session_key=session_key,
                        next_sequence=initial_sequence,
                    )
                last_error = ProbeFailure(f"{ground.name}: secure auth status NOT_AUTHENTICATED")
                time.sleep(1.0)
                break
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: failed to authenticate service {service_id}")
        raise ProbeFailure(f"{ground.name}: failed to authenticate service {service_id}: {last_error}")

    def send_secure_command_name(
        self,
        ground: GroundPath,
        session: SecureAuthSession,
        label: str,
        command_name: str,
        *args: str,
        accept_sequence: bool,
        journal_fragments: tuple[str, ...],
        timeout: float = 24.0,
        sequence_number: int | None = None,
    ) -> tuple[int, str]:
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        effective_sequence = (
            sequence_number
            if sequence_number is not None
            else (session.accept_sequence() if accept_sequence else session.claim_sequence())
        )
        attempt_context = self._send_secure_command_packet_once(
            ground,
            session,
            label,
            command_name,
            args,
            sequence=effective_sequence,
            attempt=1,
            sequence_policy="single-shot",
        )
        source = "sent"
        if journal_fragments:
            source = self.wait_ground_or_journal(
                ground,
                attempt_context.events_log_line_count,
                attempt_context.journal_since,
                journal_fragments,
                journal_fragments,
                timeout,
                label,
                prefer_target_journal=True,
            )
        self.checkpoint(
            f"{ground.name}-secure-command",
            "pass",
            label=label,
            command=command_name,
            sequence=effective_sequence,
            source=source,
            accept_sequence=accept_sequence,
        )
        return effective_sequence, source

    def send_staged_upload_packets(self, ground: GroundPath, artifact: SequenceArtifact, *, expect_accept: bool, label: str) -> None:
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        start_packet = StartPacketData(0, len(payload), str(artifact.binary), artifact.destination)
        data_packet = DataPacketData(1, 0, payload)
        end_packet = EndPacketData(2, checksum.value)
        audit_path = self.write_sequence_packet_audit(
            ground,
            artifact,
            mode=label,
            authority_profile="secure-auth-v2",
            source_id=SERVICE_ID_UHF if ground is self.uhf else SERVICE_ID_SBAND,
            key_slot=SERVICE_ID_UHF if ground is self.uhf else SERVICE_ID_SBAND,
            ingress_port=1 if ground is self.uhf else 0,
            session_id=None,
            switch_source=None,
            session_open_source="secure-auth-synthesized-session",
            packet_sequence_numbers=(0, 1, 2),
        )
        self.note(f"{label}-packet-audit {audit_path}")
        self.prepare_ground_window(ground)
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        event_start = ground.event_count()
        ground.send_file_packet_via_pipeline(start_packet)
        if expect_accept:
            self.wait_ground_or_journal(
                ground,
                event_start,
                journal_since,
                ("FILE_INGRESS_START_ACCEPTED",),
                ("FILE_INGRESS_START_ACCEPTED",),
                25.0,
                f"{label} file ingress start accepted",
                prefer_target_journal=True,
            )
            ground.send_file_packet_via_pipeline(data_packet)
            ground.send_file_packet_via_pipeline(end_packet)
            self.wait_ground_or_journal(
                ground,
                event_start,
                journal_since,
                ("FileReceived",),
                ("FileReceived",),
                90.0,
                f"{label} file received",
                prefer_target_journal=True,
            )
            self.checkpoint(f"{ground.name}-{label}-staged-upload", "pass", destination=artifact.destination, expected="accepted")
        else:
            self.wait_ground_or_journal(
                ground,
                event_start,
                journal_since,
                ("FILE_INGRESS_START_REJECTED",),
                ("FILE_INGRESS_START_REJECTED",),
                25.0,
                f"{label} file ingress start rejected",
                prefer_target_journal=True,
            )
            self.assert_no_target_journal(journal_since, ("FileReceived",), 3.0, f"{label} rejected staged upload")
            self.checkpoint(f"{ground.name}-{label}-staged-upload", "pass", destination=artifact.destination, expected="rejected")

    def open_session(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        ingress_port: int,
        session_minimum: int,
        role_fragment: str,
        retries: int = 6,
        prefer_target_journal: bool = False,
    ) -> tuple[int, str]:
        identity, role = authority_identity_role(profile.authority_profile)
        persisted_floor = read_persisted_session_floor(self.obc_target, self.runtime_root, ingress_port, identity, role)
        last_error: ProbeFailure | None = None
        for attempt in range(retries):
            session_id = next_session_id(session_minimum + attempt, persisted_floor)
            self.checkpoint(
                f"{ground.name}-session-open-attempt",
                "info",
                authority_profile=profile.authority_profile,
                attempt=attempt + 1,
                ingress_port=ingress_port,
                identity=identity,
                role=role,
                session_id=session_id,
            )
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            start = self.send_envelope(
                ground,
                f"{ground.name}-session-open-attempt-{attempt + 1}",
                profile,
                session_id,
                0,
                "OBCApp.commandIngressAuthority.SESSION_OPEN",
            )
            try:
                source = self.wait_ground_or_journal(
                    ground,
                    start,
                    journal_since,
                    ("COMMAND_SESSION_OPENED", role_fragment, f"session {session_id}"),
                    ("COMMAND_SESSION_OPENED", role_fragment, f"session {session_id}"),
                    18.0,
                    f"{ground.name} session open",
                    prefer_target_journal=prefer_target_journal,
                )
                self.note(f"{ground.name}-session-open-pass source={source} session={session_id}")
                self.checkpoint(
                    f"{ground.name}-session-open-visible",
                    "pass",
                    authority_profile=profile.authority_profile,
                    source=source,
                    session_id=session_id,
                )
                return session_id, source
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-session-open-visible",
                    "fail",
                    authority_profile=profile.authority_profile,
                    attempt=attempt + 1,
                    session_id=session_id,
                    error=str(exc),
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: session open failed without a recorded error")
        raise last_error

    def switch_to_uhf_primary(self, sband_session_id: int, prefer_target_journal: bool = False) -> str:
        profile = self.authority_profile("sband-primary")
        last_error: ProbeFailure | None = None
        for attempt in range(6):
            self.prepare_ground_window(self.sband)
            self.prepare_ground_window(self.uhf)
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            uhf_start = self.uhf.event_count()
            self.send_envelope(
                self.sband,
                f"switch-to-uhf-primary-attempt-{attempt + 1}",
                profile,
                sband_session_id,
                1 + attempt,
                "OBCApp.commController.COMM_SET_ACTIVE",
                "UHF",
            )
            try:
                source = self.wait_ground_or_journal(
                    self.uhf,
                    uhf_start,
                    journal_since,
                    ("COMM_PRIMARY_LINK_CHANGED", "command UHF", "telemetry UHF", "file UHF", "reason 1"),
                    ("COMM_PRIMARY_LINK_CHANGED", "command UHF", "telemetry UHF", "file UHF", "reason 1"),
                    20.0,
                    "switch to UHF primary",
                    prefer_target_journal=prefer_target_journal,
                )
                self.prepare_ground_window(self.uhf)
                self.note(f"switch-to-uhf-primary-pass source={source}")
                self.checkpoint("switch-to-uhf-primary", "pass", source=source, sband_session_id=sband_session_id)
                return source
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint("switch-to-uhf-primary", "fail", attempt=attempt + 1, error=str(exc))
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure("UHF primary switch failed without a recorded error")
        raise last_error

    def build_sequence_artifact(self, name: str) -> SequenceArtifact:
        source = self.sequence_src_dir / f"{name}.seq"
        source.write_text(
            "\n".join(
                [
                    "R00:00:00 OBCApp.epsBridge.EPS_GET_STATUS",
                    "R00:00:01 OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
                    "",
                ]
            ),
            encoding="utf-8",
        )
        binary = self.sequence_bin_dir / f"{name}.bin"
        subprocess.run(
            [str(self.seqgen_path), "--dictionary", str(self.dictionary_path), str(source), str(binary)],
            check=True,
            cwd=str(self.root_dir),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return SequenceArtifact(source=source, binary=binary, destination=f".sequence-staging/{name}.bin")

    def write_sequence_packet_audit(
        self,
        ground: GroundPath,
        artifact: SequenceArtifact,
        *,
        mode: str,
        authority_profile: str,
        source_id: int,
        key_slot: int,
        ingress_port: int,
        session_id: int | None,
        switch_source: str | None,
        session_open_source: str | None,
        packet_sequence_numbers: tuple[int, int, int],
    ) -> pathlib.Path:
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        start_seq, data_seq, end_seq = packet_sequence_numbers
        start_packet = StartPacketData(start_seq, len(payload), str(artifact.binary), artifact.destination)
        data_packet = DataPacketData(data_seq, 0, payload)
        end_packet = EndPacketData(end_seq, checksum.value)
        packet_records = []
        for packet in (start_packet, data_packet, end_packet):
            packet_records.append(
                {
                    "packetType": packet.packetType.name,
                    "seqID": packet.seqID,
                    "encodedHex": self.file_encoder.encode_api(packet).hex(),
                }
            )
        record = {
            "ground": ground.name,
            "carrierKind": "uhf-physical-uart" if ground is self.uhf else "sband-tcp",
            "mode": mode,
            "compiledSequenceSha256": hashlib.sha256(payload).hexdigest(),
            "sourcePath": str(artifact.source),
            "binaryPath": str(artifact.binary),
            "destinationPath": artifact.destination,
            "fileSize": len(payload),
            "checksumType": "modular",
            "checksumValue": checksum.value,
            "gdsFramingSelection": "space-packet-space-data-link",
            "vcid": ground.vcid,
            "authorityProfile": authority_profile,
            "sourceId": source_id,
            "keySlot": key_slot,
            "ingressPort": ingress_port,
            "sessionId": session_id,
            "switchSource": switch_source,
            "sessionOpenSource": session_open_source,
            "packetSequenceNumbers": {
                "start": start_seq,
                "data": data_seq,
                "end": end_seq,
            },
            "packets": packet_records,
        }
        self.packet_audit_dir.mkdir(parents=True, exist_ok=True)
        path = self.packet_audit_dir / f"{ground.name}-{mode}-packet-audit.json"
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return path

    def write_upload_audit(self, ground: GroundPath, artifact: SequenceArtifact, *, mode: str, packet_sequence_numbers: tuple[int, int, int]) -> pathlib.Path:
        if ground is self.uhf:
            authority_profile = "uhf-primary"
            source_id = UHF_SOURCE_ID
            key_slot = UHF_KEY_SLOT
        else:
            authority_profile = "sband-primary"
            source_id = SBAND_SOURCE_ID
            key_slot = SBAND_KEY_SLOT
        session_id = None
        session_open_source = None
        switch_source = None
        if ground is self.uhf:
            session_id = 0x73000000
            session_open_source = "ground-events-or-obc-log"
            switch_source = "ground-events-or-obc-log"
        elif ground is self.sband:
            session_id = 0x61000000
            session_open_source = "ground-events-or-obc-log"
        return self.write_sequence_packet_audit(
            ground,
            artifact,
            mode=mode,
            authority_profile=authority_profile,
            source_id=source_id,
            key_slot=key_slot,
            ingress_port=1,
            session_id=session_id,
            switch_source=switch_source,
            session_open_source=session_open_source,
            packet_sequence_numbers=packet_sequence_numbers,
        )

    def upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> None:
        last_error: ProbeFailure | None = None
        for attempt in range(2):
            audit_path = self.write_upload_audit(ground, artifact, mode=f"api-attempt-{attempt + 1}", packet_sequence_numbers=(0, 1, 2))
            self.note(f"{ground.name}-upload-attempt-{attempt + 1}-audit {audit_path}")
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            start = ground.upload_file(artifact.binary, artifact.destination)
            self.checkpoint(
                f"{ground.name}-sequence-upload-start",
                "info",
                attempt=attempt + 1,
                destination=artifact.destination,
                event_start=start,
            )
            try:
                try:
                    self.wait_ground_or_journal(
                        ground,
                        start,
                        journal_since,
                        ("FILE_INGRESS_START_ACCEPTED",),
                        ("FILE_INGRESS_START_ACCEPTED",),
                        20.0,
                        "sequence ingress start accepted",
                    )
                    self.checkpoint(f"{ground.name}-file-ingress-start-visible", "pass", attempt=attempt + 1, destination=artifact.destination)
                except ProbeFailure:
                    self.note(f"{ground.name}-sequence-upload-start-accept-not-observed attempt={attempt + 1}")
                    self.checkpoint(f"{ground.name}-file-ingress-start-visible", "fail", attempt=attempt + 1, destination=artifact.destination)
                self.wait_ground_or_journal(
                    ground,
                    start,
                    journal_since,
                    ("FileReceived",),
                    ("FileReceived",),
                    90.0,
                    "sequence file received",
                )
                self.checkpoint(f"{ground.name}-file-received-visible", "pass", attempt=attempt + 1, destination=artifact.destination)
                return
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-file-received-visible",
                    "fail",
                    attempt=attempt + 1,
                    destination=artifact.destination,
                    error=str(exc),
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: sequence upload failed without a recorded error")
        raise last_error

    def send_command_and_wait(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        session_id: int,
        sequence_number: int,
        command_name: str,
        args: tuple[str, ...],
        ground_fragments: tuple[str, ...],
        journal_fragments: tuple[str, ...],
        timeout: float,
        label: str,
        prefer_target_journal: bool = False,
    ) -> str:
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        start = self.send_envelope(ground, label, profile, session_id, sequence_number, command_name, *args)
        return self.wait_ground_or_journal(
            ground,
            start,
            journal_since,
            ground_fragments,
            journal_fragments,
            timeout,
            label,
            prefer_target_journal=prefer_target_journal,
        )

    def run_command_roundtrip(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        session_id: int,
        sequence_number: int,
        label: str,
        prefer_target_journal: bool = False,
    ) -> str:
        last_error: ProbeFailure | None = None
        for attempt in range(4):
            effective_sequence = sequence_number + attempt
            try:
                return self.send_command_and_wait(
                    ground,
                    profile,
                    session_id,
                    effective_sequence,
                    "OBCApp.bootManager.GET_RESET_CAUSE",
                    tuple(),
                    ("BOOT_RECOVERY_STATUS",),
                    ("BOOT_RECOVERY_STATUS",),
                    30.0,
                    f"{label}-attempt-{attempt + 1}",
                    prefer_target_journal=prefer_target_journal,
                )
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-command-roundtrip-attempt",
                    "fail",
                    attempt=attempt + 1,
                    session_id=session_id,
                    sequence_number=effective_sequence,
                    error=str(exc),
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: command roundtrip failed without a recorded error")
        raise last_error

    def search_channel_snapshot(self, ground: GroundPath, label: str, search: str, timeout_sec: int = 8) -> dict[str, object]:
        cmd = [
            str(self.cli_path),
            "channels",
            "--dictionary",
            str(self.dictionary_path),
            "--no-zmq",
            "-l",
            str(ground.cli_log_dir / "channels"),
            "--log-directly",
            "--tts-port",
            str(ground.gds_tts_port),
            "--search",
            search,
            "--timeout",
            str(timeout_sec),
        ]
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
            cwd=str(self.root_dir),
        )
        text = result.stdout
        with ground.channels_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ {shlex.join(cmd)} # {label}\n")
            handle.write(text)
            if text and not text.endswith("\n"):
                handle.write("\n")
            handle.write(f"returncode={result.returncode}\n")
        matches = [line.strip() for line in text.splitlines() if search in line]
        return {
            "label": label,
            "search": search,
            "returncode": result.returncode,
            "matched": bool(matches),
            "matches": matches[:8],
        }

    def summarize_capture_file(self, path: pathlib.Path) -> dict[str, object]:
        if not path.exists():
            return {"path": str(path), "present": False}
        digest = hashlib.sha256()
        with path.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(chunk)
        return {
            "path": str(path),
            "present": True,
            "size": path.stat().st_size,
            "sha256": digest.hexdigest(),
        }

    def capture_summary(self) -> dict[str, object]:
        return {
            "gdsToSouthbound": self.summarize_capture_file(self.capture_dir / "gds-to-southbound.bin"),
            "southboundToGds": self.summarize_capture_file(self.capture_dir / "southbound-to-gds.bin"),
        }

    def snapshot_ground_window(self, ground: GroundPath) -> GroundWindowMarker:
        return GroundWindowMarker(
            event_line_count=ground.event_count(),
            channels_size=file_size(ground.channels_log),
            gateway_size=file_size(ground.gateway_log),
            gds_log_size=file_size(ground.gds_log),
            raw_command_size=file_size(ground.raw_command_log),
        )

    def snapshot_capture_window(self) -> CaptureWindowMarker:
        return CaptureWindowMarker(
            gds_to_southbound_size=file_size(self.capture_dir / "gds-to-southbound.bin"),
            southbound_to_gds_size=file_size(self.capture_dir / "southbound-to-gds.bin"),
        )

    def snapshot_phase_window(self, *, include_sband: bool, include_uhf: bool) -> PhaseWindowMarker:
        return PhaseWindowMarker(
            sband=self.snapshot_ground_window(self.sband) if include_sband else None,
            uhf=self.snapshot_ground_window(self.uhf) if include_uhf else None,
            capture=self.snapshot_capture_window(),
        )

    def record_nonquiet_diagnosis_artifacts(
        self,
        *,
        root_cause_hypothesis: str,
        sband_session_source: str,
        switch_source: str,
        uhf_session_source: str,
        uhf_command_source: str | None,
        baseline_beacon_count: int | None,
        command_beacon_count: int | None,
        channel_snapshots: dict[str, dict[str, object]] | None,
        obc_groundlink_diagnostics: dict[str, object] | None,
        subsystem_ingress_diagnostics: dict[str, object] | None,
        beacon_artifacts: tuple[pathlib.Path, pathlib.Path, pathlib.Path] | None,
        formal_link_cleanliness: dict[str, object] | None = None,
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        capture_summary = self.capture_summary()
        beacon_capture = {
            "baselineFrameCount": baseline_beacon_count,
            "postCommandFrameCount": command_beacon_count,
        }
        if beacon_artifacts is None:
            beacon_capture["localCapturePath"] = None
            beacon_capture["firstBeaconJson"] = None
            beacon_capture["latestBeaconJson"] = None
        else:
            local_capture, first_beacon_json, resume_beacon_json = beacon_artifacts
            beacon_capture["localCapturePath"] = str(local_capture)
            beacon_capture["firstBeaconJson"] = str(first_beacon_json)
            beacon_capture["latestBeaconJson"] = str(resume_beacon_json)
        payload = {
            "mode": self.mode,
            "profile": self.profile,
            "rootCauseHypothesis": root_cause_hypothesis,
            "quietOverrideApplied": self.quiet_override_applied,
            "targetServiceProfile": self.target_service_profile,
            "targetCommandAuthorityProfile": self.target_command_authority_profile,
            "sbandSessionSource": sband_session_source,
            "switchSource": switch_source,
            "uhfSessionSource": uhf_session_source,
            "uhfCommandSource": uhf_command_source,
            "groundArtifacts": {
                "sbandEventsLog": str(self.sband.events_log),
                "sbandChannelsLog": str(self.sband.channels_log),
                "uhfEventsLog": str(self.uhf.events_log),
                "uhfChannelsLog": str(self.uhf.channels_log),
                "uhfGatewayLog": str(self.uhf.gateway_log),
                "uhfRawCommandLog": str(self.uhf.raw_command_log),
            },
            "journalArtifacts": {
                "obcService": str(self.journal_snapshot_dir / "nonquiet-diagnosis-obc.log"),
                "uhfService": str(self.journal_snapshot_dir / "nonquiet-diagnosis-uhf.log"),
            },
            "channelSnapshots": channel_snapshots or {},
            "obcGroundLinkDiagnostics": obc_groundlink_diagnostics or {},
            "subsystemIngressDiagnostics": subsystem_ingress_diagnostics or {},
            "formalLinkCleanliness": formal_link_cleanliness or {},
            "gatewayCapture": capture_summary,
            "beaconCapture": beacon_capture,
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "nonquiet-diagnosis-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def summarize_nonquiet_formal_cleanliness(self) -> dict[str, object]:
        gds_log_text = read_text(self.uhf.gds_log)
        events_text = read_text(self.uhf.events_log)
        uplink_capture_path = self.capture_dir / "gds-to-southbound.bin"
        uplink_capture = uplink_capture_path.read_bytes() if uplink_capture_path.exists() else b""

        fill_bytes = 0
        index = 0
        while index < len(uplink_capture):
            if uplink_capture[index : index + len(TC_FILL_PATTERN)] == TC_FILL_PATTERN:
                fill_bytes += len(TC_FILL_PATTERN)
                index += len(TC_FILL_PATTERN)
                continue
            index += 1

        link_down_count = events_text.count(self.uhf.link_down_fragment)
        link_up_count = events_text.count(self.uhf.link_up_fragment)
        fill_occurrences = fill_bytes // len(TC_FILL_PATTERN) if TC_FILL_PATTERN else 0
        fill_ratio = 0.0 if not uplink_capture else fill_bytes / len(uplink_capture)

        return {
            "gdsKeepaliveInterval": os.environ.get("COMMV_GDS_KEEPALIVE_INTERVAL", "0"),
            "uhfGdsChecksumWarningCount": gds_log_text.count("Checksum validation failed"),
            "uhfGroundLinkDownCount": link_down_count,
            "uhfGroundLinkUpCount": link_up_count,
            "uhfGroundLinkChurnCount": min(link_down_count, link_up_count),
            "rateGroupCycleSlipCount": events_text.count("RateGroupCycleSlip"),
            "groundBootRecoveryStatusVisible": "BOOT_RECOVERY_STATUS" in events_text,
            "uplinkCapturePath": str(uplink_capture_path),
            "uplinkCaptureBytes": len(uplink_capture),
            "uplinkFillPatternBytes": fill_bytes,
            "uplinkFillPatternOccurrences": fill_occurrences,
            "uplinkFillPatternPresent": fill_occurrences > 0,
            "uplinkFillPatternRatio": round(fill_ratio, 6),
            "uplinkCommandOffsets": {
                "sessionOpenOpcode": uplink_capture.find(bytes.fromhex("10045000")),
                "getResetCauseOpcode": uplink_capture.find(bytes.fromhex("10038006")),
                "commandEnvelopeMagic": uplink_capture.find(bytes.fromhex("0bc0de01")),
            },
        }

    def finalize_nonquiet_diagnosis_artifacts(
        self,
        *,
        root_cause_hypothesis: str,
        sband_session_source: str,
        switch_source: str,
        uhf_session_source: str,
        uhf_command_source: str | None,
        baseline_beacon_count: int | None,
        command_beacon_count: int | None,
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        channel_snapshots = {
            "uhfGroundLinkRxErrors": self.search_channel_snapshot(
                self.uhf,
                "nonquiet-uhf-ground-link-rx-errors",
                "OBCApp.uhfGroundLinkDriver.GROUND_LINK_RX_ERRORS",
            ),
            "uhfGroundLinkTxBytes": self.search_channel_snapshot(
                self.uhf,
                "nonquiet-uhf-ground-link-tx-bytes",
                "OBCApp.uhfGroundLinkDriver.GROUND_LINK_TX_BYTES",
            ),
            "uartRxErrors": self.search_channel_snapshot(
                self.uhf,
                "nonquiet-uart-rx-errors",
                "OBCApp.uartDriver.UART_RX_ERRORS",
            ),
            "sysMode": self.search_channel_snapshot(
                self.uhf,
                "nonquiet-sys-mode",
                "OBCApp.modeManager.SYS_MODE",
            ),
        }
        self.snapshot_journal(self.obc_target, self.obc_service, "nonquiet-diagnosis-obc", lines=300)
        self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "nonquiet-diagnosis-uhf", lines=500)
        obc_journal_path = self.journal_snapshot_dir / "nonquiet-diagnosis-obc.log"
        subsystem_journal_path = self.journal_snapshot_dir / "nonquiet-diagnosis-uhf.log"
        obc_groundlink_lines = [
            line.strip()
            for line in read_text(obc_journal_path).splitlines()
            if "COMM ground link diagnostic:" in line
        ]
        subsystem_ingress_lines = [
            line.strip()
            for line in read_text(subsystem_journal_path).splitlines()
            if "COMM ingress diagnostic:" in line
        ]
        obc_service_env = service_environment(self.obc_target, self.obc_service)
        obc_process_env = service_process_environment(self.obc_target, self.obc_service)
        obc_running_env_value = obc_process_env.get("COMM_GROUNDLINK_DIAGNOSTICS")
        obc_groundlink_diagnostics = {
            "enabled": self.obc_groundlink_diagnostics not in ("", "0"),
            "requestedValue": self.obc_groundlink_diagnostics,
            "configuredEnvironmentValue": obc_service_env.get("COMM_GROUNDLINK_DIAGNOSTICS"),
            "runningProcessEnvironmentValue": obc_running_env_value,
            "runningProcessEnvironmentMatchesRequested": obc_running_env_value == self.obc_groundlink_diagnostics,
            "lineCount": len(obc_groundlink_lines),
            "requestTimeoutCount": sum("failure=request-timeout" in line for line in obc_groundlink_lines),
            "requestExecutionErrorCount": sum("failure=request-execution-error" in line for line in obc_groundlink_lines),
            "replySizeMismatchCount": sum("failure=reply-size-mismatch" in line for line in obc_groundlink_lines),
            "replyVersionMismatchCount": sum("failure=reply-version-mismatch" in line for line in obc_groundlink_lines),
            "explicitReplyDisconnectCount": sum("reply-link-connected=0" in line for line in obc_groundlink_lines),
            "uplinkPollFailureCount": sum("op=uplink-poll" in line and "failure=" in line for line in obc_groundlink_lines),
            "downlinkWriteFailureCount": sum("op=downlink-write" in line and "failure=" in line for line in obc_groundlink_lines),
            "linkStatusFailureCount": sum("op=link-status" in line and "failure=" in line for line in obc_groundlink_lines),
            "tail": obc_groundlink_lines[-20:],
        }
        subsystem_service_env = service_environment(self.subsystem_target, self.uhf_comm_service)
        subsystem_process_env = service_process_environment(self.subsystem_target, self.uhf_comm_service)
        running_env_value = subsystem_process_env.get("COMM_NODE_INGRESS_DIAGNOSTICS")
        subsystem_ingress_diagnostics = {
            "enabled": self.uhf_comm_node_ingress_diagnostics not in ("", "0"),
            "requestedValue": self.uhf_comm_node_ingress_diagnostics,
            "configuredEnvironmentValue": subsystem_service_env.get("COMM_NODE_INGRESS_DIAGNOSTICS"),
            "runningProcessEnvironmentValue": running_env_value,
            "runningProcessEnvironmentMatchesRequested": running_env_value == self.uhf_comm_node_ingress_diagnostics,
            "lineCount": len(subsystem_ingress_lines),
            "tail": subsystem_ingress_lines[-12:],
        }
        beacon_artifacts: tuple[pathlib.Path, pathlib.Path, pathlib.Path] | None = None
        try:
            beacon_artifacts = self.fetch_remote_beacon_capture_artifacts()
        except ProbeFailure as exc:
            self.checkpoint(
                "nonquiet-diagnosis-beacon-artifact-fetch",
                "fail",
                error=str(exc),
            )
        formal_link_cleanliness = self.summarize_nonquiet_formal_cleanliness()
        return self.record_nonquiet_diagnosis_artifacts(
            root_cause_hypothesis=root_cause_hypothesis,
            sband_session_source=sband_session_source,
            switch_source=switch_source,
            uhf_session_source=uhf_session_source,
            uhf_command_source=uhf_command_source,
            baseline_beacon_count=baseline_beacon_count,
            command_beacon_count=command_beacon_count,
            channel_snapshots=channel_snapshots,
            obc_groundlink_diagnostics=obc_groundlink_diagnostics,
            subsystem_ingress_diagnostics=subsystem_ingress_diagnostics,
            beacon_artifacts=beacon_artifacts,
            formal_link_cleanliness=formal_link_cleanliness,
            failure=failure,
        )

    def residual_dual_link_non_claims(self) -> list[str]:
        return [
            "no simultaneous full-authority commands on both links",
            "no one-stock-GDS heterogeneous multi-upstream claim",
            "no one-gateway simultaneous S-band/UHF multiplexer claim",
            "no RF closure",
            "no UHF reliable-transfer redesign",
            "no packet-quiet redesign",
            "no beacon-suppress redesign",
            "no mandatory official file/downlink continuity in the main PASS boundary",
            "no claim that non-quiet node-6 operator observability is clean unless the official branch proved it directly",
        ]

    def phase_window_marker_payload(self, marker: PhaseWindowMarker) -> dict[str, object]:
        def ground_payload(ground_marker: GroundWindowMarker | None) -> dict[str, object] | None:
            if ground_marker is None:
                return None
            return {
                "eventLineCount": ground_marker.event_line_count,
                "channelsSize": ground_marker.channels_size,
                "gatewaySize": ground_marker.gateway_size,
                "gdsLogSize": ground_marker.gds_log_size,
                "rawCommandSize": ground_marker.raw_command_size,
            }

        return {
            "sband": ground_payload(marker.sband),
            "uhf": ground_payload(marker.uhf),
            "capture": {
                "gdsToSouthboundSize": marker.capture.gds_to_southbound_size,
                "southboundToGdsSize": marker.capture.southbound_to_gds_size,
            },
        }

    def phase_window_payload(self, window: PhaseWindow) -> dict[str, object]:
        return {
            "start": self.phase_window_marker_payload(window.start),
            "end": self.phase_window_marker_payload(window.end),
        }

    def summarize_dual_link_operator_observability(
        self,
        *,
        quiet_rescue_used: bool,
        phase_a_window: PhaseWindow,
        phase_b_window: PhaseWindow,
        phase_c_switch_window: PhaseWindow,
        phase_c_truth_window: PhaseWindow,
    ) -> dict[str, object]:
        capture_summary = self.capture_summary()
        formal_link_cleanliness = self.summarize_nonquiet_formal_cleanliness()
        sband_events_text = read_text(self.sband.events_log)
        sband_channels_text = read_text(self.sband.channels_log)
        sband_gateway_text = read_text(self.sband.gateway_log)
        uhf_events_text = read_text(self.uhf.events_log)
        uhf_channels_text = read_text(self.uhf.channels_log)
        uhf_gateway_text = read_text(self.uhf.gateway_log)
        sband_events_lines = sband_events_text.splitlines()
        uhf_events_lines = uhf_events_text.splitlines()
        uhf_capture_visible = any(
            bool(entry.get("present")) and int(entry.get("size", 0)) > 0
            for entry in capture_summary.values()
        )

        def line_window(lines: list[str], start_index: int, end_index: int) -> list[str]:
            return lines[max(0, start_index) : max(0, end_index)]

        def text_window(path: pathlib.Path, start_byte: int, end_byte: int) -> str:
            return read_text_range(path, start_byte, end_byte)

        def capture_delta(window: PhaseWindow, attribute: str) -> int:
            start_value = getattr(window.start.capture, attribute)
            end_value = getattr(window.end.capture, attribute)
            return max(0, end_value - start_value)

        phase_a_sband_events = line_window(
            sband_events_lines,
            phase_a_window.start.sband.event_line_count if phase_a_window.start.sband else 0,
            phase_a_window.end.sband.event_line_count if phase_a_window.end.sband else 0,
        )
        phase_b_sband_events = line_window(
            sband_events_lines,
            phase_b_window.start.sband.event_line_count if phase_b_window.start.sband else 0,
            phase_b_window.end.sband.event_line_count if phase_b_window.end.sband else 0,
        )
        phase_b_uhf_events = line_window(
            uhf_events_lines,
            phase_b_window.start.uhf.event_line_count if phase_b_window.start.uhf else 0,
            phase_b_window.end.uhf.event_line_count if phase_b_window.end.uhf else 0,
        )
        phase_c_switch_sband_events = line_window(
            sband_events_lines,
            phase_c_switch_window.start.sband.event_line_count if phase_c_switch_window.start.sband else 0,
            phase_c_switch_window.end.sband.event_line_count if phase_c_switch_window.end.sband else 0,
        )
        phase_c_switch_uhf_events = line_window(
            uhf_events_lines,
            phase_c_switch_window.start.uhf.event_line_count if phase_c_switch_window.start.uhf else 0,
            phase_c_switch_window.end.uhf.event_line_count if phase_c_switch_window.end.uhf else 0,
        )
        phase_c_truth_uhf_events = line_window(
            uhf_events_lines,
            phase_c_truth_window.start.uhf.event_line_count if phase_c_truth_window.start.uhf else 0,
            phase_c_truth_window.end.uhf.event_line_count if phase_c_truth_window.end.uhf else 0,
        )

        phase_b_uhf_gateway_text = text_window(
            self.uhf.gateway_log,
            phase_b_window.start.uhf.gateway_size if phase_b_window.start.uhf else 0,
            phase_b_window.end.uhf.gateway_size if phase_b_window.end.uhf else 0,
        )
        phase_b_uhf_gds_text = text_window(
            self.uhf.gds_log,
            phase_b_window.start.uhf.gds_log_size if phase_b_window.start.uhf else 0,
            phase_b_window.end.uhf.gds_log_size if phase_b_window.end.uhf else 0,
        )
        phase_b_uhf_raw_text = text_window(
            self.uhf.raw_command_log,
            phase_b_window.start.uhf.raw_command_size if phase_b_window.start.uhf else 0,
            phase_b_window.end.uhf.raw_command_size if phase_b_window.end.uhf else 0,
        )
        phase_c_switch_uhf_gateway_text = text_window(
            self.uhf.gateway_log,
            phase_c_switch_window.start.uhf.gateway_size if phase_c_switch_window.start.uhf else 0,
            phase_c_switch_window.end.uhf.gateway_size if phase_c_switch_window.end.uhf else 0,
        )
        phase_c_truth_uhf_gateway_text = text_window(
            self.uhf.gateway_log,
            phase_c_truth_window.start.uhf.gateway_size if phase_c_truth_window.start.uhf else 0,
            phase_c_truth_window.end.uhf.gateway_size if phase_c_truth_window.end.uhf else 0,
        )
        phase_c_truth_uhf_gds_text = text_window(
            self.uhf.gds_log,
            phase_c_truth_window.start.uhf.gds_log_size if phase_c_truth_window.start.uhf else 0,
            phase_c_truth_window.end.uhf.gds_log_size if phase_c_truth_window.end.uhf else 0,
        )
        phase_c_truth_uhf_raw_text = text_window(
            self.uhf.raw_command_log,
            phase_c_truth_window.start.uhf.raw_command_size if phase_c_truth_window.start.uhf else 0,
            phase_c_truth_window.end.uhf.raw_command_size if phase_c_truth_window.end.uhf else 0,
        )

        sband_event_visible = any(
            fragment in sband_events_text
            for fragment in ("COMMAND_SESSION_OPENED", "BOOT_RECOVERY_STATUS")
        )
        uhf_event_visible = any(
            fragment in uhf_events_text
            for fragment in ("COMMAND_SESSION_OPENED", "BOOT_RECOVERY_STATUS", "COMM_PRIMARY_LINK_CHANGED")
        )
        sband_artifacts_present = (
            self.sband.events_log.exists()
            and self.sband.events_log.stat().st_size > 0
            and self.sband.gateway_log.exists()
            and self.sband.gateway_log.stat().st_size > 0
        )
        uhf_artifacts_present = (
            self.uhf.gateway_log.exists()
            and self.uhf.gateway_log.stat().st_size > 0
            and self.uhf.raw_command_log.exists()
            and self.uhf.raw_command_log.stat().st_size > 0
            and uhf_capture_visible
        )

        phase_a_review = {
            "minimumEvidence": {
                "sbandArtifactsPresent": bool(phase_a_window.end.sband) and phase_a_window.end.sband.gateway_size > phase_a_window.start.sband.gateway_size,
                "sessionVisibleOnSband": any("COMMAND_SESSION_OPENED" in line and "identity 1 role 1" in line for line in phase_a_sband_events),
                "commandCompletionVisibleOnSband": any("BOOT_RECOVERY_STATUS" in line for line in phase_a_sband_events),
            },
            "windowStats": {
                "sbandEventLines": len(phase_a_sband_events),
                "sbandGatewayBytesDelta": (
                    phase_a_window.end.sband.gateway_size - phase_a_window.start.sband.gateway_size
                    if phase_a_window.end.sband and phase_a_window.start.sband
                    else 0
                ),
            },
            "eventTail": phase_a_sband_events[-8:],
        }

        phase_b_link_up_count = sum(self.uhf.link_up_fragment in line for line in phase_b_uhf_events)
        phase_b_link_down_count = sum(self.uhf.link_down_fragment in line for line in phase_b_uhf_events)
        phase_b_review = {
            "minimumEvidence": {
                "sbandCommandCompletionVisible": any("BOOT_RECOVERY_STATUS" in line for line in phase_b_sband_events),
                "sbandSessionVisible": any("COMMAND_SESSION_OPENED" in line and "identity 2 role 2" in line for line in phase_b_sband_events),
                "uhfTransportVisible": (
                    (
                        "dual-link-phase-b-uhf-backup-get-reset-cause" in phase_b_uhf_raw_text
                        or "dual-link-phase-d-quiet-rescue-get-reset-cause" in phase_b_uhf_raw_text
                    )
                    and capture_delta(phase_b_window, "gds_to_southbound_size") > 0
                    and len(phase_b_uhf_gateway_text.splitlines()) > 0
                ),
            },
            "noisySignals": {
                "uhfChecksumWarningCount": phase_b_uhf_gds_text.count("Checksum validation failed"),
                "uhfGroundLinkUpCount": phase_b_link_up_count,
                "uhfGroundLinkDownCount": phase_b_link_down_count,
                "uhfGroundLinkChurnCount": min(phase_b_link_up_count, phase_b_link_down_count),
            },
            "transportWindow": {
                "uhfGatewayBytesDelta": (
                    phase_b_window.end.uhf.gateway_size - phase_b_window.start.uhf.gateway_size
                    if phase_b_window.end.uhf and phase_b_window.start.uhf
                    else 0
                ),
                "uhfRawCommandBytesDelta": (
                    phase_b_window.end.uhf.raw_command_size - phase_b_window.start.uhf.raw_command_size
                    if phase_b_window.end.uhf and phase_b_window.start.uhf
                    else 0
                ),
                "uplinkCaptureBytesDelta": capture_delta(phase_b_window, "gds_to_southbound_size"),
                "downlinkCaptureBytesDelta": capture_delta(phase_b_window, "southbound_to_gds_size"),
            },
            "sbandEventTail": phase_b_sband_events[-8:],
            "uhfEventTail": phase_b_uhf_events[-8:],
            "uhfGatewayTail": phase_b_uhf_gateway_text.splitlines()[-8:],
        }

        phase_c_switch_review = {
            "groundSwitchVisible": (
                any("COMM_PRIMARY_LINK_CHANGED" in line for line in phase_c_switch_sband_events)
                or any("COMM_PRIMARY_LINK_CHANGED" in line for line in phase_c_switch_uhf_events)
            ),
            "windowStats": {
                "uhfGatewayBytesDelta": (
                    phase_c_switch_window.end.uhf.gateway_size - phase_c_switch_window.start.uhf.gateway_size
                    if phase_c_switch_window.end.uhf and phase_c_switch_window.start.uhf
                    else 0
                ),
                "uplinkCaptureBytesDelta": capture_delta(phase_c_switch_window, "gds_to_southbound_size"),
                "downlinkCaptureBytesDelta": capture_delta(phase_c_switch_window, "southbound_to_gds_size"),
            },
            "sbandEventTail": phase_c_switch_sband_events[-8:],
            "uhfEventTail": phase_c_switch_uhf_events[-8:],
            "uhfGatewayTail": phase_c_switch_uhf_gateway_text.splitlines()[-8:],
        }

        phase_c_link_up_count = sum(self.uhf.link_up_fragment in line for line in phase_c_truth_uhf_events)
        phase_c_link_down_count = sum(self.uhf.link_down_fragment in line for line in phase_c_truth_uhf_events)
        phase_c_truth_review = {
            "minimumEvidence": {
                "uhfTransportVisible": (
                    "dual-link-phase-c-uhf-primary-get-reset-cause" in phase_c_truth_uhf_raw_text
                    and capture_delta(phase_c_truth_window, "gds_to_southbound_size") > 0
                    and len(phase_c_truth_uhf_gateway_text.splitlines()) > 0
                ),
                "groundSwitchVisible": phase_c_switch_review["groundSwitchVisible"],
                "packetQuietExpected": True,
            },
            "groundCorroboration": {
                "sessionVisibleOnUhfEvents": any(
                    "COMMAND_SESSION_OPENED" in line and "identity 2 role 3" in line for line in phase_c_truth_uhf_events
                ),
                "commandCompletionVisibleOnUhfEvents": any("BOOT_RECOVERY_STATUS" in line for line in phase_c_truth_uhf_events),
            },
            "noisySignals": {
                "uhfChecksumWarningCount": phase_c_truth_uhf_gds_text.count("Checksum validation failed"),
                "uhfGroundLinkUpCount": phase_c_link_up_count,
                "uhfGroundLinkDownCount": phase_c_link_down_count,
                "uhfGroundLinkChurnCount": min(phase_c_link_up_count, phase_c_link_down_count),
            },
            "transportWindow": {
                "uhfGatewayBytesDelta": (
                    phase_c_truth_window.end.uhf.gateway_size - phase_c_truth_window.start.uhf.gateway_size
                    if phase_c_truth_window.end.uhf and phase_c_truth_window.start.uhf
                    else 0
                ),
                "uhfRawCommandBytesDelta": (
                    phase_c_truth_window.end.uhf.raw_command_size - phase_c_truth_window.start.uhf.raw_command_size
                    if phase_c_truth_window.end.uhf and phase_c_truth_window.start.uhf
                    else 0
                ),
                "uplinkCaptureBytesDelta": capture_delta(phase_c_truth_window, "gds_to_southbound_size"),
                "downlinkCaptureBytesDelta": capture_delta(phase_c_truth_window, "southbound_to_gds_size"),
            },
            "uhfEventTail": phase_c_truth_uhf_events[-8:],
            "uhfGatewayTail": phase_c_truth_uhf_gateway_text.splitlines()[-8:],
        }

        failure_reasons: list[str] = []
        if not sband_artifacts_present:
            failure_reasons.append("sband-ground-artifacts-missing-or-empty")
        if not uhf_artifacts_present:
            failure_reasons.append("uhf-ground-artifacts-missing-or-empty")
        if not sband_event_visible:
            failure_reasons.append("sband-ground-command-truth-not-visible")
        if not uhf_event_visible and not uhf_capture_visible:
            failure_reasons.append("uhf-ground-traffic-absent-or-unusable")
        if not bool(phase_a_review["minimumEvidence"]["commandCompletionVisibleOnSband"]):
            failure_reasons.append("phase-a-sband-command-completion-not-visible")
        if not bool(phase_b_review["minimumEvidence"]["sbandCommandCompletionVisible"]):
            failure_reasons.append("phase-b-sband-command-completion-not-visible")
        if not bool(phase_b_review["minimumEvidence"]["uhfTransportVisible"]):
            failure_reasons.append("phase-b-uhf-transport-evidence-missing")
        if not bool(phase_c_truth_review["minimumEvidence"]["uhfTransportVisible"]):
            failure_reasons.append("phase-c-post-switch-uhf-transport-evidence-missing")

        degradation_reasons: list[str] = []
        if quiet_rescue_used:
            degradation_reasons.append("phase-b-quiet-rescue-used")
        if int(phase_b_review["noisySignals"]["uhfChecksumWarningCount"]) > 0:
            degradation_reasons.append("phase-b-uhf-gds-checksum-warnings-observed")
        if int(phase_b_review["noisySignals"]["uhfGroundLinkChurnCount"]) > 0:
            degradation_reasons.append("phase-b-uhf-ground-link-churn-observed")
        if int(phase_c_truth_review["noisySignals"]["uhfChecksumWarningCount"]) > 0:
            degradation_reasons.append("phase-c-uhf-gds-checksum-warnings-observed")
        if int(phase_c_truth_review["noisySignals"]["uhfGroundLinkChurnCount"]) > 0:
            degradation_reasons.append("phase-c-post-switch-uhf-ground-link-churn-observed")

        verdict = "FAIL"
        if not failure_reasons:
            verdict = "DEGRADED" if degradation_reasons else "PASS"

        return {
            "verdict": verdict,
            "failureReasons": failure_reasons,
            "degradationReasons": degradation_reasons,
            "minimumArtifactPresence": {
                "sbandGroundArtifactsPresent": sband_artifacts_present,
                "uhfGroundArtifactsPresent": uhf_artifacts_present,
                "sbandGroundTrafficVisible": sband_event_visible,
                "uhfGroundTrafficVisible": uhf_event_visible or uhf_capture_visible,
                "phaseASbandCompletionVisible": phase_a_review["minimumEvidence"]["commandCompletionVisibleOnSband"],
                "phaseBSbandCompletionVisible": phase_b_review["minimumEvidence"]["sbandCommandCompletionVisible"],
                "phaseBUhfTransportVisible": phase_b_review["minimumEvidence"]["uhfTransportVisible"],
                "phaseCPostSwitchUhfTransportVisible": phase_c_truth_review["minimumEvidence"]["uhfTransportVisible"],
            },
            "formalLinkCleanliness": formal_link_cleanliness,
            "phaseReviewability": {
                "phaseA": phase_a_review,
                "phaseB": phase_b_review,
                "phaseCSwitch": phase_c_switch_review,
                "phaseCTruth": phase_c_truth_review,
            },
            "classificationNotes": [
                "phase-b command completion is corroborated on sband-primary because phase-b keeps live telemetry on the default node-5 path",
                "phase-c post-switch command completion is not required on uhf ground events because UHF primary packet quiet suppresses live event/tlm packet egress on that path",
            ],
            "phaseWindows": {
                "phaseA": self.phase_window_payload(phase_a_window),
                "phaseB": self.phase_window_payload(phase_b_window),
                "phaseCSwitch": self.phase_window_payload(phase_c_switch_window),
                "phaseCTruth": self.phase_window_payload(phase_c_truth_window),
            },
            "groundArtifactSizes": {
                "sbandEventsLogBytes": self.sband.events_log.stat().st_size if self.sband.events_log.exists() else 0,
                "sbandChannelsLogBytes": self.sband.channels_log.stat().st_size if self.sband.channels_log.exists() else 0,
                "sbandGatewayLogBytes": self.sband.gateway_log.stat().st_size if self.sband.gateway_log.exists() else 0,
                "uhfEventsLogBytes": self.uhf.events_log.stat().st_size if self.uhf.events_log.exists() else 0,
                "uhfChannelsLogBytes": self.uhf.channels_log.stat().st_size if self.uhf.channels_log.exists() else 0,
                "uhfGatewayLogBytes": self.uhf.gateway_log.stat().st_size if self.uhf.gateway_log.exists() else 0,
            },
            "groundArtifactSnippets": {
                "sbandGatewayTail": sband_gateway_text.splitlines()[-12:],
                "sbandEventsTail": sband_events_text.splitlines()[-12:],
                "sbandChannelsTail": sband_channels_text.splitlines()[-12:],
                "uhfGatewayTail": uhf_gateway_text.splitlines()[-12:],
                "uhfEventsTail": uhf_events_text.splitlines()[-12:],
                "uhfChannelsTail": uhf_channels_text.splitlines()[-12:],
            },
        }

    def record_dual_link_proof_artifacts(
        self,
        *,
        phase_a_session_source: str,
        phase_a_command_source: str | None,
        phase_b_session_source: str,
        phase_b_command_source: str | None,
        phase_b_result: str,
        phase_b_nonquiet_error: str | None,
        phase_c_sband_session_source: str,
        switch_source: str,
        phase_c_uhf_session_source: str,
        phase_c_command_source: str | None,
        target_claim: str,
        operator_observability: dict[str, object],
        quiet_rescue_used: bool,
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        capture_summary = self.capture_summary()
        payload = {
            "mode": self.mode,
            "profile": self.profile,
            "targetPathUnderTest": "default target node-5 S-band primary plus physical node-6 UHF backup/switched primary over target CAN plus UHF UART",
            "bandRoleSequence": [
                "phase-a: node-5 sband-primary GET_RESET_CAUSE",
                "phase-b: node-6 non-quiet uhf-backup GET_RESET_CAUSE",
                "phase-c: sband-primary COMM_SET_ACTIVE(UHF)",
                "phase-c: node-6 non-quiet uhf-primary-after-failover GET_RESET_CAUSE",
            ],
            "oraclePrecedence": [
                {
                    "phase": "A",
                    "authoritativeSurface": "target-journal-first command completion",
                    "groundRole": "reviewable corroboration only",
                },
                {
                    "phase": "B",
                    "authoritativeSurface": "target-journal-first command completion on uhf-backup adjunct",
                    "groundRole": "reviewable corroboration only",
                },
                {
                    "phase": "C",
                    "authoritativeSurface": "target-observed COMM_PRIMARY_LINK_CHANGED followed by target-journal-first UHF command completion",
                    "groundRole": "reviewable corroboration only",
                },
            ],
            "phaseResults": {
                "phaseA": {
                    "authorityProfile": "sband-primary",
                    "sessionSource": phase_a_session_source,
                    "commandSource": phase_a_command_source,
                },
                "phaseB": {
                    "authorityProfile": "uhf-backup",
                    "result": phase_b_result,
                    "sessionSource": phase_b_session_source,
                    "commandSource": phase_b_command_source,
                    "nonquietError": phase_b_nonquiet_error,
                    "quietRescueUsed": quiet_rescue_used,
                },
                "phaseC": {
                    "switchCommandAuthorityProfile": "sband-primary",
                    "sbandSessionSource": phase_c_sband_session_source,
                    "switchSource": switch_source,
                    "uhfAuthorityProfile": "uhf-primary",
                    "uhfSessionSource": phase_c_uhf_session_source,
                    "commandSource": phase_c_command_source,
                },
            },
            "verdicts": {
                "targetClaim": target_claim,
                "operatorObservability": operator_observability.get("verdict"),
            },
            "outcomeBranch": f"target-claim={target_claim}/operator-observability={operator_observability.get('verdict')}",
            "quietRescueUsed": quiet_rescue_used,
            "groundArtifacts": {
                "sbandEventsLog": str(self.sband.events_log),
                "sbandChannelsLog": str(self.sband.channels_log),
                "sbandGatewayLog": str(self.sband.gateway_log),
                "uhfEventsLog": str(self.uhf.events_log),
                "uhfChannelsLog": str(self.uhf.channels_log),
                "uhfGatewayLog": str(self.uhf.gateway_log),
                "uhfRawCommandLog": str(self.uhf.raw_command_log),
            },
            "journalArtifacts": {
                "obcService": str(self.journal_snapshot_dir / "dual-link-proof-obc.log"),
                "uhfService": str(self.journal_snapshot_dir / "dual-link-proof-uhf.log"),
            },
            "gatewayCapture": capture_summary,
            "operatorObservability": operator_observability,
            "residualNonClaims": self.residual_dual_link_non_claims(),
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "dual-link-proof-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def run_sequence_roundtrip(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_base: int, artifact_name: str) -> None:
        artifact = self.build_sequence_artifact(artifact_name)
        self.upload_sequence(ground, artifact)
        self.send_command_and_wait(
            ground,
            profile,
            session_id,
            sequence_base,
            "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
            (artifact.destination,),
            ("COMMAND_ENVELOPE_OBSERVED",),
            ("COMMAND_ENVELOPE_OBSERVED",),
            18.0,
            f"{ground.name} sequence validate",
        )
        ground.assert_no_event("SEQUENCE_CONTROL_REJECTED", timeout=2.0, start=0)
        ground.assert_no_event("COMMAND_AUTHORITY_REJECTED", timeout=2.0, start=0)
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        run_ground_start = ground.event_count()
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        self.send_envelope(
            ground,
            f"{ground.name} sequence run",
            profile,
            session_id,
            sequence_base + 1,
            "OBCApp.sequenceAdmissionController.SEQ_RUN",
            artifact.destination,
            "WAIT",
        )
        self.wait_ground_or_journal(
            ground,
            run_ground_start,
            journal_since,
            ("CS_SequenceComplete",),
            ("CS_SequenceComplete",),
            50.0,
            f"{ground.name} sequence run",
        )
        eps_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        adcs_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
        self.wait_ground_or_journal(
            ground,
            run_ground_start,
            journal_since,
            (f"Opcode 0x{eps_opcode:x} completed",),
            (f"Opcode 0x{eps_opcode:x} completed",),
            8.0,
            f"{ground.name} EPS_GET_STATUS completion",
        )
        self.wait_ground_or_journal(
            ground,
            run_ground_start,
            journal_since,
            (f"Opcode 0x{adcs_opcode:x} completed",),
            (f"Opcode 0x{adcs_opcode:x} completed",),
            8.0,
            f"{ground.name} ADCS_GET_ATTITUDE completion",
        )
        ensure_no_legacy_aliases(self.root_dir)

    def list_remote_data_product_files(self) -> list[str]:
        script = (
            f"set -euo pipefail; "
            f"if [[ -d {shq(self.runtime_root + '/data-products')} ]]; then "
            f"find {shq(self.runtime_root + '/data-products')} -maxdepth 1 -type f -name 'Dp_*.fdp' | sort; "
            f"fi"
        )
        text = ssh_capture(self.obc_target, script)
        return [line.strip() for line in text.splitlines() if line.strip()]

    def reset_remote_data_products(self) -> None:
        data_products_root = self.runtime_root + "/data-products"
        ssh_capture(
            self.obc_target,
            (
                f"set -euo pipefail; mkdir -p {shq(data_products_root)}; "
                f"find {shq(data_products_root)} -maxdepth 1 -type f "
                "\\( -name 'Dp_*.fdp' -o -name 'DpState.dat' \\) -delete"
            ),
        )

    def wait_for_remote_data_product_files(self, timeout_sec: int) -> list[str]:
        attempts = 0
        files = self.list_remote_data_product_files()
        max_attempts = max(1, int(timeout_sec))
        while len(files) < 1 and attempts < max_attempts:
            attempts += 1
            time.sleep(1.0)
            files = self.list_remote_data_product_files()
        if len(files) < 1:
            raise ProbeFailure("No official remote .fdp files appeared under target runtime for file-downlink proof")
        return files

    def fetch_remote_file(self, remote_path: str, snapshot_name: str) -> pathlib.Path:
        snapshot_dir = self.probe_root / "source-snapshots"
        snapshot_dir.mkdir(parents=True, exist_ok=True)
        snapshot_path = snapshot_dir / snapshot_name
        snapshot_path.write_bytes(ssh_capture_bytes(self.obc_target, f"set -euo pipefail; cat {shq(remote_path)}"))
        return snapshot_path

    def snapshot_remote_source_files(self, remote_source_paths: list[str], label_prefix: str) -> list[dict[str, pathlib.Path | str]]:
        snapshots = []
        for index, remote_source_path in enumerate(remote_source_paths, start=1):
            source_name = pathlib.Path(remote_source_path).name
            snapshot_name = f"{label_prefix}-{index}-{source_name}"
            snapshots.append({"source_path": remote_source_path, "snapshot_path": self.fetch_remote_file(remote_source_path, snapshot_name)})
        return snapshots

    def received_downlink_dir(self, ground: GroundPath) -> pathlib.Path:
        return ground.file_storage / "fprime-downlink"

    def remove_received_fdp_files(self, ground: GroundPath) -> None:
        for path in self.received_downlink_dir(ground).glob("*.fdp"):
            try:
                path.unlink()
            except FileNotFoundError:
                pass

    def sha256_file(self, path: pathlib.Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(chunk)
        return digest.hexdigest()

    def wait_for_any_matching_file(self, ground: GroundPath, expected_snapshots: list[dict[str, pathlib.Path | str]], timeout_sec: int) -> tuple[dict[str, pathlib.Path | str], pathlib.Path, str, int]:
        deadline = time.time() + timeout_sec
        expected_map = {
            pathlib.Path(entry["snapshot_path"]): pathlib.Path(entry["snapshot_path"]).read_bytes()  # type: ignore[arg-type]
            for entry in expected_snapshots
        }
        while time.time() < deadline:
            for received_path in sorted(self.received_downlink_dir(ground).glob("*.fdp")):
                if not received_path.is_file():
                    continue
                payload = received_path.read_bytes()
                for entry in expected_snapshots:
                    snapshot_path = pathlib.Path(entry["snapshot_path"])  # type: ignore[arg-type]
                    if payload == expected_map[snapshot_path]:
                        return entry, received_path, self.sha256_file(received_path), received_path.stat().st_size
            time.sleep(0.5)
        expected_names = ", ".join(pathlib.Path(str(entry["source_path"])).name for entry in expected_snapshots)
        raise ProbeFailure(f"Timed out waiting for any byte-matching remote .fdp downlink among [{expected_names}]")

    def reliable_transfer_receiver_target(self) -> tuple[str, str]:
        if self.profile == "uhf-primary":
            return self.uhf_comm_service, "/tmp/comm-reliable-transfer-node6"
        return self.sband_comm_service, "/tmp/comm-reliable-transfer-node5"

    def enable_reliable_transfer_receiver(self) -> None:
        service, output_dir = self.reliable_transfer_receiver_target()
        self.reliable_transfer_receiver_service = service
        self.reliable_transfer_output_dir = output_dir
        self.checkpoint(
            "reliable-transfer-receiver-enable",
            "info",
            service=service,
            output_dir=self.reliable_transfer_output_dir,
            receiver_override_dropin=self.reliable_transfer_receiver_override_dropin_name,
            admission_override_dropin=self.reliable_transfer_admission_override_dropin_name,
        )
        apply_service_override(
            self.obc_target,
            self.obc_service,
            self.reliable_transfer_admission_override_dropin_name,
            {
                "COMM_RT_OUTPUT_DIR": self.reliable_transfer_output_dir,
            },
        )
        self.reliable_transfer_admission_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.snapshot_service(self.obc_target, self.obc_service, "reliable-transfer-admission-override-applied")
        self.checkpoint(
            "reliable-transfer-admission-override-applied",
            "pass",
            service=self.obc_service,
            output_dir=self.reliable_transfer_output_dir,
            override_dropin=self.reliable_transfer_admission_override_dropin_name,
        )
        apply_service_override(
            self.subsystem_target,
            service,
            self.reliable_transfer_receiver_override_dropin_name,
            {
                "COMM_RT_OUTPUT_DIR": self.reliable_transfer_output_dir,
                "COMM_RT_ACK_TIMEOUT_POLLS": "0",
            },
        )
        self.reliable_transfer_receiver_override_applied = True
        self.checkpoint(
            "reliable-transfer-receiver-override-applied",
            "pass",
            service=service,
            output_dir=self.reliable_transfer_output_dir,
            override_dropin=self.reliable_transfer_receiver_override_dropin_name,
        )
        wait_service_active(self.subsystem_target, service, self.restart_timeout)
        self.checkpoint(
            "reliable-transfer-receiver-service-ready",
            "pass",
            service=service,
            output_dir=self.reliable_transfer_output_dir,
        )
        self.reset_remote_reliable_transfer_output()
        self.checkpoint(
            "reliable-transfer-receiver-output-reset",
            "pass",
            service=service,
            output_dir=self.reliable_transfer_output_dir,
        )

    def reset_remote_reliable_transfer_output(self) -> None:
        ssh_capture(
            self.subsystem_target,
            f"set -euo pipefail; rm -rf {shq(self.reliable_transfer_output_dir)}; mkdir -p {shq(self.reliable_transfer_output_dir)}",
            check=False,
        )

    def list_remote_reliable_transfer_files(self) -> list[str]:
        script = (
            f"set -euo pipefail; "
            f"if [[ -d {shq(self.reliable_transfer_output_dir)} ]]; then "
            f"find {shq(self.reliable_transfer_output_dir)} -maxdepth 1 -type f -name '*.fdp' | sort; "
            f"fi"
        )
        text = ssh_capture(self.subsystem_target, script)
        return [line.strip() for line in text.splitlines() if line.strip()]

    def wait_for_any_matching_reliable_transfer_file(
        self,
        expected_snapshots: list[dict[str, pathlib.Path | str]],
        timeout_sec: int,
    ) -> tuple[dict[str, pathlib.Path | str], str, pathlib.Path, str, int]:
        deadline = time.time() + timeout_sec
        expected_map = {
            pathlib.Path(entry["snapshot_path"]): pathlib.Path(entry["snapshot_path"]).read_bytes()  # type: ignore[arg-type]
            for entry in expected_snapshots
        }
        received_dir = self.probe_root / "reliable-transfer-received"
        received_dir.mkdir(parents=True, exist_ok=True)
        while time.time() < deadline:
            for remote_path in self.list_remote_reliable_transfer_files():
                payload = ssh_capture_bytes(self.subsystem_target, f"set -euo pipefail; cat {shq(remote_path)}")
                for entry in expected_snapshots:
                    snapshot_path = pathlib.Path(entry["snapshot_path"])  # type: ignore[arg-type]
                    if payload == expected_map[snapshot_path]:
                        local_path = received_dir / pathlib.Path(remote_path).name
                        local_path.write_bytes(payload)
                        digest = hashlib.sha256(payload).hexdigest()
                        return entry, remote_path, local_path, digest, len(payload)
            time.sleep(0.5)
        expected_names = ", ".join(pathlib.Path(str(entry["source_path"])).name for entry in expected_snapshots)
        raise ProbeFailure(f"Timed out waiting for any byte-matching reliable-transfer file among [{expected_names}]")

    def wait_for_sending_product_source_path(self, journal_since: str, timeout: float) -> str:
        deadline = time.time() + timeout
        last_journal = ""
        while time.time() < deadline:
            last_journal = ssh_capture(
                self.obc_target,
                f"journalctl -u {shq(self.obc_service)} --since {shq(journal_since)} --no-pager || true",
                check=False,
            )
            matches = re.findall(r"Sending product (\S+?\.fdp) of size \d+ priority \d+", last_journal)
            if matches:
                return matches[-1]
            time.sleep(0.5)
        raise ProbeFailure(
            "timed out waiting for SendingProduct source path in target journal\n"
            f"journal_tail={last_journal[-4000:]}"
        )

    def run_start_xmit_catalog(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_number: int) -> str:
        errors = []
        for argument in ("1", "NO_WAIT", "Fw.Wait.NO_WAIT"):
            try:
                self.checkpoint(
                    f"{ground.name}-start-xmit-attempt",
                    "info",
                    argument=argument,
                    session_id=session_id,
                    sequence_number=sequence_number,
                )
                self.prepare_ground_window(ground)
                time.sleep(0.3)
                journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
                start = self.send_envelope(
                    ground,
                    f"{ground.name} START_XMIT_CATALOG {argument}",
                    profile,
                    session_id,
                    sequence_number,
                    "OBCApp.dpCatalog.START_XMIT_CATALOG",
                    argument,
                )
                self.wait_ground_or_journal(
                    ground,
                    start,
                    journal_since,
                    ("SendingProduct",),
                    ("SendingProduct",),
                    25.0,
                    f"{ground.name} START_XMIT_CATALOG {argument}",
                )
                source_path = self.wait_for_sending_product_source_path(journal_since, 10.0)
                self.checkpoint(
                    f"{ground.name}-start-xmit-visible",
                    "pass",
                    argument=argument,
                    session_id=session_id,
                    sequence_number=sequence_number,
                    source_path=source_path,
                )
                return source_path
            except Exception as exc:
                errors.append(str(exc))
                self.checkpoint(
                    f"{ground.name}-start-xmit-visible",
                    "fail",
                    argument=argument,
                    session_id=session_id,
                    sequence_number=sequence_number,
                    error=str(exc),
                )
        raise ProbeFailure("START_XMIT_CATALOG failed with all enum argument forms: " + " | ".join(errors))

    def run_start_xmit_catalog_secure(self, ground: GroundPath, session: SecureAuthSession) -> str:
        errors = []
        for argument in ("1", "NO_WAIT", "Fw.Wait.NO_WAIT"):
            try:
                journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
                sequence_number, _ = self.send_secure_command_name(
                    ground,
                    session,
                    f"{ground.name} secure START_XMIT_CATALOG {argument}",
                    "OBCApp.dpCatalog.START_XMIT_CATALOG",
                    argument,
                    accept_sequence=True,
                    journal_fragments=("SendingProduct",),
                    timeout=25.0,
                )
                source_path = self.wait_for_sending_product_source_path(journal_since, 10.0)
                self.checkpoint(
                    f"{ground.name}-secure-start-xmit-visible",
                    "pass",
                    argument=argument,
                    sequence_number=sequence_number,
                    source_path=source_path,
                )
                return source_path
            except Exception as exc:
                errors.append(str(exc))
                self.checkpoint(
                    f"{ground.name}-secure-start-xmit-visible",
                    "fail",
                    argument=argument,
                    error=str(exc),
                )
        raise ProbeFailure("secure START_XMIT_CATALOG failed with all enum argument forms: " + " | ".join(errors))

    def run_file_downlink(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_base: int, label_prefix: str) -> tuple[str, str, int]:
        self.remove_received_fdp_files(ground)
        self.reset_remote_data_products()
        fresh_sources = self.wait_for_remote_data_product_files(30)
        self.checkpoint(
            f"{ground.name}-remote-data-products-reset",
            "pass",
            source_count=len(fresh_sources),
            sources=fresh_sources[:4],
        )
        last_error: ProbeFailure | None = None
        for attempt in range(1, 4):
            try:
                self.send_command_and_wait(
                    ground,
                    profile,
                    session_id,
                    sequence_base + (attempt * 2) - 2,
                    "OBCApp.dpCatalog.BUILD_CATALOG",
                    tuple(),
                    ("CatalogBuildComplete",),
                    ("CatalogBuildComplete",),
                    25.0,
                    f"{ground.name} BUILD_CATALOG attempt {attempt}",
                )
                self.checkpoint(f"{ground.name}-build-catalog-visible", "pass", attempt=attempt)
                selected_source_path = self.run_start_xmit_catalog(ground, profile, session_id, sequence_base + (attempt * 2) - 1)
                self.checkpoint(
                    f"{ground.name}-file-source-visible",
                    "pass",
                    attempt=attempt,
                    source_count=1,
                    sources=[selected_source_path],
                )
                expected_snapshots = self.snapshot_remote_source_files([selected_source_path], f"{label_prefix}-attempt-{attempt}")
                matched_entry, matched_received_path, matched_received_hash, matched_received_size = self.wait_for_any_matching_file(ground, expected_snapshots, 120)
                self.checkpoint(
                    f"{ground.name}-file-downlink-match",
                    "pass",
                    attempt=attempt,
                    source_path=str(matched_entry["source_path"]),
                    received_path=str(matched_received_path),
                    received_size=matched_received_size,
                    received_hash=matched_received_hash,
                )
                return str(matched_entry["source_path"]), str(matched_received_path), matched_received_size
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-file-downlink-match",
                    "fail",
                    attempt=attempt,
                    error=str(exc),
                )
                self.remove_received_fdp_files(ground)
                time.sleep(2.0)
        if last_error is None:
            raise ProbeFailure("file downlink failed without a recorded error")
        raise last_error

    def run_file_downlink_secure(self, ground: GroundPath, session: SecureAuthSession, label_prefix: str) -> tuple[str, str, int]:
        self.remove_received_fdp_files(ground)
        self.reset_remote_data_products()
        hk_trend_flush_oracle = self.build_ground_event_readback_oracle(
            event_log_path=ground.native_event_log,
            event_offset_attr="native_event_log_offset",
            ground_fragments=("HK_TREND_PRODUCT_WRITTEN",),
            source_name="ground-native-event-log",
        )
        self.send_secure_command_until_ground_readback(
            ground,
            session,
            f"{label_prefix}-hk-trend-flush",
            "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH",
            accept_sequence=True,
            journal_fragments=("HK_TREND_PRODUCT_WRITTEN",),
            ground_success_oracle=hk_trend_flush_oracle,
            attempt_limit=30,
            per_attempt_timeout=1.0,
        )
        fresh_sources = self.wait_for_remote_data_product_files(30)
        self.checkpoint(
            f"{ground.name}-secure-remote-data-products-reset",
            "pass",
            source_count=len(fresh_sources),
            sources=fresh_sources[:4],
        )
        last_error: ProbeFailure | None = None
        for attempt in range(1, 4):
            try:
                self.send_secure_command_name(
                    ground,
                    session,
                    f"{ground.name} secure BUILD_CATALOG attempt {attempt}",
                    "OBCApp.dpCatalog.BUILD_CATALOG",
                    accept_sequence=True,
                    journal_fragments=("CatalogBuildComplete",),
                    timeout=25.0,
                )
                self.checkpoint(f"{ground.name}-secure-build-catalog-visible", "pass", attempt=attempt)
                selected_source_path = self.run_start_xmit_catalog_secure(ground, session)
                self.checkpoint(
                    f"{ground.name}-secure-file-source-visible",
                    "pass",
                    attempt=attempt,
                    source_count=1,
                    sources=[selected_source_path],
                )
                expected_snapshots = self.snapshot_remote_source_files([selected_source_path], f"{label_prefix}-attempt-{attempt}")
                matched_entry, matched_received_path, matched_received_hash, matched_received_size = self.wait_for_any_matching_file(
                    ground,
                    expected_snapshots,
                    120,
                )
                self.checkpoint(
                    f"{ground.name}-secure-file-downlink-match",
                    "pass",
                    attempt=attempt,
                    source_path=str(matched_entry["source_path"]),
                    received_path=str(matched_received_path),
                    received_size=matched_received_size,
                    received_hash=matched_received_hash,
                )
                return str(matched_entry["source_path"]), str(matched_received_path), matched_received_size
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-secure-file-downlink-match",
                    "fail",
                    attempt=attempt,
                    error=str(exc),
                )
                self.remove_received_fdp_files(ground)
                time.sleep(2.0)
        if last_error is None:
            raise ProbeFailure("secure file downlink failed without a recorded error")
        raise last_error

    def run_reliable_transfer(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        session_id: int,
        sequence_base: int,
        label_prefix: str,
    ) -> tuple[str, str, int]:
        self.remove_received_fdp_files(ground)
        self.reset_remote_reliable_transfer_output()
        self.reset_remote_data_products()
        fresh_sources = self.wait_for_remote_data_product_files(30)
        self.checkpoint(
            f"{ground.name}-remote-data-products-reset",
            "pass",
            source_count=len(fresh_sources),
            sources=fresh_sources[:4],
        )
        last_error: ProbeFailure | None = None
        for attempt in range(1, 4):
            self.checkpoint(
                f"{ground.name}-reliable-transfer-attempt",
                "info",
                attempt=attempt,
                session_id=session_id,
                sequence_base=sequence_base,
            )
            try:
                self.send_command_and_wait(
                    ground,
                    profile,
                    session_id,
                    sequence_base + (attempt * 2) - 2,
                    "OBCApp.dpCatalog.BUILD_CATALOG",
                    tuple(),
                    ("CatalogBuildComplete",),
                    ("CatalogBuildComplete",),
                    25.0,
                    f"{ground.name} BUILD_CATALOG attempt {attempt}",
                )
                self.checkpoint(f"{ground.name}-reliable-transfer-build-visible", "pass", attempt=attempt)
                selected_source_path = self.run_start_xmit_catalog(ground, profile, session_id, sequence_base + (attempt * 2) - 1)
                self.checkpoint(
                    f"{ground.name}-reliable-transfer-source-visible",
                    "pass",
                    attempt=attempt,
                    source_count=1,
                    sources=[selected_source_path],
                )
                expected_snapshots = self.snapshot_remote_source_files([selected_source_path], f"{label_prefix}-attempt-{attempt}")
                self.checkpoint(
                    f"{ground.name}-reliable-transfer-receiver-wait",
                    "info",
                    attempt=attempt,
                    expected_sources=[str(entry["source_path"]) for entry in expected_snapshots],
                    remote_output_dir=self.reliable_transfer_output_dir,
                )
                matched_entry, remote_received_path, local_received_path, received_hash, received_size = (
                    self.wait_for_any_matching_reliable_transfer_file(expected_snapshots, 120)
                )
                if any(self.received_downlink_dir(ground).glob("*.fdp")):
                    raise ProbeFailure("reliable transfer unexpectedly populated stock GDS file storage")
                self.checkpoint(
                    f"{ground.name}-reliable-transfer-match",
                    "pass",
                    attempt=attempt,
                    source_path=str(matched_entry["source_path"]),
                    remote_received_path=remote_received_path,
                    local_received_path=str(local_received_path),
                    received_size=received_size,
                    received_hash=received_hash,
                )
                return str(matched_entry["source_path"]), remote_received_path, received_size
            except ProbeFailure as exc:
                last_error = exc
                self.checkpoint(
                    f"{ground.name}-reliable-transfer-match",
                    "fail",
                    attempt=attempt,
                    error=str(exc),
                )
                self.reset_remote_reliable_transfer_output()
                time.sleep(2.0)
        if last_error is None:
            raise ProbeFailure("reliable transfer failed without a recorded error")
        raise last_error

    def stop_sband_service(self) -> None:
        ssh_capture(self.subsystem_target, f"sudo systemctl stop {shq(self.sband_comm_service)}", check=False)

    def expect_sband_command_failure(self, session_id: int) -> None:
        profile = self.authority_profile("sband-primary")
        self.prepare_ground_window(self.sband)
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        start = self.send_envelope(
            self.sband,
            "expected-sband-failure",
            profile,
            session_id,
            90,
            "OBCApp.bootManager.GET_RESET_CAUSE",
        )
        deadline = time.time() + 10.0
        while time.time() < deadline:
            events_text = "\n".join(read_text(self.sband.events_log).splitlines()[start:])
            journal_text = ssh_capture(
                self.obc_target,
                f"journalctl -u {shq(self.obc_service)} --since {shq(journal_since)} --no-pager || true",
                check=False,
            )
            if "BOOT_RECOVERY_STATUS" in events_text or "BOOT_RECOVERY_STATUS" in journal_text:
                raise ProbeFailure("S-band command unexpectedly still completed after node-5 loss")
            time.sleep(0.5)

    def run_dual_link_proof(self) -> list[str]:
        if self.profile != "uhf-primary":
            raise ProbeFailure("dual-link target proof is governed only for the physical node-6 UHF profile")
        summary: list[str] = []
        failure_stage = "dual-link-bootstrap"
        dual_link_summary_path: pathlib.Path | None = None
        target_claim = "FAIL"
        phase_a_session_source = "not-started"
        phase_a_command_source: str | None = None
        phase_b_session_source = "not-started"
        phase_b_command_source: str | None = None
        phase_b_result = "not-started"
        phase_b_nonquiet_error: str | None = None
        phase_c_sband_session_source = "not-started"
        switch_source = "not-started"
        phase_c_uhf_session_source = "not-started"
        phase_c_command_source: str | None = None
        quiet_rescue_used = False
        phase_a_window: PhaseWindow | None = None
        phase_b_window: PhaseWindow | None = None
        phase_c_switch_window: PhaseWindow | None = None
        phase_c_truth_window: PhaseWindow | None = None
        operator_observability: dict[str, object] = {
            "verdict": "FAIL",
            "failureReasons": ["probe-did-not-complete"],
            "degradationReasons": [],
        }

        self.begin_profile()
        self.apply_uhf_ingress_diagnostics_override()
        self.start_ground_paths(need_sband=True, need_uhf=False)

        sband_profile = self.authority_profile("sband-primary")
        uhf_backup_profile = self.authority_profile("uhf-backup")
        uhf_primary_profile = self.authority_profile("uhf-primary")

        try:
            failure_stage = "phase-a-sband-session-open"
            phase_a_window_start = self.snapshot_phase_window(include_sband=True, include_uhf=False)
            phase_a_session_id, phase_a_session_source = self.open_session(
                self.sband,
                sband_profile,
                0,
                0x58000000,
                "identity 1 role 1",
                prefer_target_journal=True,
            )
            summary.append(f"phase-a-sband-session-source={phase_a_session_source}")

            failure_stage = "phase-a-sband-get-reset-cause"
            phase_a_command_source = self.run_command_roundtrip(
                self.sband,
                sband_profile,
                phase_a_session_id,
                1,
                "dual-link-phase-a-sband-get-reset-cause",
                prefer_target_journal=True,
            )
            try:
                self.sband.await_event(
                    "BOOT_RECOVERY_STATUS",
                    timeout=SBAND_CORROBORATION_SETTLE_TIMEOUT,
                    start=phase_a_window_start.sband.event_line_count if phase_a_window_start.sband else 0,
                )
            except ProbeFailure:
                self.note("phase-a-sband-boot-recovery-status-not-observed-before-window-close")
            phase_a_window = PhaseWindow(
                start=phase_a_window_start,
                end=self.snapshot_phase_window(include_sband=True, include_uhf=False),
            )
            summary.append(f"phase-a-command-source={phase_a_command_source}")

            failure_stage = "phase-b-switch-prepare"
            self.prepare_uhf_service_for_switch(
                require_pre_switch_ping=False,
                boundary="phase-b-uhf-backup-adjunct",
            )
            self.start_ground_paths(need_sband=False, need_uhf=True)
            summary.append("phase-b-pre-switch-node6-ping=not-required")

            try:
                failure_stage = "phase-b-nonquiet-session-open"
                phase_b_window_start = self.snapshot_phase_window(include_sband=True, include_uhf=True)
                phase_b_session_id, phase_b_session_source = self.open_session(
                    self.uhf,
                    uhf_backup_profile,
                    1,
                    0x58010000,
                    "identity 2 role 2",
                    prefer_target_journal=True,
                )
                summary.append(f"phase-b-session-source={phase_b_session_source}")

                failure_stage = "phase-b-nonquiet-get-reset-cause"
                phase_b_command_source = self.run_command_roundtrip(
                    self.uhf,
                    uhf_backup_profile,
                    phase_b_session_id,
                    1,
                    "dual-link-phase-b-uhf-backup-get-reset-cause",
                    prefer_target_journal=True,
                )
                try:
                    self.sband.await_event(
                        "BOOT_RECOVERY_STATUS",
                        timeout=SBAND_CORROBORATION_SETTLE_TIMEOUT,
                        start=phase_b_window_start.sband.event_line_count if phase_b_window_start.sband else 0,
                    )
                except ProbeFailure:
                    self.note("phase-b-sband-boot-recovery-status-not-observed-before-window-close")
                phase_b_result = "nonquiet-pass"
                phase_b_window = PhaseWindow(
                    start=phase_b_window_start,
                    end=self.snapshot_phase_window(include_sband=True, include_uhf=True),
                )
                summary.append(f"phase-b-command-source={phase_b_command_source}")
            except ProbeFailure as exc:
                quiet_rescue_used = True
                phase_b_nonquiet_error = str(exc)
                phase_b_result = "nonquiet-failed-quiet-rescue-required"
                self.checkpoint("phase-b-nonquiet-adjunct", "fail", error=phase_b_nonquiet_error)
                self.apply_quiet_override()
                self.restart_uhf_service_for_probe_only()
                self.wait_for_target_ready_for_comm(False)

                failure_stage = "phase-d-quiet-rescue-session-open"
                phase_b_window_start = self.snapshot_phase_window(include_sband=True, include_uhf=True)
                phase_b_session_id, phase_b_session_source = self.open_session(
                    self.uhf,
                    uhf_backup_profile,
                    1,
                    0x58011000,
                    "identity 2 role 2",
                    prefer_target_journal=True,
                )
                summary.append(f"phase-b-session-source={phase_b_session_source}")

                failure_stage = "phase-d-quiet-rescue-get-reset-cause"
                phase_b_command_source = self.run_command_roundtrip(
                    self.uhf,
                    uhf_backup_profile,
                    phase_b_session_id,
                    1,
                    "dual-link-phase-d-quiet-rescue-get-reset-cause",
                    prefer_target_journal=True,
                )
                try:
                    self.sband.await_event(
                        "BOOT_RECOVERY_STATUS",
                        timeout=SBAND_CORROBORATION_SETTLE_TIMEOUT,
                        start=phase_b_window_start.sband.event_line_count if phase_b_window_start.sband else 0,
                    )
                except ProbeFailure:
                    self.note("phase-b-sband-boot-recovery-status-not-observed-before-window-close")
                phase_b_result = "quiet-rescue-pass"
                phase_b_window = PhaseWindow(
                    start=phase_b_window_start,
                    end=self.snapshot_phase_window(include_sband=True, include_uhf=True),
                )
                summary.append(f"phase-b-command-source={phase_b_command_source}")
                summary.append("quiet-rescue-used=1")
                self.restore_nonquiet_target_after_quiet_rescue()

            failure_stage = "phase-c-sband-session-open"
            phase_c_sband_session_id, phase_c_sband_session_source = self.open_session(
                self.sband,
                sband_profile,
                0,
                0x58020000,
                "identity 1 role 1",
                prefer_target_journal=True,
            )
            summary.append(f"phase-c-sband-session-source={phase_c_sband_session_source}")

            failure_stage = "phase-c-switch-to-uhf-primary"
            phase_c_switch_window_start = self.snapshot_phase_window(include_sband=True, include_uhf=True)
            switch_source = self.switch_to_uhf_primary(
                phase_c_sband_session_id,
                prefer_target_journal=True,
            )
            phase_c_switch_window = PhaseWindow(
                start=phase_c_switch_window_start,
                end=self.snapshot_phase_window(include_sband=True, include_uhf=True),
            )
            summary.append(f"switch-source={switch_source}")

            failure_stage = "phase-c-uhf-session-open"
            phase_c_truth_window_start = self.snapshot_phase_window(include_sband=False, include_uhf=True)
            phase_c_uhf_session_id, phase_c_uhf_session_source = self.open_session(
                self.uhf,
                uhf_primary_profile,
                1,
                0x58030000,
                "identity 2 role 3",
                prefer_target_journal=True,
            )
            summary.append(f"phase-c-uhf-session-source={phase_c_uhf_session_source}")

            failure_stage = "phase-c-uhf-get-reset-cause"
            phase_c_command_source = self.run_command_roundtrip(
                self.uhf,
                uhf_primary_profile,
                phase_c_uhf_session_id,
                1,
                "dual-link-phase-c-uhf-primary-get-reset-cause",
                prefer_target_journal=True,
            )
            phase_c_truth_window = PhaseWindow(
                start=phase_c_truth_window_start,
                end=self.snapshot_phase_window(include_sband=False, include_uhf=True),
            )
            summary.append(f"phase-c-command-source={phase_c_command_source}")

            target_claim = "PASS"
            operator_observability = self.summarize_dual_link_operator_observability(
                quiet_rescue_used=quiet_rescue_used,
                phase_a_window=phase_a_window,
                phase_b_window=phase_b_window,
                phase_c_switch_window=phase_c_switch_window,
                phase_c_truth_window=phase_c_truth_window,
            )
            self.snapshot_journal(self.obc_target, self.obc_service, "dual-link-proof-obc", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "dual-link-proof-uhf", lines=500)
            dual_link_summary_path = self.record_dual_link_proof_artifacts(
                phase_a_session_source=phase_a_session_source,
                phase_a_command_source=phase_a_command_source,
                phase_b_session_source=phase_b_session_source,
                phase_b_command_source=phase_b_command_source,
                phase_b_result=phase_b_result,
                phase_b_nonquiet_error=phase_b_nonquiet_error,
                phase_c_sband_session_source=phase_c_sband_session_source,
                switch_source=switch_source,
                phase_c_uhf_session_source=phase_c_uhf_session_source,
                phase_c_command_source=phase_c_command_source,
                target_claim=target_claim,
                operator_observability=operator_observability,
                quiet_rescue_used=quiet_rescue_used,
            )
            summary.append(f"dual-link-proof-summary={dual_link_summary_path}")
            summary.append(f"target-claim={target_claim}")
            summary.append(f"operator-observability={operator_observability.get('verdict')}")
            if operator_observability.get("verdict") == "FAIL":
                failure_stage = "operator-observability-minimum-evidence"
                raise ProbeFailure("operator observability minimum evidence failed")
            ensure_no_legacy_aliases(self.root_dir)
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "dual-link-proof-obc", lines=320)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "dual-link-proof-uhf", lines=500)
            if (
                phase_a_window is not None
                and phase_b_window is not None
                and phase_c_switch_window is not None
                and phase_c_truth_window is not None
            ):
                operator_observability = self.summarize_dual_link_operator_observability(
                    quiet_rescue_used=quiet_rescue_used,
                    phase_a_window=phase_a_window,
                    phase_b_window=phase_b_window,
                    phase_c_switch_window=phase_c_switch_window,
                    phase_c_truth_window=phase_c_truth_window,
                )
            if dual_link_summary_path is None:
                dual_link_summary_path = self.record_dual_link_proof_artifacts(
                    phase_a_session_source=phase_a_session_source,
                    phase_a_command_source=phase_a_command_source,
                    phase_b_session_source=phase_b_session_source,
                    phase_b_command_source=phase_b_command_source,
                    phase_b_result=phase_b_result,
                    phase_b_nonquiet_error=phase_b_nonquiet_error,
                    phase_c_sband_session_source=phase_c_sband_session_source,
                    switch_source=switch_source,
                    phase_c_uhf_session_source=phase_c_uhf_session_source,
                    phase_c_command_source=phase_c_command_source,
                    target_claim=target_claim,
                    operator_observability=operator_observability,
                    quiet_rescue_used=quiet_rescue_used,
                    failure={
                        "stage": failure_stage,
                        "error": str(exc),
                    },
                )
                summary.append(f"dual-link-proof-summary={dual_link_summary_path}")
            raise

    def run_failover(self) -> list[str]:
        self.begin_profile()
        self.start_ground_paths(need_sband=True, need_uhf=False)
        sband_profile = self.authority_profile("sband-primary")
        uhf_profile = self.authority_profile("uhf-primary")
        summary: list[str] = []
        sband_session_id, sband_session_source = self.open_session(self.sband, sband_profile, 0, 0x51000000, "identity 1 role 1")
        summary.append(f"sband-session-source={sband_session_source}")
        pre_source = self.send_command_and_wait(
            self.sband,
            sband_profile,
            sband_session_id,
            1,
            "OBCApp.bootManager.GET_RESET_CAUSE",
            tuple(),
            ("BOOT_RECOVERY_STATUS",),
            ("BOOT_RECOVERY_STATUS",),
            30.0,
            "pre-failover sband GET_RESET_CAUSE",
        )
        summary.append(f"sband-command-source={pre_source}")
        self.ensure_uhf_service_ready()
        self.start_ground_paths(need_sband=False, need_uhf=True)
        switch_source = self.switch_to_uhf_primary(sband_session_id)
        summary.append(f"switch-source={switch_source}")
        self.stop_sband_service()
        self.expect_sband_command_failure(sband_session_id)
        summary.append("sband-post-loss=no-command-completion")
        uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x53000000, "identity 2 role 3")
        summary.append(f"uhf-session-source={uhf_session_source}")
        uhf_source = self.send_command_and_wait(
            self.uhf,
            uhf_profile,
            uhf_session_id,
            1,
            "OBCApp.bootManager.GET_RESET_CAUSE",
            tuple(),
            ("BOOT_RECOVERY_STATUS",),
            ("BOOT_RECOVERY_STATUS",),
            30.0,
            "post-failover uhf GET_RESET_CAUSE",
        )
        summary.append(f"uhf-command-source={uhf_source}")
        ensure_no_legacy_aliases(self.root_dir)
        return summary

    def run_beacon_suppression(self) -> list[str]:
        if self.profile != "uhf-primary":
            raise ProbeFailure("beacon suppression target proof is governed only for the quiet node-6 UHF profile")
        summary: list[str] = []
        self.begin_profile()
        self.apply_uhf_beacon_target_override()
        self.apply_uhf_beacon_capture_override()
        self.apply_uhf_ingress_diagnostics_override()
        self.restart_uhf_service_for_probe_only()
        self.start_ground_paths(need_sband=True, need_uhf=False)

        baseline_count = self.remote_beacon_frame_count()
        baseline_count = self.wait_for_remote_beacon_frame(baseline_count, 25.0)
        summary.append(f"baseline-beacon-count={baseline_count}")

        sband_profile = self.authority_profile("sband-primary")
        uhf_profile = self.authority_profile("uhf-primary")
        sband_session_id, sband_session_source = self.open_session(self.sband, sband_profile, 0, 0x54000000, "identity 1 role 1")
        summary.append(f"sband-session-source={sband_session_source}")
        sband_command_source = self.send_command_and_wait(
            self.sband,
            sband_profile,
            sband_session_id,
            1,
            "OBCApp.bootManager.GET_RESET_CAUSE",
            tuple(),
            ("BOOT_RECOVERY_STATUS",),
            ("BOOT_RECOVERY_STATUS",),
            30.0,
            "pre-switch sband GET_RESET_CAUSE",
        )
        summary.append(f"sband-command-source={sband_command_source}")
        sband_negative_count = self.remote_beacon_frame_count()
        sband_negative_count = self.wait_for_remote_beacon_frame(sband_negative_count, 25.0)
        summary.append("negative-accepted-sband-does-not-suppress=PASS")

        self.start_ground_paths(need_sband=False, need_uhf=True)
        switch_source = self.switch_to_uhf_primary(sband_session_id)
        summary.append(f"switch-source={switch_source}")

        invalid_session_id = 0x55000001
        self.prepare_ground_window(self.uhf)
        time.sleep(0.3)
        invalid_journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        invalid_start = self.uhf.event_count()
        self.send_envelope(
            self.uhf,
            "uhf-invalid-session-open",
            uhf_profile,
            invalid_session_id,
            1,
            "OBCApp.commandIngressAuthority.SESSION_OPEN",
        )
        self.wait_ground_or_journal(
            self.uhf,
            invalid_start,
            invalid_journal_since,
            ("COMMAND_SESSION_REJECTED", "ingress 1 identity 2 role 3", f"session {invalid_session_id}", "sequence 1"),
            ("COMMAND_SESSION_REJECTED", "ingress 1 identity 2 role 3", f"session {invalid_session_id}", "sequence 1"),
            18.0,
            "rejected UHF SESSION_OPEN(seq!=0)",
        )
        invalid_negative_count = self.remote_beacon_frame_count()
        invalid_negative_count = self.wait_for_remote_beacon_frame(invalid_negative_count, 25.0)
        summary.append("negative-rejected-uhf-session-open-does-not-suppress=PASS")

        suppress_event_start = self.uhf.event_count()
        suppress_journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x55000002, "identity 2 role 3")
        summary.append(f"uhf-session-source={uhf_session_source}")
        suppress_count = self.remote_beacon_frame_count()
        self.wait_ground_or_journal(
            self.uhf,
            suppress_event_start,
            suppress_journal_since,
            ("COMM_UHF_BEACON_SUPPRESS_STARTED", "ingress 1 role 3", f"session {uhf_session_id}", "timeoutTicks 60"),
            ("COMM_UHF_BEACON_SUPPRESS_STARTED", "ingress 1 role 3", f"session {uhf_session_id}", "timeoutTicks 60"),
            18.0,
            "UHF beacon suppress started",
        )
        summary.append("suppress-start=accepted UHF SESSION_OPEN(seq0)")
        self.assert_no_remote_beacon_growth(suppress_count, 25.0)

        time.sleep(5.0)
        refresh_source = ""
        refresh_error: ProbeFailure | None = None
        for attempt in range(3):
            self.checkpoint("uhf-refresh-attempt", "info", attempt=attempt + 1, session_id=uhf_session_id, sequence=1)
            try:
                refresh_source = self.send_command_and_wait(
                    self.uhf,
                    uhf_profile,
                    uhf_session_id,
                    1,
                    "OBCApp.modeManager.MODE_GET",
                    tuple(),
                    ("COMM_UHF_BEACON_SUPPRESS_REFRESHED", "sequence 1 remainingTicks 60"),
                    ("COMM_UHF_BEACON_SUPPRESS_REFRESHED", "sequence 1 remainingTicks 60"),
                    12.0,
                    "UHF MODE_GET refresh",
                )
                self.checkpoint("uhf-refresh-attempt", "pass", attempt=attempt + 1, session_id=uhf_session_id, source=refresh_source)
                break
            except ProbeFailure as exc:
                refresh_error = exc
                self.checkpoint("uhf-refresh-attempt", "fail", attempt=attempt + 1, session_id=uhf_session_id, error=str(exc))
                time.sleep(1.0)
        if not refresh_source:
            assert refresh_error is not None
            raise refresh_error
        summary.append(f"refresh-source={refresh_source}")
        summary.append("refresh=accepted UHF MODE_GET(seq1)")
        refreshed_count = self.remote_beacon_frame_count()
        self.assert_no_remote_beacon_growth(refreshed_count, 50.0)

        self.wait_ground_or_journal(
            self.uhf,
            self.uhf.event_count(),
            ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip(),
            ("COMM_UHF_BEACON_SUPPRESS_CLEARED", "reason 1 ingress 1 role 3", f"session {uhf_session_id}", "lastSequence 1"),
            ("COMM_UHF_BEACON_SUPPRESS_CLEARED", "reason 1 ingress 1 role 3", f"session {uhf_session_id}", "lastSequence 1"),
            25.0,
            "UHF beacon suppress cleared by inactivity",
        )
        resumed_count = self.wait_for_remote_beacon_frame(refreshed_count, 30.0)
        summary.append("resume=bounded inactivity timeout clear plus resumed beacon capture")
        summary.append(f"resume-beacon-count={resumed_count}")

        capture_path, first_beacon_json, resume_beacon_json = self.fetch_remote_beacon_capture_artifacts()
        summary.append(f"beacon-capture={capture_path}")
        summary.append(f"beacon-first-json={first_beacon_json}")
        summary.append(f"beacon-resume-json={resume_beacon_json}")
        ensure_no_legacy_aliases(self.root_dir)
        return summary

    def run_nonquiet_diagnosis(self) -> list[str]:
        if self.profile != "uhf-primary":
            raise ProbeFailure("nonquiet diagnosis is governed only for the target CAN node-6 UHF profile")
        summary: list[str] = []
        failure_stage = "nonquiet-bootstrap"
        sband_session_source = "not-started"
        switch_source = "not-started"
        uhf_session_source = "not-started"
        uhf_command_source: str | None = None
        baseline_beacon_count: int | None = None
        command_beacon_count: int | None = None
        self.begin_profile()
        self.apply_uhf_beacon_target_override()
        if self.nonquiet_beacon_capture_enabled:
            self.apply_uhf_beacon_capture_override()
        else:
            self.checkpoint(
                "uhf-beacon-capture-override-skipped",
                "pass",
                reason="disabled-for-nonquiet-diagnosis",
            )
        self.apply_uhf_ingress_diagnostics_override()
        self.restart_uhf_service_for_probe_only()

        if self.nonquiet_beacon_capture_enabled:
            baseline_beacon_count = self.remote_beacon_frame_count()
            baseline_beacon_count = self.wait_for_remote_beacon_frame(baseline_beacon_count, 25.0)
            summary.append(f"baseline-beacon-count={baseline_beacon_count}")
        else:
            summary.append("baseline-beacon-count=skipped")
        summary.append("quiet-override=disabled")

        try:
            failure_stage = "nonquiet-sband-session-open"
            self.start_ground_paths(need_sband=True, need_uhf=False)
            sband_profile = self.authority_profile("sband-primary")
            uhf_profile = self.authority_profile("uhf-primary")
            sband_session_id, sband_session_source = self.open_session(
                self.sband,
                sband_profile,
                0,
                0x56000000,
                "identity 1 role 1",
            )
            summary.append(f"sband-session-source={sband_session_source}")
            failure_stage = "nonquiet-switch-prepare"
            self.prepare_uhf_service_for_switch(
                require_pre_switch_ping=False,
                boundary="post-switch-journal-first-command",
            )
            summary.append("pre-switch-node6-ping=not-required")
            failure_stage = "nonquiet-switch-to-uhf-primary"
            self.start_ground_paths(need_sband=False, need_uhf=True)
            switch_source = self.switch_to_uhf_primary(sband_session_id)
            summary.append(f"switch-source={switch_source}")
            failure_stage = "nonquiet-uhf-session-open"
            uhf_session_id, uhf_session_source = self.open_session(
                self.uhf,
                uhf_profile,
                1,
                0x57000000,
                "identity 2 role 3",
            )
            summary.append(f"uhf-session-source={uhf_session_source}")
            failure_stage = "nonquiet-uhf-command-roundtrip"
            uhf_command_source = self.run_command_roundtrip(
                self.uhf,
                uhf_profile,
                uhf_session_id,
                1,
                "nonquiet-uhf-get-reset-cause",
            )
            summary.append(f"uhf-command-source={uhf_command_source}")

            if self.nonquiet_beacon_capture_enabled:
                command_beacon_count = self.remote_beacon_frame_count()
                summary.append(f"post-command-beacon-count={command_beacon_count}")
                try:
                    command_beacon_count = self.wait_for_remote_beacon_frame(command_beacon_count, 5.0)
                    summary.append("post-command-beacon-growth=observed")
                    self.checkpoint(
                        "nonquiet-post-command-beacon-growth",
                        "pass",
                        observed=True,
                        frame_count=command_beacon_count,
                    )
                except ProbeFailure:
                    summary.append("post-command-beacon-growth=not-observed")
                    self.checkpoint(
                        "nonquiet-post-command-beacon-growth",
                        "pass",
                        observed=False,
                        frame_count=command_beacon_count,
                        note="bounded observation only; active beacon suppression window may keep count stable",
                    )
            else:
                summary.append("post-command-beacon-count=skipped")
                self.checkpoint(
                    "nonquiet-post-command-beacon-growth",
                    "pass",
                    observed=None,
                    frame_count=None,
                    note="skipped because TARGET_CAN_NONQUIET_BEACON_CAPTURE=0",
                )
            diagnosis_summary = self.finalize_nonquiet_diagnosis_artifacts(
                root_cause_hypothesis="nonquiet-command-roundtrip-succeeded-beacon-growth-not-required",
                sband_session_source=sband_session_source,
                switch_source=switch_source,
                uhf_session_source=uhf_session_source,
                uhf_command_source=uhf_command_source,
                baseline_beacon_count=baseline_beacon_count,
                command_beacon_count=command_beacon_count,
            )
            summary.append(f"diagnosis-summary={diagnosis_summary}")
        except ProbeFailure as exc:
            diagnosis_summary = self.finalize_nonquiet_diagnosis_artifacts(
                root_cause_hypothesis="mixed-command-ingress-degradation",
                sband_session_source=sband_session_source,
                switch_source=switch_source,
                uhf_session_source=uhf_session_source,
                uhf_command_source=uhf_command_source,
                baseline_beacon_count=baseline_beacon_count,
                command_beacon_count=command_beacon_count,
                failure={
                    "stage": failure_stage,
                    "error": str(exc),
                },
            )
            summary.append(f"diagnosis-summary={diagnosis_summary}")
            raise
        ensure_no_legacy_aliases(self.root_dir)
        return summary

    def secure_auth_non_claims(self) -> list[str]:
        return [
            "no encryption claim",
            "no RF closure",
            "no boot-trust expansion",
            "no hardware-backed key storage",
            "no persistent secure key storage",
            "no generic file authority beyond .sequence-staging/<leaf>",
            "no UHF primary staged-upload success claim",
            "no one-GDS aggregation claim",
            "no one-gateway simultaneous S-band/UHF multiplexer claim",
            "no legacy command envelope v1 retirement",
        ]

    def write_secure_auth_proof_summary(
        self,
        *,
        case_results: list[dict[str, object]],
        failure: dict[str, object] | None = None,
    ) -> pathlib.Path:
        payload: dict[str, object] = {
            "mode": self.mode,
            "profile": self.profile,
            "verdict": "FAIL" if failure else "PASS",
            "targetPathUnderTest": {
                "sband": "entry-59 macOS fprime-gds + ground_ttc_gateway -> subsystem.local node 5 -> SocketCAN -> obc.local",
                "uhf": "entry-69 physical node-6 uhf-backup adjunct plus explicit switch to uhf-primary-after-failover",
            },
            "hostedAncestryOnly": ["43E", "43F"],
            "caseResults": case_results,
            "artifacts": {
                "provenance": str(self.diagnostics_dir / "secure-auth-keystore-provenance.json"),
                "checkpoints": str(self.checkpoints_log),
                "gatewayCaptures": self.secure_capture_summary(),
                "obcJournal": str(self.journal_snapshot_dir / "secure-auth-proof-obc.log"),
                "sbandServiceJournal": str(self.journal_snapshot_dir / "secure-auth-proof-sband-service.log"),
                "uhfServiceJournal": str(self.journal_snapshot_dir / "secure-auth-proof-uhf-service.log"),
                "cleanupStatus": str(self.diagnostics_dir / "cleanup-status.json"),
            },
            "residualNonClaims": self.secure_auth_non_claims(),
        }
        if failure is not None:
            payload["failure"] = failure
        path = self.diagnostics_dir / "secure-auth-proof-summary.json"
        self.write_json_artifact(path, payload)
        return path

    def run_secure_auth_proof(self) -> list[str]:
        if self.profile != "uhf-primary":
            raise ProbeFailure("secure-auth target proof is governed for the physical node-6 UHF-capable target profile")
        summary: list[str] = []
        case_results: list[dict[str, object]] = []
        failure_stage = "secure-auth-bootstrap"
        switched_to_uhf = False
        try:
            failure_stage = "secure-auth-begin-profile"
            self.begin_profile()

            failure_stage = "secure-auth-provenance"
            self.record_secure_auth_provenance()
            case_results.append({"case": "installed-release-keystore-provenance", "verdict": "PASS"})
            summary.append("case-installed-release-keystore-provenance=PASS")

            failure_stage = "secure-auth-server-start"
            self.apply_sband_ingress_diagnostics_override()
            self.start_security_server()
            self.wait_for_sband_tcp_reachability(10.0)
            self.checkpoint(
                "sband-southbound-tcp-reachability",
                "pass",
                host=self.sband_tcp_host,
                port=self.sband_tcp_port,
            )
            self.start_ground_paths(need_sband=True, need_uhf=False)
            readiness = self.ensure_sband_ground_ready()
            summary.append(f"sband-ground-readiness={readiness}")
            self.prepare_ground_window(self.sband, timeout=8.0)

            failure_stage = "sband-malformed-handshake"
            self.verify_malformed_secure_handshake_fail_closed(self.sband)
            case_results.append({"case": "sband-malformed-handshake-fail-closed", "verdict": "PASS"})
            summary.append("case-sband-malformed-handshake-fail-closed=PASS")

            failure_stage = "sband-secure-auth"
            sband_session = self.authenticate_secure_service(
                self.sband,
                service_id=SERVICE_ID_SBAND,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            case_results.append({"case": "sband-apid-00fe-secure-auth", "verdict": "PASS"})
            summary.append("case-sband-apid-00fe-secure-auth=PASS")

            failure_stage = "sband-secure-command-sequence"
            first_sequence, first_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                "sband-secure-get-reset-cause-seq41",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )
            second_sequence, second_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                "sband-secure-get-reset-cause-seq42",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )
            duplicate_sequence, duplicate_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                "sband-secure-duplicate-seq42-rejected",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=False,
                sequence_number=second_sequence,
                journal_fragments=("Secure command rejected ingress 0 identity 1 role 1", f"sequence {second_sequence}"),
                timeout=20.0,
            )
            case_results.append(
                {
                    "case": "sband-secure-command-sequence",
                    "verdict": "PASS",
                    "firstAcceptedSequence": first_sequence,
                    "secondAcceptedSequence": second_sequence,
                    "duplicateRejectedSequence": duplicate_sequence,
                    "sources": [first_source, second_source, duplicate_source],
                }
            )
            summary.append(f"case-sband-secure-command-first-sequence={first_sequence}")
            summary.append("case-sband-secure-command-strict-next-sequence=PASS")

            failure_stage = "sband-staged-upload"
            sband_artifact = self.build_sequence_artifact("target-secure-auth-sband-staged")
            self.send_staged_upload_packets(
                self.sband,
                sband_artifact,
                expect_accept=True,
                label="sband-secure-auth-staged-upload",
            )
            case_results.append({"case": "sband-secure-auth-staged-upload", "verdict": "PASS", "destination": sband_artifact.destination})
            summary.append("case-sband-secure-auth-staged-upload=PASS")

            failure_stage = "uhf-service-prepare"
            self.apply_uhf_ingress_diagnostics_override()
            self.prepare_uhf_service_for_switch(
                require_pre_switch_ping=True,
                boundary="target-secure-auth-uhf-backup",
            )
            self.start_ground_paths(need_sband=False, need_uhf=True)
            self.prepare_ground_window(self.uhf, timeout=8.0)

            failure_stage = "uhf-backup-secure-auth"
            uhf_backup_session = self.authenticate_secure_service(
                self.uhf,
                service_id=SERVICE_ID_UHF,
                ingress_port=1,
                role_fragment="identity 2 role 2",
                initial_sequence=41,
            )
            case_results.append({"case": "uhf-backup-serviceid-2-secure-auth", "verdict": "PASS"})
            summary.append("case-uhf-backup-serviceid-2-secure-auth=PASS")

            failure_stage = "uhf-backup-read-and-deny"
            uhf_read_sequence, uhf_read_source = self.send_secure_command_name(
                self.uhf,
                uhf_backup_session,
                "uhf-backup-secure-get-reset-cause-seq41",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )
            denied_sequence, denied_source = self.send_secure_command_name(
                self.uhf,
                uhf_backup_session,
                "uhf-backup-secure-mode-set-denied",
                "OBCApp.modeManager.MODE_SET",
                "IDLE",
                accept_sequence=False,
                journal_fragments=(
                    f"Command authority rejected opcode 0x{self.opcodes['OBCApp.modeManager.MODE_SET']:x} ingress 1 identity 2 role 2",
                ),
                timeout=24.0,
            )
            continuity_sequence, continuity_source = self.send_secure_command_name(
                self.uhf,
                uhf_backup_session,
                "uhf-backup-secure-get-reset-cause-after-deny",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )
            case_results.append(
                {
                    "case": "uhf-backup-read-status-and-high-authority-deny",
                    "verdict": "PASS",
                    "readSequence": uhf_read_sequence,
                    "deniedSequence": denied_sequence,
                    "postDenyAcceptedSequence": continuity_sequence,
                    "sources": [uhf_read_source, denied_source, continuity_source],
                }
            )
            summary.append("case-uhf-backup-read-status-and-high-authority-deny=PASS")

            failure_stage = "uhf-backup-staged-upload-deny"
            uhf_denied_artifact = self.build_sequence_artifact("target-secure-auth-uhf-backup-denied")
            self.send_staged_upload_packets(
                self.uhf,
                uhf_denied_artifact,
                expect_accept=False,
                label="uhf-backup-secure-auth-staged-upload-denied",
            )
            case_results.append(
                {"case": "uhf-backup-secure-auth-staged-upload-denied", "verdict": "PASS", "destination": uhf_denied_artifact.destination}
            )
            summary.append("case-uhf-backup-secure-auth-staged-upload-denied=PASS")

            failure_stage = "switch-to-uhf-primary"
            switch_sequence, switch_source = self.send_secure_command_name(
                self.sband,
                sband_session,
                "sband-secure-switch-to-uhf-primary",
                "OBCApp.commController.COMM_SET_ACTIVE",
                "UHF",
                accept_sequence=True,
                journal_fragments=(
                    "COMM_PRIMARY_LINK_CHANGED",
                    "command UHF",
                    "telemetry UHF",
                    "file UHF",
                    "reason 1",
                    "Secure auth revoked ingress 1 service 2 reason 2",
                    "Command session revoked ingress 1 identity 2 role 2",
                ),
                timeout=35.0,
            )
            switched_to_uhf = True
            case_results.append({"case": "uhf-role-switch-invalidates-backup-auth", "verdict": "PASS", "switchSequence": switch_sequence, "source": switch_source})
            summary.append("case-uhf-role-switch-invalidates-backup-auth=PASS")

            failure_stage = "old-uhf-backup-session-rejected-after-switch"
            rejected_sequence, rejected_source = self.send_secure_command_name(
                self.uhf,
                uhf_backup_session,
                "old-uhf-backup-secure-command-rejected-after-switch",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=False,
                journal_fragments=("Secure command rejected ingress 1 identity 2 role 3 session 0",),
                timeout=24.0,
            )
            case_results.append(
                {
                    "case": "old-uhf-backup-auth-state-rejected-after-role-switch",
                    "verdict": "PASS",
                    "rejectedSequence": rejected_sequence,
                    "source": rejected_source,
                }
            )
            summary.append("case-old-uhf-backup-auth-state-rejected-after-role-switch=PASS")

            self.wait_for_target_ready_for_comm(require_uhf=True)
            self.checkpoint(
                "uhf-primary-ready-after-role-switch",
                "pass",
            )

            self.prepare_ground_window(self.uhf, timeout=8.0)
            self.checkpoint(
                "uhf-ground-helper-retained-before-primary-reauth",
                "pass",
                gds_port=self.uhf.gds_port,
                gds_tts_port=self.uhf.gds_tts_port,
                reason="preserve-apid-sequence-continuity-across-role-switch",
            )

            failure_stage = "uhf-primary-reauth"
            uhf_primary_session = self.authenticate_secure_service(
                self.uhf,
                service_id=SERVICE_ID_UHF,
                ingress_port=1,
                role_fragment="identity 2 role 3",
                initial_sequence=41,
            )
            primary_sequence, primary_source = self.send_secure_command_name(
                self.uhf,
                uhf_primary_session,
                "uhf-primary-after-failover-secure-get-reset-cause",
                "OBCApp.bootManager.GET_RESET_CAUSE",
                accept_sequence=True,
                journal_fragments=("BOOT_RECOVERY_STATUS",),
                timeout=30.0,
            )
            case_results.append(
                {
                    "case": "uhf-primary-after-failover-reauth-secure-command",
                    "verdict": "PASS",
                    "acceptedSequence": primary_sequence,
                    "source": primary_source,
                }
            )
            summary.append("case-uhf-primary-after-failover-reauth-secure-command=PASS")

            if self.secure_auth_proof_include_file_downlink:
                failure_stage = "uhf-primary-hk-file-downlink"
                source_path, received_path, received_size = self.run_file_downlink_secure(
                    self.uhf,
                    uhf_primary_session,
                    "uhf-primary-hk-file",
                )
                case_results.append(
                    {
                        "case": "uhf-primary-hk-file-downlink",
                        "verdict": "PASS",
                        "sourcePath": source_path,
                        "receivedPath": received_path,
                        "receivedSize": received_size,
                    }
                )
                summary.append("case-uhf-primary-hk-file-downlink=PASS")
                summary.append(f"uhf-primary-hk-source-path={source_path}")
                summary.append(f"uhf-primary-hk-received-path={received_path}")
                summary.append(f"uhf-primary-hk-received-size={received_size}")

            failure_stage = "restore-sband-primary"
            restore_sequence, restore_source = self.send_secure_command_name(
                self.uhf,
                uhf_primary_session,
                "uhf-primary-secure-switch-back-to-sband",
                "OBCApp.commController.COMM_SET_ACTIVE",
                "SBAND",
                accept_sequence=True,
                journal_fragments=(
                    "COMM_PRIMARY_LINK_CHANGED",
                    "command SBAND",
                    "telemetry SBAND",
                    "file SBAND",
                    "reason 1",
                ),
                timeout=35.0,
            )
            switched_to_uhf = False
            case_results.append({"case": "restore-sband-primary", "verdict": "PASS", "restoreSequence": restore_sequence, "source": restore_source})
            summary.append("case-restore-sband-primary=PASS")

            self.snapshot_journal(self.obc_target, self.obc_service, "secure-auth-proof-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "secure-auth-proof-sband-service", lines=300)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "secure-auth-proof-uhf-service", lines=520)
            summary_path = self.write_secure_auth_proof_summary(case_results=case_results)
            summary.append(f"secure-auth-proof-summary={summary_path}")
            summary.append("target-secure-auth-proof=PASS")
            ensure_no_legacy_aliases(self.root_dir)
            return summary
        except ProbeFailure as exc:
            self.snapshot_journal(self.obc_target, self.obc_service, "secure-auth-proof-obc", lines=520)
            self.snapshot_journal(self.subsystem_target, self.sband_comm_service, "secure-auth-proof-sband-service", lines=300)
            self.snapshot_journal(self.subsystem_target, self.uhf_comm_service, "secure-auth-proof-uhf-service", lines=520)
            summary_path = self.write_secure_auth_proof_summary(
                case_results=case_results,
                failure={"stage": failure_stage, "error": str(exc)},
            )
            self.checkpoint("secure-auth-proof-summary-written", "fail", artifact=str(summary_path), stage=failure_stage, error=str(exc))
            if switched_to_uhf and not self.baseline_managed_externally:
                try:
                    ssh_capture(self.obc_target, f"sudo systemctl restart {shq(self.obc_service)}", check=False)
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.checkpoint("secure-auth-proof-failure-obc-restarted", "pass", reason="restore-sband-after-failed-uhf-switch")
                except Exception as cleanup_exc:
                    self.checkpoint("secure-auth-proof-failure-obc-restarted", "fail", error=str(cleanup_exc))
            elif switched_to_uhf:
                self.checkpoint(
                    "secure-auth-proof-failure-obc-restart-skipped",
                    "pass",
                    reason="externally-managed-baseline",
                )
            raise

    def run(self) -> list[str]:
        self.start()
        summary: list[str] = []
        run_error: Exception | None = None
        try:
            if self.mode == "secure-auth-proof":
                summary.extend(self.run_secure_auth_proof())
                return summary
            if self.mode == "dual-link-proof":
                summary.extend(self.run_dual_link_proof())
                return summary
            if self.mode == "failover-command":
                summary.extend(self.run_failover())
                return summary
            if self.mode == "beacon-suppression":
                summary.extend(self.run_beacon_suppression())
                return summary
            if self.mode == "nonquiet-diagnosis":
                summary.extend(self.run_nonquiet_diagnosis())
                return summary
            self.begin_profile()
            if self.mode == "reliable-transfer":
                self.require_legacy_session_open_surface(
                    "review the historical quiet switched node-6 reliable-transfer wrapper only via "
                    "explicit historical opt-in, or migrate the path to secure-auth before treating it as current"
                )
                if self.profile not in ("sband", "uhf-primary"):
                    raise ProbeFailure("reliable-transfer target proof is governed only for the default node-5 S-band profile or the quiet switched UHF primary profile")
                self.enable_reliable_transfer_receiver()
                self.checkpoint(
                    "reliable-transfer-receiver-enabled",
                    "pass",
                    service=self.reliable_transfer_receiver_service,
                    output_dir=self.reliable_transfer_output_dir,
                    receiver_override_dropin=self.reliable_transfer_receiver_override_dropin_name,
                    admission_override_dropin=self.reliable_transfer_admission_override_dropin_name,
                )
            need_sband = self.profile == "sband" or self.mode == "failover-command" or self.profile == "uhf-primary"
            need_uhf = False
            self.start_ground_paths(need_sband=need_sband, need_uhf=need_uhf)
            if self.profile == "sband":
                ground = self.sband
                profile = self.authority_profile("sband-primary")
                self.checkpoint("sband-flow-begin", "info", mode=self.mode, authority_profile=profile.authority_profile)
                session_id, session_source = self.open_session(ground, profile, 0, 0x52000000, "identity 1 role 1")
                summary.append(f"sband-session-source={session_source}")
                if self.mode == "sequence-subsystem":
                    self.run_sequence_roundtrip(ground, profile, session_id, 1, "target-can-sband-roundtrip")
                    summary.append("same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on target CAN node-5 path")
                elif self.mode == "command":
                    command_source = self.run_command_roundtrip(ground, profile, session_id, 1, "target-can-sband-command")
                    summary.append(f"sband-command-source={command_source}")
                elif self.mode == "file-downlink":
                    source_path, received_path, received_size = self.run_file_downlink(ground, profile, session_id, 1, "sband-file")
                    summary.extend(
                        [
                            f"source-path={source_path}",
                            f"received-path={received_path}",
                            f"received-size={received_size}",
                        ]
                    )
                elif self.mode == "reliable-transfer":
                    source_path, received_path, received_size = self.run_reliable_transfer(
                        ground, profile, session_id, 1, "sband-reliable-transfer"
                    )
                    summary.extend(
                        [
                            f"source-path={source_path}",
                            f"received-path={received_path}",
                            f"received-size={received_size}",
                            f"rt-output-dir={self.reliable_transfer_output_dir}",
                            "legacy-gds-file-storage=absent",
                        ]
                    )
                else:
                    raise ProbeFailure(f"unsupported mode/profile combination: {self.mode}/{self.profile}")
                self.checkpoint("sband-flow-complete", "pass", mode=self.mode)
                return summary

            sband_profile = self.authority_profile("sband-primary")
            uhf_profile = self.authority_profile("uhf-primary")
            sband_session_id, sband_session_source = self.open_session(self.sband, sband_profile, 0, 0x51000000, "identity 1 role 1")
            summary.append(f"sband-session-source={sband_session_source}")
            require_pre_switch_ping = self.mode not in ("command", "reliable-transfer")
            boundary = "pre-switch-node6-csp-ping" if require_pre_switch_ping else "post-switch-journal-first-command"
            self.prepare_uhf_service_for_switch(
                require_pre_switch_ping=require_pre_switch_ping,
                boundary=boundary,
            )
            if not require_pre_switch_ping:
                summary.append("pre-switch-node6-ping=not-required")
            self.start_ground_paths(need_sband=False, need_uhf=True)
            switch_source = self.switch_to_uhf_primary(sband_session_id)
            summary.append(f"switch-source={switch_source}")
            uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x53000000, "identity 2 role 3")
            summary.append(f"uhf-session-source={uhf_session_source}")
            if self.mode == "sequence-subsystem":
                self.run_sequence_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-can-uhf-roundtrip")
                summary.append("same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on target CAN quiet node-6 path")
            elif self.mode == "command":
                command_source = self.run_command_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-can-uhf-command")
                summary.append(f"uhf-command-source={command_source}")
            elif self.mode == "file-downlink":
                source_path, received_path, received_size = self.run_file_downlink(self.uhf, uhf_profile, uhf_session_id, 1, "uhf-file")
                summary.extend(
                    [
                        f"source-path={source_path}",
                        f"received-path={received_path}",
                        f"received-size={received_size}",
                    ]
                )
            elif self.mode == "reliable-transfer":
                source_path, received_path, received_size = self.run_reliable_transfer(
                    self.uhf, uhf_profile, uhf_session_id, 1, "uhf-reliable-transfer"
                )
                summary.extend(
                    [
                        f"source-path={source_path}",
                        f"received-path={received_path}",
                        f"received-size={received_size}",
                        f"rt-output-dir={self.reliable_transfer_output_dir}",
                        "legacy-gds-file-storage=absent",
                        "quiet-override=probe-owned",
                    ]
                )
            else:
                raise ProbeFailure(f"unsupported mode/profile combination: {self.mode}/{self.profile}")
            return summary
        except Exception as exc:
            run_error = exc
            raise
        finally:
            cleanup_errors: list[str] = []

            def cleanup_step(label: str, action) -> None:
                try:
                    action()
                except Exception as exc:
                    cleanup_errors.append(f"{label}: {exc}")
                    self.checkpoint(label, "fail", error=str(exc))

            if self.reliable_transfer_receiver_override_applied:
                def remove_reliable_transfer_receiver_override() -> None:
                    remove_service_override(
                        self.subsystem_target,
                        self.reliable_transfer_receiver_service,
                        self.reliable_transfer_receiver_override_dropin_name,
                    )
                    wait_service_active(self.subsystem_target, self.reliable_transfer_receiver_service, self.restart_timeout)
                    self.snapshot_service(
                        self.subsystem_target,
                        self.reliable_transfer_receiver_service,
                        "reliable-transfer-receiver-override-removed",
                    )
                    self.checkpoint(
                        "reliable-transfer-receiver-override-removed",
                        "pass",
                        service=self.reliable_transfer_receiver_service,
                        override_dropin=self.reliable_transfer_receiver_override_dropin_name,
                    )
                    self.reliable_transfer_receiver_override_applied = False

                cleanup_step("reliable-transfer-receiver-override-removed", remove_reliable_transfer_receiver_override)

            if self.reliable_transfer_admission_override_applied:
                def remove_reliable_transfer_admission_override() -> None:
                    remove_service_override(
                        self.obc_target,
                        self.obc_service,
                        self.reliable_transfer_admission_override_dropin_name,
                    )
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.snapshot_service(self.obc_target, self.obc_service, "reliable-transfer-admission-override-removed")
                    self.checkpoint(
                        "reliable-transfer-admission-override-removed",
                        "pass",
                        service=self.obc_service,
                        override_dropin=self.reliable_transfer_admission_override_dropin_name,
                    )
                    self.reliable_transfer_admission_override_applied = False

                cleanup_step("reliable-transfer-admission-override-removed", remove_reliable_transfer_admission_override)

            if self.beacon_capture_override_applied:
                def remove_beacon_capture_override() -> None:
                    remove_service_override(self.subsystem_target, self.uhf_comm_service, "53-uhf-beacon-capture.conf")
                    wait_service_active(self.subsystem_target, self.uhf_comm_service, self.restart_timeout)
                    self.beacon_capture_override_applied = False

                cleanup_step("uhf-beacon-capture-override-removed", remove_beacon_capture_override)

            if self.uhf_ingress_diagnostics_override_applied:
                def remove_uhf_ingress_diagnostics_override() -> None:
                    remove_service_override(
                        self.subsystem_target,
                        self.uhf_comm_service,
                        self.uhf_ingress_diagnostics_override_dropin_name,
                    )
                    wait_service_active(self.subsystem_target, self.uhf_comm_service, self.restart_timeout)
                    self.uhf_ingress_diagnostics_override_applied = False

                cleanup_step("uhf-ingress-diagnostics-override-removed", remove_uhf_ingress_diagnostics_override)

            if self.sband_ingress_diagnostics_override_applied:
                def remove_sband_ingress_diagnostics_override() -> None:
                    remove_service_override(
                        self.subsystem_target,
                        self.sband_comm_service,
                        self.sband_ingress_diagnostics_override_dropin_name,
                    )
                    wait_service_active(self.subsystem_target, self.sband_comm_service, self.restart_timeout)
                    self.sband_ingress_diagnostics_override_applied = False

                cleanup_step("sband-ingress-diagnostics-override-removed", remove_sband_ingress_diagnostics_override)

            cleanup_step("remote-beacon-capture-stopped", self.stop_remote_beacon_capture)

            if self.uhf_beacon_target_override_applied:
                def remove_uhf_beacon_target_override() -> None:
                    remove_service_override(
                        self.obc_target,
                        self.obc_service,
                        self.uhf_beacon_target_override_dropin_name,
                    )
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.uhf_beacon_target_override_applied = False

                cleanup_step("uhf-beacon-target-override-removed", remove_uhf_beacon_target_override)

            if self.obc_groundlink_diagnostics_override_applied:
                def remove_obc_groundlink_diagnostics_override() -> None:
                    remove_service_override(
                        self.obc_target,
                        self.obc_service,
                        self.obc_groundlink_diagnostics_override_dropin_name,
                    )
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.obc_groundlink_diagnostics_override_applied = False

                cleanup_step("obc-groundlink-diagnostics-override-removed", remove_obc_groundlink_diagnostics_override)

            if self.obc_groundlink_timeout_override_applied:
                def remove_obc_groundlink_timeout_override() -> None:
                    remove_service_override(
                        self.obc_target,
                        self.obc_service,
                        self.obc_groundlink_timeout_override_dropin_name,
                    )
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.obc_groundlink_timeout_override_applied = False

                cleanup_step("obc-groundlink-timeout-override-removed", remove_obc_groundlink_timeout_override)

            if self.csp_socketcan_canfd_override_applied:
                def remove_csp_socketcan_canfd_override() -> None:
                    subsystem_services = (
                        self.sband_comm_service,
                        self.uhf_comm_service,
                    )
                    for service_name in subsystem_services:
                        remove_service_override(
                            self.subsystem_target,
                            service_name,
                            self.csp_socketcan_canfd_override_dropin_name,
                        )
                        wait_service_active(self.subsystem_target, service_name, self.restart_timeout)
                    remove_service_override(
                        self.obc_target,
                        self.obc_service,
                        self.csp_socketcan_canfd_override_dropin_name,
                    )
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    self.csp_socketcan_canfd_override_applied = False

                cleanup_step("csp-socketcan-canfd-override-removed", remove_csp_socketcan_canfd_override)

            if self.quiet_override_applied:
                cleanup_step("quiet-override-removed", self.remove_quiet_override)

            if self.profile_override_applied:
                def remove_profile_override() -> None:
                    remove_service_override(self.obc_target, self.obc_service, self.profile_override_dropin_name)
                    wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
                    restored_env = service_environment(self.obc_target, self.obc_service)
                    restored_profile = restored_env.get("TARGET_COMM_PROFILE", "sband")
                    if restored_profile != self.baseline_target_comm_profile:
                        raise ProbeFailure(
                            f"failed to restore original target COMM profile; expected {self.baseline_target_comm_profile} observed {restored_profile}"
                        )
                    self.profile_override_applied = False

                cleanup_step("profile-override-removed", remove_profile_override)

            cleanup_step("subsystem-environment-restored", self.restore_subsystem_environment)
            cleanup_step("sband-ground-stopped", self.sband.stop)
            cleanup_step("uhf-ground-stopped", self.uhf.stop)
            self.write_json_artifact(
                self.diagnostics_dir / "cleanup-status.json",
                {
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
                    "mode": self.mode,
                    "profile": self.profile,
                    "verdict": "PASS" if not cleanup_errors else "FAIL",
                    "errors": cleanup_errors,
                },
            )

            if self.mode == "reliable-transfer" and self.profile == "uhf-primary" and not cleanup_errors:
                self.snapshot_service_set("reliable-transfer-quiet-proof-restored")
                self.checkpoint(
                    "reliable-transfer-nonquiet-baseline-restored",
                    "pass",
                    obc_service=self.obc_service,
                    receiver_service=self.reliable_transfer_receiver_service,
                )

            if cleanup_errors:
                detail = "; ".join(cleanup_errors)
                self.note(f"cleanup-failures: {detail}")
                if run_error is None:
                    raise ProbeFailure(f"cleanup failed: {detail}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--mode",
        choices=(
            "command",
            "dual-link-proof",
            "secure-auth-proof",
            "nonquiet-diagnosis",
            "file-downlink",
            "reliable-transfer",
            "sequence-subsystem",
            "failover-command",
            "beacon-suppression",
        ),
        required=True,
    )
    parser.add_argument("--profile", choices=("sband", "uhf-primary"), required=True)
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    scenario = TargetCanScenario(args.mode, args.profile, pathlib.Path(args.probe_root))
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    try:
        summary_lines = [
            "target-can-matrix-probe: PASS",
            f"mode={args.mode}",
            f"profile={args.profile}",
            f"probe-root={args.probe_root}",
        ]
        summary_lines.extend(scenario.run())
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        output = f"target-can-matrix-probe: FAIL {exc}\n"
        exit_code = 1
    except Exception:
        output = "target-can-matrix-probe: FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        scenario.sband.force_stop()
        scenario.uhf.force_stop()
        if exit_code == 0:
            sys.stdout.write(output)
        else:
            sys.stderr.write(output)
        sys.stdout.flush()
        sys.stderr.flush()
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
