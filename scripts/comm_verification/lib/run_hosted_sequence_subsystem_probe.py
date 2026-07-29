#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import pathlib
import pty
import re
import shutil
import signal
import socket
import struct
import subprocess
import sys
import termios
import time
from dataclasses import dataclass

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parents[2]
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
from secure_link_auth_lib import default_command_auth_keystore_path, legacy_auth_material_for_profile, load_command_auth_keystore


COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001
COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01
COMMAND_ENVELOPE_V1_VERSION = 1
COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28
COMMAND_ENVELOPE_V1_MAC_LENGTH = 32

KEYSTORE = load_command_auth_keystore(default_command_auth_keystore_path(ROOT_DIR))
SBAND_AUTH = legacy_auth_material_for_profile(KEYSTORE, "sband-primary")
UHF_AUTH = legacy_auth_material_for_profile(KEYSTORE, "uhf-backup")

SBAND_SOURCE_ID = SBAND_AUTH.source_id
SBAND_KEY_SLOT = SBAND_AUTH.key_slot
SBAND_KEY_BYTES = SBAND_AUTH.key_bytes
UHF_SOURCE_ID = UHF_AUTH.source_id
UHF_KEY_SLOT = UHF_AUTH.key_slot
UHF_KEY_BYTES = UHF_AUTH.key_bytes


class ProbeFailure(RuntimeError):
    pass


PROCESS_TERM_TIMEOUT = 1.0
PROCESS_KILL_TIMEOUT = 1.0


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
        return path.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text(path)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for {fragment!r} in {path}")


def ensure_no_legacy_aliases(root_dir: pathlib.Path) -> None:
    for alias in (root_dir / ".sequence-staging", root_dir / ".sequence-admitted"):
        if alias.exists() or alias.is_symlink():
            raise ProbeFailure(f"legacy checkout-level alias should not be created: {alias.name}")


def command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise ProbeFailure(f"command {name!r} not found in dictionary")


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
class ManagedProcess:
    name: str
    process: subprocess.Popen[str]
    handle: object


@dataclass(frozen=True)
class AuthProfile:
    source_id: int
    key_slot: int
    key_bytes: bytes


@dataclass(frozen=True)
class SequenceArtifact:
    source: pathlib.Path
    binary: pathlib.Path
    destination: str


class PtyPeer:
    def __init__(self) -> None:
        self.master_fd, slave_fd = pty.openpty()
        self.slave_path = os.ttyname(slave_fd)
        os.close(slave_fd)
        os.set_blocking(self.master_fd, False)
        attrs = termios.tcgetattr(self.master_fd)
        attrs[3] = attrs[3] & ~(termios.ICANON | termios.ECHO)
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(self.master_fd, termios.TCSANOW, attrs)

    def close(self) -> None:
        if self.master_fd >= 0:
            os.close(self.master_fd)
            self.master_fd = -1


class GroundPath:
    def __init__(self, name: str, cli_path: pathlib.Path, dictionary_path: pathlib.Path, probe_root: pathlib.Path, vcid: int) -> None:
        self.name = name
        self.cli_path = cli_path
        self.dictionary_path = dictionary_path
        self.root = probe_root / name
        self.gds_port = free_port()
        self.gds_tts_port = free_port()
        self.file_storage = self.root / "gds-files"
        self.gds_log = self.root / "gds.log"
        self.events_log = self.root / "events.log"
        self.channels_log = self.root / "channels.log"
        self.raw_command_log = self.root / "raw-command.log"
        self.file_uplink_log = self.root / "file-uplink.log"
        self.processes: list[ManagedProcess] = []
        self.pipeline: StandardPipeline | None = None
        self.api: IntegrationTestAPI | None = None
        self.vcid = vcid
        if vcid == 1:
            self.link_up_fragment = "OBCApp.groundLinkDriver.GROUND_LINK_UP"
            self.link_down_fragment = "OBCApp.groundLinkDriver.GROUND_LINK_DOWN"
        else:
            self.link_up_fragment = "OBCApp.uhfGroundLinkDriver.GROUND_LINK_UP"
            self.link_down_fragment = "OBCApp.uhfGroundLinkDriver.GROUND_LINK_DOWN"

    def start(self, root_dir: pathlib.Path) -> None:
        self.root.mkdir(parents=True, exist_ok=True)
        self.file_storage.mkdir(parents=True, exist_ok=True)
        self._start(
            "ground_gds",
            [
                str(root_dir / "fprime-venv/bin/fprime-gds"),
                "-n",
                "-g",
                "none",
                "--framing-selection",
                "space-packet-space-data-link",
                "--scid",
                "68",
                "--vcid",
                str(self.vcid),
                "--frame-size",
                "1024",
                "--dictionary",
                str(self.dictionary_path),
                "--no-zmq",
                "--ip-address",
                "127.0.0.1",
                "--ip-port",
                str(self.gds_port),
                "--tts-port",
                str(self.gds_tts_port),
                "--tts-addr",
                "127.0.0.1",
                "--file-storage-directory",
                str(self.file_storage),
                "--log-directly",
                "--logs",
                str(self.root / "gds-logs"),
            ],
            self.gds_log,
        )
        wait_port(self.gds_port, 20.0)
        wait_port(self.gds_tts_port, 20.0)
        self._start(
            "events",
            [str(self.cli_path), "events", "--dictionary", str(self.dictionary_path), "--no-zmq", "--tts-port", str(self.gds_tts_port)],
            self.events_log,
        )
        time.sleep(1.0)

    def _start(self, name: str, args: list[str], log_path: pathlib.Path, env: dict[str, str] | None = None) -> None:
        handle = log_path.open("a", encoding="utf-8", buffering=1)
        handle.write("$ " + " ".join(args) + "\n")
        handle.flush()
        process = subprocess.Popen(
            args,
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            cwd=str(ROOT_DIR),
        )
        self.processes.append(ManagedProcess(name, process, handle))

    def connect_api(self) -> None:
        if self.pipeline is not None and self.api is not None:
            return
        event_start = self.event_count()
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
        pipeline.connect(f"127.0.0.1:{self.gds_tts_port}")
        time.sleep(3.0)
        recent_events = read_text(self.events_log).splitlines()[event_start:]
        if any(self.link_down_fragment in line for line in recent_events):
            self.await_event(self.link_up_fragment, timeout=10.0, start=event_start)
            time.sleep(0.2)
        self.pipeline = pipeline
        self.api = IntegrationTestAPI(pipeline, logpath=str(self.root / "test-api"))

    def upload_file(self, local_path: pathlib.Path, destination: str) -> int:
        self.connect_api()
        if self.api is None:
            raise ProbeFailure(f"{self.name}: API connection missing")
        event_start = self.event_count()
        with self.file_uplink_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"api-upload: source={local_path} destination={destination} size={local_path.stat().st_size}\n"
            )
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

    def event_count(self) -> int:
        return len(read_text(self.events_log).splitlines())

    def event_lines(self) -> list[str]:
        return read_text(self.events_log).splitlines()

    def await_event(self, fragment: str, timeout: float = 10.0, start: int | None = None) -> str:
        deadline = time.time() + timeout
        start_index = 0 if start is None else start
        while time.time() < deadline:
            lines = self.event_lines()
            for line in lines[start_index:]:
                if fragment in line:
                    return line
            time.sleep(0.2)
        raise ProbeFailure(f"{self.name}: timed out waiting for {fragment!r}")

    def assert_no_event(self, fragment: str, timeout: float = 2.0, start: int | None = None) -> None:
        deadline = time.time() + timeout
        start_index = 0 if start is None else start
        while time.time() < deadline:
            lines = self.event_lines()
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
            "--tts-port",
            str(self.gds_tts_port),
            "--search",
            search,
            "--timeout",
            "8",
        ]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False, cwd=str(ROOT_DIR))
        text = result.stdout
        with self.channels_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ {' '.join(cmd)} # {label}\n")
            handle.write(text)
            if text and not text.endswith("\n"):
                handle.write("\n")
            handle.write(f"returncode={result.returncode}\n")
        if result.returncode != 0 and search not in text:
            raise ProbeFailure(f"{self.name}: channel search failed for {label}")
        return text

    def link_event_lines(self, lines: list[str]) -> list[str]:
        return [
            line for line in lines
            if self.link_up_fragment in line or self.link_down_fragment in line
        ]

    def latest_link_state(self) -> str | None:
        lines = self.link_event_lines(self.event_lines())
        for line in reversed(lines):
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
            lines = self.event_lines()
            link_lines = self.link_event_lines(lines)
            current_count = len(link_lines)
            current_state = "UP" if link_lines and self.link_up_fragment in link_lines[-1] else "DOWN"
            if current_count != last_link_event_count:
                last_link_event_count = current_count
                quiet_start = time.time() if current_state == "UP" else None
            if current_state == "UP" and quiet_start is not None and (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.1)
        raise ProbeFailure(f"{self.name}: timed out waiting for a quiet ground-link UP window")

    def stop(self) -> None:
        (self.root / "stop.log").parent.mkdir(parents=True, exist_ok=True)
        with (self.root / "stop.log").open("a", encoding="utf-8") as handle:
            handle.write("ground-stop-begin\n")
        # Ground-side API teardown can block while repo-owned listeners are still
        # alive. Kill the owned helpers first and let process exit reclaim the
        # local pipeline objects.
        self.api = None
        self.pipeline = None
        while self.processes:
            managed = self.processes.pop()
            with (self.root / "stop.log").open("a", encoding="utf-8") as handle:
                handle.write(f"stop-process {managed.name} pid={managed.process.pid}\n")
            if managed.process.poll() is None:
                try:
                    os.killpg(os.getpgid(managed.process.pid), signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    managed.process.wait(timeout=PROCESS_TERM_TIMEOUT)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    managed.process.wait(timeout=PROCESS_KILL_TIMEOUT)
            managed.handle.close()
        with (self.root / "stop.log").open("a", encoding="utf-8") as handle:
            handle.write("ground-stop-complete\n")

    def force_stop(self) -> None:
        self.api = None
        self.pipeline = None
        while self.processes:
            managed = self.processes.pop()
            try:
                os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            try:
                managed.process.wait(timeout=0.2)
            except subprocess.TimeoutExpired:
                pass
            managed.handle.close()


class HostedScenario:
    def __init__(
        self,
        root_dir: pathlib.Path,
        bin_dir: pathlib.Path,
        dictionary_path: pathlib.Path,
        cli_path: pathlib.Path,
        seqgen_path: pathlib.Path,
        probe_root: pathlib.Path,
    ) -> None:
        self.root_dir = root_dir
        self.bin_dir = bin_dir
        self.dictionary_path = dictionary_path
        self.cli_path = cli_path
        self.seqgen_path = seqgen_path
        self.probe_root = probe_root
        runtime_suffix = hashlib.sha1(str(probe_root).encode("utf-8")).hexdigest()[:8]
        self.runtime_root = pathlib.Path("/tmp") / f"commv-seq-{runtime_suffix}"
        self.sband = GroundPath("sband-ground", cli_path, dictionary_path, probe_root, vcid=1)
        self.uhf = GroundPath("uhf-ground", cli_path, dictionary_path, probe_root, vcid=2)
        self.csp_sub_port = free_port()
        self.csp_pub_port = free_port()
        self.radio_port = free_port()
        self.sband_tcp_port = free_port()
        self.processes: list[ManagedProcess] = []
        self.uhf_beacon = PtyPeer()
        self.sequence_src_dir = self.probe_root / "sequence-src"
        self.sequence_bin_dir = self.probe_root / "sequence-bin"
        self.sequence_session_sband = 9501
        self.sequence_session_uhf = 9601
        self.opcodes: dict[str, int] = {}
        self.command_dictionaries: Dictionaries | None = None
        self.command_encoder: CmdEncoder | None = None
        self.file_encoder = FileEncoder()
        self.status_log = self.probe_root / "status.log"
        self.ingress_boundaries_log = self.probe_root / "ingress-boundaries.jsonl"
        self.packet_audit_dir = self.probe_root / "packet-audit"
        self.last_switch_source: str | None = None
        self.last_session_sources: dict[str, str] = {}
        self.active_sessions: dict[str, int] = {}
        self.common_env: dict[str, str] | None = None
        self.uhf_stack_started = False

    def note(self, message: str) -> None:
        self.probe_root.mkdir(parents=True, exist_ok=True)
        with self.status_log.open("a", encoding="utf-8") as handle:
            handle.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')} {message}\n")

    def record_boundary(
        self,
        ground: GroundPath,
        stage: str,
        status: str,
        detail: str,
        event_start: int | None = None,
    ) -> None:
        record: dict[str, object] = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "ground": ground.name,
            "stage": stage,
            "status": status,
            "detail": detail,
            "linkState": ground.latest_link_state(),
        }
        if event_start is not None:
            event_lines = read_text(ground.events_log).splitlines()[event_start:]
            record["eventTail"] = event_lines[-8:]
        obc_lines = read_text(self.probe_root / "obc.log").splitlines()
        if obc_lines:
            record["obcTail"] = obc_lines[-8:]
        with self.ingress_boundaries_log.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, sort_keys=True) + "\n")

    def start(self) -> None:
        if self.probe_root.exists():
            shutil.rmtree(self.probe_root)
        if self.runtime_root.exists():
            shutil.rmtree(self.runtime_root)
        self.probe_root.mkdir(parents=True, exist_ok=True)
        self.note("scenario-start")
        self.sequence_src_dir.mkdir(parents=True, exist_ok=True)
        self.sequence_bin_dir.mkdir(parents=True, exist_ok=True)
        self.packet_audit_dir.mkdir(parents=True, exist_ok=True)
        (self.runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
        (self.runtime_root / "staging").mkdir(parents=True, exist_ok=True)
        self.sband.start(self.root_dir)
        ensure_no_legacy_aliases(self.root_dir)

        common_env = os.environ.copy()
        common_env["CSP_TRANSPORT"] = "zmqhub"
        common_env["CSP_HUB_HOST"] = "127.0.0.1"
        common_env["CSP_HUB_SUB_PORT"] = str(self.csp_sub_port)
        common_env["CSP_HUB_PUB_PORT"] = str(self.csp_pub_port)
        common_env["EPS_CSP_NODE_ID"] = "2"
        common_env["ADCS_CSP_NODE_ID"] = "3"
        common_env["COMM_NODE_INGRESS_DIAGNOSTICS"] = os.environ.get("COMM_NODE_INGRESS_DIAGNOSTICS", "0")
        common_env["COMM_NODE_STRIP_TC_FILL_PATTERN"] = os.environ.get("COMM_NODE_STRIP_TC_FILL_PATTERN", "0")
        self.common_env = common_env

        self._start("csp_zmqproxy", [str(self.bin_dir / "csp_zmqproxy"), "-s", f"tcp://0.0.0.0:{self.csp_sub_port}", "-p", f"tcp://0.0.0.0:{self.csp_pub_port}"], self.probe_root / "csp_zmqproxy.log", common_env)
        self._start("eps_simulator", [str(self.bin_dir / "eps_simulator"), "--node-id", "2"], self.probe_root / "eps_simulator.log", common_env)
        self._start("adcs_simulator", [str(self.bin_dir / "adcs_simulator"), "--node-id", "3"], self.probe_root / "adcs_simulator.log", common_env)
        self._start("radio_mock_server", [str(self.bin_dir / "radio_mock_server"), "--port", str(self.radio_port)], self.probe_root / "radio_mock_server.log", common_env)

        self._start(
            "sband_comm_csp_node",
            [str(self.bin_dir / "sband_comm_csp_node"), "--tcp-listen-host", "127.0.0.1", "--tcp-listen-port", str(self.sband_tcp_port), "--node-id", "5"],
            self.probe_root / "sband_comm_csp_node.log",
            common_env,
        )
        wait_port(self.sband_tcp_port, 20.0)

        self._start(
            "sband_ground_ttc_gateway",
            self.gateway_args(
                link_identity="sband",
                gds_port=self.sband.gds_port,
                rf_tcp_port=self.sband_tcp_port,
                capture_dir=self.probe_root / "sband-gateway-captures",
            ),
            self.probe_root / "sband_ground_ttc_gateway.log",
            common_env,
        )

        self._start(
            "OBC",
            [
                str(self.bin_dir / "OBC"),
                "--comm",
                "tcp",
                "--comm-host",
                "127.0.0.1",
                "--comm-port",
                str(self.radio_port),
                "--radio-protocol",
                "mock-text",
                "--ground-link",
                "comm-csp",
                "--comm-csp-node",
                "5",
                "--gds-host",
                "127.0.0.1",
                "--gds-port",
                str(self.sband.gds_port),
                "--runtime-root",
                str(self.runtime_root),
                "--persistent-root",
                str(self.runtime_root / "persistent-data"),
                "--staging-root",
                str(self.runtime_root / "staging"),
                "--command-authority-profile",
                "sband-primary",
                "--tick-ms",
                "1000",
                *(["--diagnostic-quiet-packet-egress"] if os.environ.get("DIAGNOSTIC_QUIET_PACKET_EGRESS", "0") == "1" else []),
                "--headless",
            ],
            self.probe_root / "obc.log",
            common_env,
            cwd=self.probe_root,
        )
        wait_text(self.probe_root / "obc.log", "Runtime mode: headless", 30.0)
        ensure_no_legacy_aliases(self.root_dir)
        self.load_opcodes()

    def gateway_args(
        self,
        *,
        link_identity: str,
        gds_port: int,
        rf_tcp_port: int | None = None,
        serial_device: str | None = None,
        capture_dir: pathlib.Path | None = None,
    ) -> list[str]:
        args = [
            str(self.bin_dir / "ground_ttc_gateway"),
        ]
        if rf_tcp_port is not None:
            args.extend(
                [
                    "--rf-tcp-host",
                    "127.0.0.1",
                    "--rf-tcp-port",
                    str(rf_tcp_port),
                ]
            )
        elif serial_device is not None:
            args.extend(
                [
                    "--serial-device",
                    serial_device,
                    "--baudrate",
                    "115200",
                ]
            )
        else:
            raise ProbeFailure("gateway args require either rf_tcp_port or serial_device")
        args.extend(
            [
                "--link-identity",
                link_identity,
                "--gds-host",
                "127.0.0.1",
                "--gds-port",
                str(gds_port),
            ]
        )
        if os.environ.get("COMMV_CAPTURE_GATEWAY", "0") == "1" and capture_dir is not None:
            capture_dir.mkdir(parents=True, exist_ok=True)
            args.extend(
                [
                    "--capture-gds-to-southbound",
                    str(capture_dir / "gds-to-southbound.bin"),
                    "--capture-southbound-to-gds",
                    str(capture_dir / "southbound-to-gds.bin"),
                ]
            )
        return args

    def start_uhf_stack(self) -> None:
        if self.uhf_stack_started:
            return
        if self.common_env is None:
            raise ProbeFailure("hosted common environment is not initialized")
        self.uhf.start(self.root_dir)
        pty_log = (self.probe_root / "uhf-pty-bridge.log").open("a", encoding="utf-8", buffering=1)
        pty_bridge = subprocess.Popen(
            [str(self.bin_dir / "pty_pair_bridge")],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            bufsize=1,
            cwd=str(self.root_dir),
        )
        if pty_bridge.stdout is None:
            raise ProbeFailure("pty_pair_bridge did not expose stdout")
        pty_paths: dict[str, str] = {}
        for _ in range(2):
            line = pty_bridge.stdout.readline()
            if not line:
                raise ProbeFailure("pty_pair_bridge did not report PTY paths")
            pty_log.write(line)
            key, value = line.strip().split("=", 1)
            pty_paths[key] = value
        self.processes.append(ManagedProcess("pty_pair_bridge", pty_bridge, pty_log))
        gateway_serial = pty_paths["PTY_A"]
        uhf_serial = pty_paths["PTY_B"]
        self._start(
            "uhf_comm_csp_node",
            [
                str(self.bin_dir / "uhf_comm_csp_node"),
                "--serial-device",
                uhf_serial,
                "--baudrate",
                "115200",
                "--node-id",
                "6",
                "--beacon-serial-device",
                self.uhf_beacon.slave_path,
                "--beacon-baudrate",
                "115200",
            ],
            self.probe_root / "uhf_comm_csp_node.log",
            self.common_env,
        )
        self._start(
            "uhf_ground_ttc_gateway",
            self.gateway_args(
                link_identity="uhf",
                gds_port=self.uhf.gds_port,
                serial_device=gateway_serial,
                capture_dir=self.probe_root / "uhf-gateway-captures",
            ),
            self.probe_root / "uhf_ground_ttc_gateway.log",
            self.common_env,
        )
        self.uhf_stack_started = True
        self.note("uhf-stack-started")

    def load_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes = {
            "OBCApp.commandIngressAuthority.SESSION_OPEN": command_opcode(dictionary, "OBCApp.commandIngressAuthority.SESSION_OPEN"),
            "OBCApp.commController.COMM_SET_ACTIVE": command_opcode(dictionary, "OBCApp.commController.COMM_SET_ACTIVE"),
            "OBCApp.sequenceAdmissionController.SEQ_VALIDATE": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_VALIDATE"),
            "OBCApp.sequenceAdmissionController.SEQ_RUN": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_RUN"),
            "OBCApp.epsBridge.EPS_GET_STATUS": command_opcode(dictionary, "OBCApp.epsBridge.EPS_GET_STATUS"),
            "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": command_opcode(dictionary, "OBCApp.adcsBridge.ADCS_GET_ATTITUDE"),
        }
        dictionaries = Dictionaries()
        dictionaries.load_dictionaries(str(self.dictionary_path), None, None)
        self.command_dictionaries = dictionaries
        self.command_encoder = CmdEncoder()

    def obc_line_count(self) -> int:
        return len(read_text(self.probe_root / "obc.log").splitlines())

    def _start(
        self,
        name: str,
        args: list[str],
        log_path: pathlib.Path,
        env: dict[str, str] | None = None,
        cwd: pathlib.Path | None = None,
    ) -> None:
        handle = log_path.open("a", encoding="utf-8", buffering=1)
        handle.write("$ " + " ".join(args) + "\n")
        handle.flush()
        process = subprocess.Popen(
            args,
            env=env,
            stdin=subprocess.DEVNULL,
            stdout=handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            cwd=str(cwd or self.root_dir),
        )
        self.processes.append(ManagedProcess(name, process, handle))

    def stop(self) -> None:
        self.note("scenario-stop-begin")
        while self.processes:
            managed = self.processes.pop()
            self.note(f"stop-process {managed.name} pid={managed.process.pid}")
            if managed.process.poll() is None:
                try:
                    os.killpg(os.getpgid(managed.process.pid), signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    managed.process.wait(timeout=PROCESS_TERM_TIMEOUT)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    managed.process.wait(timeout=PROCESS_KILL_TIMEOUT)
            managed.handle.close()
        self.note("scenario-stop-ground-sband")
        self.sband.stop()
        self.note("scenario-stop-ground-uhf")
        self.uhf.stop()
        self.uhf_beacon.close()
        self.note("scenario-stop-complete")

    def force_stop(self) -> None:
        while self.processes:
            managed = self.processes.pop()
            try:
                os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
            except ProcessLookupError:
                pass
            try:
                managed.process.wait(timeout=0.2)
            except subprocess.TimeoutExpired:
                pass
            managed.handle.close()
        self.sband.force_stop()
        self.uhf.force_stop()
        self.uhf_beacon.close()

    def build_sequence(self, name: str) -> SequenceArtifact:
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
            "carrierKind": "uhf-hosted-serial-standin" if ground is self.uhf else "sband-tcp",
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
        path = self.packet_audit_dir / f"{ground.name}-{mode}-packet-audit.json"
        path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return path

    def encode_inner_command(self, command_name: str, *args: str) -> bytes:
        if self.command_dictionaries is None or self.command_encoder is None:
            raise ProbeFailure("command dictionary is not initialized")
        template = self.command_dictionaries.command_name[command_name]
        encoded = self.command_encoder.encode_api(CmdData(tuple(args), template))
        return encoded[8:]

    def send_file_packet(self, ground: GroundPath, label: str, packet: StartPacketData | DataPacketData | EndPacketData) -> None:
        payload = self.file_encoder.encode_api(packet)
        with ground.file_uplink_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: packet_type={packet.packetType.name} seq={packet.seqID} encoded={payload.hex()}\n"
            )
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)

    def send_envelope(self, ground: GroundPath, label: str, profile: AuthProfile, session_id: int, sequence_number: int, command_name: str, *args: str) -> int:
        inner = self.encode_inner_command(command_name, *args)
        opcode = self.opcodes[command_name]
        payload = authenticated_envelope(inner, profile.source_id, profile.key_slot, profile.key_bytes, session_id, sequence_number)
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: command={command_name} args={list(args)} opcode=0x{opcode:x} "
                f"session={session_id} seq={sequence_number} outer={payload.hex()}\n"
            )
        start = ground.event_count()
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)
        return start

    def open_session(self, ground: GroundPath, profile: AuthProfile, session_id: int, role_fragment: str) -> int:
        last_error: ProbeFailure | None = None
        for attempt in range(2):
            active_session_id = session_id + attempt
            self.note(f"{ground.name}-session-open-attempt-{attempt + 1}-begin session={active_session_id}")
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            obc_start = self.obc_line_count()
            start = self.send_envelope(
                ground,
                f"{ground.name} session open attempt {attempt + 1}",
                profile,
                active_session_id,
                0,
                "OBCApp.commandIngressAuthority.SESSION_OPEN",
            )
            try:
                source = self.wait_ground_or_obc(
                    ground,
                    start,
                    obc_start,
                    ("COMMAND_SESSION_OPENED", role_fragment),
                    ("COMMAND_SESSION_OPENED", role_fragment),
                    12.0,
                    f"{ground.name} session open",
                )
                self.last_session_sources[ground.name] = source
                self.active_sessions[ground.name] = active_session_id
                self.note(f"{ground.name}-session-open-attempt-{attempt + 1}-pass session={active_session_id} source={source}")
                return active_session_id
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-session-open-attempt-{attempt + 1}-retry {exc}")
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: session open failed without a recorded error")
        raise last_error

    def switch_to_uhf_primary(self, session_id: int) -> None:
        last_error: ProbeFailure | None = None
        for attempt in range(6):
            active_sequence = 1 + attempt
            self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-begin seq={active_sequence}")
            self.sband.await_link_up_if_needed()
            self.sband.wait_link_quiet()
            time.sleep(0.3)
            uhf_event_start = self.uhf.event_count()
            sband_event_start = self.sband.event_count()
            obc_start = self.obc_line_count()
            start = self.send_envelope(
                self.sband,
                f"switch-to-uhf-primary-attempt-{attempt + 1}",
                AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES),
                session_id,
                active_sequence,
                "OBCApp.commController.COMM_SET_ACTIVE",
                "UHF",
            )
            try:
                source = self.wait_ground_or_obc(
                    self.uhf,
                    uhf_event_start,
                    obc_start,
                    ("Comm primary links command UHF telemetry UHF file UHF reason 1",),
                    ("COMM_PRIMARY_LINK_CHANGED", "command UHF", "telemetry UHF", "file UHF", "reason 1"),
                    12.0,
                    "switch to UHF primary",
                )
                try:
                    self.uhf.await_link_up_if_needed(timeout=8.0)
                except ProbeFailure:
                    self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-uhf-link-up-not-replayed")
                try:
                    self.uhf.wait_link_quiet(timeout=8.0)
                except ProbeFailure:
                    self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-uhf-quiet-window-not-observed")
                self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-uhf-primary-visible")
                try:
                    self.sband.await_event(
                        f"Opcode 0x{self.opcodes['OBCApp.commController.COMM_SET_ACTIVE']:x} completed",
                        timeout=4.0,
                        start=sband_event_start,
                    )
                except ProbeFailure:
                    # The success oracle is the UHF-side primary-link event. After
                    # the switch, command/event ownership can move before the
                    # originating S-band event stream records a matching completion.
                    self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-sband-opcode-complete-not-observed")
                self.last_switch_source = source
                self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-pass seq={active_sequence} source={source}")
                return
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"switch-to-uhf-primary-attempt-{attempt + 1}-retry {exc}")
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure("UHF primary switch failed without a recorded error")
        raise last_error

    def write_upload_audit(self, ground: GroundPath, artifact: SequenceArtifact, mode: str, packet_sequence_numbers: tuple[int, int, int]) -> pathlib.Path:
        if ground is self.uhf:
            authority_profile = "uhf-primary"
            source_id = UHF_SOURCE_ID
            key_slot = UHF_KEY_SLOT
        else:
            authority_profile = "sband-primary"
            source_id = SBAND_SOURCE_ID
            key_slot = SBAND_KEY_SLOT
        return self.write_sequence_packet_audit(
            ground,
            artifact,
            mode=mode,
            authority_profile=authority_profile,
            source_id=source_id,
            key_slot=key_slot,
            ingress_port=1,
            session_id=self.active_sessions.get(ground.name),
            switch_source=self.last_switch_source,
            session_open_source=self.last_session_sources.get(ground.name),
            packet_sequence_numbers=packet_sequence_numbers,
        )

    def wait_ground_or_obc(
        self,
        ground: GroundPath,
        ground_start: int,
        obc_start: int,
        ground_fragments: tuple[str, ...],
        obc_fragments: tuple[str, ...],
        timeout: float,
        label: str,
    ) -> str:
        deadline = time.time() + timeout
        last_events = ""
        last_obc = ""
        while time.time() < deadline:
            event_lines = ground.event_lines()
            last_events = "\n".join(event_lines[ground_start:])
            if all(fragment in last_events for fragment in ground_fragments):
                return "ground-events"
            obc_lines = read_text(self.probe_root / "obc.log").splitlines()
            last_obc = "\n".join(obc_lines[obc_start:])
            if all(fragment in last_obc for fragment in obc_fragments):
                return "obc-log"
            time.sleep(0.2)
        raise ProbeFailure(
            f"timed out waiting for {label}; ground_fragments={ground_fragments} obc_fragments={obc_fragments}\n"
            f"ground_tail={last_events[-4000:]}\nobc_tail={last_obc[-4000:]}"
        )

    def prepare_ground_window(
        self,
        ground: GroundPath,
        timeout: float = 8.0,
        *,
        strict: bool = False,
        quiet_sec: float = 0.3,
    ) -> None:
        try:
            ground.await_link_up_if_needed(timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-link-up-not-observed-within-{timeout:g}s")
            if strict:
                raise
        try:
            ground.wait_link_quiet(quiet_sec=quiet_sec, timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-quiet-window-not-observed-within-{timeout:g}s")
            if strict:
                raise

    def prepare_upload_window(self, ground: GroundPath) -> None:
        if ground is self.uhf:
            # Hosted node-6 stand-in currently does not reliably replay
            # uhfGroundLinkDriver link events onto the UHF ground event stream
            # after switch/session-open, even when the target OBC accepts UHF
            # commands. Use the successful UHF session-open plus a bounded settle
            # window as the readiness gate instead of requiring a fresh ground
            # link-up replay that often never arrives on this path.
            self.prepare_ground_window(ground, strict=False, quiet_sec=0.5)
            settle_sec = float(os.environ.get("HOSTED_UHF_UPLOAD_SETTLE_SEC", "1.5"))
            self.note(f"{ground.name}-upload-settle-window-{settle_sec:.1f}s")
            time.sleep(settle_sec)
            return
        self.prepare_ground_window(ground, strict=True, quiet_sec=0.5)
        time.sleep(0.3)

    def upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> int:
        last_error: ProbeFailure | None = None
        if ground is self.uhf and os.environ.get("HOSTED_UHF_START_ONLY_DIAGNOSTIC", "0") == "1":
            self.run_start_only_diagnostic(ground, artifact)
        for attempt in range(2):
            self.note(f"{ground.name}-upload-attempt-{attempt + 1}-begin path={artifact.destination}")
            audit_path = self.write_upload_audit(ground, artifact, mode=f"api-attempt-{attempt + 1}", packet_sequence_numbers=(0, 1, 2))
            self.note(f"{ground.name}-upload-attempt-{attempt + 1}-audit {audit_path}")
            self.prepare_upload_window(ground)
            start = ground.upload_file(artifact.binary, artifact.destination)
            self.record_boundary(
                ground,
                "upload-start",
                "sent",
                f"attempt={attempt + 1} path={artifact.destination}",
                start,
            )
            try:
                saw_start_accept = True
                try:
                    ground.await_event("FILE_INGRESS_START_ACCEPTED", timeout=30.0, start=start)
                    self.record_boundary(
                        ground,
                        "upload-start-accepted",
                        "observed",
                        f"attempt={attempt + 1}",
                        start,
                    )
                except ProbeFailure:
                    saw_start_accept = False
                    self.note(f"{ground.name}-upload-attempt-{attempt + 1}-start-accept-not-observed")
                    self.record_boundary(
                        ground,
                        "upload-start-accepted",
                        "missing",
                        f"attempt={attempt + 1}",
                        start,
                    )
                ground.await_event(
                    "FileReceived",
                    timeout=90.0 if not saw_start_accept else 60.0,
                    start=start,
                )
                self.record_boundary(
                    ground,
                    "upload-file-received",
                    "observed",
                    f"attempt={attempt + 1}",
                    start,
                )
                self.note(f"{ground.name}-upload-attempt-{attempt + 1}-pass path={artifact.destination}")
                return start
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-upload-attempt-{attempt + 1}-retry {exc}")
                self.record_boundary(
                    ground,
                    "upload-file-received",
                    "timeout",
                    f"attempt={attempt + 1} error={exc}",
                    start,
                )
                time.sleep(1.0)
        if ground is self.uhf:
            self.note(f"{ground.name}-api-upload-failed-falling-back-to-manual")
            return self.manual_upload_sequence(ground, artifact)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: sequence upload failed without a recorded error")
        raise last_error

    def run_start_only_diagnostic(self, ground: GroundPath, artifact: SequenceArtifact) -> None:
        self.note(f"{ground.name}-start-only-diagnostic-begin path={artifact.destination}")
        self.prepare_upload_window(ground)
        start = ground.start_file_only(artifact.binary, artifact.destination)
        self.record_boundary(
            ground,
            "start-only-upload",
            "sent",
            f"path={artifact.destination}",
            start,
        )
        try:
            ground.await_event("FILE_INGRESS_START_ACCEPTED", timeout=15.0, start=start)
            self.record_boundary(
                ground,
                "start-only-upload-accepted",
                "observed",
                f"path={artifact.destination}",
                start,
            )
            self.note(f"{ground.name}-start-only-diagnostic-pass path={artifact.destination}")
        except ProbeFailure as exc:
            self.record_boundary(
                ground,
                "start-only-upload-accepted",
                "missing",
                f"path={artifact.destination} error={exc}",
                start,
            )
            self.note(f"{ground.name}-start-only-diagnostic-missing-start-accept {exc}")
        time.sleep(1.0)

    def manual_upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> int:
        ground.connect_api()
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        last_error: ProbeFailure | None = None
        for attempt in range(3):
            self.note(f"{ground.name}-manual-upload-attempt-{attempt + 1}-begin path={artifact.destination}")
            audit_path = self.write_upload_audit(ground, artifact, mode=f"manual-attempt-{attempt + 1}", packet_sequence_numbers=(0, 1, 2))
            self.note(f"{ground.name}-manual-upload-attempt-{attempt + 1}-audit {audit_path}")
            self.prepare_upload_window(ground)
            ground_start = ground.event_count()
            obc_start = self.obc_line_count()
            self.send_file_packet(
                ground,
                f"{ground.name}-manual-start-attempt-{attempt + 1}",
                StartPacketData(0, len(payload), str(artifact.binary), artifact.destination),
            )
            self.record_boundary(
                ground,
                "upload-start",
                "sent",
                f"attempt={attempt + 1} mode=manual path={artifact.destination}",
                ground_start,
            )
            try:
                self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    ("FILE_INGRESS_START_ACCEPTED",),
                    ("FILE_INGRESS_START_ACCEPTED",),
                    20.0,
                    "sequence ingress start accepted",
                )
                self.record_boundary(
                    ground,
                    "upload-start-accepted",
                    "observed",
                    f"attempt={attempt + 1} mode=manual",
                    ground_start,
                )
                self.prepare_upload_window(ground)
                self.send_file_packet(
                    ground,
                    f"{ground.name}-manual-data-attempt-{attempt + 1}",
                    DataPacketData(1, 0, payload),
                )
                self.prepare_upload_window(ground)
                self.send_file_packet(
                    ground,
                    f"{ground.name}-manual-end-attempt-{attempt + 1}",
                    EndPacketData(2, checksum.value),
                )
                self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    ("FileReceived",),
                    ("FileReceived",),
                    60.0,
                    "sequence file received",
                )
                self.record_boundary(
                    ground,
                    "upload-file-received",
                    "observed",
                    f"attempt={attempt + 1} mode=manual",
                    ground_start,
                )
                self.note(f"{ground.name}-manual-upload-attempt-{attempt + 1}-pass path={artifact.destination}")
                return ground_start
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-manual-upload-attempt-{attempt + 1}-retry {exc}")
                self.record_boundary(
                    ground,
                    "upload-file-received",
                    "timeout",
                    f"attempt={attempt + 1} mode=manual error={exc}",
                    ground_start,
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: manual sequence upload failed without a recorded error")
        raise last_error

    def validate_sequence(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_number: int, artifact: SequenceArtifact) -> int:
        last_error: ProbeFailure | None = None
        for attempt in range(2):
            active_sequence = sequence_number + attempt
            self.note(f"{ground.name}-validate-attempt-{attempt + 1}-begin seq={active_sequence}")
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            start = self.send_envelope(
                ground,
                f"sequence-validate-attempt-{attempt + 1}",
                profile,
                session_id,
                active_sequence,
                "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
                artifact.destination,
            )
            self.record_boundary(
                ground,
                "seq-validate",
                "sent",
                f"attempt={attempt + 1} seq={active_sequence}",
                start,
            )
            try:
                ground.await_event("COMMAND_ENVELOPE_OBSERVED", timeout=10.0, start=start)
                ground.assert_no_event("SEQUENCE_CONTROL_REJECTED", timeout=2.0, start=start)
                ground.assert_no_event("COMMAND_AUTHORITY_REJECTED", timeout=2.0, start=start)
                self.record_boundary(
                    ground,
                    "seq-validate",
                    "observed",
                    f"attempt={attempt + 1} seq={active_sequence}",
                    start,
                )
                self.note(f"{ground.name}-validate-attempt-{attempt + 1}-pass seq={active_sequence}")
                return start
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-validate-attempt-{attempt + 1}-retry {exc}")
                self.record_boundary(
                    ground,
                    "seq-validate",
                    "timeout",
                    f"attempt={attempt + 1} seq={active_sequence} error={exc}",
                    start,
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: sequence validate failed without a recorded error")
        raise last_error

    def run_sequence(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_number: int, artifact: SequenceArtifact) -> int:
        last_error: ProbeFailure | None = None
        for attempt in range(3):
            active_sequence = sequence_number + attempt
            self.note(f"{ground.name}-run-attempt-{attempt + 1}-begin seq={active_sequence}")
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            start = self.send_envelope(
                ground,
                f"sequence-run-attempt-{attempt + 1}",
                profile,
                session_id,
                active_sequence,
                "OBCApp.sequenceAdmissionController.SEQ_RUN",
                artifact.destination,
                "WAIT",
            )
            self.record_boundary(
                ground,
                "seq-run",
                "sent",
                f"attempt={attempt + 1} seq={active_sequence}",
                start,
            )
            try:
                ground.await_event("CS_SequenceComplete", timeout=40.0, start=start)
                self.record_boundary(
                    ground,
                    "seq-run",
                    "observed",
                    f"attempt={attempt + 1} seq={active_sequence}",
                    start,
                )
                self.note(f"{ground.name}-run-attempt-{attempt + 1}-pass seq={active_sequence}")
                return start
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-run-attempt-{attempt + 1}-retry {exc}")
                self.record_boundary(
                    ground,
                    "seq-run",
                    "timeout",
                    f"attempt={attempt + 1} seq={active_sequence} error={exc}",
                    start,
                )
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: sequence run failed without a recorded error")
        raise last_error

    def assert_subsystem_roundtrip(self, ground: GroundPath, start: int, obc_start: int) -> None:
        eps_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        adcs_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
        self.wait_ground_or_obc(
            ground,
            start,
            obc_start,
            (f"Opcode 0x{eps_opcode:x} completed",),
            (f"Opcode 0x{eps_opcode:x} completed",),
            8.0,
            f"{ground.name} EPS_GET_STATUS completion",
        )
        self.wait_ground_or_obc(
            ground,
            start,
            obc_start,
            (f"Opcode 0x{adcs_opcode:x} completed",),
            (f"Opcode 0x{adcs_opcode:x} completed",),
            8.0,
            f"{ground.name} ADCS_GET_ATTITUDE completion",
        )

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", choices=("sband-primary", "uhf-primary"), required=True)
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    bin_dir = ROOT_DIR / "build-fprime-automatic-native" / "bin" / "Darwin"
    dict_path = ROOT_DIR / "build-fprime-automatic-native" / "OBC" / "TopCcsds" / "AppTopologyDictionary.json"
    cli_path = ROOT_DIR / "fprime-venv" / "bin" / "fprime-cli"
    seqgen_path = ROOT_DIR / "fprime-venv" / "bin" / "fprime-seqgen"
    if not bin_dir.exists() or not dict_path.exists() or not cli_path.exists() or not seqgen_path.exists():
        raise SystemExit("Required build outputs or F Prime tools are missing.")

    scenario = HostedScenario(ROOT_DIR, bin_dir, dict_path, cli_path, seqgen_path, pathlib.Path(args.probe_root))
    summary_lines: list[str] = [
        "hosted-sequence-subsystem-probe: PASS",
        f"profile={args.profile}",
    ]
    output = ""
    exit_code = 0
    try:
        scenario.note("main-begin")
        scenario.start()
        scenario.note("main-start-complete")
        artifact = scenario.build_sequence("subsystem-roundtrip")
        scenario.note("artifact-built")
        if args.profile == "sband-primary":
            sband_session_id = scenario.open_session(
                scenario.sband,
                AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES),
                scenario.sequence_session_sband,
                "identity 1 role 1 session",
            )
            scenario.note("sband-session-opened")
            scenario.upload_sequence(scenario.sband, artifact)
            scenario.note("sband-sequence-uploaded")
            scenario.validate_sequence(
                scenario.sband,
                AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES),
                sband_session_id,
                1,
                artifact,
            )
            scenario.note("sband-sequence-validated")
            obc_start = scenario.obc_line_count()
            run_start = scenario.run_sequence(
                scenario.sband,
                AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES),
                sband_session_id,
                2,
                artifact,
            )
            scenario.note("sband-sequence-ran")
            scenario.assert_subsystem_roundtrip(scenario.sband, run_start, obc_start)
            scenario.note("sband-subsystem-roundtrip-observed")
            ensure_no_legacy_aliases(ROOT_DIR)
            scenario.note("sband-proof-complete")
            summary_lines.extend(
                [
                    "verdict=sband-primary-pass",
                    f"artifact={artifact.destination}",
                    f"probe-root={scenario.probe_root}",
                    "same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on hosted node-5 path",
                ]
            )
        else:
            sband_session_id = scenario.open_session(
                scenario.sband,
                AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES),
                scenario.sequence_session_sband,
                "identity 1 role 1 session",
            )
            scenario.note("uhf-bootstrap-session-opened")
            scenario.start_uhf_stack()
            time.sleep(1.0)
            scenario.switch_to_uhf_primary(sband_session_id)
            scenario.note("uhf-primary-switched")
            uhf_session_id = scenario.open_session(
                scenario.uhf,
                AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES),
                scenario.sequence_session_uhf,
                "identity 2 role 3 session",
            )
            scenario.note("uhf-session-opened")
            scenario.upload_sequence(scenario.uhf, artifact)
            scenario.note("uhf-sequence-uploaded")
            scenario.validate_sequence(
                scenario.uhf,
                AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES),
                uhf_session_id,
                1,
                artifact,
            )
            scenario.note("uhf-sequence-validated")
            obc_start = scenario.obc_line_count()
            run_start = scenario.run_sequence(
                scenario.uhf,
                AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES),
                uhf_session_id,
                2,
                artifact,
            )
            scenario.note("uhf-sequence-ran")
            scenario.assert_subsystem_roundtrip(scenario.uhf, run_start, obc_start)
            scenario.note("uhf-subsystem-roundtrip-observed")
            ensure_no_legacy_aliases(ROOT_DIR)
            scenario.note("uhf-proof-complete")
            summary_lines.extend(
                [
                    "verdict=uhf-primary-pass",
                    f"artifact={artifact.destination}",
                    f"probe-root={scenario.probe_root}",
                    "same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on hosted node-6 primary-after-switch path",
                ]
            )
        scenario.note("main-success-before-print")
        output = "\n".join(summary_lines)
        exit_code = 0
    except ProbeFailure as exc:
        scenario.note(f"main-failure {exc}")
        output = f"hosted-sequence-subsystem-probe: FAIL\nprofile={args.profile}\nmessage={exc}\nprobe-root={scenario.probe_root}"
        exit_code = 1
    except Exception as exc:
        scenario.note(f"main-unexpected-failure {exc}")
        output = f"hosted-sequence-subsystem-probe: FAIL\nprofile={args.profile}\nmessage={exc}\nprobe-root={scenario.probe_root}"
        exit_code = 1

    if output:
        print(output, flush=True)
    scenario.note("main-force-stop")
    scenario.force_stop()
    sys.stdout.flush()
    sys.stderr.flush()
    os._exit(exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
