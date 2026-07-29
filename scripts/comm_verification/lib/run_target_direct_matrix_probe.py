#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time
from dataclasses import dataclass

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.models.dictionaries import Dictionaries

from run_target_can_matrix_probe import (
    GroundPath,
    ManagedProcess,
    ProbeFailure,
    authority_identity_role,
    authenticated_envelope,
    command_opcode,
    ensure_no_legacy_aliases,
    free_port,
    read_text,
    reap_local_process_pattern,
    require_env,
    ssh_capture,
    ssh_command,
    shq,
)


PROCESS_TERM_TIMEOUT = 1.0
PROCESS_KILL_TIMEOUT = 1.0
DIRECT_SOURCE_ID = 3
DIRECT_KEY_SLOT = 3
DIRECT_KEY_BYTES = bytes.fromhex("505152535455565758595A5B5C5D5E5F")
DIRECT_GDS_FRAMING_SELECTION = "space-packet-space-data-link"
DIRECT_GDS_SCID = "68"
DIRECT_GDS_FRAME_SIZE = "1024"
COMMAND_DESCRIPTOR = 0x5A5A5A5A
FW_PACKET_COMMAND = 0x0000
DIRECT_SESSION_ID = 0x73000003
DIRECT_REMOTE_SERVICE_NAMES = ("obc-comm-csp-stack.service", "obc-installed-stack.service")


@dataclass(frozen=True)
class DirectCommandSpec:
    command_name: str
    cli_args: tuple[str, ...]
    label: str


@dataclass(frozen=True)
class DirectCommandAttempt:
    spec: DirectCommandSpec
    opcode: int
    cli_command: list[str]
    cli_returncode: int | None
    cli_output: list[str]
    plain_inner_hex: str
    plain_outer_hex: str
    envelope_outer_hex: str
    completion_observed: bool
    completion_source: str | None
    timeout: bool = False


@dataclass(frozen=True)
class DirectEnvelopeAttempt:
    spec: DirectCommandSpec
    session_id: int
    sequence_number: int
    payload_hex: str
    observed_envelope: bool
    observed_session_open: bool
    completion_observed: bool
    observation_source: str | None


def inner_command_packet(opcode: int, args: bytes = b"") -> bytes:
    return struct.pack(">HI", FW_PACKET_COMMAND, opcode) + args


def gds_command_packet(opcode: int, args: bytes = b"") -> bytes:
    packet = inner_command_packet(opcode, args)
    return struct.pack(">II", COMMAND_DESCRIPTOR, len(packet)) + packet


def wait_host_port(host: str, port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return
        except OSError:
            time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for {host}:{port}")


def wait_log_fragment(path: pathlib.Path, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return
        time.sleep(0.2)
    raise ProbeFailure(f"timed out waiting for {fragment!r} in {path}")


@dataclass(frozen=True)
class RemoteStackConfig:
    ssh_target: str
    remote_dir: str
    command: str
    env: dict[str, str]


class TargetDirectControlScenario:
    def __init__(self, environment: str, probe_root: pathlib.Path) -> None:
        self.environment = environment
        self.probe_root = probe_root
        self.root_dir = pathlib.Path(__file__).resolve().parents[3]
        self.bin_dir = self.root_dir / "build-fprime-automatic-native" / "bin" / "Darwin"
        self.dictionary_path = self.root_dir / "build-fprime-automatic-native" / "OBC" / "TopCcsds" / "AppTopologyDictionary.json"
        self.cli_path = self.root_dir / "fprime-venv" / "bin" / "fprime-cli"
        self.proxy_bin = self.bin_dir / "csp_zmqproxy"
        self.probe_root.mkdir(parents=True, exist_ok=True)
        self.ground = GroundPath("direct-ground", self.root_dir, self.cli_path, self.dictionary_path, self.probe_root, vcid=1)
        self.notes_log = self.probe_root / "notes.log"
        self.subsystem_log = self.probe_root / "subsystem.log"
        self.obc_log = self.probe_root / "obc.log"
        self.cli_log = self.probe_root / "cli.log"
        self.contract_dir = self.probe_root / "contract"
        self.timeline_log = self.probe_root / "timeline.log"
        self.historical_reference = self.contract_dir / "historical-direct-path.txt"
        self.processes: list[ManagedProcess] = []
        self.runtime_token = self.probe_root.name
        self.command_results: list[DirectCommandAttempt] = []
        self.envelope_results: list[DirectEnvelopeAttempt] = []
        self.suspended_remote_services: dict[str, list[str]] = {}
        self.diagnostic_order = os.environ.get("DIRECT_DIAGNOSTIC_ORDER", "plain-then-envelope")
        self.command_dictionaries: Dictionaries | None = None
        self.command_encoder: CmdEncoder | None = None
        self.opcodes: dict[str, int] = {}
        self.direct_commands = (
            DirectCommandSpec("OBCApp.modeManager.MODE_GET", tuple(), "plain-mode-get"),
            DirectCommandSpec("OBCApp.bootManager.GET_RESET_CAUSE", tuple(), "plain-get-reset-cause"),
        )

        if not self.bin_dir.exists():
            raise ProbeFailure(f"native bin dir is missing: {self.bin_dir}")
        if not self.dictionary_path.exists():
            raise ProbeFailure(f"dictionary is missing: {self.dictionary_path}")
        if not self.cli_path.exists():
            raise ProbeFailure(f"fprime-cli is missing: {self.cli_path}")

        self.obc_target = require_env("OBC_SSH_TARGET")
        self.subsystem_target = require_env("SUBSYSTEM_SIM_SSH_TARGET")
        self.obc_remote_dir = require_env("RPI_REMOTE_DIR")
        self.subsystem_remote_dir = require_env("SUBSYSTEM_SIM_REMOTE_DIR")
        self.gds_host = require_env("GDS_HOST")

        if environment == "rpi_tcp":
            self.remote_host = require_env("REMOTE_HOST")
            self.csp_sub_port = int(os.environ.get("CSP_HUB_SUB_PORT", "16610"))
            self.csp_pub_port = int(os.environ.get("CSP_HUB_PUB_PORT", "17610"))
            self.radio_port = int(os.environ.get("DIRECT_REMOTE_RADIO_PORT", "17000"))
            self.remote_runtime_root = os.environ.get(
                "REMOTE_RUNTIME_ROOT",
                f"{self.obc_remote_dir}/runtime/comm-verification/target-direct-tcp-{self.runtime_token}",
            )
        elif environment == "rpi_can":
            self.remote_host = ""
            self.obc_can_device = os.environ.get("OBC_CSP_CAN_DEVICE", "can0")
            self.subsystem_can_device = os.environ.get("SUBSYSTEM_SIM_CSP_CAN_DEVICE", "can0")
            self.subsystem_reserved_can_device = os.environ.get("SUBSYSTEM_SIM_RESERVED_CAN_DEVICE", "can1")
            self.csp_can_promisc = os.environ.get("CSP_CAN_PROMISC", "0")
            self.remote_runtime_root = os.environ.get(
                "REMOTE_RUNTIME_ROOT",
                f"{self.obc_remote_dir}/runtime/comm-verification/target-direct-can-{self.runtime_token}",
            )
        else:
            raise ProbeFailure(f"unsupported environment: {environment}")
        self.load_opcodes()

    def note(self, message: str) -> None:
        with self.notes_log.open("a", encoding="utf-8") as handle:
            handle.write(message + "\n")

    def write_contract(self, name: str, payload: dict[str, object]) -> None:
        self.contract_dir.mkdir(parents=True, exist_ok=True)
        path = self.contract_dir / f"{name}.json"
        path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def write_historical_reference(self) -> None:
        self.contract_dir.mkdir(parents=True, exist_ok=True)
        self.historical_reference.write_text(
            "\n".join(
                (
                    "registered_path=evidence/records/rpi-target-integration-v1/README.md",
                    "historical_connectivity_contract=fprime-gds -n -g none --framing-selection fprime --ip-port 50001",
                    "target_wrapper=scripts/run_rpi_stack.sh",
                    "target_wrapper_defaults=GROUND_LINK_MODE=direct-tcp COMM_CSP_NODE=4 CSP_TRANSPORT=zmqhub",
                    "target_wrapper_remote_command=scripts/run_dev_stack.sh",
                    "direct_path_truth=Pi OBC connects directly to host fprime-gds without ground_ttc_gateway node5 node6",
                    "historical_limit=registered evidence proves direct connectivity, not plain-command completion semantics",
                    "matrix_command_truth=TopCcsds direct uplink still enters GroundLinkDriver->ComStub->TcDeframer->SpacePacketDeframer->FprimeRouter",
                    "matrix_ground_contract=use the same CCSDS uplink framing truth as hosted direct-control, not the historical connectivity-only fprime framing",
                )
            )
            + "\n",
            encoding="utf-8",
        )
        checklist = {
            "registeredPath": "evidence/records/rpi-target-integration-v1/README.md",
            "historicalConnectivityGround": {
                "component": "fprime-gds",
                "framingSelection": "fprime",
                "ipPort": 50001,
                "claimBoundary": "connectivity-only",
            },
            "expectedMatrixGround": {
                "component": "fprime-gds",
                "framingSelection": DIRECT_GDS_FRAMING_SELECTION,
                "scid": int(DIRECT_GDS_SCID),
                "vcid": self.ground.vcid,
                "frameSize": int(DIRECT_GDS_FRAME_SIZE),
                "wirePath": "direct OBC -> GDS without ground_ttc_gateway/node5/node6, but still through TopCcsds CCSDS uplink deframers",
            },
            "expectedTarget": {
                "groundLinkMode": "direct-tcp",
                "commCspNode": "4",
                "headless": True,
                "commandAuthDisabled": True,
                "authorityProfile": "dev-direct",
                "commandAuthSourceId": DIRECT_SOURCE_ID,
                "commandAuthKeySlot": DIRECT_KEY_SLOT,
            },
            "comparisonGoal": "matrix wrapper contract must match TopCcsds direct uplink framing truth before product-path conclusions",
        }
        self.write_contract("historical-direct-checklist", checklist)

    def load_opcodes(self) -> None:
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.legacy_session_open_supported = "OBCApp.commandIngressAuthority.SESSION_OPEN" in (
            dictionary.get("commands", {}) or {}
        )
        if self.legacy_session_open_supported:
            self.opcodes["OBCApp.commandIngressAuthority.SESSION_OPEN"] = command_opcode(
                dictionary, "OBCApp.commandIngressAuthority.SESSION_OPEN"
            )
        for spec in self.direct_commands:
            self.opcodes[spec.command_name] = command_opcode(dictionary, spec.command_name)
        dictionaries = Dictionaries()
        dictionaries.load_dictionaries(str(self.dictionary_path), None, None)
        self.command_dictionaries = dictionaries
        self.command_encoder = CmdEncoder()

    def proxy_pattern(self) -> str | None:
        if self.environment != "rpi_tcp":
            return None
        return rf"csp_zmqproxy -s tcp://0\.0\.0\.0:{self.csp_sub_port} -p tcp://0\.0\.0\.0:{self.csp_pub_port}"

    def suspend_remote_services(self, target: str) -> None:
        stopped: list[str] = []
        for service_name in DIRECT_REMOTE_SERVICE_NAMES:
            active = ssh_capture(
                target,
                f"sudo -n systemctl is-active {shq(service_name)} >/dev/null 2>&1 && echo active || true",
                check=False,
            ).strip()
            if active != "active":
                continue
            ssh_capture(target, f"sudo -n systemctl stop {shq(service_name)}", check=True)
            stopped.append(service_name)
        self.suspended_remote_services[target] = stopped
        self.write_contract(
            f"service-suspension-{target.replace('@', '_').replace('.', '_')}",
            {
                "sshTarget": target,
                "stoppedServices": stopped,
            },
        )

    def resume_remote_services(self) -> None:
        for target, services in self.suspended_remote_services.items():
            for service_name in services:
                ssh_capture(target, f"sudo -n systemctl start {shq(service_name)}", check=False)
        self.suspended_remote_services.clear()

    def reap_owned_local_processes(self) -> None:
        pattern = self.proxy_pattern()
        if pattern:
            reap_local_process_pattern(pattern)

    def _start_local(self, name: str, args: list[str], log_path: pathlib.Path, env: dict[str, str] | None = None) -> None:
        handle = log_path.open("a", encoding="utf-8", buffering=1)
        handle.write("$ " + " ".join(args) + "\n")
        handle.flush()
        self.write_contract(
            f"{name}-launcher",
            {
                "kind": "local",
                "name": name,
                "args": args,
                "cwd": str(self.root_dir),
                "envOverrides": {} if env is None else {key: env[key] for key in sorted(env.keys()) if os.environ.get(key) != env[key]},
            },
        )
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

    def _start_remote(self, name: str, config: RemoteStackConfig, log_path: pathlib.Path) -> None:
        remote_env = " ".join(f"{key}={shq(value)}" for key, value in config.env.items())
        remote_cmd = f"set -euo pipefail; cd {shq(config.remote_dir)}; env {remote_env} bash {shq(config.command)}"
        handle = log_path.open("a", encoding="utf-8", buffering=1)
        handle.write(f"$ ssh {config.ssh_target} {remote_cmd}\n")
        handle.flush()
        self.write_contract(
            f"{name}-launcher",
            {
                "kind": "remote",
                "name": name,
                "sshTarget": config.ssh_target,
                "remoteDir": config.remote_dir,
                "command": config.command,
                "env": {key: config.env[key] for key in sorted(config.env.keys())},
                "sshCommand": ssh_command(config.ssh_target, f"/bin/bash -lc {shq(remote_cmd)}"),
            },
        )
        process = subprocess.Popen(
            ssh_command(config.ssh_target, f"/bin/bash -lc {shq(remote_cmd)}"),
            stdin=subprocess.DEVNULL,
            stdout=handle,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            cwd=str(self.root_dir),
        )
        self.processes.append(ManagedProcess(name, process, handle))

    def encode_plain_command(self, spec: DirectCommandSpec) -> tuple[int, bytes, bytes]:
        if self.command_dictionaries is None or self.command_encoder is None:
            raise ProbeFailure("command dictionary is not initialized")
        template = self.command_dictionaries.command_name[spec.command_name]
        encoded = self.command_encoder.encode_api(CmdData(spec.cli_args, template))
        inner = encoded[8:]
        opcode = self.opcodes[spec.command_name]
        outer = gds_command_packet(opcode, inner[6:])
        return opcode, inner, outer

    def encode_diagnostic_envelope(self, spec: DirectCommandSpec) -> str:
        opcode, inner, _ = self.encode_plain_command(spec)
        payload = authenticated_envelope(
            inner,
            DIRECT_SOURCE_ID,
            DIRECT_KEY_SLOT,
            DIRECT_KEY_BYTES,
            session_id=1,
            sequence_number=1,
        )
        self.write_contract(
            f"{spec.label}-envelope-diagnostic",
            {
                "commandName": spec.command_name,
                "opcode": f"0x{opcode:x}",
                "sourceId": DIRECT_SOURCE_ID,
                "keySlot": DIRECT_KEY_SLOT,
                "outerHex": payload.hex(),
                "note": "diagnostic-only comparator; matrix direct-control still asserts plain command path",
            },
        )
        return payload.hex()

    def send_gds_tts_payload(self, payload: bytes) -> None:
        with socket.create_connection(("127.0.0.1", self.ground.gds_tts_port), timeout=5.0) as sock:
            sock.sendall(b"Register GUI\n")
            time.sleep(0.1)
            sock.sendall(b"A5A5 FSW " + payload)
            time.sleep(0.2)

    def wait_direct_observation(
        self,
        *,
        opcode: int,
        expect_session_id: int | None = None,
        timeout: float,
    ) -> tuple[bool, bool, bool, str | None]:
        envelope_fragment = "COMMAND_ENVELOPE_OBSERVED"
        completion_fragment = f"Opcode 0x{opcode:x} completed"
        identity, role = authority_identity_role("dev-direct")
        session_fragment = None if expect_session_id is None else f"session {expect_session_id}"
        role_fragment = f"identity {identity} role {role}"
        deadline = time.time() + timeout
        observed_envelope = False
        observed_session_open = False
        completion_observed = False
        source = None
        while time.time() < deadline:
            ground_text = read_text(self.ground.events_log)
            obc_text = read_text(self.obc_log)
            if not observed_envelope and envelope_fragment in ground_text:
                observed_envelope = True
                source = source or "gds"
            if not observed_envelope and envelope_fragment in obc_text:
                observed_envelope = True
                source = source or "obc-log"
            if session_fragment is not None:
                if not observed_session_open and all(fragment in ground_text for fragment in ("COMMAND_SESSION_OPENED", role_fragment, session_fragment)):
                    observed_session_open = True
                    source = source or "gds"
                if not observed_session_open and all(fragment in obc_text for fragment in ("COMMAND_SESSION_OPENED", role_fragment, session_fragment)):
                    observed_session_open = True
                    source = source or "obc-log"
            if not completion_observed and completion_fragment in ground_text:
                completion_observed = True
                source = source or "gds"
            if not completion_observed and completion_fragment in obc_text:
                completion_observed = True
                source = source or "obc-log"
            if observed_envelope and (session_fragment is None or observed_session_open) and completion_observed:
                return observed_envelope, observed_session_open, completion_observed, source
            time.sleep(0.2)
        return observed_envelope, observed_session_open, completion_observed, source

    def send_diagnostic_envelope_session_open(self) -> DirectEnvelopeAttempt:
        if not self.legacy_session_open_supported:
            self.note("diagnostic-session-open=skipped:retired-public-surface")
            return DirectEnvelopeAttempt(
                spec=DirectCommandSpec("OBCApp.commandIngressAuthority.SESSION_OPEN", tuple(), "diagnostic-session-open"),
                session_id=DIRECT_SESSION_ID,
                sequence_number=0,
                payload_hex="",
                observed_envelope=False,
                observed_session_open=False,
                completion_observed=False,
                observation_source="skipped",
            )
        spec = DirectCommandSpec("OBCApp.commandIngressAuthority.SESSION_OPEN", tuple(), "diagnostic-session-open")
        opcode = self.opcodes[spec.command_name]
        payload = authenticated_envelope(
            inner_command_packet(opcode),
            DIRECT_SOURCE_ID,
            DIRECT_KEY_SLOT,
            DIRECT_KEY_BYTES,
            session_id=DIRECT_SESSION_ID,
            sequence_number=0,
        )
        self.write_contract(
            "diagnostic-session-open-envelope",
            {
                "commandName": spec.command_name,
                "opcode": f"0x{opcode:x}",
                "sourceId": DIRECT_SOURCE_ID,
                "keySlot": DIRECT_KEY_SLOT,
                "sessionId": DIRECT_SESSION_ID,
                "sequenceNumber": 0,
                "outerHex": payload.hex(),
                "note": "diagnostic-only comparator for direct adapter envelope/session behavior",
            },
        )
        with self.cli_log.open("a", encoding="utf-8") as handle:
            handle.write("$ diagnostic envelope SESSION_OPEN\n")
        self.send_gds_tts_payload(payload)
        observed_envelope, observed_session_open, completion_observed, observation_source = self.wait_direct_observation(
            opcode=opcode,
            expect_session_id=DIRECT_SESSION_ID,
            timeout=10.0,
        )
        attempt = DirectEnvelopeAttempt(
            spec=spec,
            session_id=DIRECT_SESSION_ID,
            sequence_number=0,
            payload_hex=payload.hex(),
            observed_envelope=observed_envelope,
            observed_session_open=observed_session_open,
            completion_observed=completion_observed,
            observation_source=observation_source,
        )
        self.envelope_results.append(attempt)
        self.note(
            f"{spec.label}=observed_envelope:{observed_envelope}:session_open:{observed_session_open}:completion:{completion_observed}"
        )
        return attempt

    def send_diagnostic_envelope_command(self, spec: DirectCommandSpec, sequence_number: int) -> DirectEnvelopeAttempt:
        opcode, inner, _ = self.encode_plain_command(spec)
        payload = authenticated_envelope(
            inner,
            DIRECT_SOURCE_ID,
            DIRECT_KEY_SLOT,
            DIRECT_KEY_BYTES,
            session_id=DIRECT_SESSION_ID,
            sequence_number=sequence_number,
        )
        self.write_contract(
            f"{spec.label}-envelope-comparator",
            {
                "commandName": spec.command_name,
                "opcode": f"0x{opcode:x}",
                "sourceId": DIRECT_SOURCE_ID,
                "keySlot": DIRECT_KEY_SLOT,
                "sessionId": DIRECT_SESSION_ID,
                "sequenceNumber": sequence_number,
                "outerHex": payload.hex(),
                "note": "diagnostic-only comparator; does not change matrix direct-control plain-command oracle",
            },
        )
        with self.cli_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ diagnostic envelope {spec.command_name} session={DIRECT_SESSION_ID} seq={sequence_number}\n")
        self.send_gds_tts_payload(payload)
        observed_envelope, observed_session_open, completion_observed, observation_source = self.wait_direct_observation(
            opcode=opcode,
            expect_session_id=None,
            timeout=10.0,
        )
        attempt = DirectEnvelopeAttempt(
            spec=spec,
            session_id=DIRECT_SESSION_ID,
            sequence_number=sequence_number,
            payload_hex=payload.hex(),
            observed_envelope=observed_envelope,
            observed_session_open=observed_session_open,
            completion_observed=completion_observed,
            observation_source=observation_source,
        )
        self.envelope_results.append(attempt)
        self.note(
            f"{spec.label}-envelope=observed_envelope:{observed_envelope}:completion:{completion_observed}"
        )
        return attempt

    def start_ground(self) -> None:
        prior = {
            "COMMV_GDS_FRAMING_SELECTION": os.environ.get("COMMV_GDS_FRAMING_SELECTION"),
            "COMMV_GDS_SCID": os.environ.get("COMMV_GDS_SCID"),
            "COMMV_GDS_VCID": os.environ.get("COMMV_GDS_VCID"),
            "COMMV_GDS_FRAME_SIZE": os.environ.get("COMMV_GDS_FRAME_SIZE"),
        }
        try:
            os.environ["COMMV_GDS_FRAMING_SELECTION"] = DIRECT_GDS_FRAMING_SELECTION
            os.environ["COMMV_GDS_SCID"] = DIRECT_GDS_SCID
            os.environ["COMMV_GDS_VCID"] = str(self.ground.vcid)
            os.environ["COMMV_GDS_FRAME_SIZE"] = DIRECT_GDS_FRAME_SIZE
            self.ground.start_gds()
        finally:
            for key, value in prior.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value
        self.write_contract(
            "ground-contract",
            {
                "gdsHost": self.gds_host,
                "gdsPort": self.ground.gds_port,
                "ttsPort": self.ground.gds_tts_port,
                "framingSelection": DIRECT_GDS_FRAMING_SELECTION,
                "vcid": self.ground.vcid,
                "scid": int(DIRECT_GDS_SCID),
                "frameSize": int(DIRECT_GDS_FRAME_SIZE),
            },
        )

    def start_tcp_obc(self) -> None:
        self.suspend_remote_services(self.obc_target)
        env = os.environ.copy()
        env.update(
            {
                "OBC_SSH_TARGET": self.obc_target,
                "RPI_REMOTE_DIR": self.obc_remote_dir,
                "GDS_HOST": self.gds_host,
                "GDS_PORT": str(self.ground.gds_port),
                "RADIO_PORT": str(self.radio_port),
                "GROUND_LINK_MODE": "direct-tcp",
                "COMM_CSP_NODE": "4",
                "CSP_TRANSPORT": "zmqhub",
                "CSP_HUB_HOST": "127.0.0.1",
                "CSP_HUB_SUB_PORT": str(self.csp_sub_port),
                "CSP_HUB_PUB_PORT": str(self.csp_pub_port),
                "RUNTIME_ROOT": self.remote_runtime_root,
                "PERSISTENT_ROOT": f"{self.remote_runtime_root}/persistent-data",
                "STAGING_ROOT": f"{self.remote_runtime_root}/staging",
                "COMMAND_AUTHORITY_PROFILE": "dev-direct",
                "COMMAND_AUTH": "disabled",
                "HEADLESS": "1",
            }
        )
        self._start_local(
            "tcp_obc_direct",
            ["bash", str(self.root_dir / "scripts/run_rpi_stack.sh")],
            self.obc_log,
            env,
        )

    def start_can_subsystem_services(self) -> None:
        env = os.environ.copy()
        env.update(
            {
                "SUBSYSTEM_SIM_SSH_TARGET": self.subsystem_target,
                "SUBSYSTEM_SIM_REMOTE_DIR": self.subsystem_remote_dir,
                "CSP_TRANSPORT": "socketcan",
                "SUBSYSTEM_SIM_CSP_CAN_DEVICE": self.subsystem_can_device,
                "SUBSYSTEM_SIM_RESERVED_CAN_DEVICE": self.subsystem_reserved_can_device,
                "CSP_CAN_PROMISC": self.csp_can_promisc,
                "KILL_EXISTING_PIDS": "1",
            }
        )
        self._start_local(
            "can_subsystem_services",
            ["bash", str(self.root_dir / "scripts/run_subsystem_sim_remote_can_stack.sh")],
            self.subsystem_log,
            env,
        )
        wait_log_fragment(self.subsystem_log, "Starting subsystem CAN simulator stack", 15.0)

    def start_can_obc(self) -> None:
        self.suspend_remote_services(self.obc_target)
        env = os.environ.copy()
        env.update(
            {
                "OBC_SSH_TARGET": self.obc_target,
                "RPI_REMOTE_DIR": self.obc_remote_dir,
                "CSP_TRANSPORT": "socketcan",
                "OBC_CSP_CAN_DEVICE": self.obc_can_device,
                "GROUND_LINK_MODE": "direct-tcp",
                "COMM_CSP_NODE": "4",
                "GDS_HOST": self.gds_host,
                "GDS_PORT": str(self.ground.gds_port),
                "RUNTIME_ROOT": self.remote_runtime_root,
                "PERSISTENT_ROOT": f"{self.remote_runtime_root}/persistent-data",
                "STAGING_ROOT": f"{self.remote_runtime_root}/staging",
                "LOG_ROOT": f"{self.remote_runtime_root}/logs",
                "COMMAND_AUTHORITY_PROFILE": "dev-direct",
                "COMMAND_AUTH": "disabled",
                "HEADLESS": "1",
                "KILL_EXISTING_PIDS": "1",
                "MANAGE_AUTOSTART": "0",
                "SERVICE_NAME": "obc-comm-csp-stack.service",
            }
        )
        self._start_local(
            "can_obc_direct",
            ["bash", str(self.root_dir / "scripts/run_rpi_can_csp_stack.sh")],
            self.obc_log,
            env,
        )
        wait_log_fragment(self.obc_log, "Starting OBC CAN stack", 15.0)

    def await_command_completion(self, opcode: int, timeout: float) -> tuple[bool, str | None]:
        completion_fragment = f"Opcode 0x{opcode:x} completed"
        try:
            self.ground.await_event(completion_fragment, timeout=timeout)
            return True, "gds"
        except ProbeFailure:
            try:
                wait_log_fragment(self.obc_log, completion_fragment, timeout)
                return True, "obc-log"
            except ProbeFailure:
                return False, None

    def send_plain_command(self, spec: DirectCommandSpec) -> DirectCommandAttempt:
        opcode, encoded_inner, encoded_outer = self.encode_plain_command(spec)
        command = [
            str(self.cli_path),
            "command-send",
            "--dictionary",
            str(self.dictionary_path),
            "--no-zmq",
            "--tts-port",
            str(self.ground.gds_tts_port),
            spec.command_name,
        ]
        command.extend(spec.cli_args)
        diagnostic_envelope_hex = self.encode_diagnostic_envelope(spec)
        self.write_contract(
            f"{spec.label}-command",
            {
                "kind": "local",
                "commandName": spec.command_name,
                "commandArgs": list(spec.cli_args),
                "dictionaryOpcode": f"0x{opcode:x}",
                "cliArgs": command,
                "cwd": str(self.root_dir),
                "gdsFramingSelection": DIRECT_GDS_FRAMING_SELECTION,
                "commandAuthEnabled": False,
                "commandAuthMode": "disabled",
                "authorityProfile": "dev-direct",
                "sourceId": None,
                "keySlot": None,
                "ingressPort": "direct-gds",
                "cmdEncoderHex": encoded_inner.hex(),
                "rawGdsCommandHex": encoded_outer.hex(),
                "diagnosticEnvelopeHex": diagnostic_envelope_hex,
            },
        )
        try:
            result = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                cwd=str(self.root_dir),
                check=False,
                timeout=8.0,
            )
        except subprocess.TimeoutExpired as exc:
            output = exc.stdout or ""
            with self.cli_log.open("a", encoding="utf-8") as handle:
                handle.write(f"$ {' '.join(command)}\n")
                handle.write(output)
                handle.write("\n")
            attempt = DirectCommandAttempt(
                spec=spec,
                opcode=opcode,
                cli_command=command,
                cli_returncode=None,
                cli_output=output.splitlines(),
                plain_inner_hex=encoded_inner.hex(),
                plain_outer_hex=encoded_outer.hex(),
                envelope_outer_hex=diagnostic_envelope_hex,
                completion_observed=False,
                completion_source=None,
                timeout=True,
            )
            self.command_results.append(attempt)
            self.note(f"{spec.label}=timeout")
            return attempt
        with self.cli_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ {' '.join(command)}\n")
            handle.write(result.stdout)
            if not result.stdout.endswith("\n"):
                handle.write("\n")
        completion_observed, completion_source = self.await_command_completion(opcode, timeout=10.0)
        attempt = DirectCommandAttempt(
            spec=spec,
            opcode=opcode,
            cli_command=command,
            cli_returncode=result.returncode,
            cli_output=result.stdout.splitlines(),
            plain_inner_hex=encoded_inner.hex(),
            plain_outer_hex=encoded_outer.hex(),
            envelope_outer_hex=diagnostic_envelope_hex,
            completion_observed=completion_observed,
            completion_source=completion_source,
        )
        self.command_results.append(attempt)
        self.note(f"{spec.label}=rc:{result.returncode}:completion={completion_observed}")
        if result.returncode != 0:
            raise ProbeFailure(f"{spec.command_name} command failed rc={result.returncode}")
        return attempt

    def _extract_log_fragments(self, path: pathlib.Path, fragments: tuple[str, ...]) -> list[str]:
        matches: list[str] = []
        for line in read_text(path).splitlines():
            if any(fragment in line for fragment in fragments):
                matches.append(line)
        return matches

    def write_timeline(self, verdict: str, reason: str | None = None) -> None:
        gds_lines = self._extract_log_fragments(
            self.ground.gds_log,
            (
                "Server connected",
                "Uplink failed",
                "Checksum validation failed",
                "Downlink",
                "Client connected",
                "OpCodeCompleted",
            ),
        )
        obc_lines = self._extract_log_fragments(
            self.obc_log,
            (
                "GROUND_LINK_UP",
                "GROUND_LINK_DOWN",
                "Ground link target:",
                "Runtime mode: headless",
                "OBC CCSDS S-band runtime started",
                "OpCodeCompleted",
                "authority",
                "reject",
                "unknown opcode",
            ),
        )
        cli_lines = read_text(self.cli_log).splitlines()
        process_states = []
        for managed in self.processes:
            process_states.append(
                {
                    "name": managed.name,
                    "pid": managed.process.pid,
                    "returncode": managed.process.poll(),
                }
            )
        payload = {
            "environment": self.environment,
            "verdict": verdict,
            "reason": reason,
            "commands": [
                {
                    "label": attempt.spec.label,
                    "commandName": attempt.spec.command_name,
                    "commandArgs": list(attempt.spec.cli_args),
                    "opcode": f"0x{attempt.opcode:x}",
                    "cliReturncode": attempt.cli_returncode,
                    "timeout": attempt.timeout,
                    "completionObserved": attempt.completion_observed,
                    "completionSource": attempt.completion_source,
                    "plainInnerHex": attempt.plain_inner_hex,
                    "plainOuterHex": attempt.plain_outer_hex,
                    "envelopeOuterHex": attempt.envelope_outer_hex,
                    "cliOutputTail": attempt.cli_output[-20:],
                }
                for attempt in self.command_results
            ],
            "envelopeComparators": [
                {
                    "label": attempt.spec.label,
                    "commandName": attempt.spec.command_name,
                    "sessionId": attempt.session_id,
                    "sequenceNumber": attempt.sequence_number,
                    "payloadHex": attempt.payload_hex,
                    "observedEnvelope": attempt.observed_envelope,
                    "observedSessionOpen": attempt.observed_session_open,
                    "completionObserved": attempt.completion_observed,
                    "observationSource": attempt.observation_source,
                }
                for attempt in self.envelope_results
            ],
            "gdsTimeline": gds_lines,
            "obcTimeline": obc_lines,
            "cliOutput": cli_lines[-40:],
            "processStates": process_states,
        }
        self.timeline_log.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    def start(self) -> None:
        if self.probe_root.exists():
            shutil.rmtree(self.probe_root)
        self.probe_root.mkdir(parents=True, exist_ok=True)
        self.write_historical_reference()
        self.note(f"environment={self.environment}")
        ensure_no_legacy_aliases(self.root_dir)
        self.reap_owned_local_processes()
        self.start_ground()
        if self.environment == "rpi_tcp":
            self.start_tcp_obc()
        else:
            self.start_can_subsystem_services()
            self.start_can_obc()
        wait_log_fragment(self.ground.gds_log, "tcp_handler: Server connected", 20.0)
        wait_log_fragment(self.obc_log, "GROUND_LINK_UP", 20.0)

    def exercise(self) -> None:
        if self.diagnostic_order == "envelope-then-plain":
            self.send_diagnostic_envelope_session_open()
            for index, spec in enumerate(self.direct_commands, start=1):
                self.send_diagnostic_envelope_command(spec, index)

        attempts = [self.send_plain_command(spec) for spec in self.direct_commands]
        if not any(attempt.completion_observed for attempt in attempts):
            if self.diagnostic_order != "envelope-then-plain":
                self.send_diagnostic_envelope_session_open()
                for index, spec in enumerate(self.direct_commands, start=1):
                    self.send_diagnostic_envelope_command(spec, index)
            labels = ", ".join(spec.label for spec in self.direct_commands)
            raise ProbeFailure(f"no target-side completion observed for direct plain commands: {labels}")
        ensure_no_legacy_aliases(self.root_dir)

    def stop(self) -> None:
        while self.processes:
            managed = self.processes.pop()
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
        self.ground.stop()
        self.resume_remote_services()
        self.reap_owned_local_processes()

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
        self.ground.force_stop()
        self.resume_remote_services()
        self.reap_owned_local_processes()


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Run target direct-control matrix probes.")
    parser.add_argument("--environment", choices=("rpi_tcp", "rpi_can"), required=True)
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args(argv)

    probe_root = pathlib.Path(args.probe_root)
    scenario = TargetDirectControlScenario(args.environment, probe_root)
    try:
        scenario.start()
        scenario.exercise()
        scenario.note("verdict=pass")
        scenario.write_timeline("pass")
        return 0
    except ProbeFailure as exc:
        scenario.note(f"verdict=fail reason={exc}")
        scenario.write_timeline("fail", str(exc))
        print(str(exc), file=sys.stderr)
        return 1
    finally:
        try:
            scenario.stop()
        except Exception:
            scenario.force_stop()


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
