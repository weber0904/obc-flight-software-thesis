from __future__ import annotations

import argparse
import json
import os
import pathlib
import pty
import selectors
import shutil
import socket
import subprocess
import sys
import termios
import time
from dataclasses import dataclass
from typing import Any, Sequence

from probe_process_utils import (
    ManagedProcess,
    build_gds_stale_match_groups,
    cleanup_managed_processes,
    install_signal_cleanup,
    reap_matching_processes,
    start_managed_process,
    stop_managed_process,
)


DEFAULT_GDS_BIND_HOST = "127.0.0.1"
DEFAULT_SBAND_GDS_PORT = 50150
DEFAULT_SBAND_GDS_TTS_PORT = 50151
DEFAULT_UHF_GDS_PORT = 50160
DEFAULT_UHF_GDS_TTS_PORT = 50161
DEFAULT_SBAND_GUI_PORT = 5000
DEFAULT_UHF_GUI_PORT = 5001
DEFAULT_CSP_SUB_PORT = 56250
DEFAULT_CSP_PUB_PORT = 57250
DEFAULT_RADIO_PORT = 17050
DEFAULT_SBAND_TCP_PORT = 18520
DEFAULT_EPS_CSP_NODE_ID = 2
DEFAULT_ADCS_CSP_NODE_ID = 3
DEFAULT_SBAND_SCID = 68
DEFAULT_SBAND_VCID = 1
DEFAULT_UHF_SCID = 68
DEFAULT_UHF_VCID = 2
DEFAULT_FRAME_SIZE = 4096
DEFAULT_BAUDRATE = 115200
DEFAULT_TICK_MS = 250
READY_TIMEOUT_SEC = 30.0
LOG_TAIL_BYTES = 65536
_ALLOCATED_PORTS: set[int] = set()


def parse_bool(value: str | bool | None, default: bool = False) -> bool:
    if value is None:
        return default
    if isinstance(value, bool):
        return value
    return value.strip().lower() in {"1", "true", "yes", "on"}


def free_port() -> int:
    while True:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.bind(("127.0.0.1", 0))
            port = int(sock.getsockname()[1])
        if port not in _ALLOCATED_PORTS:
            _ALLOCATED_PORTS.add(port)
            return port


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


def read_text(path: pathlib.Path, limit: int = LOG_TAIL_BYTES) -> str:
    try:
        size = path.stat().st_size
        with path.open("rb") as handle:
            if size > limit:
                handle.seek(size - limit)
            return handle.read().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path}")


def wait_text_or_none(path: pathlib.Path, fragment: str, timeout: float) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return True
        time.sleep(0.2)
    return False


def find_fprime_cli(root_dir: pathlib.Path) -> str:
    candidate = root_dir / "fprime-venv/bin/fprime-cli"
    if candidate.exists():
        return str(candidate)
    resolved = shutil.which("fprime-cli")
    if resolved:
        return resolved
    raise RuntimeError("fprime-cli not found in fprime-venv or PATH")


def resolve_runtime_root(mode_name: str, stack_root: pathlib.Path, runtime_root: pathlib.Path | None) -> pathlib.Path:
    if runtime_root is None:
        return stack_root / "runtime"
    try:
        runtime_root.relative_to(stack_root)
        return runtime_root
    except ValueError:
        # Treat externally supplied roots as launcher-owned namespaces so
        # per-band entrypoints cannot wipe one another's runtime state.
        return runtime_root / mode_name


def process_instance_label(path: pathlib.Path) -> str:
    return str(path.resolve())


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


@dataclass
class GroundPath:
    name: str
    band: str
    bind_host: str
    dictionary_path: pathlib.Path
    cli_path: str
    stack_root: pathlib.Path
    gds_port: int
    gds_tts_port: int
    framing: str
    scid: int
    vcid: int
    frame_size: int

    def __post_init__(self) -> None:
        self.root = self.stack_root / self.name
        self.file_storage = self.root / "gds-files"
        self.log_root = self.root / "logs"
        self.gds_log = self.log_root / "gds.log"
        self.events_log = self.log_root / "events.log"
        self.channels_log = self.log_root / "channels.log"
        self.command_log = self.log_root / "command-send.log"
        self.raw_command_log = self.log_root / "raw-command.log"
        self.processes: list[ManagedProcess] = []
        self.events_process: ManagedProcess | None = None
        self.channels_process: ManagedProcess | None = None
        self.gds_ui_mode = "headless"
        self.enable_passive_listeners = True
        self.gui_port = 5000
        self.gui_addr = "127.0.0.1"

    def start(self, root_dir: pathlib.Path) -> None:
        self.root.mkdir(parents=True, exist_ok=True)
        self.file_storage.mkdir(parents=True, exist_ok=True)
        self.log_root.mkdir(parents=True, exist_ok=True)
        env = os.environ.copy()
        env["DICT_PATH"] = str(self.dictionary_path)
        env["GDS_BIND_HOST"] = self.bind_host
        env["GDS_PORT"] = str(self.gds_port)
        env["GDS_TTS_PORT"] = str(self.gds_tts_port)
        env["GDS_FILE_STORAGE_DIR"] = str(self.file_storage)
        env["GDS_LOG_DIR"] = str(self.log_root / "gds-runtime")
        env["GDS_FRAMING_SELECTION"] = self.framing
        env["GDS_SCID"] = str(self.scid)
        env["GDS_VCID"] = str(self.vcid)
        env["GDS_FRAME_SIZE"] = str(self.frame_size)
        env["GDS_UI_MODE"] = self.gds_ui_mode
        env["GDS_GUI_ADDR"] = self.gui_addr
        env["GDS_GUI_PORT"] = str(self.gui_port)
        managed = start_managed_process(
            "ground_gds",
            ["bash", str(root_dir / "scripts/run_ground_gds_only_stack.sh")],
            self.gds_log,
            env=env,
            cwd=root_dir,
            stale_match_groups=build_gds_stale_match_groups(
                ip_port=self.gds_port,
                tts_port=self.gds_tts_port,
                file_storage_dir=self.file_storage,
            ),
            stale_match_markers=("fprime-gds", "fprime_gds.executables.comm", "fprime_gds.executables.tcpserver"),
        )
        self.processes.append(managed)
        wait_port(self.gds_port, READY_TIMEOUT_SEC)
        wait_port(self.gds_tts_port, READY_TIMEOUT_SEC)
        if self.enable_passive_listeners:
            self.events_process = start_managed_process(
                "events",
                [self.cli_path, "events", "--dictionary", str(self.dictionary_path), "--no-zmq", "--tts-port", str(self.gds_tts_port)],
                self.events_log,
            )
            self.processes.append(self.events_process)
            self.start_passive_channels_listener()
        time.sleep(1.0)

    def stop(self) -> None:
        cleanup_managed_processes(self.processes, timeout_sec=5.0)

    def start_passive_channels_listener(self) -> None:
        if self.channels_process is not None and self.channels_process.process.poll() is None:
            return
        self.channels_process = start_managed_process(
            "channels",
            [self.cli_path, "channels", "--dictionary", str(self.dictionary_path), "--no-zmq", "--tts-port", str(self.gds_tts_port)],
            self.channels_log,
        )
        self.processes.append(self.channels_process)

    def stop_passive_channels_listener(self) -> None:
        if self.channels_process is None:
            return
        stop_managed_process(self.channels_process, timeout_sec=5.0)
        self.processes = [managed for managed in self.processes if managed is not self.channels_process]
        self.channels_process = None

    def run_channel_capture(self, label: str, search: str) -> str:
        capture_dir = self.root / "channel-captures" / "".join(ch if ch.isalnum() else "-" for ch in label.lower()).strip("-")
        shutil.rmtree(capture_dir, ignore_errors=True)
        capture_dir.mkdir(parents=True, exist_ok=True)
        command = [
            self.cli_path,
            "channels",
            "--dictionary",
            str(self.dictionary_path),
            "--no-zmq",
            "--tts-port",
            str(self.gds_tts_port),
            "--search",
            search,
            "--logs",
            str(capture_dir),
            "--log-directly",
            "--timeout",
            "8",
        ]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
        output = result.stdout
        capture_text = read_text(capture_dir / "channel.log")
        with self.channels_log.open("a", encoding="utf-8") as handle:
            handle.write(f"$ {' '.join(command)} # {label}\n")
            handle.write(output)
            if output and not output.endswith("\n"):
                handle.write("\n")
            if capture_text:
                handle.write(capture_text)
                if not capture_text.endswith("\n"):
                    handle.write("\n")
            handle.write(f"returncode={result.returncode}\n")
        if result.returncode != 0 and search not in output and search not in capture_text:
            raise RuntimeError(f"{self.name}: channel capture failed for {label}")
        return output + "\n" + capture_text

    def manifest(self, *, role: str, observability_role: str, command_class_role: str, southbound: dict[str, Any], capture_paths: dict[str, str]) -> dict[str, Any]:
        return {
            "band": self.band,
            "role": role,
            "commandClassRole": command_class_role,
            "observabilityRole": observability_role,
            "gdsBind": f"{self.bind_host}:{self.gds_port}",
            "gdsPort": self.gds_port,
            "gdsTtsPort": self.gds_tts_port,
            "gdsFraming": self.framing,
            "gdsScid": self.scid,
            "gdsVcid": self.vcid,
            "gdsFrameSize": self.frame_size,
            "gdsUiMode": self.gds_ui_mode,
            "guiAddr": self.gui_addr,
            "guiPort": self.gui_port,
            "guiUrl": f"http://{self.gui_addr}:{self.gui_port}" if self.gds_ui_mode == "ui" else None,
            "southbound": southbound,
            "fileStorageDir": str(self.file_storage),
            "logRoot": str(self.log_root),
            "logs": {
                "gds": str(self.gds_log),
                "events": str(self.events_log),
                "channels": str(self.channels_log),
                "commandSend": str(self.command_log),
                "rawCommand": str(self.raw_command_log),
            },
            "captures": capture_paths,
        }


class HostedPerBandStockStacks:
    def __init__(
        self,
        *,
        mode_name: str,
        root_dir: pathlib.Path,
        bin_dir: pathlib.Path,
        dictionary_path: pathlib.Path,
        cli_path: str,
        stack_root: pathlib.Path,
        runtime_root: pathlib.Path | None = None,
        expose_sband_surface: bool,
        expose_uhf_surface: bool,
        preserve_sband_primary: bool,
        enable_uhf_beacon_side_channel: bool = False,
        auto_ports: bool = False,
        gds_bind_host: str = DEFAULT_GDS_BIND_HOST,
        sband_gds_port: int = DEFAULT_SBAND_GDS_PORT,
        sband_gds_tts_port: int = DEFAULT_SBAND_GDS_TTS_PORT,
        uhf_gds_port: int = DEFAULT_UHF_GDS_PORT,
        uhf_gds_tts_port: int = DEFAULT_UHF_GDS_TTS_PORT,
        sband_gui_port: int = DEFAULT_SBAND_GUI_PORT,
        uhf_gui_port: int = DEFAULT_UHF_GUI_PORT,
        csp_sub_port: int = DEFAULT_CSP_SUB_PORT,
        csp_pub_port: int = DEFAULT_CSP_PUB_PORT,
        radio_port: int = DEFAULT_RADIO_PORT,
        sband_tcp_port: int = DEFAULT_SBAND_TCP_PORT,
        command_authority_profile: str = "sband-primary",
        tick_ms: int = DEFAULT_TICK_MS,
        enable_passive_listeners: bool = True,
        gds_ui_mode: str = "headless",
    ) -> None:
        self.mode_name = mode_name
        self.root_dir = root_dir
        self.bin_dir = bin_dir
        self.dictionary_path = dictionary_path
        self.cli_path = cli_path
        self.stack_root = stack_root
        self.requested_runtime_root = runtime_root
        self.runtime_root = resolve_runtime_root(mode_name, stack_root, runtime_root)
        self.persistent_root = self.runtime_root / "persistent-data"
        self.staging_root = self.runtime_root / "staging"
        self.runtime_logs_root = self.runtime_root / "logs"
        self.process_cwd = self.runtime_root / "process-cwd"
        self.log_root = self.stack_root / "logs"
        self.captures_root = self.stack_root / "captures"
        self.manifest_path = self.stack_root / "manifest.json"
        self.expose_sband_surface = expose_sband_surface
        self.expose_uhf_surface = expose_uhf_surface
        self.preserve_sband_primary = preserve_sband_primary
        self.enable_uhf_beacon_side_channel = enable_uhf_beacon_side_channel
        self.command_authority_profile = command_authority_profile
        self.tick_ms = tick_ms
        self.enable_passive_listeners = enable_passive_listeners
        self.gds_ui_mode = gds_ui_mode
        self.gds_bind_host = gds_bind_host
        self.processes: list[ManagedProcess] = []
        self.sband_tcp_host = "127.0.0.1"
        self.sband_tcp_port = free_port() if auto_ports else sband_tcp_port
        self.radio_port = free_port() if auto_ports else radio_port
        self.csp_sub_port = free_port() if auto_ports else csp_sub_port
        self.csp_pub_port = free_port() if auto_ports else csp_pub_port
        self.sband_process_log = self.log_root / "sband-comm.log"
        self.uhf_process_log = self.log_root / "uhf-comm.log"
        self.sband_gateway_log = self.log_root / "sband-gateway.log"
        self.uhf_gateway_log = self.log_root / "uhf-gateway.log"
        self.obc_log = self.log_root / "obc.log"
        self.csp_proxy_log = self.log_root / "csp-zmqproxy.log"
        self.eps_log = self.log_root / "eps-simulator.log"
        self.adcs_log = self.log_root / "adcs-simulator.log"
        self.radio_log = self.log_root / "radio-mock-server.log"
        self.pty_log = self.log_root / "pty-pair-bridge.log"
        self.pty_instance_label = process_instance_label(self.stack_root / "pty-bridge")
        self.start_sband_runtime = expose_sband_surface or preserve_sband_primary
        self.start_uhf_runtime = expose_uhf_surface
        self.uhf_beacon_peer: PtyPeer | None = PtyPeer() if self.enable_uhf_beacon_side_channel else None
        self.uhf_gateway_serial: str | None = None
        self.uhf_comm_serial: str | None = None
        self.sband = (
            GroundPath(
                name="sband-ground",
                band="sband",
                bind_host=gds_bind_host,
                dictionary_path=dictionary_path,
                cli_path=cli_path,
                stack_root=stack_root,
                gds_port=(free_port() if auto_ports else sband_gds_port),
                gds_tts_port=(free_port() if auto_ports else sband_gds_tts_port),
                framing="space-packet-space-data-link",
                scid=DEFAULT_SBAND_SCID,
                vcid=DEFAULT_SBAND_VCID,
                frame_size=DEFAULT_FRAME_SIZE,
            )
            if expose_sband_surface
            else None
        )
        self.uhf = (
            GroundPath(
                name="uhf-ground",
                band="uhf",
                bind_host=gds_bind_host,
                dictionary_path=dictionary_path,
                cli_path=cli_path,
                stack_root=stack_root,
                gds_port=(free_port() if auto_ports else uhf_gds_port),
                gds_tts_port=(free_port() if auto_ports else uhf_gds_tts_port),
                framing="space-packet-space-data-link",
                scid=DEFAULT_UHF_SCID,
                vcid=DEFAULT_UHF_VCID,
                frame_size=DEFAULT_FRAME_SIZE,
            )
            if expose_uhf_surface
            else None
        )
        for ground in (self.sband, self.uhf):
            if ground is not None:
                ground.enable_passive_listeners = self.enable_passive_listeners
                ground.gds_ui_mode = self.gds_ui_mode
        if self.sband is not None:
            self.sband.gui_port = free_port() if auto_ports else sband_gui_port
        if self.uhf is not None:
            self.uhf.gui_port = free_port() if auto_ports else uhf_gui_port

    def startup_order(self) -> list[str]:
        steps = [
            "prepare owned stack/runtime roots",
            "start exposed stock GDS surfaces",
            "start hosted CSP proxy and hosted subsystem simulators",
        ]
        if self.start_sband_runtime:
            steps.append("start hosted S-band COMM node 5 southbound runtime")
        if self.start_uhf_runtime:
            steps.append("start hosted UHF PTY bridge, beacon side channel, and COMM node 6 southbound runtime")
        if self.expose_sband_surface or self.expose_uhf_surface:
            steps.append("start exposed ground_ttc_gateway southbound bindings")
        steps.append("start shared hosted OBC/TopCcsds runtime")
        return steps

    def shutdown_order(self) -> list[str]:
        return [
            "stop shared hosted OBC/TopCcsds runtime",
            "stop exposed ground_ttc_gateway bindings",
            "stop COMM-node southbound helpers and hosted subsystem simulators",
            "stop exposed stock GDS surfaces",
            "reap owned stale listeners and helper processes",
        ]

    def start(self) -> None:
        self.stop()
        if self.enable_uhf_beacon_side_channel and self.uhf_beacon_peer is None:
            self.uhf_beacon_peer = PtyPeer()
        shutil.rmtree(self.stack_root, ignore_errors=True)
        shutil.rmtree(self.runtime_root, ignore_errors=True)
        self.log_root.mkdir(parents=True, exist_ok=True)
        self.captures_root.mkdir(parents=True, exist_ok=True)
        self.process_cwd.mkdir(parents=True, exist_ok=True)
        self.persistent_root.mkdir(parents=True, exist_ok=True)
        self.staging_root.mkdir(parents=True, exist_ok=True)
        self.runtime_logs_root.mkdir(parents=True, exist_ok=True)
        if self.sband is not None:
            self.sband.start(self.root_dir)
        if self.uhf is not None:
            self.uhf.start(self.root_dir)

        common_env = os.environ.copy()
        common_env["CSP_TRANSPORT"] = "zmqhub"
        common_env["CSP_HUB_HOST"] = "127.0.0.1"
        common_env["CSP_HUB_SUB_PORT"] = str(self.csp_sub_port)
        common_env["CSP_HUB_PUB_PORT"] = str(self.csp_pub_port)
        common_env["EPS_CSP_NODE_ID"] = str(DEFAULT_EPS_CSP_NODE_ID)
        common_env["ADCS_CSP_NODE_ID"] = str(DEFAULT_ADCS_CSP_NODE_ID)
        eps_args = [str(self.bin_dir / "eps_simulator"), "--node-id", str(DEFAULT_EPS_CSP_NODE_ID)]
        eps_control_socket = os.environ.get("EPS_SIM_CONTROL_SOCKET", "").strip()
        if eps_control_socket:
            eps_args.extend(["--control-socket", eps_control_socket])

        self._start_process(
            "csp_zmqproxy",
            [
                str(self.bin_dir / "csp_zmqproxy"),
                "-s",
                f"tcp://0.0.0.0:{self.csp_sub_port}",
                "-p",
                f"tcp://0.0.0.0:{self.csp_pub_port}",
            ],
            self.csp_proxy_log,
            env=common_env,
            stale_match_groups=((f"tcp://0.0.0.0:{self.csp_sub_port}", f"tcp://0.0.0.0:{self.csp_pub_port}"),),
            stale_match_markers=("csp_zmqproxy",),
        )
        self._start_process(
            "eps_simulator",
            eps_args,
            self.eps_log,
            env=common_env,
            stale_match_groups=((f"--node-id {DEFAULT_EPS_CSP_NODE_ID}",),),
            stale_match_markers=("eps_simulator",),
            stale_require_orphan=True,
        )
        self._start_process(
            "adcs_simulator",
            [str(self.bin_dir / "adcs_simulator"), "--node-id", str(DEFAULT_ADCS_CSP_NODE_ID)],
            self.adcs_log,
            env=common_env,
            stale_match_groups=((f"--node-id {DEFAULT_ADCS_CSP_NODE_ID}",),),
            stale_match_markers=("adcs_simulator",),
            stale_require_orphan=True,
        )
        self._start_process(
            "radio_mock_server",
            [str(self.bin_dir / "radio_mock_server"), "--port", str(self.radio_port)],
            self.radio_log,
            env=common_env,
            stale_match_groups=((f"--port {self.radio_port}",),),
            stale_match_markers=("radio_mock_server",),
        )

        if self.start_sband_runtime:
            self._start_process(
                "sband_comm_csp_node",
                [
                    str(self.bin_dir / "sband_comm_csp_node"),
                    "--tcp-listen-host",
                    self.sband_tcp_host,
                    "--tcp-listen-port",
                    str(self.sband_tcp_port),
                    "--node-id",
                    "5",
                ],
                self.sband_process_log,
                env=common_env,
                stale_match_groups=((f"--tcp-listen-port {self.sband_tcp_port}", "--node-id 5"),),
                stale_match_markers=("sband_comm_csp_node",),
            )
            wait_port(self.sband_tcp_port, READY_TIMEOUT_SEC)

        if self.start_uhf_runtime:
            self._start_pty_bridge()
            assert self.uhf_comm_serial is not None
            uhf_comm_args = [
                str(self.bin_dir / "uhf_comm_csp_node"),
                "--serial-device",
                self.uhf_comm_serial,
                "--baudrate",
                str(DEFAULT_BAUDRATE),
                "--node-id",
                "6",
            ]
            if self.enable_uhf_beacon_side_channel:
                assert self.uhf_beacon_peer is not None
                uhf_comm_args.extend(
                    [
                        "--beacon-serial-device",
                        self.uhf_beacon_peer.slave_path,
                        "--beacon-baudrate",
                        str(DEFAULT_BAUDRATE),
                    ]
                )
            self._start_process(
                "uhf_comm_csp_node",
                uhf_comm_args,
                self.uhf_process_log,
                env=common_env,
                stale_match_groups=((f"--serial-device {self.uhf_comm_serial}", "--node-id 6"),),
                stale_match_markers=("uhf_comm_csp_node",),
            )
            wait_text(self.uhf_process_log, "node=6", READY_TIMEOUT_SEC)
            time.sleep(0.5)

        if self.sband is not None:
            self._start_process(
                "sband_ground_ttc_gateway",
                [
                    str(self.bin_dir / "ground_ttc_gateway"),
                    "--rf-tcp-host",
                    self.sband_tcp_host,
                    "--rf-tcp-port",
                    str(self.sband_tcp_port),
                    "--link-identity",
                    "sband",
                    "--gds-host",
                    self.gds_bind_host,
                    "--gds-port",
                    str(self.sband.gds_port),
                    "--capture-gds-to-southbound",
                    str(self.captures_root / "sband-gds-to-southbound.bin"),
                    "--capture-southbound-to-gds",
                    str(self.captures_root / "sband-southbound-to-gds.bin"),
                ],
                self.sband_gateway_log,
                env=common_env,
                stale_match_groups=((f"--rf-tcp-port {self.sband_tcp_port}", f"--gds-port {self.sband.gds_port}", "--link-identity sband"),),
                stale_match_markers=("ground_ttc_gateway",),
            )
            wait_text(self.sband_gateway_log, "gds-connected", READY_TIMEOUT_SEC)
            wait_text(self.sband_gateway_log, "southbound-opened", READY_TIMEOUT_SEC)

        if self.uhf is not None:
            assert self.uhf_gateway_serial is not None
            self._start_process(
                "uhf_ground_ttc_gateway",
                [
                    str(self.bin_dir / "ground_ttc_gateway"),
                    "--serial-device",
                    self.uhf_gateway_serial,
                    "--baudrate",
                    str(DEFAULT_BAUDRATE),
                    "--link-identity",
                    "uhf",
                    "--gds-host",
                    self.gds_bind_host,
                    "--gds-port",
                    str(self.uhf.gds_port),
                    "--capture-gds-to-southbound",
                    str(self.captures_root / "uhf-gds-to-southbound.bin"),
                    "--capture-southbound-to-gds",
                    str(self.captures_root / "uhf-southbound-to-gds.bin"),
                ],
                self.uhf_gateway_log,
                env=common_env,
                stale_match_groups=((f"--serial-device {self.uhf_gateway_serial}", f"--gds-port {self.uhf.gds_port}", "--link-identity uhf"),),
                stale_match_markers=("ground_ttc_gateway",),
            )
            wait_text(self.uhf_gateway_log, "gds-connected", READY_TIMEOUT_SEC)
            wait_text(self.uhf_gateway_log, "southbound-opened", READY_TIMEOUT_SEC)

        obc_env = common_env.copy()
        obc_env["RADIO_PORT"] = str(self.radio_port)
        obc_args = [
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
            "--runtime-root",
            str(self.runtime_root),
            "--persistent-root",
            str(self.persistent_root),
            "--staging-root",
            str(self.staging_root),
            "--command-authority-profile",
            self.command_authority_profile,
            "--tick-ms",
            str(self.tick_ms),
            "--headless",
        ]
        if self.enable_uhf_beacon_side_channel and self.uhf_beacon_peer is not None:
            obc_args += ["--uhf-beacon-csp-node", "6"]
        self._start_process(
            "OBC",
            obc_args,
            self.obc_log,
            env=obc_env,
            stale_match_groups=((str(self.runtime_root),),),
            stale_match_markers=("OBC",),
        )
        wait_text(self.obc_log, "Runtime mode: headless", READY_TIMEOUT_SEC)
        wait_text(self.obc_log, "Ground link via COMM CSP node: 5", READY_TIMEOUT_SEC)
        if self.start_sband_runtime:
            wait_text(self.sband_process_log, "node=5", READY_TIMEOUT_SEC)
            wait_text(self.obc_log, "Comm link SBAND", READY_TIMEOUT_SEC)
        if self.start_uhf_runtime:
            wait_text(self.obc_log, "Comm link UHF", READY_TIMEOUT_SEC)
        self.write_manifest()

    def stop(self) -> None:
        cleanup_managed_processes(self.processes, timeout_sec=5.0)
        if self.sband is not None:
            self.sband.stop()
        if self.uhf is not None:
            self.uhf.stop()
        if self.uhf_beacon_peer is not None:
            self.uhf_beacon_peer.close()
            self.uhf_beacon_peer = None
        self.uhf_gateway_serial = None
        self.uhf_comm_serial = None

    def stop_process(self, name: str) -> None:
        survivors: list[ManagedProcess] = []
        matched: list[ManagedProcess] = []
        for managed in self.processes:
            if managed.name == name:
                matched.append(managed)
            else:
                survivors.append(managed)
        self.processes = survivors
        cleanup_managed_processes(matched, timeout_sec=5.0)

    def run_until_stopped(self, hold_seconds: float | None = None) -> None:
        self.start()
        self.print_manifest()
        if hold_seconds is not None and hold_seconds > 0:
            time.sleep(hold_seconds)
            return
        while True:
            time.sleep(1.0)

    def manifest(self) -> dict[str, Any]:
        non_claims = [
            "no simultaneous dual-link runtime arbitration claim",
            "no one stock GDS heterogeneous multi-upstream claim",
            "no one gateway simultaneous multiplexer claim",
            "no target-bearing simultaneous S-band plus UHF proof",
            "no RF closure",
            "no UHF reliable transfer redesign",
        ]
        surfaces: dict[str, Any] = {}
        if self.sband is not None:
            surfaces["sband"] = self.sband.manifest(
                role="nominal high-authority operator path",
                command_class_role="full formal S-band TT&C path",
                observability_role="formal events, channels, and official file/data-product visibility while S-band remains primary",
                southbound={
                    "kind": "tcp",
                    "endpoint": f"{self.sband_tcp_host}:{self.sband_tcp_port}",
                    "commNode": 5,
                    "gatewayLog": str(self.sband_gateway_log),
                },
                capture_paths={
                    "gdsToSouthbound": str(self.captures_root / "sband-gds-to-southbound.bin"),
                    "southboundToGds": str(self.captures_root / "sband-southbound-to-gds.bin"),
                },
            )
        if self.uhf is not None:
            surfaces["uhf"] = self.uhf.manifest(
                role="bounded backup ingress until explicit switch",
                command_class_role="current maintained node-6 allowlisted read/status ingress only; high-authority and target-bearing simultaneous claims remain deferred",
                observability_role="separate stock GDS operator surface for bounded backup command/readback review; UHF beacon side-channel remains distinct from the stock GDS surface",
                southbound={
                    "kind": "serial",
                    "gatewaySerialDevice": self.uhf_gateway_serial,
                    "commSerialPeer": self.uhf_comm_serial,
                    "beaconSerialDevice": None if self.uhf_beacon_peer is None else self.uhf_beacon_peer.slave_path,
                    "baudrate": DEFAULT_BAUDRATE,
                    "commNode": 6,
                    "gatewayLog": str(self.uhf_gateway_log),
                },
                capture_paths={
                    "gdsToSouthbound": str(self.captures_root / "uhf-gds-to-southbound.bin"),
                    "southboundToGds": str(self.captures_root / "uhf-southbound-to-gds.bin"),
                },
            )

        return {
            "formalChange": "per-band-stock-ground-stacks-v1",
            "mode": self.mode_name,
            "hostedOnly": True,
            "requestedRuntimeRoot": None if self.requested_runtime_root is None else str(self.requested_runtime_root),
            "sharedHostedRuntime": str(self.runtime_root),
            "stackRoot": str(self.stack_root),
            "persistentRoot": str(self.persistent_root),
            "stagingRoot": str(self.staging_root),
            "logsRoot": str(self.log_root),
            "capturesRoot": str(self.captures_root),
            "commandAuthorityProfile": self.command_authority_profile,
            "gdsUiMode": self.gds_ui_mode,
            "passiveListenersEnabled": self.enable_passive_listeners,
            "primaryAuthorityTruth": "S-band remains the configured nominal high-authority path",
            "uhfAuthorityTruth": "UHF node 6 remains bounded backup ingress until a separate explicit switch or future orchestration change proves otherwise",
            "combinedWrapperTruth": "combined mode only starts/stops the maintained stock stacks together; it does not claim a new policy owner or orchestrator",
            "startupOrder": self.startup_order(),
            "shutdownOrder": self.shutdown_order(),
            "operatorSurfaces": surfaces,
            "internalOnlyRuntime": {
                "sbandCommNodePresent": self.start_sband_runtime,
                "uhfCommNodePresent": self.start_uhf_runtime,
                "cspHubSubPort": self.csp_sub_port,
                "cspHubPubPort": self.csp_pub_port,
                "radioMockPort": self.radio_port,
                "sbandCommTcpPort": self.sband_tcp_port if self.start_sband_runtime else None,
                "sharedOBCLog": str(self.obc_log),
                "cspProxyLog": str(self.csp_proxy_log),
                "epsSimulatorLog": str(self.eps_log),
                "adcsSimulatorLog": str(self.adcs_log),
                "radioMockLog": str(self.radio_log),
                "ptyBridgeLog": str(self.pty_log) if self.start_uhf_runtime else None,
                "uhfBeaconSideChannelEnabled": self.enable_uhf_beacon_side_channel,
            },
            "nonClaims": non_claims,
        }

    def write_manifest(self) -> pathlib.Path:
        manifest = self.manifest()
        self.manifest_path.parent.mkdir(parents=True, exist_ok=True)
        self.manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        return self.manifest_path

    def print_manifest(self) -> None:
        manifest = self.manifest()
        print("per-band-stock-ground-stacks: READY")
        print(f"mode={manifest['mode']}")
        print(f"stack-root={manifest['stackRoot']}")
        print(f"shared-runtime-root={manifest['sharedHostedRuntime']}")
        print(f"manifest-json={self.manifest_path}")
        print(f"startup-order={' | '.join(manifest['startupOrder'])}")
        print(f"shutdown-order={' | '.join(manifest['shutdownOrder'])}")
        print(f"primary-authority={manifest['primaryAuthorityTruth']}")
        print(f"uhf-authority={manifest['uhfAuthorityTruth']}")
        if "sband" in manifest["operatorSurfaces"]:
            surface = manifest["operatorSurfaces"]["sband"]
            print(
                "sband-surface="
                f"gds={surface['gdsBind']} "
                f"tts={surface['gdsTtsPort']} "
                f"southbound={surface['southbound']['kind']}:{surface['southbound']['endpoint']} "
                f"file-store={surface['fileStorageDir']}"
            )
        if "uhf" in manifest["operatorSurfaces"]:
            surface = manifest["operatorSurfaces"]["uhf"]
            print(
                "uhf-surface="
                f"gds={surface['gdsBind']} "
                f"tts={surface['gdsTtsPort']} "
                f"southbound={surface['southbound']['kind']}:{surface['southbound']['gatewaySerialDevice']} "
                f"file-store={surface['fileStorageDir']}"
            )
        print(f"combined-wrapper-truth={manifest['combinedWrapperTruth']}")
        for non_claim in manifest["nonClaims"]:
            print(f"non-claim={non_claim}")

    def _start_process(
        self,
        name: str,
        args: Sequence[str],
        log_path: pathlib.Path,
        *,
        env: dict[str, str] | None = None,
        stale_match_groups: Sequence[Sequence[str]] = (),
        stale_match_markers: Sequence[str] = (),
        stale_require_orphan: bool = False,
    ) -> ManagedProcess:
        managed = start_managed_process(
            name,
            args,
            log_path,
            env=env,
            cwd=self.process_cwd,
            stale_match_groups=stale_match_groups,
            stale_match_markers=stale_match_markers or (name,),
            stale_require_orphan=stale_require_orphan,
        )
        self.processes.append(managed)
        return managed

    def _start_pty_bridge(self) -> None:
        pty_path = str(self.bin_dir / "pty_pair_bridge")
        instance_label = self.pty_instance_label
        reap_matching_processes(((pty_path, f"--instance-label {instance_label}"),), markers=("pty_pair_bridge",))
        handle = self.pty_log.open("a", encoding="utf-8", buffering=1)
        handle.write(f"$ {pty_path} --instance-label {instance_label}\n")
        handle.flush()
        process = subprocess.Popen(
            [pty_path, "--instance-label", instance_label],
            cwd=str(self.process_cwd),
            env=os.environ.copy(),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            bufsize=1,
        )
        if process.stdout is None:
            handle.close()
            raise RuntimeError("pty_pair_bridge did not expose stdout")
        pty_paths: dict[str, str] = {}
        selector = selectors.DefaultSelector()
        buffer = b""
        try:
            selector.register(process.stdout, selectors.EVENT_READ)
            while len(pty_paths) < 2:
                ready = selector.select(READY_TIMEOUT_SEC)
                if not ready:
                    process.kill()
                    process.wait(timeout=5.0)
                    handle.close()
                    raise RuntimeError("timed out waiting for pty_pair_bridge to report PTY paths")
                chunk = os.read(process.stdout.fileno(), 4096)
                if not chunk:
                    process.kill()
                    process.wait(timeout=5.0)
                    handle.close()
                    raise RuntimeError("pty_pair_bridge did not report PTY paths")
                buffer += chunk
                while b"\n" in buffer and len(pty_paths) < 2:
                    raw_line, buffer = buffer.split(b"\n", 1)
                    line = raw_line.decode("utf-8", "replace") + "\n"
                    handle.write(line)
                    key, value = line.strip().split("=", 1)
                    pty_paths[key] = value
        finally:
            selector.close()
        self.uhf_gateway_serial = pty_paths["PTY_A"]
        self.uhf_comm_serial = pty_paths["PTY_B"]
        self.processes.append(
            ManagedProcess(
                name="pty_pair_bridge",
                process=process,
                handle=handle,
                stale_match_groups=((pty_path, f"--instance-label {instance_label}"),),
                stale_match_markers=("pty_pair_bridge",),
            )
        )


def default_stack_root(root_dir: pathlib.Path, mode: str) -> pathlib.Path:
    return pathlib.Path(os.environ.get("PER_BAND_STACK_ROOT", f"/tmp/per-band-stock-ground-stacks-v1/{mode}"))


def build_runtime(mode: str, args: argparse.Namespace) -> HostedPerBandStockStacks:
    root_dir = pathlib.Path(args.root_dir).resolve()
    bin_dir = pathlib.Path(args.bin_dir).resolve()
    dictionary_path = pathlib.Path(args.dictionary_path).resolve()
    cli_path = args.cli_path or find_fprime_cli(root_dir)
    stack_root = pathlib.Path(args.stack_root).resolve()
    runtime_root = None if args.runtime_root is None else pathlib.Path(args.runtime_root).resolve()

    expose_sband_surface = mode in {"sband", "combined"}
    expose_uhf_surface = mode in {"uhf", "combined"}
    preserve_sband_primary = parse_bool(args.preserve_sband_primary, default=True)
    enable_uhf_beacon_side_channel = parse_bool(args.enable_uhf_beacon_side_channel, default=False)
    if mode == "sband":
        preserve_sband_primary = True
    if mode == "combined":
        preserve_sband_primary = True

    return HostedPerBandStockStacks(
        mode_name=mode,
        root_dir=root_dir,
        bin_dir=bin_dir,
        dictionary_path=dictionary_path,
        cli_path=cli_path,
        stack_root=stack_root,
        runtime_root=runtime_root,
        expose_sband_surface=expose_sband_surface,
        expose_uhf_surface=expose_uhf_surface,
        preserve_sband_primary=preserve_sband_primary,
        enable_uhf_beacon_side_channel=enable_uhf_beacon_side_channel,
        auto_ports=args.auto_ports,
        gds_bind_host=args.gds_bind_host,
        sband_gds_port=args.sband_gds_port,
        sband_gds_tts_port=args.sband_gds_tts_port,
        uhf_gds_port=args.uhf_gds_port,
        uhf_gds_tts_port=args.uhf_gds_tts_port,
        sband_gui_port=args.sband_gui_port,
        uhf_gui_port=args.uhf_gui_port,
        csp_sub_port=args.csp_sub_port,
        csp_pub_port=args.csp_pub_port,
        radio_port=args.radio_port,
        sband_tcp_port=args.sband_tcp_port,
        command_authority_profile=args.command_authority_profile,
        tick_ms=args.tick_ms,
        enable_passive_listeners=parse_bool(args.enable_passive_listeners, default=True),
        gds_ui_mode=args.gds_ui_mode,
    )


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    root_dir = pathlib.Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Run the maintained hosted per-band stock ground stacks.")
    parser.add_argument("--mode", choices=("sband", "uhf", "combined"), required=True)
    parser.add_argument("--root-dir", default=str(root_dir))
    parser.add_argument("--bin-dir", required=True)
    parser.add_argument("--dictionary-path", required=True)
    parser.add_argument("--cli-path")
    parser.add_argument("--stack-root")
    parser.add_argument("--runtime-root", default=os.environ.get("PER_BAND_RUNTIME_ROOT"))
    parser.add_argument("--hold-seconds", type=float, default=(float(os.environ["STACK_HOLD_SECS"]) if os.environ.get("STACK_HOLD_SECS") else None))
    parser.add_argument("--gds-bind-host", default=os.environ.get("PER_BAND_GDS_BIND_HOST", DEFAULT_GDS_BIND_HOST))
    parser.add_argument("--sband-gds-port", type=int, default=int(os.environ.get("SBAND_GDS_PORT", str(DEFAULT_SBAND_GDS_PORT))))
    parser.add_argument("--sband-gds-tts-port", type=int, default=int(os.environ.get("SBAND_GDS_TTS_PORT", str(DEFAULT_SBAND_GDS_TTS_PORT))))
    parser.add_argument("--uhf-gds-port", type=int, default=int(os.environ.get("UHF_GDS_PORT", str(DEFAULT_UHF_GDS_PORT))))
    parser.add_argument("--uhf-gds-tts-port", type=int, default=int(os.environ.get("UHF_GDS_TTS_PORT", str(DEFAULT_UHF_GDS_TTS_PORT))))
    parser.add_argument("--sband-gui-port", type=int, default=int(os.environ.get("SBAND_GDS_GUI_PORT", str(DEFAULT_SBAND_GUI_PORT))))
    parser.add_argument("--uhf-gui-port", type=int, default=int(os.environ.get("UHF_GDS_GUI_PORT", str(DEFAULT_UHF_GUI_PORT))))
    parser.add_argument("--csp-sub-port", type=int, default=int(os.environ.get("CSP_HUB_SUB_PORT", str(DEFAULT_CSP_SUB_PORT))))
    parser.add_argument("--csp-pub-port", type=int, default=int(os.environ.get("CSP_HUB_PUB_PORT", str(DEFAULT_CSP_PUB_PORT))))
    parser.add_argument("--radio-port", type=int, default=int(os.environ.get("RADIO_PORT", str(DEFAULT_RADIO_PORT))))
    parser.add_argument("--sband-tcp-port", type=int, default=int(os.environ.get("SBAND_TCP_PORT", str(DEFAULT_SBAND_TCP_PORT))))
    parser.add_argument("--command-authority-profile", default=os.environ.get("COMMAND_AUTHORITY_PROFILE", "sband-primary"))
    parser.add_argument("--tick-ms", type=int, default=int(os.environ.get("TICK_MS", str(DEFAULT_TICK_MS))))
    parser.add_argument("--preserve-sband-primary", default=os.environ.get("PRESERVE_SBAND_PRIMARY"))
    parser.add_argument("--enable-uhf-beacon-side-channel", default=os.environ.get("ENABLE_UHF_BEACON_SIDE_CHANNEL"))
    parser.add_argument("--auto-ports", action="store_true", default=parse_bool(os.environ.get("PER_BAND_AUTO_PORTS"), False))
    parser.add_argument("--enable-passive-listeners", default=os.environ.get("PER_BAND_ENABLE_PASSIVE_LISTENERS"))
    parser.add_argument("--gds-ui-mode", choices=("ui", "headless"), default=os.environ.get("GDS_UI_MODE", "headless"))
    args = parser.parse_args(list(argv))
    if args.stack_root is None:
        args.stack_root = str(default_stack_root(root_dir, args.mode))
    return args


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    stack = build_runtime(args.mode, args)

    cleaned = False

    def cleanup() -> None:
        nonlocal cleaned
        if cleaned:
            return
        cleaned = True
        stack.stop()

    install_signal_cleanup(cleanup)
    try:
        stack.run_until_stopped(hold_seconds=args.hold_seconds)
    finally:
        cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
