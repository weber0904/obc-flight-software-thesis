from __future__ import annotations

import argparse
import signal
import os
import pathlib
import shutil
import subprocess
import sys
import time
import uuid
from typing import Any

from common import (
    FPRIME_VENV_BIN,
    ROOT_DIR,
    apply_service_override,
    atomic_write_json,
    ensure_dir,
    find_free_port,
    install_termination_handler,
    pid_alive,
    read_json,
    remove_service_override,
    reserve_free_port,
    safe_remove,
    service_environment,
    service_invocation_id,
    ssh_capture,
    ssh_capture_bytes,
    shell_join,
    wait_remote_journal_fragments,
    wait_remote_service_active,
    utc_timestamp,
    wait_port,
)

SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from per_band_stock_ground_stacks import (  # type: ignore
    DEFAULT_GDS_BIND_HOST,
    build_runtime,
    find_fprime_cli,
    wait_text,
)
from probe_process_utils import (  # type: ignore
    ManagedProcess,
    build_gds_stale_match_groups,
    build_gateway_stale_match_groups,
    cleanup_managed_processes,
    reap_matching_processes,
    start_managed_process,
    wait_for_no_matching_processes,
)


READY_TIMEOUT_SEC = 30.0
BEACON_FRAME_SIZE = 108
DEFAULT_HOSTED_OWNER_ROOT = pathlib.Path("/tmp/manual-dual-gds/hosted")
DEFAULT_TARGET_GROUND_OWNER_ROOT = pathlib.Path("/tmp/manual-dual-gds/target-ground")
DEFAULT_TARGET_BASELINE_ROOT = pathlib.Path("/tmp/manual-dual-gds/target-baseline")

DEFAULT_OBC_GROUNDLINK_DIAGNOSTICS_DROPIN = "55-obc-groundlink-diagnostics.conf"
DEFAULT_OBC_GROUNDLINK_TIMEOUTS_DROPIN = "56-obc-groundlink-timeouts.conf"
DEFAULT_CSP_SOCKETCAN_CANFD_DROPIN = "57-csp-socketcan-canfd.conf"
DEFAULT_SBAND_INGRESS_DIAGNOSTICS_DROPIN = "58-sband-ingress-diagnostics.conf"
TARGET_SBAND_STATIC_READY_FRAGMENTS = (
    "OBC CCSDS S-band runtime started.",
    "Ground link via COMM CSP node: 5",
    "CSP initialized for node 1",
)
TARGET_SBAND_AVAILABILITY_FRAGMENT = "COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available "
TARGET_SBAND_GROUND_LINK_UP_FRAGMENTS = ("groundLinkDriver) GROUND_LINK_UP",)


def root_dict_path() -> pathlib.Path:
    candidate = ROOT_DIR / "build-artifacts"
    for path in candidate.glob("*/OBC/dict/AppTopologyDictionary.json"):
        return path
    raise RuntimeError("Dictionary not found. Run PATH=\"$PWD/fprime-venv/bin:$PATH\" fprime-util build first.")


def root_bin_dir() -> pathlib.Path:
    candidate_root = ROOT_DIR / "build-fprime-automatic-native" / "bin"
    if not candidate_root.exists():
        raise RuntimeError("Native build output not found. Run PATH=\"$PWD/fprime-venv/bin:$PATH\" fprime-util build first.")
    for path in candidate_root.iterdir():
        if path.is_dir() and (path / "OBC").exists() and (path / "ground_ttc_gateway").exists():
            return path
    raise RuntimeError("Native build output not found. Run PATH=\"$PWD/fprime-venv/bin:$PATH\" fprime-util build first.")


def lifecycle_paths(owner_root: pathlib.Path) -> dict[str, pathlib.Path]:
    return {
        "manifest": owner_root / "manifest.json",
        "status": owner_root / "status.json",
        "pid": owner_root / "owner.pid",
    }


def terminal_surface_payload(
    payload: dict[str, Any],
    lifecycle_state: str,
    *,
    failure: str | None = None,
) -> dict[str, Any]:
    terminal = dict(payload)
    terminal["lifecycleState"] = lifecycle_state
    terminal["timestamp"] = utc_timestamp()
    if failure is None:
        terminal.pop("failure", None)
    else:
        terminal["failure"] = failure
    return terminal


def write_terminal_surface_state(
    owner_root: pathlib.Path,
    payload: dict[str, Any],
    *,
    lifecycle_state: str = "stopped",
    failure: str | None = None,
) -> None:
    terminal = terminal_surface_payload(payload, lifecycle_state, failure=failure)
    paths = lifecycle_paths(owner_root)
    atomic_write_json(paths["manifest"], terminal)
    atomic_write_json(paths["status"], terminal)
    safe_remove(paths["pid"])


def owner_pid_from_payload(owner_root: pathlib.Path, payload: dict[str, Any]) -> int | None:
    pid_path = lifecycle_paths(owner_root)["pid"]
    if pid_path.exists():
        try:
            return int(pid_path.read_text(encoding="utf-8").strip())
        except ValueError:
            pass
    owner_pid = payload.get("ownerPid")
    return owner_pid if isinstance(owner_pid, int) else None


def terminate_owner_pid(pid: int | None, *, timeout_sec: float = 8.0) -> None:
    if pid is None or not pid_alive(pid):
        return
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if not pid_alive(pid):
            return
        time.sleep(0.2)
    try:
        os.kill(pid, signal.SIGKILL)
    except ProcessLookupError:
        return
    deadline = time.time() + 2.0
    while time.time() < deadline and pid_alive(pid):
        time.sleep(0.1)


def target_ground_reap_specs(payload: dict[str, Any]) -> list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]]:
    specs: list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]] = []
    surfaces = payload.get("operatorSurfaces", {})
    for band_name, link_identity in (("sband", "sband"), ("uhf", "uhf")):
        surface = surfaces.get(band_name)
        if not isinstance(surface, dict):
            continue
        gds_port = surface.get("gdsPort")
        gds_tts_port = surface.get("gdsTtsPort")
        file_storage_dir = surface.get("fileStorageDir")
        if gds_port is None:
            continue
        specs.append(
            (
                build_gds_stale_match_groups(
                    ip_port=gds_port,
                    tts_port=gds_tts_port,
                    file_storage_dir=file_storage_dir,
                ),
                ("fprime-gds", "fprime_gds.executables.comm", "fprime_gds.executables.tcpserver", "CustomDataHandlers"),
                False,
            )
        )
        specs.append(
            (
                build_gateway_stale_match_groups(gds_port=gds_port, link_identity=link_identity),
                ("ground_ttc_gateway",),
                False,
            )
        )
    return specs


def hosted_reap_specs(payload: dict[str, Any]) -> list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]]:
    specs: list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]] = []
    surfaces = payload.get("operatorSurfaces", {})
    for band_name, link_identity in (("sband", "sband"), ("uhf", "uhf")):
        surface = surfaces.get(band_name)
        if not isinstance(surface, dict):
            continue
        gds_port = surface.get("gdsPort")
        gds_tts_port = surface.get("gdsTtsPort")
        file_storage_dir = surface.get("fileStorageDir")
        if gds_port is not None:
            specs.append(
                (
                    build_gds_stale_match_groups(
                        ip_port=gds_port,
                        tts_port=gds_tts_port,
                        file_storage_dir=file_storage_dir,
                    ),
                    ("fprime-gds", "fprime_gds.executables.comm", "fprime_gds.executables.tcpserver", "CustomDataHandlers"),
                    False,
                )
            )
        southbound = surface.get("southbound", {})
        if isinstance(southbound, dict) and gds_port is not None:
            southbound_fragment = None
            if southbound.get("kind") == "tcp":
                endpoint = southbound.get("endpoint")
                if isinstance(endpoint, str) and ":" in endpoint:
                    southbound_fragment = f"--rf-tcp-port {endpoint.rsplit(':', 1)[1]}"
            elif southbound.get("kind") == "serial":
                gateway_serial = southbound.get("gatewaySerialDevice")
                if gateway_serial:
                    southbound_fragment = f"--serial-device {gateway_serial}"
            specs.append(
                (
                    build_gateway_stale_match_groups(
                        gds_port=gds_port,
                        link_identity=link_identity,
                        southbound_fragment=southbound_fragment,
                    ),
                    ("ground_ttc_gateway",),
                    False,
                )
            )
            if band_name == "uhf":
                comm_serial = southbound.get("commSerialPeer")
                if comm_serial:
                    specs.append((((f"--serial-device {comm_serial}", "--node-id 6"),), ("uhf_comm_csp_node",), False))

    internal = payload.get("internalOnlyRuntime", {})
    if isinstance(internal, dict):
        csp_sub_port = internal.get("cspHubSubPort")
        csp_pub_port = internal.get("cspHubPubPort")
        if csp_sub_port is not None and csp_pub_port is not None:
            specs.append((((f"tcp://0.0.0.0:{csp_sub_port}", f"tcp://0.0.0.0:{csp_pub_port}"),), ("csp_zmqproxy",), False))
        radio_port = internal.get("radioMockPort")
        if radio_port is not None:
            specs.append((((f"--port {radio_port}",),), ("radio_mock_server",), False))
        sband_tcp_port = internal.get("sbandCommTcpPort")
        if sband_tcp_port is not None:
            specs.append((((f"--tcp-listen-port {sband_tcp_port}", "--node-id 5"),), ("sband_comm_csp_node",), False))
    shared_runtime = payload.get("sharedHostedRuntime")
    if shared_runtime:
        specs.append((((str(shared_runtime),),), ("OBC",), False))

    specs.append((((f"--node-id 2",),), ("eps_simulator",), True))
    specs.append((((f"--node-id 3",),), ("adcs_simulator",), True))
    return specs


def reap_surface_specs(
    specs: list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]],
) -> None:
    for match_groups, markers, require_orphan in specs:
        if not match_groups:
            continue
        reap_matching_processes(match_groups, markers=markers, require_orphan=require_orphan, term_wait_sec=0.2)


def _surface_residue_matches(
    specs: list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]],
    *,
    timeout_sec: float = 6.0,
) -> list[str]:
    residue: list[str] = []
    deadline = time.monotonic() + timeout_sec
    for match_groups, markers, require_orphan in specs:
        if not match_groups:
            continue
        remaining_sec = max(0.0, deadline - time.monotonic())
        survivors = wait_for_no_matching_processes(
            match_groups,
            markers=markers,
            require_orphan=require_orphan,
            timeout_sec=remaining_sec,
        )
        if survivors:
            residue.extend(f"pid={match.pid} command={match.command}" for match in survivors)
    return residue


def ensure_surface_processes_stopped(
    specs: list[tuple[tuple[tuple[str, ...], ...], tuple[str, ...], bool]],
    *,
    timeout_sec: float = 6.0,
) -> None:
    residue = _surface_residue_matches(specs, timeout_sec=timeout_sec)
    if residue:
        raise RuntimeError(
            "surface still owns live processes after cleanup: " + " | ".join(residue[:6])
        )


def remote_journal_fragment_count(
    target: str,
    unit: str,
    fragment: str,
    *,
    invocation_id: str | None = None,
) -> int:
    if invocation_id:
        journal = ssh_capture(
            target,
            f"journalctl _SYSTEMD_INVOCATION_ID={shell_join([invocation_id])} --no-pager || true",
            check=False,
        )
    else:
        journal = ssh_capture(target, f"journalctl -u {shell_join([unit])} --no-pager || true", check=False)
    return sum(1 for line in journal.splitlines() if fragment in line)


def latest_link_state_from_journal(
    journal: str,
    stem: str,
    *,
    invocation_scoped: bool,
    availability_fragment: str | None = None,
) -> str | None:
    for line in reversed(journal.splitlines()):
        if not invocation_scoped and "OBC CCSDS S-band runtime started." in line:
            break
        if availability_fragment is not None:
            if availability_fragment in line:
                if "available 1" in line:
                    return "UP"
                if "available 0" in line:
                    return "DOWN"
                return None
            continue
        if f"{stem}) GROUND_LINK_UP" in line:
            return "UP"
        if f"{stem}) GROUND_LINK_DOWN" in line:
            return "DOWN"
    return None


def remote_latest_link_state(
    target: str,
    unit: str,
    stem: str,
    *,
    invocation_id: str | None = None,
    availability_fragment: str | None = None,
) -> str | None:
    if invocation_id:
        journal = ssh_capture(
            target,
            f"journalctl _SYSTEMD_INVOCATION_ID={shell_join([invocation_id])} --no-pager || true",
            check=False,
        )
    else:
        journal = ssh_capture(target, f"journalctl -u {shell_join([unit])} --no-pager || true", check=False)
    return latest_link_state_from_journal(
        journal,
        stem,
        invocation_scoped=bool(invocation_id),
        availability_fragment=availability_fragment,
    )


def wait_remote_latest_link_state(
    target: str,
    unit: str,
    stem: str,
    expected_state: str,
    timeout: float,
    *,
    invocation_id: str | None = None,
    availability_fragment: str | None = None,
) -> None:
    deadline = time.time() + timeout
    last_state: str | None = None
    while time.time() < deadline:
        current_invocation_id = service_invocation_id(target, unit) or invocation_id or None
        last_state = remote_latest_link_state(
            target,
            unit,
            stem,
            invocation_id=current_invocation_id,
            availability_fragment=availability_fragment,
        )
        if last_state == expected_state:
            return
        time.sleep(0.5)
    raise RuntimeError(
        f"timed out waiting for latest state {expected_state!r} on {target}:{unit}; last_state={last_state!r}"
    )


def wait_remote_fresh_link_state(
    target: str,
    unit: str,
    marker_fragment: str,
    stem: str,
    expected_state: str,
    timeout: float,
    *,
    baseline_invocation_id: str | None,
    baseline_count: int,
    availability_fragment: str | None = None,
) -> None:
    deadline = time.time() + timeout
    last_state: str | None = None
    last_invocation_id = baseline_invocation_id
    last_count = baseline_count
    while time.time() < deadline:
        current_invocation_id = service_invocation_id(target, unit) or None
        scoped_invocation_id = current_invocation_id or baseline_invocation_id
        current_count = remote_journal_fragment_count(
            target,
            unit,
            marker_fragment,
            invocation_id=scoped_invocation_id,
        )
        fresh_marker_seen = False
        if scoped_invocation_id and scoped_invocation_id != baseline_invocation_id:
            fresh_marker_seen = current_count > 0
        else:
            fresh_marker_seen = current_count > baseline_count
        if fresh_marker_seen:
            last_state = remote_latest_link_state(
                target,
                unit,
                stem,
                invocation_id=scoped_invocation_id,
                availability_fragment=availability_fragment,
            )
            if last_state == expected_state:
                return
        last_invocation_id = scoped_invocation_id
        last_count = current_count
        time.sleep(0.5)
    raise RuntimeError(
        f"timed out waiting for fresh {marker_fragment!r} with latest state {expected_state!r} "
        f"on {target}:{unit}; last_invocation_id={last_invocation_id!r} "
        f"baseline_count={baseline_count} last_count={last_count} last_state={last_state!r}"
    )


def stop_hosted_surface(owner_root: pathlib.Path) -> None:
    paths = lifecycle_paths(owner_root)
    source = paths["status"] if paths["status"].exists() else paths["manifest"]
    payload = read_json(source)
    terminate_owner_pid(owner_pid_from_payload(owner_root, payload))
    specs = hosted_reap_specs(payload)
    reap_surface_specs(specs)
    try:
        ensure_surface_processes_stopped(specs)
    except Exception as exc:
        write_terminal_surface_state(
            owner_root,
            payload,
            lifecycle_state="cleanup_failed",
            failure=str(exc),
        )
        raise
    write_terminal_surface_state(owner_root, payload)


def stop_target_ground_surface(owner_root: pathlib.Path) -> None:
    paths = lifecycle_paths(owner_root)
    source = paths["status"] if paths["status"].exists() else paths["manifest"]
    payload = read_json(source)
    terminate_owner_pid(owner_pid_from_payload(owner_root, payload))
    specs = target_ground_reap_specs(payload)
    reap_surface_specs(specs)
    try:
        ensure_surface_processes_stopped(specs)
    except Exception as exc:
        write_terminal_surface_state(
            owner_root,
            payload,
            lifecycle_state="cleanup_failed",
            failure=str(exc),
        )
        raise
    write_terminal_surface_state(owner_root, payload)


class HostedManualSurfaceOwner:
    def __init__(self, owner_root: pathlib.Path, gds_ui_mode: str, runtime_root: pathlib.Path | None, auto_ports: bool) -> None:
        self.owner_root = owner_root
        self.stack_root = owner_root / "stack"
        self.runtime_root = runtime_root
        self.gds_ui_mode = gds_ui_mode
        self.auto_ports = auto_ports
        self.paths = lifecycle_paths(owner_root)
        self.owner_pid = os.getpid()
        self.stack = None
        self.beacon_capture_path = self.owner_root / "beacons" / "hosted-uhf-beacon.bin"
        self.beacon_source_device: str | None = None

    def build_stack(self) -> Any:
        args = argparse.Namespace(
            mode="combined",
            root_dir=str(ROOT_DIR),
            bin_dir=str(root_bin_dir()),
            dictionary_path=str(root_dict_path()),
            cli_path=find_fprime_cli(ROOT_DIR),
            stack_root=str(self.stack_root),
            runtime_root=(None if self.runtime_root is None else str(self.runtime_root)),
            hold_seconds=None,
            gds_bind_host=DEFAULT_GDS_BIND_HOST,
            sband_gds_port=50150,
            sband_gds_tts_port=50151,
            uhf_gds_port=50160,
            uhf_gds_tts_port=50161,
            sband_gui_port=5000,
            uhf_gui_port=5001,
            csp_sub_port=56250,
            csp_pub_port=57250,
            radio_port=17050,
            sband_tcp_port=18520,
            command_authority_profile="sband-primary",
            tick_ms=1000,
            preserve_sband_primary="1",
            enable_uhf_beacon_side_channel="1",
            auto_ports=self.auto_ports,
            enable_passive_listeners=False,
            gds_ui_mode=self.gds_ui_mode,
        )
        return build_runtime("combined", args)

    def beacon_payload(self) -> dict[str, Any] | None:
        if not self.beacon_source_device:
            return None
        return {
            "supported": True,
            "sourceKind": "hosted-pty-side-channel",
            "sourceBand": "uhf-backup",
            "capturePath": str(self.beacon_capture_path),
            "frameSize": BEACON_FRAME_SIZE,
            "decodeTool": str(ROOT_DIR / "scripts" / "decode_beacon_v1.py"),
            "device": self.beacon_source_device,
        }

    def start_beacon_capture(self) -> None:
        assert self.stack is not None
        manifest = self.stack.manifest()
        uhf_surface = manifest["operatorSurfaces"]["uhf"]
        southbound = dict(uhf_surface.get("southbound", {}))
        device = southbound.get("beaconSerialDevice")
        if not device:
            return
        self.beacon_source_device = str(device)
        ensure_dir(self.beacon_capture_path.parent)
        if self.beacon_capture_path.exists():
            self.beacon_capture_path.unlink()

    def drain_beacon_capture(self) -> None:
        assert self.stack is not None
        peer = getattr(self.stack, "uhf_beacon_peer", None)
        if peer is None:
            return
        try:
            os.set_blocking(peer.master_fd, False)
        except OSError:
            return
        while True:
            try:
                chunk = os.read(peer.master_fd, 4096)
            except BlockingIOError:
                break
            if not chunk:
                break
            with self.beacon_capture_path.open("ab") as handle:
                handle.write(chunk)

    def stop_beacon_capture(self) -> None:
        return

    def payload(self, lifecycle_state: str, failure: str | None = None) -> dict[str, Any]:
        assert self.stack is not None
        stack_manifest = self.stack.manifest()
        payload = {
            "formalChange": "manual-dual-gds-secure-ops-surface-v1",
            "surfaceRoot": str(self.owner_root),
            "ownerPid": self.owner_pid,
            "lifecycleState": lifecycle_state,
            "surfaceType": "hosted-manual-dual-gds",
            "gdsUiMode": self.gds_ui_mode,
            "timestamp": utc_timestamp(),
            "dictionaryPath": str(root_dict_path()),
            "sharedHostedRuntime": stack_manifest["sharedHostedRuntime"],
            "stackRoot": stack_manifest["stackRoot"],
            "operatorSurfaces": {
                "sband": {
                    **stack_manifest["operatorSurfaces"]["sband"],
                    "canonicalBands": ["sband"],
                },
                "uhf": {
                    **stack_manifest["operatorSurfaces"]["uhf"],
                    "canonicalBands": ["uhf-backup", "uhf-primary-after-failover"],
                    "beacon": self.beacon_payload(),
                },
            },
            "internalOnlyRuntime": stack_manifest["internalOnlyRuntime"],
            "underlyingHostedManifestPath": str(self.stack.manifest_path),
            "startupOrder": stack_manifest["startupOrder"],
            "shutdownOrder": stack_manifest["shutdownOrder"],
            "nonClaims": stack_manifest["nonClaims"]
            + [
                "no GDS UI command interception",
                "secure auth and secure-v2 commands remain helper-owned",
            ],
        }
        if failure is not None:
            payload["failure"] = failure
        return payload

    def write_status(self, lifecycle_state: str, failure: str | None = None) -> None:
        payload = self.payload(lifecycle_state, failure=failure)
        atomic_write_json(self.paths["manifest"], payload)
        atomic_write_json(self.paths["status"], payload)
        self.paths["pid"].write_text(f"{self.owner_pid}\n", encoding="utf-8")

    def cleanup(self, *, write_terminal_state: bool = True) -> None:
        try:
            if self.stack is not None:
                payload = self.payload("running")
                try:
                    self.stop_beacon_capture()
                    self.stack.stop()
                    if write_terminal_state:
                        try:
                            ensure_surface_processes_stopped(hosted_reap_specs(payload))
                        except Exception as exc:
                            try:
                                self.write_status("cleanup_failed", failure=str(exc))
                            except Exception:
                                pass
                        else:
                            try:
                                self.write_status("stopped")
                            except Exception:
                                pass
                except Exception as exc:
                    if write_terminal_state:
                        try:
                            self.write_status("cleanup_failed", failure=str(exc))
                        except Exception:
                            pass
                    raise
        finally:
            safe_remove(self.paths["pid"])

    def run(self) -> int:
        ensure_dir(self.owner_root)
        if self.paths["status"].exists():
            try:
                existing = read_json(self.paths["status"])
            except Exception:
                existing = {}
            existing_pid = existing.get("ownerPid")
            if isinstance(existing_pid, int) and existing_pid != self.owner_pid and pid_alive(existing_pid):
                raise RuntimeError(f"owner root already active with pid {existing_pid}: {self.owner_root}")
        self.stack = self.build_stack()
        install_termination_handler(self.cleanup)
        try:
            self.stack.start()
            self.start_beacon_capture()
            wait_text(self.stack.obc_log, "groundLinkDriver) GROUND_LINK_UP", READY_TIMEOUT_SEC)
            self.write_status("running")
            while True:
                self.drain_beacon_capture()
                self.write_status("running")
                time.sleep(1.0)
        except Exception as exc:
            self.write_status("startup_failed", failure=str(exc))
            self.cleanup(write_terminal_state=False)
            raise


class TargetGroundManualSurfaceOwner:
    def __init__(self, owner_root: pathlib.Path, gds_ui_mode: str, auto_ports: bool) -> None:
        self.owner_root = owner_root
        self.gds_ui_mode = gds_ui_mode
        self.auto_ports = auto_ports
        self.paths = lifecycle_paths(owner_root)
        self.owner_pid = os.getpid()
        self.processes: list[ManagedProcess] = []
        self.port_reservations: dict[str, Any] = {}
        self.sband_root = owner_root / "sband"
        self.uhf_root = owner_root / "uhf"
        self.sband_gds_port = int(os.environ.get("MANUAL_TARGET_SBAND_GDS_PORT", "51900"))
        self.sband_tts_port = int(os.environ.get("MANUAL_TARGET_SBAND_TTS_PORT", "51901"))
        self.uhf_gds_port = int(os.environ.get("MANUAL_TARGET_UHF_GDS_PORT", "51910"))
        self.uhf_tts_port = int(os.environ.get("MANUAL_TARGET_UHF_GDS_TTS_PORT", "51911"))
        self.sband_gui_port = int(os.environ.get("MANUAL_TARGET_SBAND_GUI_PORT", "5100"))
        self.uhf_gui_port = int(os.environ.get("MANUAL_TARGET_UHF_GUI_PORT", "5101"))
        if self.auto_ports:
            self.sband_gds_port, self.port_reservations["sband-gds"] = reserve_free_port()
            self.sband_tts_port, self.port_reservations["sband-tts"] = reserve_free_port()
            self.uhf_gds_port, self.port_reservations["uhf-gds"] = reserve_free_port()
            self.uhf_tts_port, self.port_reservations["uhf-tts"] = reserve_free_port()
            if self.gds_ui_mode == "ui":
                self.sband_gui_port, self.port_reservations["sband-gui"] = reserve_free_port()
                self.uhf_gui_port, self.port_reservations["uhf-gui"] = reserve_free_port()
        self.sband_ground_readiness: str | None = None
        self.subsystem_target = os.environ.get("SUBSYSTEM_SIM_SSH_TARGET", "operator@subsystem.local")
        self.uhf_comm_service = os.environ.get("UHF_COMM_SERVICE_NAME", "subsystem-uhf-csp.service")
        self.obc_service = os.environ.get("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service")
        self.comm_baudrate = int(os.environ.get("COMM_BAUDRATE", "115200"))
        self.target_comm_csp_node = int(os.environ.get("MANUAL_TARGET_UHF_BEACON_CSP_NODE", "6"))
        self.uhf_beacon_target_override_dropin_name = os.environ.get(
            "MANUAL_TARGET_UHF_BEACON_TARGET_DROPIN_NAME",
            "52-uhf-beacon-csp-node.conf",
        )
        self.uhf_beacon_capture_dropin_name = os.environ.get(
            "MANUAL_TARGET_UHF_BEACON_CAPTURE_DROPIN_NAME",
            "53-uhf-beacon-capture.conf",
        )
        self.remote_beacon_capture_path = ""
        self.remote_beacon_instance_label = f"mission-console-beacon-{uuid.uuid4().hex}"
        self.remote_beacon_service_device = ""
        self.remote_beacon_peer_device = ""
        self.remote_beacon_working_directory = ""
        self.remote_beacon_bridge_process: subprocess.Popen[str] | None = None
        self.remote_beacon_capture_process: subprocess.Popen[str] | None = None
        self.remote_beacon_bridge_handle: Any | None = None
        self.remote_beacon_capture_handle: Any | None = None
        self.uhf_beacon_target_override_applied = False
        self.beacon_capture_override_applied = False
        self.beacon_local_capture_path = self.owner_root / "beacons" / "target-uhf-beacon.bin"
        self.remote_beacon_size_bytes = -1
        self.remote_beacon_capture_marker: tuple[int, int, int] | None = None
        self.target_beacon_sidecar = self.baseline_metadata().get("targetBeaconSidecar")

    def release_band_reservations(self, band_key: str) -> None:
        for key in (f"{band_key}-gds", f"{band_key}-tts", f"{band_key}-gui"):
            reservation = self.port_reservations.pop(key, None)
            if reservation is not None:
                reservation.close()

    def baseline_metadata(self) -> dict[str, Any]:
        baseline_root = pathlib.Path(os.environ.get("MANUAL_TARGET_BASELINE_ROOT", str(DEFAULT_TARGET_BASELINE_ROOT))).resolve()
        manifest_path = baseline_root / "manifest.json"
        payload: dict[str, Any] = {
            "obcSshTarget": os.environ.get("OBC_SSH_TARGET", "operator@obc.local"),
            "subsystemSshTarget": os.environ.get("SUBSYSTEM_SIM_SSH_TARGET", "operator@subsystem.local"),
            "obcCommServiceName": os.environ.get("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service"),
            "installedReleaseRoot": os.environ.get("OBC_INSTALLED_RELEASE_ROOT", "/home/operator/obc-deploy"),
            "baselineManifestHint": str(manifest_path),
        }
        if manifest_path.exists():
            baseline_manifest = read_json(manifest_path)
            payload["baselineManifestPath"] = str(manifest_path)
            if "targetAuthPreflight" in baseline_manifest:
                payload["targetAuthPreflight"] = baseline_manifest["targetAuthPreflight"]
            if "sbandCommServiceName" in baseline_manifest:
                payload["sbandCommServiceName"] = baseline_manifest["sbandCommServiceName"]
            sidecar = baseline_manifest.get("targetBeaconSidecar")
            if isinstance(sidecar, dict):
                payload["targetBeaconSidecar"] = sidecar
        return payload

    def band_payload(
        self,
        *,
        band_key: str,
        role_names: list[str],
        profile: str,
        gds_port: int,
        gds_tts_port: int,
        root: pathlib.Path,
    ) -> dict[str, Any]:
        capture_root = self.owner_root / "captures"
        if band_key == "sband":
            southbound = {
                "kind": "tcp",
                "profile": profile,
                "endpoint": f"{os.environ.get('SBAND_TCP_HOST', 'subsystem.local')}:{os.environ.get('SBAND_TCP_PORT', '18520')}",
                "commNode": 5,
            }
        else:
            southbound = {
                "kind": "serial",
                "profile": profile,
                "endpoint": os.environ.get("HOST_SERIAL_DEVICE", "/dev/cu.usbserial-CHANGE_ME"),
                "commNode": 6,
            }
        payload = {
            "canonicalBands": role_names,
            "gdsBind": f"0.0.0.0:{gds_port}",
            "gdsPort": gds_port,
            "gdsTtsPort": gds_tts_port,
            "guiAddr": "127.0.0.1",
            "guiPort": self.sband_gui_port if band_key == "sband" else self.uhf_gui_port,
            "guiUrl": f"http://127.0.0.1:{self.sband_gui_port if band_key == 'sband' else self.uhf_gui_port}" if self.gds_ui_mode == "ui" else None,
            "fileStorageDir": str(root / "files"),
            "southbound": southbound,
            "logRoot": str(root / "logs"),
            "logs": {
                "gds": str(root / "logs" / "fprime-gds.log"),
                "gateway": str(root / "logs" / "ground-ttc-gateway.log"),
                "owner": str(root / "owner.log"),
            },
            "captures": {
                "gdsToSouthbound": str(capture_root / f"{band_key}-gds-to-southbound.bin"),
                "southboundToGds": str(capture_root / f"{band_key}-southbound-to-gds.bin"),
            },
        }
        if band_key == "uhf" and isinstance(self.target_beacon_sidecar, dict) and self.target_beacon_sidecar.get("ready"):
            payload["beacon"] = {
                "supported": True,
                "sourceKind": "target-remote-sidecar",
                "sourceBand": "uhf-backup",
                "capturePath": str(self.beacon_local_capture_path),
                "frameSize": int(self.target_beacon_sidecar.get("frameSize", BEACON_FRAME_SIZE)),
                "decodeTool": str(ROOT_DIR / "scripts" / "decode_beacon_v1.py"),
                "remoteService": self.target_beacon_sidecar.get("uhfService"),
                "baselineOwned": True,
            }
        return payload

    def remote_service_working_directory(self, target: str, unit: str) -> str:
        return ssh_capture(
            target,
            f"systemctl show {shell_join([unit])} --property=WorkingDirectory --value",
        ).strip()

    def start_remote_beacon_capture(self) -> None:
        if self.remote_beacon_capture_path:
            return
        working_directory = self.remote_service_working_directory(self.subsystem_target, self.uhf_comm_service)
        if not working_directory:
            raise RuntimeError(f"unable to resolve WorkingDirectory for {self.uhf_comm_service}")
        self.remote_beacon_working_directory = working_directory
        ensure_dir(self.beacon_local_capture_path.parent)
        self.remote_beacon_bridge_handle = (self.owner_root / "beacons" / "target-remote-beacon-pty-bridge.log").open(
            "w",
            encoding="utf-8",
            buffering=1,
        )
        bridge_command = (
            f"cd {shell_join([working_directory])} && "
            f"{shell_join([working_directory + '/build-fprime-automatic-native/bin/Linux/pty_pair_bridge', '--instance-label', self.remote_beacon_instance_label])}"
        )
        bridge_process = subprocess.Popen(
            ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", self.subsystem_target, bridge_command],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        self.remote_beacon_bridge_process = bridge_process
        if bridge_process.stdout is None:
            raise RuntimeError("remote beacon PTY bridge did not expose stdout")
        pty_paths: dict[str, str] = {}
        for _ in range(2):
            line = bridge_process.stdout.readline()
            if not line:
                raise RuntimeError("remote beacon PTY bridge did not report PTY paths")
            assert self.remote_beacon_bridge_handle is not None
            self.remote_beacon_bridge_handle.write(line)
            key, value = line.strip().split("=", 1)
            pty_paths[key] = value
        self.remote_beacon_service_device = pty_paths["PTY_A"]
        self.remote_beacon_peer_device = pty_paths["PTY_B"]
        self.remote_beacon_capture_path = f"/tmp/{self.owner_root.name}-remote-uhf-beacon.bin"
        self.remote_beacon_capture_handle = (self.owner_root / "beacons" / "target-remote-beacon-capture.log").open(
            "w",
            encoding="utf-8",
            buffering=1,
        )
        remote_helper = f"{working_directory}/scripts/manual_ops/lib/beacon_sidecar.py"
        capture_command = (
            f"rm -f {shell_join([self.remote_beacon_capture_path])} && "
            f"python3 {shell_join([remote_helper])} capture "
            f"--device {shell_join([self.remote_beacon_peer_device])} "
            f"--output {shell_join([self.remote_beacon_capture_path])}"
        )
        capture_process = subprocess.Popen(
            ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", self.subsystem_target, capture_command],
            stdin=subprocess.DEVNULL,
            stdout=self.remote_beacon_capture_handle,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        self.remote_beacon_capture_process = capture_process

    def apply_beacon_capture_override(self) -> None:
        if self.beacon_capture_override_applied:
            return
        self.start_remote_beacon_capture()
        apply_service_override(
            self.subsystem_target,
            self.uhf_comm_service,
            self.uhf_beacon_capture_dropin_name,
            {
                "SUBSYSTEM_SIM_COMM_BEACON_DEVICE": self.remote_beacon_service_device,
                "COMM_BEACON_BAUDRATE": str(self.comm_baudrate),
            },
        )
        wait_remote_service_active(self.subsystem_target, self.uhf_comm_service, READY_TIMEOUT_SEC)
        self.beacon_capture_override_applied = True

    def apply_uhf_beacon_target_override(self) -> None:
        if self.uhf_beacon_target_override_applied:
            return
        apply_service_override(
            os.environ.get("OBC_SSH_TARGET", "operator@obc.local"),
            self.obc_service,
            self.uhf_beacon_target_override_dropin_name,
            {"UHF_BEACON_CSP_NODE": str(self.target_comm_csp_node)},
        )
        obc_target = os.environ.get("OBC_SSH_TARGET", "operator@obc.local")
        wait_remote_service_active(obc_target, self.obc_service, READY_TIMEOUT_SEC)
        wait_remote_journal_fragments(
            obc_target,
            self.obc_service,
            TARGET_SBAND_STATIC_READY_FRAGMENTS,
            READY_TIMEOUT_SEC,
            invocation_id=(service_invocation_id(obc_target, self.obc_service) or None),
        )
        wait_remote_latest_link_state(
            obc_target,
            self.obc_service,
            "groundLinkDriver",
            "UP",
            READY_TIMEOUT_SEC,
            invocation_id=(service_invocation_id(obc_target, self.obc_service) or None),
            availability_fragment=TARGET_SBAND_AVAILABILITY_FRAGMENT,
        )
        self.uhf_beacon_target_override_applied = True

    def remote_beacon_size(self) -> int:
        if not self.remote_beacon_capture_path:
            return 0
        text = ssh_capture(
            self.subsystem_target,
            (
                "python3 -c "
                + shell_join(
                    [
                        "import os,sys; path=sys.argv[1]; print(os.path.getsize(path) if os.path.exists(path) else 0)",
                        self.remote_beacon_capture_path,
                    ]
                )
            ),
            check=False,
        ).strip()
        return int(text or "0")

    def refresh_beacon_mirror(self) -> None:
        sidecar = self.target_beacon_sidecar
        if not isinstance(sidecar, dict) or not sidecar.get("ready"):
            return
        target = str(sidecar.get("subsystemTarget", ""))
        capture_path = str(sidecar.get("capturePath", ""))
        if not target or not capture_path:
            return
        text = ssh_capture(
            target,
            "python3 -c "
            + shell_join(
                [
                    "import os,sys; path=sys.argv[1]; stat=os.stat(path) if os.path.exists(path) else None; print(f'{stat.st_size} {stat.st_mtime_ns} {stat.st_ino}' if stat else '0')",
                    capture_path,
                ]
            ),
            check=False,
        ).strip()
        try:
            fields = text.split()
            current_size = int(fields[0] if fields else "0")
            marker = (
                (current_size, int(fields[1]), int(fields[2]))
                if len(fields) == 3
                else (current_size, 0, 0)
            )
        except ValueError:
            return
        if current_size <= 0:
            self.beacon_local_capture_path.unlink(missing_ok=True)
            self.remote_beacon_size_bytes = -1
            self.remote_beacon_capture_marker = None
            return
        if marker == self.remote_beacon_capture_marker and self.beacon_local_capture_path.exists():
            return
        try:
            payload = ssh_capture_bytes(
                target,
                f"set -euo pipefail; cat {shell_join([capture_path])}",
            )
        except RuntimeError:
            self.beacon_local_capture_path.unlink(missing_ok=True)
            return
        if len(payload) != current_size:
            self.beacon_local_capture_path.unlink(missing_ok=True)
            return
        ensure_dir(self.beacon_local_capture_path.parent)
        self.beacon_local_capture_path.write_bytes(payload)
        self.remote_beacon_size_bytes = current_size
        self.remote_beacon_capture_marker = marker

    def stop_beacon_sidecar(self) -> None:
        if self.beacon_capture_override_applied:
            remove_service_override(self.subsystem_target, self.uhf_comm_service, self.uhf_beacon_capture_dropin_name)
            wait_remote_service_active(self.subsystem_target, self.uhf_comm_service, READY_TIMEOUT_SEC)
            self.beacon_capture_override_applied = False
        if self.uhf_beacon_target_override_applied:
            obc_target = os.environ.get("OBC_SSH_TARGET", "operator@obc.local")
            remove_service_override(
                obc_target,
                self.obc_service,
                self.uhf_beacon_target_override_dropin_name,
            )
            wait_remote_service_active(obc_target, self.obc_service, READY_TIMEOUT_SEC)
            self.uhf_beacon_target_override_applied = False
        commands: list[str] = []
        if self.remote_beacon_capture_path:
            commands.append(f"pkill -f {shell_join([self.remote_beacon_capture_path])} >/dev/null 2>&1 || true")
            commands.append(f"rm -f {shell_join([self.remote_beacon_capture_path])} >/dev/null 2>&1 || true")
        if self.remote_beacon_working_directory:
            commands.append(
                "pkill -f -- "
                + shell_join([f"--instance-label {self.remote_beacon_instance_label}"])
                + " >/dev/null 2>&1 || true"
            )
        if commands:
            ssh_capture(self.subsystem_target, " ; ".join(commands), check=False)
        for process in (self.remote_beacon_capture_process, self.remote_beacon_bridge_process):
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=2.0)
        self.remote_beacon_bridge_process = None
        self.remote_beacon_capture_process = None
        if self.remote_beacon_bridge_handle is not None:
            self.remote_beacon_bridge_handle.close()
            self.remote_beacon_bridge_handle = None
        if self.remote_beacon_capture_handle is not None:
            self.remote_beacon_capture_handle.close()
            self.remote_beacon_capture_handle = None
        self.remote_beacon_capture_path = ""
        self.remote_beacon_service_device = ""
        self.remote_beacon_peer_device = ""
        self.remote_beacon_working_directory = ""
        self.remote_beacon_size_bytes = -1
        self.remote_beacon_capture_marker = None

    def wait_for_sband_ground_readiness(
        self,
        *,
        obc_target: str,
        obc_service: str,
        baseline_invocation_id: str | None,
        baseline_count: int,
        timeout: float,
    ) -> str:
        deadline = time.time() + timeout
        gateway_log = self.sband_root / "logs" / "ground-ttc-gateway.log"
        downlink_capture = self.owner_root / "captures" / "sband-southbound-to-gds.bin"
        gateway_opened_since: float | None = None
        while time.time() < deadline:
            current_invocation_id = service_invocation_id(obc_target, obc_service) or None
            scoped_invocation_id = current_invocation_id or baseline_invocation_id
            current_count = remote_journal_fragment_count(
                obc_target,
                obc_service,
                TARGET_SBAND_GROUND_LINK_UP_FRAGMENTS[0],
                invocation_id=scoped_invocation_id,
            )
            fresh_link_seen = False
            if scoped_invocation_id and scoped_invocation_id != baseline_invocation_id:
                fresh_link_seen = current_count > 0
            else:
                fresh_link_seen = current_count > baseline_count
            if fresh_link_seen and remote_latest_link_state(
                obc_target,
                obc_service,
                "groundLinkDriver",
                invocation_id=scoped_invocation_id,
            ) == "UP":
                return "fresh-link-up"
            if downlink_capture.exists() and downlink_capture.stat().st_size > 0:
                return "downlink-bytes"
            if gateway_log.exists():
                gateway_text = gateway_log.read_text(encoding="utf-8", errors="replace")
                if "southbound-opened mode=tcp-client" in gateway_text:
                    if gateway_opened_since is None:
                        gateway_opened_since = time.time()
                    elif (time.time() - gateway_opened_since) >= 2.0:
                        return "southbound-opened-stable"
                else:
                    gateway_opened_since = None
            time.sleep(0.25)
        raise RuntimeError(
            "timed out waiting for target manual S-band ground readiness "
            "(need fresh-link-up, downlink-bytes, or southbound-opened-stable)"
        )

    def payload(self, lifecycle_state: str, failure: str | None = None) -> dict[str, Any]:
        payload = {
            "formalChange": "manual-dual-gds-secure-ops-surface-v1",
            "surfaceRoot": str(self.owner_root),
            "ownerPid": self.owner_pid,
            "lifecycleState": lifecycle_state,
            "surfaceType": "target-manual-ground-dual-gds",
            "gdsUiMode": self.gds_ui_mode,
            "timestamp": utc_timestamp(),
            "dictionaryPath": str(root_dict_path()),
            "operatorSurfaces": {
                "sband": self.band_payload(
                    band_key="sband",
                    role_names=["sband"],
                    profile="sband",
                    gds_port=self.sband_gds_port,
                    gds_tts_port=self.sband_tts_port,
                    root=self.sband_root,
                ),
                "uhf": self.band_payload(
                    band_key="uhf",
                    role_names=["uhf-backup", "uhf-primary-after-failover"],
                    profile="uhf-backup",
                    gds_port=self.uhf_gds_port,
                    gds_tts_port=self.uhf_tts_port,
                    root=self.uhf_root,
                ),
            },
            "targetBaseline": self.baseline_metadata(),
            "nonClaims": [
                "no ownership of remote shared target baseline shutdown",
                "no GDS UI command interception",
                "no simultaneous full-authority command closure on both links",
            ],
        }
        if failure is not None:
            payload["failure"] = failure
        if self.sband_ground_readiness is not None:
            payload["sbandGroundReadiness"] = self.sband_ground_readiness
        return payload

    def write_status(self, lifecycle_state: str, failure: str | None = None) -> None:
        payload = self.payload(lifecycle_state, failure=failure)
        atomic_write_json(self.paths["manifest"], payload)
        atomic_write_json(self.paths["status"], payload)
        self.paths["pid"].write_text(f"{self.owner_pid}\n", encoding="utf-8")

    def start_band(
        self,
        *,
        band_key: str,
        profile: str,
        root: pathlib.Path,
        gds_port: int,
        gds_tts_port: int,
    ) -> None:
        self.release_band_reservations(band_key)
        ensure_dir(root / "files")
        ensure_dir(root / "logs")
        ensure_dir(self.owner_root / "captures")
        env = os.environ.copy()
        env.update(
            {
                "TARGET_COMM_PROFILE": profile,
                "GDS_UI_MODE": self.gds_ui_mode,
                "GDS_PORT": str(gds_port),
                "GDS_TTS_PORT": str(gds_tts_port),
                "GDS_GUI_ADDR": "127.0.0.1",
                "GDS_GUI_PORT": str(self.sband_gui_port if band_key == "sband" else self.uhf_gui_port),
                "GDS_FILE_STORAGE_DIR": str(root / "files"),
                "TARGET_COMM_CSP_GROUND_LOG_DIR": str(root / "logs"),
                "GROUND_TTC_GATEWAY_CAPTURE_GDS_TO_SOUTHBOUND": str(self.owner_root / "captures" / f"{band_key}-gds-to-southbound.bin"),
                "GROUND_TTC_GATEWAY_CAPTURE_SOUTHBOUND_TO_GDS": str(self.owner_root / "captures" / f"{band_key}-southbound-to-gds.bin"),
            }
        )
        managed = start_managed_process(
            f"{band_key}-ground-stack",
            ["bash", str(ROOT_DIR / "scripts" / "run_target_comm_csp_ground_stack.sh")],
            root / "owner.log",
            env=env,
            cwd=ROOT_DIR,
            stale_match_groups=build_gds_stale_match_groups(
                ip_port=gds_port,
                tts_port=gds_tts_port,
                file_storage_dir=root / "files",
            ),
            stale_match_markers=("fprime-gds", "ground_ttc_gateway"),
        )
        self.processes.append(managed)
        wait_port("127.0.0.1", gds_port, READY_TIMEOUT_SEC)
        wait_port("127.0.0.1", gds_tts_port, READY_TIMEOUT_SEC)

    def cleanup(self, *, write_terminal_state: bool = True) -> None:
        try:
            try:
                for reservation in self.port_reservations.values():
                    reservation.close()
                self.port_reservations.clear()
                cleanup_managed_processes(self.processes, timeout_sec=5.0)
                if write_terminal_state:
                    try:
                        self.write_status("stopped")
                    except Exception:
                        pass
            except Exception as exc:
                if write_terminal_state:
                    try:
                        self.write_status("cleanup_failed", failure=str(exc))
                    except Exception:
                        pass
                raise
        finally:
            safe_remove(self.paths["pid"])

    def run(self) -> int:
        ensure_dir(self.owner_root)
        if self.paths["status"].exists():
            try:
                existing = read_json(self.paths["status"])
            except Exception:
                existing = {}
            existing_pid = existing.get("ownerPid")
            if isinstance(existing_pid, int) and existing_pid != self.owner_pid and pid_alive(existing_pid):
                raise RuntimeError(f"owner root already active with pid {existing_pid}: {self.owner_root}")
        install_termination_handler(self.cleanup)
        try:
            obc_target = os.environ.get("OBC_SSH_TARGET", "operator@obc.local")
            obc_service = os.environ.get("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service")
            obc_invocation_id = service_invocation_id(obc_target, obc_service) or None
            ground_link_up_baseline_count = remote_journal_fragment_count(
                obc_target,
                obc_service,
                TARGET_SBAND_GROUND_LINK_UP_FRAGMENTS[0],
                invocation_id=obc_invocation_id,
            )
            self.start_band(
                band_key="sband",
                profile="sband",
                root=self.sband_root,
                gds_port=self.sband_gds_port,
                gds_tts_port=self.sband_tts_port,
            )
            self.start_band(
                band_key="uhf",
                profile="uhf-backup",
                root=self.uhf_root,
                gds_port=self.uhf_gds_port,
                gds_tts_port=self.uhf_tts_port,
            )
            self.refresh_beacon_mirror()
            self.sband_ground_readiness = self.wait_for_sband_ground_readiness(
                obc_target=obc_target,
                obc_service=obc_service,
                baseline_invocation_id=obc_invocation_id,
                baseline_count=ground_link_up_baseline_count,
                timeout=READY_TIMEOUT_SEC,
            )
            self.write_status("running")
            while True:
                for managed in self.processes:
                    if managed.process.poll() is not None:
                        raise RuntimeError(f"{managed.name} exited with rc={managed.process.returncode}")
                if self.remote_beacon_bridge_process is not None and self.remote_beacon_bridge_process.poll() is not None:
                    raise RuntimeError(f"target beacon bridge exited with rc={self.remote_beacon_bridge_process.returncode}")
                if self.remote_beacon_capture_process is not None and self.remote_beacon_capture_process.poll() is not None:
                    raise RuntimeError(f"target beacon capture exited with rc={self.remote_beacon_capture_process.returncode}")
                self.refresh_beacon_mirror()
                self.write_status("running")
                time.sleep(1.0)
        except Exception as exc:
            self.write_status("startup_failed", failure=str(exc))
            self.cleanup(write_terminal_state=False)
            raise


def render_surface_status(path: pathlib.Path) -> str:
    payload = read_json(path)
    manifest_path = pathlib.Path(payload.get("surfaceRoot", str(path.parent))) / "manifest.json"
    lines = [
        f"surfaceType={payload.get('surfaceType')}",
        f"surfaceRoot={payload.get('surfaceRoot')}",
        f"ownerPid={payload.get('ownerPid')}",
        f"lifecycleState={payload.get('lifecycleState')}",
        f"gdsUiMode={payload.get('gdsUiMode')}",
        f"manifest={manifest_path}",
    ]
    for name, surface in payload.get("operatorSurfaces", {}).items():
        lines.append(
            f"{name}=gds:{surface.get('gdsPort')} tts:{surface.get('gdsTtsPort')} "
            f"store:{surface.get('fileStorageDir')}"
        )
        beacon = surface.get("beacon")
        if isinstance(beacon, dict) and beacon.get("supported"):
            lines.append(
                f"{name}-beacon=kind:{beacon.get('sourceKind')} "
                f"band:{beacon.get('sourceBand')} capture:{beacon.get('capturePath')}"
            )
        if surface.get("guiUrl"):
            lines.append(f"{name}-gui={surface.get('guiUrl')}")
    for non_claim in payload.get("nonClaims", []):
        lines.append(f"non-claim={non_claim}")
    target_auth_preflight = payload.get("targetAuthPreflight")
    if isinstance(target_auth_preflight, dict):
        lines.append(f"target-auth-preflight-applied={target_auth_preflight.get('applied')}")
        for row in target_auth_preflight.get("managedOverrides", []):
            target = row.get("target")
            service = row.get("service")
            dropin = row.get("dropInName")
            if target and service and dropin:
                lines.append(f"target-auth-override={target}:{service}:{dropin}")
    if payload.get("failure"):
        lines.append(f"failure={payload['failure']}")
    return "\n".join(lines)


def target_manual_auth_preflight_spec() -> dict[str, Any]:
    obc_target = os.environ.get("OBC_SSH_TARGET", "operator@obc.local")
    subsystem_target = os.environ.get("SUBSYSTEM_SIM_SSH_TARGET", "operator@subsystem.local")
    obc_service = os.environ.get("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service")
    sband_service = os.environ.get("SBAND_COMM_SERVICE_NAME", "subsystem-sband-csp.service")
    uhf_service = os.environ.get("UHF_COMM_SERVICE_NAME", "subsystem-uhf-csp.service")
    obc_groundlink_diagnostics = os.environ.get("MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS", "0").strip()
    sband_ingress_diagnostics = os.environ.get("MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS", "1").strip()
    timeouts_env = {
        "COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS": os.environ.get("MANUAL_TARGET_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS", "").strip(),
        "COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS": os.environ.get("MANUAL_TARGET_COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS", "").strip(),
        "COMM_GROUNDLINK_HEALTH_TIMEOUT_MS": os.environ.get("MANUAL_TARGET_COMM_GROUNDLINK_HEALTH_TIMEOUT_MS", "").strip(),
    }
    managed_overrides: list[dict[str, Any]] = []
    if obc_groundlink_diagnostics not in ("", "0"):
        managed_overrides.append(
            {
                "target": obc_target,
                "service": obc_service,
                "dropInName": os.environ.get(
                    "MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS_DROPIN_NAME",
                    DEFAULT_OBC_GROUNDLINK_DIAGNOSTICS_DROPIN,
                ),
                "env": {"COMM_GROUNDLINK_DIAGNOSTICS": obc_groundlink_diagnostics},
            }
        )
    timeout_payload = {key: value for key, value in timeouts_env.items() if value not in ("", "0")}
    if timeout_payload:
        managed_overrides.append(
            {
                "target": obc_target,
                "service": obc_service,
                "dropInName": os.environ.get(
                    "MANUAL_TARGET_OBC_GROUNDLINK_TIMEOUTS_DROPIN_NAME",
                    DEFAULT_OBC_GROUNDLINK_TIMEOUTS_DROPIN,
                ),
                "env": timeout_payload,
            }
        )
    if sband_ingress_diagnostics not in ("", "0"):
        managed_overrides.append(
            {
                "target": subsystem_target,
                "service": sband_service,
                "dropInName": os.environ.get(
                    "MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS_DROPIN_NAME",
                    DEFAULT_SBAND_INGRESS_DIAGNOSTICS_DROPIN,
                ),
                "env": {"COMM_NODE_INGRESS_DIAGNOSTICS": sband_ingress_diagnostics},
            }
        )
    return {
        "applied": False,
        "obcTarget": obc_target,
        "subsystemTarget": subsystem_target,
        "obcCommServiceName": obc_service,
        "sbandCommServiceName": sband_service,
        "uhfCommServiceName": uhf_service,
        "managedOverrides": managed_overrides,
    }


def apply_target_manual_auth_preflight(preflight: dict[str, Any]) -> None:
    managed_overrides = list(preflight.get("managedOverrides", []))
    applied: list[dict[str, Any]] = []
    obc_target = str(preflight["obcTarget"])
    obc_service = str(preflight["obcCommServiceName"])
    try:
        for row in managed_overrides:
            apply_service_override(
                str(row["target"]),
                str(row["service"]),
                str(row["dropInName"]),
                {str(key): str(value) for key, value in dict(row["env"]).items()},
            )
            wait_remote_service_active(str(row["target"]), str(row["service"]), 120.0)
            applied.append(row)
        wait_remote_journal_fragments(
            obc_target,
            obc_service,
            TARGET_SBAND_STATIC_READY_FRAGMENTS,
            20.0,
            invocation_id=(service_invocation_id(obc_target, obc_service) or None),
        )
        wait_remote_latest_link_state(
            obc_target,
            obc_service,
            "groundLinkDriver",
            "UP",
            20.0,
            invocation_id=(service_invocation_id(obc_target, obc_service) or None),
            availability_fragment=TARGET_SBAND_AVAILABILITY_FRAGMENT,
        )
        time.sleep(2.0)
    except Exception:
        for row in reversed(applied):
            try:
                remove_service_override(str(row["target"]), str(row["service"]), str(row["dropInName"]))
                wait_remote_service_active(str(row["target"]), str(row["service"]), 120.0)
            except Exception:
                pass
        raise
    preflight["applied"] = True


def write_target_baseline_snapshot(owner_root: pathlib.Path) -> pathlib.Path:
    ensure_dir(owner_root)
    json_out = owner_root / "ensure-target-baseline.json"
    managed_externally = os.environ.get("TARGET_BASELINE_MANAGED_EXTERNALLY", "0") == "1"
    if managed_externally:
        ready_json = os.environ.get("TARGET_BASELINE_READY_JSON", "").strip()
        if not ready_json:
            raise RuntimeError("TARGET_BASELINE_READY_JSON is required when TARGET_BASELINE_MANAGED_EXTERNALLY=1")
        ready_path = pathlib.Path(ready_json).resolve()
        if not ready_path.is_file():
            raise RuntimeError(f"target baseline readiness snapshot not found: {ready_path}")
        json_out.write_text(ready_path.read_text(encoding="utf-8"), encoding="utf-8")
    else:
        env = os.environ.copy()
        env["JSON_OUT"] = str(json_out)
        result = subprocess.run(
            [str(shutil.which("bash") or "/bin/bash"), str(ROOT_DIR / "scripts" / "ensure_target_comm_lab_baseline.sh")],
            env=env,
            check=False,
        )
        if result.returncode != 0:
            raise RuntimeError("ensure_target_comm_lab_baseline.sh failed")
    readiness_payload = read_json(json_out)
    preflight = target_manual_auth_preflight_spec()
    if managed_externally:
        preflight["applied"] = True
    else:
        apply_target_manual_auth_preflight(preflight)
    obc_env = service_environment(str(preflight["obcTarget"]), str(preflight["obcCommServiceName"]))
    sband_env = service_environment(str(preflight["subsystemTarget"]), str(preflight["sbandCommServiceName"]))
    uhf_env = service_environment(str(preflight["subsystemTarget"]), str(preflight["uhfCommServiceName"]))
    payload = {
        "formalChange": "manual-dual-gds-secure-ops-surface-v1",
        "surfaceRoot": str(owner_root),
        "ownerPid": os.getpid(),
        "lifecycleState": "prepared",
        "surfaceType": "target-manual-baseline",
        "managedExternally": managed_externally,
        "gdsUiMode": None,
        "timestamp": utc_timestamp(),
        "readinessSnapshotPath": str(json_out),
        "targetBeaconSidecar": readiness_payload.get("targetBeaconSidecar"),
        "obcSshTarget": os.environ.get("OBC_SSH_TARGET", "operator@obc.local"),
        "subsystemSshTarget": os.environ.get("SUBSYSTEM_SIM_SSH_TARGET", "operator@subsystem.local"),
        "obcCommServiceName": os.environ.get("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service"),
        "sbandCommServiceName": os.environ.get("SBAND_COMM_SERVICE_NAME", "subsystem-sband-csp.service"),
        "installedReleaseRoot": os.environ.get("OBC_INSTALLED_RELEASE_ROOT", "/home/operator/obc-deploy"),
        "targetAuthPreflight": {
            **preflight,
            "observedObcEnvironment": {
                key: obc_env.get(key, "")
                for key in (
                    "COMM_GROUNDLINK_DIAGNOSTICS",
                    "COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS",
                    "COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS",
                    "COMM_GROUNDLINK_HEALTH_TIMEOUT_MS",
                    "COMM_CSP_SOCKETCAN_USE_CANFD",
                    "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST",
                    "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST",
                )
            },
            "observedSbandEnvironment": {
                key: sband_env.get(key, "")
                for key in (
                    "COMM_NODE_INGRESS_DIAGNOSTICS",
                    "COMM_CSP_SOCKETCAN_USE_CANFD",
                    "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST",
                    "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST",
                )
            },
            "observedUhfEnvironment": {
                key: uhf_env.get(key, "")
                for key in (
                    "COMM_CSP_SOCKETCAN_USE_CANFD",
                    "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST",
                    "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST",
                )
            },
        },
        "nonClaims": [
            "snapshot only; no long-running local owner process",
            (
                "remote baseline and secure-auth preflight remain externally managed"
                if managed_externally
                else "stop script retires local metadata and removes manual-auth preflight overrides only"
            ),
        ],
    }
    atomic_write_json(owner_root / "manifest.json", payload)
    atomic_write_json(owner_root / "status.json", payload)
    return owner_root / "manifest.json"


def retire_target_baseline_snapshot(owner_root: pathlib.Path) -> None:
    manifest_path = owner_root / "manifest.json"
    if manifest_path.exists():
        payload = read_json(manifest_path)
        preflight = payload.get("targetAuthPreflight", {})
        if not payload.get("managedExternally", False):
            for row in reversed(list(preflight.get("managedOverrides", []))):
                try:
                    remove_service_override(str(row["target"]), str(row["service"]), str(row["dropInName"]))
                    wait_remote_service_active(str(row["target"]), str(row["service"]), 120.0)
                except Exception:
                    pass
    safe_remove(owner_root / "manifest.json")
    safe_remove(owner_root / "status.json")
    safe_remove(owner_root / "ensure-target-baseline.json")


def ensure_target_manual_auth_preflight_only() -> None:
    preflight = target_manual_auth_preflight_spec()
    apply_target_manual_auth_preflight(preflight)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Manual operator surface owner helpers.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    hosted = subparsers.add_parser("hosted-owner")
    hosted.add_argument("--owner-root", required=True)
    hosted.add_argument("--gds-ui-mode", choices=("ui", "headless"), default="ui")
    hosted.add_argument("--runtime-root")
    hosted.add_argument("--auto-ports", default="1")

    target_ground = subparsers.add_parser("target-ground-owner")
    target_ground.add_argument("--owner-root", required=True)
    target_ground.add_argument("--gds-ui-mode", choices=("ui", "headless"), default="ui")
    target_ground.add_argument("--auto-ports", default="1")

    render = subparsers.add_parser("render-status")
    render.add_argument("status_json")

    hosted_stop = subparsers.add_parser("hosted-stop")
    hosted_stop.add_argument("--owner-root", required=True)

    target_ground_stop = subparsers.add_parser("target-ground-stop")
    target_ground_stop.add_argument("--owner-root", required=True)

    baseline_start = subparsers.add_parser("target-baseline-start")
    baseline_start.add_argument("--owner-root", required=True)

    baseline_stop = subparsers.add_parser("target-baseline-stop")
    baseline_stop.add_argument("--owner-root", required=True)

    subparsers.add_parser("target-manual-auth-preflight-apply")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if args.command == "hosted-owner":
        owner = HostedManualSurfaceOwner(
            pathlib.Path(args.owner_root).resolve(),
            args.gds_ui_mode,
            None if args.runtime_root is None else pathlib.Path(args.runtime_root).resolve(),
            str(args.auto_ports).strip().lower() in {"1", "true", "yes", "on"},
        )
        return owner.run()
    if args.command == "target-ground-owner":
        owner = TargetGroundManualSurfaceOwner(
            pathlib.Path(args.owner_root).resolve(),
            args.gds_ui_mode,
            str(args.auto_ports).strip().lower() in {"1", "true", "yes", "on"},
        )
        return owner.run()
    if args.command == "render-status":
        print(render_surface_status(pathlib.Path(args.status_json).resolve()))
        return 0
    if args.command == "hosted-stop":
        stop_hosted_surface(pathlib.Path(args.owner_root).resolve())
        print("Hosted manual surface stopped.")
        return 0
    if args.command == "target-ground-stop":
        stop_target_ground_surface(pathlib.Path(args.owner_root).resolve())
        print("Target manual ground surface stopped.")
        return 0
    if args.command == "target-baseline-start":
        manifest_path = write_target_baseline_snapshot(pathlib.Path(args.owner_root).resolve())
        print(f"Target manual baseline prepared: {manifest_path}")
        return 0
    if args.command == "target-baseline-stop":
        retire_target_baseline_snapshot(pathlib.Path(args.owner_root).resolve())
        print("Target manual baseline local metadata retired. Remote shared services remain running.")
        return 0
    if args.command == "target-manual-auth-preflight-apply":
        ensure_target_manual_auth_preflight_only()
        print("Target manual auth preflight applied.")
        return 0
    raise RuntimeError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
