#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import shlex
import signal
import socket
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

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parents[2]
sys.path.insert(0, str(ROOT_DIR / "scripts"))

from probe_process_utils import install_signal_cleanup

from run_target_can_matrix_probe import (
    AuthProfile,
    GroundPath,
    ManagedProcess,
    ProbeFailure,
    SBAND_KEY_BYTES,
    SBAND_KEY_SLOT,
    SBAND_SOURCE_ID,
    UHF_KEY_BYTES,
    UHF_KEY_SLOT,
    UHF_SOURCE_ID,
    authenticated_envelope,
    authority_identity_role,
    command_opcode,
    ensure_no_legacy_aliases,
    free_port,
    next_session_id,
    read_persisted_session_floor,
    read_text,
    reap_local_process_pattern,
    require_env,
    ssh_command,
    ssh_capture,
    shq,
)


PROCESS_TERM_TIMEOUT = 1.0
PROCESS_KILL_TIMEOUT = 1.0


def choose_free_port(excluded: set[int] | None = None) -> int:
    excluded = excluded or set()
    for _ in range(32):
        candidate = free_port()
        if candidate not in excluded:
            return candidate
    raise ProbeFailure(f"unable to allocate a distinct free TCP port excluded={sorted(excluded)}")


def wait_host_port(host: str, port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return
        except OSError:
            time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for {host}:{port}")


@dataclass(frozen=True)
class RemoteStackConfig:
    ssh_target: str
    remote_dir: str
    command: str
    env: dict[str, str]


@dataclass(frozen=True)
class SequenceArtifact:
    source: pathlib.Path
    binary: pathlib.Path
    destination: str


class TargetTcpScenario:
    def __init__(self, mode: str, profile: str, probe_root: pathlib.Path) -> None:
        self.mode = mode
        self.profile = profile
        self.probe_root = probe_root
        self.root_dir = pathlib.Path(require_env("COMMV_ROOT_DIR")) if os.environ.get("COMMV_ROOT_DIR") else pathlib.Path(__file__).resolve().parents[3]
        self.bin_dir = self.root_dir / "build-fprime-automatic-native" / "bin" / "Darwin"
        if not self.bin_dir.exists():
            raise ProbeFailure(f"native bin dir is missing: {self.bin_dir}")
        self.dictionary_path = pathlib.Path(
            os.environ.get(
                "DICT_PATH",
                str(self.root_dir / "build-fprime-automatic-native" / "OBC" / "TopCcsds" / "AppTopologyDictionary.json"),
            )
        )
        if not self.dictionary_path.exists():
            raise ProbeFailure(f"dictionary is missing: {self.dictionary_path}")
        self.cli_path = self.root_dir / "fprime-venv" / "bin" / "fprime-cli"
        if not self.cli_path.exists():
            raise ProbeFailure(f"fprime-cli is missing: {self.cli_path}")
        self.seqgen_path = self.root_dir / "fprime-venv" / "bin" / "fprime-seqgen"
        if not self.seqgen_path.exists():
            raise ProbeFailure(f"fprime-seqgen is missing: {self.seqgen_path}")
        self.gateway_bin = self.bin_dir / "ground_ttc_gateway"
        self.proxy_bin = self.bin_dir / "csp_zmqproxy"
        self.obc_target = require_env("OBC_SSH_TARGET")
        self.subsystem_target = require_env("SUBSYSTEM_SIM_SSH_TARGET")
        self.obc_remote_dir = require_env("RPI_REMOTE_DIR")
        self.subsystem_remote_dir = require_env("SUBSYSTEM_SIM_REMOTE_DIR")
        self.remote_host = require_env("REMOTE_HOST")
        self.sband_tcp_host = require_env("SBAND_TCP_HOST")
        self.uhf_tcp_host = os.environ.get("UHF_TCP_HOST", self.sband_tcp_host)
        default_uhf_southbound_mode = (
            "serial-pty" if self.profile == "uhf-primary" and self.mode == "sequence-subsystem" else "tcp-listen"
        )
        self.uhf_southbound_mode = os.environ.get("TARGET_TCP_UHF_SOUTHBOUND_MODE", default_uhf_southbound_mode)
        self.runtime_token = pathlib.Path(probe_root).name
        self.remote_runtime_root = os.environ.get(
            "REMOTE_RUNTIME_ROOT",
            f"{self.obc_remote_dir}/runtime/comm-verification/target-tcp-{self.runtime_token}",
        )
        self.csp_sub_port = int(os.environ.get("CSP_HUB_SUB_PORT", str(choose_free_port())))
        self.csp_pub_port = int(os.environ.get("CSP_HUB_PUB_PORT", str(choose_free_port({self.csp_sub_port}))))
        self.sband_tcp_port = int(os.environ.get("SBAND_TCP_PORT", str(choose_free_port({self.csp_sub_port, self.csp_pub_port}))))
        self.uhf_tcp_port = int(
            os.environ.get(
                "UHF_TCP_PORT",
                str(choose_free_port({self.csp_sub_port, self.csp_pub_port, self.sband_tcp_port})),
            )
        )
        self.sband = GroundPath("sband-ground", self.root_dir, self.cli_path, self.dictionary_path, self.probe_root, 1)
        self.uhf = GroundPath("uhf-ground", self.root_dir, self.cli_path, self.dictionary_path, self.probe_root, 2)
        self.host_log = self.probe_root / "host-csp.log"
        self.subsystem_log = self.probe_root / "subsystem.log"
        self.obc_log = self.probe_root / "obc.log"
        self.sequence_src_dir = self.probe_root / "sequence-src"
        self.sequence_bin_dir = self.probe_root / "sequence-bin"
        self.packet_audit_dir = self.probe_root / "packet-audit"
        self.bridge_helper_path = self.root_dir / "scripts" / "comm_verification" / "lib" / "tcp_ssh_pty_bridge.py"
        self.probe_root.mkdir(parents=True, exist_ok=True)
        self.processes: list[ManagedProcess] = []
        self.obc_process: subprocess.Popen[str] | None = None
        self.command_encoder: CmdEncoder | None = None
        self.file_encoder = FileEncoder()
        self.command_dictionaries: Dictionaries | None = None
        self.opcodes: dict[str, int] = {}
        self.last_switch_source: str | None = None
        self.last_session_sources: dict[str, str] = {}
        self.active_sessions: dict[str, int] = {}
        self.next_session_sequences: dict[tuple[str, int], int] = {}
        self.remote_uhf_bridge_serial: str | None = None
        self.load_opcodes()

    def stop_remote_process_tree_by_command(self, ssh_target: str, command_fragment: str) -> None:
        script = f"""
set -euo pipefail
fragment={shq(command_fragment)}
pids="$(pgrep -f "$fragment" | awk -v self="$$" '$1 != self' || true)"
if [[ -n "$pids" ]]; then
  for pid in $pids; do
    pkill -TERM -P "$pid" >/dev/null 2>&1 || true
    kill "$pid" >/dev/null 2>&1 || true
  done
  sleep 1
  for pid in $pids; do
    pkill -KILL -P "$pid" >/dev/null 2>&1 || true
    kill -9 "$pid" >/dev/null 2>&1 || true
  done
fi
pgrep -fa "$fragment" || true
""".strip()
        output = ssh_capture(
            ssh_target,
            f"/bin/bash -lc {shq(script)}",
            check=False,
        ).strip()
        if output:
            self.note(f"remote-cleanup {ssh_target} fragment={command_fragment} pids={output}")

    def primary_ground_link_driver_enabled(self) -> bool:
        return os.environ.get("ENABLE_PRIMARY_GROUND_LINK_DRIVER", "1") != "0"

    def owned_local_process_patterns(self) -> tuple[str, ...]:
        return (
            rf"csp_zmqproxy -s tcp://0\.0\.0\.0:{self.csp_sub_port} -p tcp://0\.0\.0\.0:{self.csp_pub_port}",
        )

    def reap_owned_local_processes(self) -> None:
        for pattern in self.owned_local_process_patterns():
            reap_local_process_pattern(pattern)

    def note(self, text: str) -> None:
        with (self.probe_root / "notes.log").open("a", encoding="utf-8") as handle:
            handle.write(text + "\n")

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
        advertised_source_path: str | None = None,
    ) -> pathlib.Path:
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        start_seq, data_seq, end_seq = packet_sequence_numbers
        source_path = advertised_source_path or str(artifact.binary)
        start_packet = StartPacketData(start_seq, len(payload), source_path, artifact.destination)
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
            "carrierKind": "uhf-tcp-southbound" if ground is self.uhf else "sband-tcp",
            "mode": mode,
            "compiledSequenceSha256": hashlib.sha256(payload).hexdigest(),
            "sourcePath": str(artifact.source),
            "binaryPath": str(artifact.binary),
            "advertisedSourcePath": source_path,
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

    def load_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes = {
            "OBCApp.commandIngressAuthority.SESSION_OPEN": command_opcode(dictionary, "OBCApp.commandIngressAuthority.SESSION_OPEN"),
            "OBCApp.commController.COMM_SET_ACTIVE": command_opcode(dictionary, "OBCApp.commController.COMM_SET_ACTIVE"),
            "OBCApp.bootManager.GET_RESET_CAUSE": command_opcode(dictionary, "OBCApp.bootManager.GET_RESET_CAUSE"),
            "OBCApp.dpCatalog.BUILD_CATALOG": command_opcode(dictionary, "OBCApp.dpCatalog.BUILD_CATALOG"),
            "OBCApp.dpCatalog.START_XMIT_CATALOG": command_opcode(dictionary, "OBCApp.dpCatalog.START_XMIT_CATALOG"),
            "OBCApp.sequenceAdmissionController.SEQ_VALIDATE": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_VALIDATE"),
            "OBCApp.sequenceAdmissionController.SEQ_RUN": command_opcode(dictionary, "OBCApp.sequenceAdmissionController.SEQ_RUN"),
            "OBCApp.epsBridge.EPS_GET_STATUS": command_opcode(dictionary, "OBCApp.epsBridge.EPS_GET_STATUS"),
            "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": command_opcode(dictionary, "OBCApp.adcsBridge.ADCS_GET_ATTITUDE"),
        }
        dictionaries = Dictionaries()
        dictionaries.load_dictionaries(str(self.dictionary_path), None, None)
        self.command_dictionaries = dictionaries
        self.command_encoder = CmdEncoder()

    def encode_inner_command(self, command_name: str, *args: str) -> bytes:
        if self.command_dictionaries is None or self.command_encoder is None:
            raise ProbeFailure("command dictionary is not initialized")
        template = self.command_dictionaries.command_name[command_name]
        encoded = self.command_encoder.encode_api(CmdData(tuple(args), template))
        return encoded[8:]

    def authority_profile(self, name: str) -> AuthProfile:
        mapping = {
            "sband-primary": AuthProfile(SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES, "sband-primary"),
            "uhf-primary": AuthProfile(UHF_SOURCE_ID, UHF_KEY_SLOT, UHF_KEY_BYTES, "uhf-primary"),
        }
        try:
            return mapping[name]
        except KeyError as exc:
            raise ProbeFailure(f"unsupported authority profile {name}") from exc

    def _start_local(self, name: str, args: list[str], log_path: pathlib.Path, env: dict[str, str] | None = None) -> subprocess.Popen[str]:
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
            cwd=str(self.root_dir),
        )
        self.processes.append(ManagedProcess(name, process, handle))
        return process

    def _start_remote(self, name: str, config: RemoteStackConfig, log_path: pathlib.Path, interactive: bool = False) -> subprocess.Popen[str]:
        remote_env = " ".join(f"{key}={shlex.quote(value)}" for key, value in config.env.items())
        remote_cmd = f"set -euo pipefail; cd {shq(config.remote_dir)}; env {remote_env} bash {shq(config.command)}"
        handle = log_path.open("a", encoding="utf-8", buffering=1)
        handle.write(f"$ ssh {config.ssh_target} {remote_cmd}\n")
        handle.flush()
        process = subprocess.Popen(
            ssh_command(config.ssh_target, f"/bin/bash -lc {shq(remote_cmd)}"),
            stdin=subprocess.PIPE if interactive else subprocess.DEVNULL,
            stdout=handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            cwd=str(self.root_dir),
        )
        self.processes.append(ManagedProcess(name, process, handle))
        return process

    def _start_once(self) -> None:
        self.reap_owned_local_processes()
        enable_primary_ground_link_driver = os.environ.get("ENABLE_PRIMARY_GROUND_LINK_DRIVER", "1")
        enable_comm_subsystem_health_detector = os.environ.get("ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR", "1")
        manage_radio_mock_server = os.environ.get("MANAGE_RADIO_MOCK_SERVER", "0")
        radio_protocol = os.environ.get("RADIO_PROTOCOL", "mock-text")
        host_env = os.environ.copy()
        self._start_local(
            "csp_zmqproxy",
            [
                str(self.proxy_bin),
                "-s",
                f"tcp://0.0.0.0:{self.csp_sub_port}",
                "-p",
                f"tcp://0.0.0.0:{self.csp_pub_port}",
            ],
            self.host_log,
            host_env,
        )
        wait_host_port("127.0.0.1", self.csp_sub_port, 10.0)
        wait_host_port("127.0.0.1", self.csp_pub_port, 10.0)
        subsystem_env = {
            "CSP_TRANSPORT": "zmqhub",
            "CSP_HUB_HOST": self.remote_host,
            "CSP_HUB_SUB_PORT": str(self.csp_sub_port),
            "CSP_HUB_PUB_PORT": str(self.csp_pub_port),
            "SBAND_TCP_HOST": "0.0.0.0",
            "SBAND_TCP_PORT": str(self.sband_tcp_port),
            "UHF_TCP_HOST": "0.0.0.0",
            "UHF_TCP_PORT": str(self.uhf_tcp_port),
            "UHF_SOUTHBOUND_MODE": self.uhf_southbound_mode,
            "UHF_BAUDRATE": "115200",
            "COMM_NODE_INGRESS_DIAGNOSTICS": os.environ.get("COMM_NODE_INGRESS_DIAGNOSTICS", "0"),
            "COMM_NODE_STRIP_TC_FILL_PATTERN": (
                os.environ.get("COMM_NODE_STRIP_TC_FILL_PATTERN", "1")
                if self.uhf_southbound_mode == "serial-pty"
                else os.environ.get("COMM_NODE_STRIP_TC_FILL_PATTERN", "0")
            ),
            "KILL_EXISTING_PIDS": "1",
        }
        self._start_remote(
            "subsystem-stack",
            RemoteStackConfig(
                ssh_target=self.subsystem_target,
                remote_dir=self.subsystem_remote_dir,
                command="scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh",
                env=subsystem_env,
            ),
            self.subsystem_log,
        )
        wait_host_port(self.sband_tcp_host, self.sband_tcp_port, 20.0)
        if self.uhf_southbound_mode == "tcp-listen":
            wait_host_port(self.uhf_tcp_host, self.uhf_tcp_port, 20.0)
        else:
            self.require_remote_uhf_bridge_serial()
        subsystem_settle_sec = float(os.environ.get("TARGET_TCP_SUBSYSTEM_SETTLE_SEC", "3.0"))
        self.note(f"subsystem-startup-settle-sleep={subsystem_settle_sec:g}s")
        time.sleep(subsystem_settle_sec)
        comm_subsystem_ping_timeout_ms = os.environ.get("COMM_SUBSYSTEM_PING_TIMEOUT_MS", "1000")
        comm_primary_unavailable_failure_threshold = os.environ.get(
            "COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD", "10"
        )
        obc_env = {
            "CSP_TRANSPORT": "zmqhub",
            "CSP_HUB_HOST": self.remote_host,
            "CSP_HUB_SUB_PORT": str(self.csp_sub_port),
            "CSP_HUB_PUB_PORT": str(self.csp_pub_port),
            "GROUND_LINK_MODE": "comm-csp",
            "COMM_CSP_NODE": "5",
            "COMMAND_AUTHORITY_PROFILE": "sband-primary",
            "COMMAND_AUTH_MODE": "hmac-sha256",
            "COMMAND_AUTH_SOURCE_ID": str(SBAND_SOURCE_ID),
            "COMMAND_AUTH_KEY_SLOT": str(SBAND_KEY_SLOT),
            "COMMAND_AUTH_KEY_HEX": SBAND_KEY_BYTES.hex(),
            "INITIAL_COMM_BAND": "sband",
            "ENABLE_PRIMARY_GROUND_LINK_DRIVER": enable_primary_ground_link_driver,
            "ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR": enable_comm_subsystem_health_detector,
            "COMM_SUBSYSTEM_PING_TIMEOUT_MS": comm_subsystem_ping_timeout_ms,
            "COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD": comm_primary_unavailable_failure_threshold,
            "MANAGE_RADIO_MOCK_SERVER": manage_radio_mock_server,
            "DIAGNOSTIC_QUIET_PACKET_EGRESS": os.environ.get("DIAGNOSTIC_QUIET_PACKET_EGRESS", "0"),
            "RADIO_PROTOCOL": radio_protocol,
            "GDS_HOST": "127.0.0.1",
            "GDS_PORT": "0",
            "RUNTIME_ROOT": self.remote_runtime_root,
            "PERSISTENT_ROOT": f"{self.remote_runtime_root}/persistent-data",
            "STAGING_ROOT": f"{self.remote_runtime_root}/staging",
            "KILL_EXISTING_PIDS": "1",
        }
        self.note(
            "obc-env-overrides "
            f"ENABLE_PRIMARY_GROUND_LINK_DRIVER={enable_primary_ground_link_driver} "
            f"ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR={enable_comm_subsystem_health_detector} "
            f"COMM_SUBSYSTEM_PING_TIMEOUT_MS={comm_subsystem_ping_timeout_ms} "
            f"COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD={comm_primary_unavailable_failure_threshold} "
            f"MANAGE_RADIO_MOCK_SERVER={manage_radio_mock_server} "
            f"DIAGNOSTIC_QUIET_PACKET_EGRESS={obc_env['DIAGNOSTIC_QUIET_PACKET_EGRESS']} "
            f"RADIO_PROTOCOL={radio_protocol}"
        )
        self.obc_process = self._start_remote(
            "obc-stack",
            RemoteStackConfig(
                ssh_target=self.obc_target,
                remote_dir=self.obc_remote_dir,
                command="scripts/comm_verification/lib/run_target_tcp_obc_stack.sh",
                env=obc_env,
            ),
            self.obc_log,
            interactive=True,
        )
        node5_ready_timeout = float(os.environ.get("TARGET_TCP_NODE5_READY_TIMEOUT_SEC", "30"))
        startup_fragments = ("OBC CCSDS S-band runtime started.",)
        if self.primary_ground_link_driver_enabled():
            startup_fragments += ("groundLinkDriver) GROUND_LINK_UP",)
        self.wait_for_obc(
            startup_fragments,
            node5_ready_timeout,
            "target TCP runtime startup",
        )

    def start(self) -> None:
        last_error: ProbeFailure | None = None
        startup_attempts = int(os.environ.get("TARGET_TCP_STARTUP_ATTEMPTS", "2"))
        for attempt in range(1, startup_attempts + 1):
            self.note(f"startup-attempt={attempt}")
            try:
                self._start_once()
                return
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"startup-attempt={attempt} failed: {exc}")
                self.force_stop()
                time.sleep(2.0)
        if last_error is None:
            raise ProbeFailure("target TCP startup failed without a recorded error")
        raise last_error

    def wait_for_obc(self, fragments: tuple[str, ...], timeout: float, label: str, start: int | None = None) -> None:
        deadline = time.time() + timeout
        start_index = 0 if start is None else start
        last = ""
        while time.time() < deadline:
            lines = read_text(self.obc_log).splitlines()
            last = "\n".join(lines[start_index:])
            if all(fragment in last for fragment in fragments):
                return
            time.sleep(0.2)
        raise ProbeFailure(f"timed out waiting for {label}; last={last[-4000:]}")

    def obc_line_count(self) -> int:
        return len(read_text(self.obc_log).splitlines())

    def send_obc_shell(self, command: str, fragment: str, timeout: float) -> None:
        if self.obc_process is None or self.obc_process.stdin is None:
            raise ProbeFailure("interactive OBC shell is unavailable")
        start = self.obc_line_count()
        self.obc_process.stdin.write(command + "\n")
        self.obc_process.stdin.flush()
        self.wait_for_obc((fragment,), timeout, command, start=start)

    def send_obc_shell_with_retries(self, command: str, fragment: str, timeout: float, attempts: int = 4, delay: float = 0.8) -> None:
        last_error: ProbeFailure | None = None
        for attempt in range(1, attempts + 1):
            try:
                self.send_obc_shell(command, fragment, timeout)
                return
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"shell-retry command={command} attempt={attempt} error={exc}")
                time.sleep(delay)
        if last_error is None:
            raise ProbeFailure(f"OBC shell command {command!r} failed without a recorded error")
        raise last_error

    def wait_for_auto_uhf_primary(self, timeout: float = 25.0) -> str:
        obc_start = max(0, self.obc_line_count() - 400)
        ground_start = max(0, self.uhf.event_count() - 200) if self.uhf.events_log.exists() else 0
        deadline = time.time() + timeout
        last_events = ""
        last_obc = ""
        fragments = (
            "COMM_PRIMARY_LINK_CHANGED",
            "command UHF",
            "telemetry UHF",
            "file UHF",
        )
        while time.time() < deadline:
            if self.uhf.events_log.exists():
                event_lines = read_text(self.uhf.events_log).splitlines()
                last_events = "\n".join(event_lines[ground_start:])
                if all(fragment in last_events for fragment in fragments):
                    self.note("auto-failover-to-uhf-primary-observed-via-ground-events")
                    return "ground-events-auto-failover"
            obc_lines = read_text(self.obc_log).splitlines()
            last_obc = "\n".join(obc_lines[obc_start:])
            if all(fragment in last_obc for fragment in fragments):
                self.note("auto-failover-to-uhf-primary-observed-via-obc-log")
                return "obc-log-auto-failover"
            time.sleep(0.2)
        raise ProbeFailure(
            "timed out waiting for auto-failover to UHF primary;"
            f" ground_tail={last_events[-4000:]}\nobc_tail={last_obc[-4000:]}"
        )

    def start_sband_ground(self) -> None:
        self.sband.start_gds()
        self.sband.start_gateway_tcp(
            self.gateway_bin,
            self.sband_tcp_host,
            self.sband_tcp_port,
            "sband",
            capture_dir=self.sband.root / "gateway-captures",
        )

    def start_uhf_ground(self) -> None:
        self.uhf.start_gds()
        if self.uhf_southbound_mode == "serial-pty":
            local_pty = self.start_local_uhf_serial_bridge(self.require_remote_uhf_bridge_serial())
            preamble_lines = int(os.environ.get("TARGET_TCP_UHF_SERIAL_PREAMBLE_LINES", "0"))
            preamble_delay_ms = int(os.environ.get("TARGET_TCP_UHF_SERIAL_PREAMBLE_DELAY_MS", "0"))
            self.note(
                f"uhf-ground-serial-pty-preamble lines={preamble_lines} delay-ms={preamble_delay_ms}"
            )
            self.uhf.start_gateway_serial(
                self.gateway_bin,
                local_pty,
                115200,
                capture_dir=self.uhf.root / "gateway-captures",
                preamble_lines=preamble_lines,
                preamble_delay_ms=preamble_delay_ms,
            )
        else:
            self.uhf.start_gateway_tcp(
                self.gateway_bin,
                self.uhf_tcp_host,
                self.uhf_tcp_port,
                "uhf",
                capture_dir=self.uhf.root / "gateway-captures",
            )

    def stop_sband_ground(self) -> None:
        self.note("stopping-local-sband-ground")
        self.sband.stop()

    def rearm_uhf_ground(self, uhf_profile: AuthProfile, session_minimum: int = 0x62000000) -> tuple[int, str]:
        restart_ground = os.environ.get("TARGET_TCP_RESTART_UHF_GROUND_ON_REARM", "0") == "1"
        if restart_ground:
            self.note("rearming-local-uhf-ground")
            self.uhf.stop()
            time.sleep(1.0)
            self.start_uhf_ground()
        else:
            self.note("rearming-local-uhf-session-only")
        self.post_uhf_session_settle()
        return self.open_session(self.uhf, uhf_profile, 1, session_minimum, "identity 2 role 3")

    def require_remote_uhf_bridge_serial(self) -> str:
        if self.remote_uhf_bridge_serial:
            return self.remote_uhf_bridge_serial
        deadline = time.time() + 20.0
        marker = "UHF_BRIDGE_SERIAL="
        last = ""
        while time.time() < deadline:
            text = read_text(self.subsystem_log)
            last = text
            for line in text.splitlines():
                if line.startswith(marker):
                    self.remote_uhf_bridge_serial = line.split("=", 1)[1].strip()
                    return self.remote_uhf_bridge_serial
            time.sleep(0.2)
        raise ProbeFailure(f"timed out waiting for remote UHF bridge serial path; last={last[-2000:]}")

    def start_local_uhf_serial_bridge(self, remote_pty: str) -> str:
        if not self.bridge_helper_path.exists():
            raise ProbeFailure(f"UHF serial bridge helper is missing: {self.bridge_helper_path}")
        pattern = rf"{self.bridge_helper_path.name} --ssh-target {shq(self.subsystem_target)} --remote-pty {shq(remote_pty)}"
        reap_local_process_pattern(pattern)
        bridge_log = self.uhf.root / "pty-ssh-pty-bridge.log"
        process = self._start_local(
            "uhf_serial_pty_bridge",
            [
                sys.executable,
                str(self.bridge_helper_path),
                "--ssh-target",
                self.subsystem_target,
                "--remote-pty",
                remote_pty,
            ],
            bridge_log,
        )
        deadline = time.time() + 10.0
        marker = "LOCAL_PTY="
        while time.time() < deadline:
            text = read_text(bridge_log)
            for line in text.splitlines():
                if line.startswith(marker):
                    local_pty = line.split("=", 1)[1].strip()
                    if process.poll() is not None:
                        raise ProbeFailure(f"local UHF serial bridge exited early rc={process.returncode}")
                    return local_pty
            if process.poll() is not None:
                raise ProbeFailure(f"local UHF serial bridge exited early rc={process.returncode}")
            time.sleep(0.1)
        raise ProbeFailure(f"timed out waiting for local UHF serial bridge readiness marker {marker!r}")

    def post_uhf_session_settle(self) -> None:
        settle_seconds = float(os.environ.get("TARGET_TCP_POST_UHF_SESSION_SETTLE_SEC", "0"))
        if settle_seconds <= 0:
            return
        self.note(f"post-uhf-session-settle-sleep={settle_seconds:g}s")
        time.sleep(settle_seconds)

    def reserve_sequence_numbers(self, ground: GroundPath, session_id: int, *, count: int = 1, minimum: int | None = None) -> int:
        key = (ground.name, session_id)
        next_sequence = self.next_session_sequences.get(key, 1)
        if minimum is not None:
            next_sequence = max(next_sequence, minimum)
        base = next_sequence
        self.next_session_sequences[key] = next_sequence + count
        return base

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
        if ground is self.uhf and self.uhf_southbound_mode == "serial-pty":
            self.note(f"{ground.name}-serial-pty-skip-link-window")
            return
        try:
            ground.await_link_up_if_needed(timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-link-up-not-observed-within-{timeout:g}s")
        try:
            ground.wait_link_quiet(timeout=timeout)
        except ProbeFailure:
            self.note(f"{ground.name}-quiet-window-not-observed-within-{timeout:g}s")

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
            event_lines = read_text(ground.events_log).splitlines()
            last_events = "\n".join(event_lines[ground_start:])
            if all(fragment in last_events for fragment in ground_fragments):
                return "ground-events"
            obc_lines = read_text(self.obc_log).splitlines()
            last_obc = "\n".join(obc_lines[obc_start:])
            if all(fragment in last_obc for fragment in obc_fragments):
                return "obc-log"
            time.sleep(0.2)
        raise ProbeFailure(
            f"timed out waiting for {label}; ground_fragments={ground_fragments} obc_fragments={obc_fragments}\n"
            f"ground_tail={last_events[-4000:]}\nobc_tail={last_obc[-4000:]}"
        )

    def send_file_packet(self, ground: GroundPath, label: str, packet: StartPacketData | DataPacketData | EndPacketData) -> None:
        payload = self.file_encoder.encode_api(packet)
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: packet_type={packet.packetType.name} seq={packet.seqID} encoded={payload.hex()}\n"
            )
        with socket.create_connection(("127.0.0.1", ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)

    def send_file_packet_via_pipeline(self, ground: GroundPath, label: str, packet: StartPacketData | DataPacketData | EndPacketData) -> None:
        payload = self.file_encoder.encode_api(packet)
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{label}: sender=pipeline packet_type={packet.packetType.name} seq={packet.seqID} encoded={payload.hex()}\n"
            )
        ground.send_file_packet_via_pipeline(packet)

    def open_session(self, ground: GroundPath, profile: AuthProfile, ingress_port: int, session_minimum: int, role_fragment: str) -> tuple[int, str]:
        identity, role = authority_identity_role(profile.authority_profile)
        persisted_floor = read_persisted_session_floor(self.obc_target, self.remote_runtime_root, ingress_port, identity, role)
        last_error: ProbeFailure | None = None
        for attempt in range(6):
            session_id = next_session_id(session_minimum + attempt, persisted_floor)
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            ground_start = ground.event_count()
            obc_start = self.obc_line_count()
            self.send_envelope(
                ground,
                f"{ground.name}-session-open-attempt-{attempt + 1}",
                profile,
                session_id,
                0,
                "OBCApp.commandIngressAuthority.SESSION_OPEN",
            )
            try:
                source = self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    ("COMMAND_SESSION_OPENED", role_fragment, f"session {session_id}"),
                    ("COMMAND_SESSION_OPENED", role_fragment, f"session {session_id}"),
                    18.0,
                    f"{ground.name} session open",
                )
                self.last_session_sources[ground.name] = source
                self.active_sessions[ground.name] = session_id
                self.next_session_sequences[(ground.name, session_id)] = 1
                return session_id, source
            except ProbeFailure as exc:
                last_error = exc
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: session open failed without a recorded error")
        raise last_error

    def switch_to_uhf_primary(self, sband_session_id: int) -> str:
        profile = self.authority_profile("sband-primary")
        last_error: ProbeFailure | None = None
        sequence_base = self.reserve_sequence_numbers(self.sband, sband_session_id, count=6)
        for attempt in range(6):
            self.prepare_ground_window(self.sband)
            self.prepare_ground_window(self.uhf)
            ground_start = self.uhf.event_count()
            obc_start = self.obc_line_count()
            self.send_envelope(
                self.sband,
                f"switch-to-uhf-primary-attempt-{attempt + 1}",
                profile,
                sband_session_id,
                sequence_base + attempt,
                "OBCApp.commController.COMM_SET_ACTIVE",
                "UHF",
            )
            try:
                source = self.wait_ground_or_obc(
                    self.uhf,
                    ground_start,
                    obc_start,
                    ("COMM_PRIMARY_LINK_CHANGED", "command UHF", "telemetry UHF", "file UHF", "reason 1"),
                    ("COMM_PRIMARY_LINK_CHANGED", "command UHF", "telemetry UHF", "file UHF", "reason 1"),
                    20.0,
                    "switch to UHF primary",
                )
                self.prepare_ground_window(self.uhf)
                self.last_switch_source = source
                return source
            except ProbeFailure as exc:
                last_error = exc
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure("UHF primary switch failed without a recorded error")
        raise last_error

    def run_command_roundtrip(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        session_id: int,
        sequence_number: int,
        label: str,
        *,
        require_channel_readback: bool = True,
    ) -> str:
        opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        last_error: ProbeFailure | None = None
        for attempt in range(4):
            effective_sequence = sequence_number + attempt
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            ground_start = ground.event_count()
            obc_start = self.obc_line_count()
            self.send_envelope(
                ground,
                f"{label}-attempt-{attempt + 1}",
                profile,
                session_id,
                effective_sequence,
                "OBCApp.epsBridge.EPS_GET_STATUS",
            )
            try:
                source = self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    (f"inner opcode 0x{opcode:x}",),
                    (f"inner opcode 0x{opcode:x}",),
                    20.0,
                    f"{label} envelope observation",
                )
                if require_channel_readback:
                    eps_text = ground.channel_search("eps-soc", "OBCApp.epsBridge.EPS_SOC")
                    if "OBCApp.epsBridge.EPS_SOC" not in eps_text:
                        raise ProbeFailure(f"{ground.name}: EPS_SOC was not visible through ground path")
                return source
            except ProbeFailure as exc:
                last_error = exc
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: command roundtrip failed without a recorded error")
        raise last_error

    def maybe_run_pre_switch_sband_command(self, sband_profile: AuthProfile, sband_session_id: int) -> str | None:
        if os.environ.get("TARGET_TCP_PRE_SWITCH_SBAND_COMMAND", "0") != "1":
            return None
        sequence_base = self.reserve_sequence_numbers(self.sband, sband_session_id, minimum=90)
        source = self.run_command_roundtrip(
            self.sband,
            sband_profile,
            sband_session_id,
            sequence_base,
            "target-tcp-pre-switch-sband-command",
            require_channel_readback=False,
        )
        self.note(f"pre-switch-sband-command-source={source}")
        return source

    def write_upload_audit(
        self,
        ground: GroundPath,
        artifact: SequenceArtifact,
        mode: str,
        packet_sequence_numbers: tuple[int, int, int],
        *,
        advertised_source_path: str | None = None,
    ) -> pathlib.Path:
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
            advertised_source_path=advertised_source_path,
        )

    def manual_sequence_start_source_path(self, artifact: SequenceArtifact) -> str:
        if self.profile == "uhf-primary" and self.mode == "sequence-subsystem":
            normalize_default = "1"
        else:
            normalize_default = "0"
        if os.environ.get("NORMALIZE_SEQUENCE_START_SOURCE", "0") == "1":
            return artifact.binary.name
        if os.environ.get("NORMALIZE_SEQUENCE_START_SOURCE", normalize_default) == "1":
            return artifact.binary.name
        return str(artifact.binary)

    def manual_sequence_packet_delay_sec(self) -> float:
        configured = os.environ.get("TARGET_TCP_MANUAL_SEQUENCE_PACKET_DELAY_SEC")
        if configured is not None:
            return max(0.0, float(configured))
        if self.profile == "uhf-primary" and self.mode == "sequence-subsystem" and self.uhf_southbound_mode == "serial-pty":
            return 0.5
        return 0.0

    def list_remote_data_product_files(self) -> list[str]:
        script = (
            f"set -euo pipefail; "
            f"if [[ -d {shq(self.remote_runtime_root + '/data-products')} ]]; then "
            f"find {shq(self.remote_runtime_root + '/data-products')} -maxdepth 1 -type f -name 'Dp_*.fdp' | sort; "
            f"fi"
        )
        text = ssh_capture(self.obc_target, script)
        return [line.strip() for line in text.splitlines() if line.strip()]

    def reset_remote_data_products(self) -> None:
        data_products_root = self.remote_runtime_root + "/data-products"
        ssh_capture(
            self.obc_target,
            (
                f"set -euo pipefail; mkdir -p {shq(data_products_root)}; "
                f"find {shq(data_products_root)} -maxdepth 1 -type f "
                "\\( -name 'Dp_*.fdp' -o -name 'DpState.dat' \\) -delete"
            ),
        )

    def wait_for_remote_data_product_files(self, timeout_sec: int) -> list[str]:
        for _ in range(max(1, int(timeout_sec))):
            files = self.list_remote_data_product_files()
            if files:
                return files
            time.sleep(1.0)
        raise ProbeFailure("No official remote .fdp files appeared under target TCP runtime")

    def fetch_remote_file(self, remote_path: str, snapshot_name: str) -> pathlib.Path:
        snapshot_dir = self.probe_root / "source-snapshots"
        snapshot_dir.mkdir(parents=True, exist_ok=True)
        snapshot_path = snapshot_dir / snapshot_name
        result = subprocess.run(
            ssh_command(self.obc_target, f"set -euo pipefail; cat {shq(remote_path)}"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode != 0:
            stderr = result.stderr.decode("utf-8", errors="replace").strip()
            raise ProbeFailure(f"failed to fetch remote source file {remote_path}: {stderr}")
        snapshot_path.write_bytes(result.stdout)
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

    def wait_for_any_matching_file(self, ground: GroundPath, expected_snapshots: list[dict[str, pathlib.Path | str]], timeout_sec: int) -> tuple[dict[str, pathlib.Path | str], pathlib.Path]:
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
                        return entry, received_path
            time.sleep(0.5)
        raise ProbeFailure("timed out waiting for a byte-matching remote .fdp downlink")

    def wait_for_sending_product_source_path(self, obc_start: int, timeout: float) -> str:
        deadline = time.time() + timeout
        last_obc = ""
        while time.time() < deadline:
            lines = read_text(self.obc_log).splitlines()
            last_obc = "\n".join(lines[obc_start:])
            matches = re.findall(r"Sending product (\S+?\.fdp) of size \d+ priority \d+", last_obc)
            if matches:
                return matches[-1]
            time.sleep(0.2)
        raise ProbeFailure(
            "timed out waiting for SendingProduct source path in OBC log\n"
            f"obc_tail={last_obc[-4000:]}"
        )

    def send_command_and_wait(
        self,
        ground: GroundPath,
        profile: AuthProfile,
        session_id: int,
        sequence_number: int,
        command_name: str,
        args: tuple[str, ...],
        fragments: tuple[str, ...],
        timeout: float,
        label: str,
    ) -> str:
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        ground_start = ground.event_count()
        obc_start = self.obc_line_count()
        self.send_envelope(ground, label, profile, session_id, sequence_number, command_name, *args)
        return self.wait_ground_or_obc(ground, ground_start, obc_start, fragments, fragments, timeout, label)

    def run_start_xmit_catalog(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_number: int) -> str:
        errors = []
        for argument in ("1", "NO_WAIT", "Fw.Wait.NO_WAIT"):
            try:
                self.prepare_ground_window(ground)
                time.sleep(0.3)
                ground_start = ground.event_count()
                obc_start = self.obc_line_count()
                self.send_envelope(
                    ground,
                    f"{ground.name} START_XMIT_CATALOG {argument}",
                    profile,
                    session_id,
                    sequence_number,
                    "OBCApp.dpCatalog.START_XMIT_CATALOG",
                    argument,
                )
                self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    ("SendingProduct",),
                    ("SendingProduct",),
                    25.0,
                    f"{ground.name} START_XMIT_CATALOG {argument}",
                )
                return self.wait_for_sending_product_source_path(obc_start, 5.0)
            except ProbeFailure as exc:
                errors.append(str(exc))
        raise ProbeFailure("START_XMIT_CATALOG failed with all enum argument forms: " + " | ".join(errors))

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
                    25.0,
                    f"{ground.name} BUILD_CATALOG attempt {attempt}",
                )
                selected_source_path = self.run_start_xmit_catalog(ground, profile, session_id, sequence_base + (attempt * 2) - 1)
                self.checkpoint(
                    f"{ground.name}-file-source-visible",
                    "pass",
                    attempt=attempt,
                    source_count=1,
                    sources=[selected_source_path],
                )
                expected_snapshots = self.snapshot_remote_source_files([selected_source_path], f"{label_prefix}-attempt-{attempt}")
                matched_entry, matched_received_path = self.wait_for_any_matching_file(ground, expected_snapshots, 120)
                return str(matched_entry["source_path"]), str(matched_received_path), matched_received_path.stat().st_size
            except ProbeFailure as exc:
                last_error = exc
                self.remove_received_fdp_files(ground)
                time.sleep(2.0)
        if last_error is None:
            raise ProbeFailure("file downlink failed without a recorded error")
        raise last_error

    def build_sequence_artifact(self, name: str) -> SequenceArtifact:
        self.sequence_src_dir.mkdir(parents=True, exist_ok=True)
        self.sequence_bin_dir.mkdir(parents=True, exist_ok=True)
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

    def upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> None:
        upload_mode = os.environ.get("TARGET_TCP_SEQUENCE_UPLOAD_MODE", "api-then-manual")
        if (
            ground is self.uhf
            and self.profile == "uhf-primary"
            and os.environ.get("TARGET_TCP_SKIP_API_SEQUENCE_UPLOAD", "0") == "1"
        ):
            self.note(f"{ground.name}-api-upload-skipped-via-env")
            self.manual_upload_sequence(ground, artifact)
            return
        last_error: ProbeFailure | None = None
        for attempt in range(2):
            audit_path = self.write_upload_audit(ground, artifact, mode=f"api-attempt-{attempt + 1}", packet_sequence_numbers=(0, 1, 2))
            self.note(f"{ground.name}-upload-attempt-{attempt + 1}-audit {audit_path}")
            self.prepare_ground_window(ground)
            time.sleep(0.3)
            obc_start = self.obc_line_count()
            ground_start = ground.upload_file(artifact.binary, artifact.destination)
            try:
                self.wait_ground_or_obc(
                    ground,
                    ground_start,
                    obc_start,
                    ("FileReceived",),
                    ("FileReceived",),
                    90.0,
                    "sequence file received",
                )
                return
            except ProbeFailure as exc:
                last_error = exc
                time.sleep(1.0)
        if upload_mode == "api-only":
            if last_error is None:
                raise ProbeFailure(f"{ground.name}: api-only sequence upload failed without a recorded error")
            raise last_error
        if ground is self.uhf and self.profile == "uhf-primary":
            self.note(f"{ground.name}-api-upload-failed-falling-back-to-manual")
            self.manual_upload_sequence(ground, artifact)
            return
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: sequence upload failed without a recorded error")
        raise last_error

    def manual_upload_sequence(self, ground: GroundPath, artifact: SequenceArtifact) -> None:
        ground.connect_api()
        payload = artifact.binary.read_bytes()
        checksum = CFDPChecksum()
        checksum.update(payload, 0)
        last_error: ProbeFailure | None = None
        sender_order = os.environ.get("TARGET_TCP_MANUAL_SEQUENCE_SENDER_ORDER", "")
        if sender_order:
            sender_plan = tuple(part.strip() for part in sender_order.split(",") if part.strip())
        elif (
            ground is self.uhf
            and self.profile == "uhf-primary"
            and os.environ.get("TARGET_TCP_SKIP_API_SEQUENCE_UPLOAD", "0") == "1"
        ):
            sender_plan = ("raw-socket", "pipeline")
        else:
            sender_plan = ("pipeline", "raw-socket")
        for attempt, sender_mode in enumerate(sender_plan, start=1):
            self.note(f"{ground.name}-manual-upload-attempt-{attempt}-begin path={artifact.destination} sender={sender_mode}")
            start_source_path = self.manual_sequence_start_source_path(artifact)
            audit_path = self.write_upload_audit(
                ground,
                artifact,
                mode=f"manual-attempt-{attempt}-{sender_mode}",
                packet_sequence_numbers=(0, 1, 2),
                advertised_source_path=start_source_path,
            )
            self.note(f"{ground.name}-manual-upload-attempt-{attempt}-audit {audit_path}")
            if start_source_path != str(artifact.binary):
                self.note(f"{ground.name}-manual-upload-attempt-{attempt}-advertised-source {start_source_path}")
            self.prepare_ground_window(ground, timeout=12.0)
            time.sleep(0.3)
            start_ground = ground.event_count()
            start_obc = self.obc_line_count()
            start_packet = StartPacketData(0, len(payload), start_source_path, artifact.destination)
            if sender_mode == "pipeline":
                self.send_file_packet_via_pipeline(
                    ground,
                    f"{ground.name}-manual-start-attempt-{attempt}",
                    start_packet,
                )
            else:
                self.send_file_packet(
                    ground,
                    f"{ground.name}-manual-start-attempt-{attempt}",
                    start_packet,
                )
            try:
                self.wait_ground_or_obc(
                    ground,
                    start_ground,
                    start_obc,
                    ("FILE_INGRESS_START_ACCEPTED",),
                    ("FILE_INGRESS_START_ACCEPTED",),
                    20.0,
                    "sequence ingress start accepted",
                )
                self.note(f"{ground.name}-manual-upload-attempt-{attempt}-start-accepted sender={sender_mode}")
                self.prepare_ground_window(ground, timeout=12.0)
                packet_delay = self.manual_sequence_packet_delay_sec()
                if packet_delay > 0.0:
                    self.note(f"{ground.name}-manual-upload-attempt-{attempt}-packet-delay={packet_delay:g}s")
                    time.sleep(packet_delay)
                data_packet = DataPacketData(1, 0, payload)
                if sender_mode == "pipeline":
                    self.send_file_packet_via_pipeline(
                        ground,
                        f"{ground.name}-manual-data-attempt-{attempt}",
                        data_packet,
                    )
                else:
                    self.send_file_packet(
                        ground,
                        f"{ground.name}-manual-data-attempt-{attempt}",
                        data_packet,
                    )
                self.prepare_ground_window(ground, timeout=12.0)
                if packet_delay > 0.0:
                    time.sleep(packet_delay)
                end_packet = EndPacketData(2, checksum.value)
                if sender_mode == "pipeline":
                    self.send_file_packet_via_pipeline(
                        ground,
                        f"{ground.name}-manual-end-attempt-{attempt}",
                        end_packet,
                    )
                else:
                    self.send_file_packet(
                        ground,
                        f"{ground.name}-manual-end-attempt-{attempt}",
                        end_packet,
                    )
                self.wait_ground_or_obc(
                    ground,
                    start_ground,
                    start_obc,
                    ("FileReceived",),
                    ("FileReceived",),
                    60.0,
                    "sequence file received",
                )
                self.note(f"{ground.name}-manual-upload-attempt-{attempt}-pass path={artifact.destination} sender={sender_mode}")
                return
            except ProbeFailure as exc:
                last_error = exc
                self.note(f"{ground.name}-manual-upload-attempt-{attempt}-retry sender={sender_mode} {exc}")
                time.sleep(1.0)
        if last_error is None:
            raise ProbeFailure(f"{ground.name}: manual sequence upload failed without a recorded error")
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
        obc_fragments: tuple[str, ...],
        timeout: float,
        label: str,
    ) -> str:
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        ground_start = ground.event_count()
        obc_start = self.obc_line_count()
        self.send_envelope(ground, label, profile, session_id, sequence_number, command_name, *args)
        return self.wait_ground_or_obc(ground, ground_start, obc_start, ground_fragments, obc_fragments, timeout, label)

    def run_sequence_roundtrip(self, ground: GroundPath, profile: AuthProfile, session_id: int, sequence_base: int, artifact_name: str) -> None:
        artifact = self.build_sequence_artifact(artifact_name)
        self.upload_sequence(ground, artifact)
        validate_opcode = self.opcodes["OBCApp.sequenceAdmissionController.SEQ_VALIDATE"]
        validate_session_id = session_id
        validate_sequence_number = sequence_base
        last_validate_error: ProbeFailure | None = None
        validate_attempts = 4 if ground is self.uhf and self.uhf_southbound_mode == "serial-pty" else 1
        for attempt in range(validate_attempts):
            effective_sequence = self.reserve_sequence_numbers(
                ground,
                validate_session_id,
                minimum=validate_sequence_number,
            )
            try:
                self.send_command_and_wait(
                    ground,
                    profile,
                    validate_session_id,
                    effective_sequence,
                    "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
                    (artifact.destination,),
                    (
                        "COMMAND_ENVELOPE_OBSERVED",
                        f"session {validate_session_id}",
                        f"sequence {effective_sequence}",
                        f"inner opcode 0x{validate_opcode:x}",
                    ),
                    (
                        "COMMAND_ENVELOPE_OBSERVED",
                        f"session {validate_session_id}",
                        f"sequence {effective_sequence}",
                        f"inner opcode 0x{validate_opcode:x}",
                    ),
                    18.0,
                    f"{ground.name} sequence validate",
                )
                validate_sequence_number = effective_sequence
                last_validate_error = None
                break
            except ProbeFailure as exc:
                last_validate_error = exc
                self.note(
                    f"{ground.name}-sequence-validate-retry-{attempt + 1} "
                    f"session={validate_session_id} seq={effective_sequence} error={exc}"
                )
                if attempt >= validate_attempts - 1:
                    break
                if ground is self.uhf and self.uhf_southbound_mode == "serial-pty":
                    validate_session_id, rearm_source = self.rearm_uhf_ground(
                        profile,
                        session_minimum=max(0x62000000, validate_session_id + 1),
                    )
                    validate_sequence_number = 1
                    self.note(
                        f"{ground.name}-sequence-validate-session-rearmed "
                        f"source={rearm_source} session={validate_session_id}"
                    )
                time.sleep(1.0)
        if last_validate_error is not None:
            raise last_validate_error
        ground.assert_no_event("SEQUENCE_CONTROL_REJECTED", timeout=2.0, start=0)
        ground.assert_no_event("COMMAND_AUTHORITY_REJECTED", timeout=2.0, start=0)
        self.prepare_ground_window(ground)
        time.sleep(0.3)
        run_ground_start = ground.event_count()
        run_obc_start = self.obc_line_count()
        self.send_envelope(
            ground,
            f"{ground.name} sequence run",
            profile,
            validate_session_id,
            self.reserve_sequence_numbers(ground, validate_session_id, minimum=validate_sequence_number + 1),
            "OBCApp.sequenceAdmissionController.SEQ_RUN",
            artifact.destination,
            "WAIT",
        )
        self.wait_ground_or_obc(
            ground,
            run_ground_start,
            run_obc_start,
            ("CS_SequenceComplete",),
            ("CS_SequenceComplete",),
            50.0,
            f"{ground.name} sequence run",
        )
        eps_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
        adcs_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
        self.wait_ground_or_obc(
            ground,
            run_ground_start,
            run_obc_start,
            (f"Opcode 0x{eps_opcode:x} completed",),
            (f"Opcode 0x{eps_opcode:x} completed",),
            8.0,
            f"{ground.name} EPS_GET_STATUS completion",
        )
        self.wait_ground_or_obc(
            ground,
            run_ground_start,
            run_obc_start,
            (f"Opcode 0x{adcs_opcode:x} completed",),
            (f"Opcode 0x{adcs_opcode:x} completed",),
            8.0,
            f"{ground.name} ADCS_GET_ATTITUDE completion",
        )
        ensure_no_legacy_aliases(self.root_dir)

    def stop_remote_sband_node(self) -> None:
        ssh_capture(
            self.subsystem_target,
            (
                "pkill -TERM -f "
                + shq(f"sband_comm_csp_node --tcp-listen-host 0.0.0.0 --tcp-listen-port {self.sband_tcp_port}")
                + " >/dev/null 2>&1 || true"
            ),
            check=False,
        )

    def expect_sband_command_failure(self, session_id: int) -> None:
        profile = self.authority_profile("sband-primary")
        self.prepare_ground_window(self.sband)
        ground_start = self.sband.event_count()
        obc_start = self.obc_line_count()
        self.send_envelope(
            self.sband,
            "expected-sband-failure",
            profile,
            session_id,
            90,
            "OBCApp.bootManager.GET_RESET_CAUSE",
        )
        deadline = time.time() + 10.0
        while time.time() < deadline:
            events_text = "\n".join(read_text(self.sband.events_log).splitlines()[ground_start:])
            obc_text = "\n".join(read_text(self.obc_log).splitlines()[obc_start:])
            if "BOOT_RECOVERY_STATUS" in events_text or "BOOT_RECOVERY_STATUS" in obc_text:
                raise ProbeFailure("S-band command unexpectedly still completed after node-5 loss")
            time.sleep(0.5)

    def run_failover(self) -> list[str]:
        self.start_sband_ground()
        self.start_uhf_ground()
        sband_profile = self.authority_profile("sband-primary")
        uhf_profile = self.authority_profile("uhf-primary")
        summary: list[str] = []
        sband_session_id, sband_session_source = self.open_session(self.sband, sband_profile, 0, 0x71000000, "identity 1 role 1")
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
        switch_source = self.switch_to_uhf_primary(sband_session_id)
        summary.append(f"switch-source={switch_source}")
        self.stop_remote_sband_node()
        time.sleep(1.0)
        self.expect_sband_command_failure(sband_session_id)
        summary.append("sband-post-loss=no-command-completion")
        uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x73000000, "identity 2 role 3")
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

    def run_reachability(self) -> list[str]:
        self.send_obc_shell("status", "mode=SAFE", 10.0)
        self.send_obc_shell_with_retries("csp ping 2", "csp ping response=0 success=yes", 10.0)
        self.send_obc_shell_with_retries("csp ping 3", "csp ping response=0 success=yes", 10.0)
        self.send_obc_shell_with_retries("csp ping 5", "csp ping response=0 success=yes", 10.0)
        self.send_obc_shell_with_retries("csp ping 6", "csp ping response=0 success=yes", 10.0)
        self.send_obc_shell_with_retries("eps get", "eps soc=", 10.0)
        self.send_obc_shell_with_retries("adcs get", "adcs mode=", 10.0)
        return ["reachable-nodes=2,3,5,6", "same-topology=macOS-hub+ground / obc.local / subsystem.local"]

    def run(self) -> list[str]:
        self.start()
        summary: list[str] = []
        try:
            if self.mode == "failover-command":
                summary.extend(self.run_failover())
                ensure_no_legacy_aliases(self.root_dir)
                return summary

            if self.mode == "reachability":
                summary.extend(self.run_reachability())
                ensure_no_legacy_aliases(self.root_dir)
                return summary

            prewarmed_uhf_ground = False
            if self.profile == "uhf-primary" and self.mode == "sequence-subsystem":
                if os.environ.get("TARGET_TCP_DELAY_UHF_GROUND_UNTIL_SWITCH", "0") == "1":
                    self.note("delayed-uhf-ground-start-until-post-switch")
                else:
                    self.start_uhf_ground()
                    if os.environ.get("TARGET_TCP_SKIP_UHF_API_PREWARM", "0") == "1":
                        self.note("skipped-uhf-ground-api-prewarm-via-env")
                    else:
                        self.uhf.connect_api()
                    prewarmed_uhf_ground = True
                    self.note("prewarmed-uhf-ground-api-for-sequence-ingress")

            if self.profile == "uhf-primary" and not self.primary_ground_link_driver_enabled():
                switch_source = self.wait_for_auto_uhf_primary()
                summary.append(f"switch-source={switch_source}")
                uhf_profile = self.authority_profile("uhf-primary")
                uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x62000000, "identity 2 role 3")
                summary.append(f"uhf-session-source={uhf_session_source}")
                if self.mode == "command":
                    command_source = self.run_command_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-tcp-uhf-command")
                    summary.append(f"uhf-command-source={command_source}")
                elif self.mode == "file-downlink":
                    source_path, received_path, received_size = self.run_file_downlink(self.uhf, uhf_profile, uhf_session_id, 1, "target-tcp-uhf-file")
                    summary.extend([f"source-path={source_path}", f"received-path={received_path}", f"received-size={received_size}"])
                elif self.mode == "sequence-subsystem":
                    self.run_sequence_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-can-uhf-roundtrip")
                    summary.append("same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on target TCP node-6 path")
                else:
                    raise ProbeFailure(f"unsupported mode/profile combination: {self.mode}/{self.profile}")
                ensure_no_legacy_aliases(self.root_dir)
                return summary

            self.start_sband_ground()
            sband_profile = self.authority_profile("sband-primary")
            sband_session_id, sband_session_source = self.open_session(self.sband, sband_profile, 0, 0x61000000, "identity 1 role 1")
            summary.append(f"sband-session-source={sband_session_source}")

            if self.profile == "sband":
                if self.mode == "command":
                    command_source = self.run_command_roundtrip(self.sband, sband_profile, sband_session_id, 1, "target-tcp-sband-command")
                    summary.append(f"sband-command-source={command_source}")
                elif self.mode == "file-downlink":
                    source_path, received_path, received_size = self.run_file_downlink(self.sband, sband_profile, sband_session_id, 1, "target-tcp-sband-file")
                    summary.extend([f"source-path={source_path}", f"received-path={received_path}", f"received-size={received_size}"])
                elif self.mode == "sequence-subsystem":
                    self.run_sequence_roundtrip(self.sband, sband_profile, sband_session_id, 1, "target-tcp-sband-roundtrip")
                    summary.append("same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on target TCP node-5 path")
                else:
                    raise ProbeFailure(f"unsupported mode/profile combination: {self.mode}/{self.profile}")
                ensure_no_legacy_aliases(self.root_dir)
                return summary

            if not prewarmed_uhf_ground:
                self.start_uhf_ground()
            self.maybe_run_pre_switch_sband_command(sband_profile, sband_session_id)
            switch_source = self.switch_to_uhf_primary(sband_session_id)
            summary.append(f"switch-source={switch_source}")
            uhf_profile = self.authority_profile("uhf-primary")
            uhf_session_id, uhf_session_source = self.open_session(self.uhf, uhf_profile, 1, 0x62000000, "identity 2 role 3")
            summary.append(f"uhf-session-source={uhf_session_source}")
            self.post_uhf_session_settle()
            if os.environ.get("TARGET_TCP_REARM_UHF_GROUND_AFTER_SESSION", "0") == "1":
                uhf_session_id, uhf_session_source = self.rearm_uhf_ground(
                    uhf_profile,
                    session_minimum=max(0x62000000, uhf_session_id + 1),
                )
                summary.append(f"uhf-session-rearmed-source={uhf_session_source}")
            if (
                self.mode == "sequence-subsystem"
                and os.environ.get("TARGET_TCP_STOP_SBAND_GROUND_AFTER_SWITCH", "0") == "1"
            ):
                self.stop_sband_ground()
            if self.mode == "command":
                command_source = self.run_command_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-tcp-uhf-command")
                summary.append(f"uhf-command-source={command_source}")
            elif self.mode == "file-downlink":
                source_path, received_path, received_size = self.run_file_downlink(self.uhf, uhf_profile, uhf_session_id, 1, "target-tcp-uhf-file")
                summary.extend([f"source-path={source_path}", f"received-path={received_path}", f"received-size={received_size}"])
            elif self.mode == "sequence-subsystem":
                self.run_sequence_roundtrip(self.uhf, uhf_profile, uhf_session_id, 1, "target-can-uhf-roundtrip")
                summary.append("same-path-proof=EPS_GET_STATUS and ADCS_GET_ATTITUDE completed on target TCP node-6 path")
            else:
                raise ProbeFailure(f"unsupported mode/profile combination: {self.mode}/{self.profile}")
            ensure_no_legacy_aliases(self.root_dir)
            return summary
        finally:
            self.sband.stop()
            self.uhf.stop()

    def force_stop(self) -> None:
        self.stop_remote_process_tree_by_command(
            self.subsystem_target,
            "build-fprime-automatic-native/bin/Linux/pty_pair_bridge",
        )
        self.stop_remote_process_tree_by_command(
            self.subsystem_target,
            "scripts/comm_verification/lib/run_target_tcp_subsystem_stack.sh",
        )
        self.stop_remote_process_tree_by_command(
            self.obc_target,
            "scripts/comm_verification/lib/run_target_tcp_obc_stack.sh",
        )
        self.sband.force_stop()
        self.uhf.force_stop()
        for managed in reversed(self.processes):
            process = managed.process
            handle = managed.handle
            if process.poll() is None:
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                deadline = time.time() + PROCESS_TERM_TIMEOUT
                while time.time() < deadline and process.poll() is None:
                    time.sleep(0.1)
                if process.poll() is None:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    deadline = time.time() + PROCESS_KILL_TIMEOUT
                    while time.time() < deadline and process.poll() is None:
                        time.sleep(0.1)
            try:
                handle.close()
            except Exception:
                pass
        self.reap_owned_local_processes()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("reachability", "command", "file-downlink", "sequence-subsystem", "failover-command"), required=True)
    parser.add_argument("--profile", choices=("sband", "uhf-primary"), required=True)
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    scenario = TargetTcpScenario(args.mode, args.profile, pathlib.Path(args.probe_root))
    install_signal_cleanup(scenario.force_stop)
    exit_code = 0
    output = ""
    try:
        summary_lines = [
            "target-tcp-matrix-probe: PASS",
            f"mode={args.mode}",
            f"profile={args.profile}",
            f"probe-root={args.probe_root}",
        ]
        summary_lines.extend(scenario.run())
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        output = f"target-tcp-matrix-probe: FAIL {exc}\n"
        exit_code = 1
    finally:
        scenario.force_stop()
        if exit_code == 0:
            sys.stdout.write(output)
        else:
            sys.stderr.write(output)
        sys.stdout.flush()
        sys.stderr.flush()
        os._exit(exit_code)


if __name__ == "__main__":
    raise SystemExit(main())
