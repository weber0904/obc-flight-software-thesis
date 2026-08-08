from __future__ import annotations

import argparse
import json
import os
import pathlib
import shutil
import socket
import sys
import time
from typing import Any, Sequence

from per_band_stock_ground_stacks import (
    DEFAULT_CSP_PUB_PORT,
    DEFAULT_CSP_SUB_PORT,
    DEFAULT_GDS_BIND_HOST,
    DEFAULT_RADIO_PORT,
    DEFAULT_SBAND_GDS_PORT,
    DEFAULT_SBAND_GDS_TTS_PORT,
    DEFAULT_SBAND_TCP_PORT,
    DEFAULT_TICK_MS,
    DEFAULT_UHF_GDS_PORT,
    DEFAULT_UHF_GDS_TTS_PORT,
    build_runtime,
    find_fprime_cli,
    install_signal_cleanup,
    parse_bool,
)


def default_owner_root() -> pathlib.Path:
    return pathlib.Path(
        os.environ.get(
            "DUAL_LINK_ORCHESTRATION_ROOT",
            "/tmp/comm-dual-link-orchestration-v1/hosted-owner",
        )
    )


def utc_timestamp() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


class HostedDualLinkOrchestrationOwner:
    def __init__(self, args: argparse.Namespace) -> None:
        self.root_dir = pathlib.Path(args.root_dir).resolve()
        self.owner_root = pathlib.Path(args.owner_root).resolve()
        self.owner_log_root = self.owner_root / "logs"
        self.manifest_path = self.owner_root / "manifest.json"
        self.status_path = self.owner_root / "status.json"
        self.startup_claim_path = self.owner_root.parent / f".{self.owner_root.name}.startup-claim.json"
        self.requested_mode = "combined"
        self.orchestrator_pid = os.getpid()
        self.phase_history: list[dict[str, str]] = []
        self.failure: dict[str, Any] | None = None
        self.cleanup: dict[str, Any] | None = None
        self.startup_claim_acquired = False
        self.startup_claim_delay = float(os.environ.get("DUAL_LINK_ORCHESTRATION_STARTUP_CLAIM_DELAY_SECS", "0"))

        runtime_root = None if args.runtime_root is None else pathlib.Path(args.runtime_root).resolve()
        stack_root = self.owner_root / "layer1-baseline"
        stack_args = argparse.Namespace(
            mode="combined",
            root_dir=args.root_dir,
            bin_dir=args.bin_dir,
            dictionary_path=args.dictionary_path,
            cli_path=args.cli_path,
            stack_root=str(stack_root),
            runtime_root=(None if runtime_root is None else str(runtime_root)),
            hold_seconds=None,
            gds_bind_host=args.gds_bind_host,
            sband_gds_port=args.sband_gds_port,
            sband_gds_tts_port=args.sband_gds_tts_port,
            uhf_gds_port=args.uhf_gds_port,
            uhf_gds_tts_port=args.uhf_gds_tts_port,
            csp_sub_port=args.csp_sub_port,
            csp_pub_port=args.csp_pub_port,
            radio_port=args.radio_port,
            sband_tcp_port=args.sband_tcp_port,
            command_authority_profile=args.command_authority_profile,
            tick_ms=args.tick_ms,
            preserve_sband_primary="1",
            enable_uhf_beacon_side_channel=None,
            auto_ports=args.auto_ports,
        )
        self.stack = build_runtime("combined", stack_args)
        self.hold_seconds = args.hold_seconds

    def _phase(self, phase: str, detail: str | None = None) -> None:
        entry = {"phase": phase, "timestamp": utc_timestamp()}
        if detail:
            entry["detail"] = detail
        self.phase_history.append(entry)
        self.write_status(phase)

    def _owned_processes(self) -> list[Any]:
        owned = list(self.stack.processes)
        if self.stack.sband is not None:
            owned.extend(self.stack.sband.processes)
        if self.stack.uhf is not None:
            owned.extend(self.stack.uhf.processes)
        return owned

    def _port_checks(self) -> list[dict[str, Any]]:
        checks = []
        named_ports = [
            (
                "sbandGdsPort",
                self.stack.gds_bind_host,
                None if self.stack.sband is None else self.stack.sband.gds_port,
            ),
            (
                "sbandGdsTtsPort",
                self.stack.gds_bind_host,
                None if self.stack.sband is None else self.stack.sband.gds_tts_port,
            ),
            (
                "uhfGdsPort",
                self.stack.gds_bind_host,
                None if self.stack.uhf is None else self.stack.uhf.gds_port,
            ),
            (
                "uhfGdsTtsPort",
                self.stack.gds_bind_host,
                None if self.stack.uhf is None else self.stack.uhf.gds_tts_port,
            ),
            ("cspHubSubPort", "0.0.0.0", self.stack.csp_sub_port),
            ("cspHubPubPort", "0.0.0.0", self.stack.csp_pub_port),
            ("radioMockPort", "0.0.0.0", self.stack.radio_port),
            (
                "sbandCommTcpPort",
                self.stack.sband_tcp_host,
                self.stack.sband_tcp_port if self.stack.start_sband_runtime else None,
            ),
        ]
        for label, host, port in named_ports:
            if port is None:
                continue
            checks.append({"label": label, "host": host, "port": int(port)})
        return checks

    def _assert_port_available(self, host: str, port: int, label: str) -> None:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                sock.bind((host, port))
            except OSError as exc:
                raise RuntimeError(f"{label} port {port} on host {host} is already in use") from exc

    @staticmethod
    def _pid_alive(pid: int) -> bool:
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return False
        except PermissionError:
            return True
        return True

    def _active_owner_conflict(self) -> str | None:
        if not self.status_path.exists():
            return None
        try:
            status = json.loads(self.status_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return None
        lifecycle = status.get("lifecyclePhase")
        owner_pid = status.get("orchestratorPid")
        if isinstance(owner_pid, int) and owner_pid != self.orchestrator_pid and self._pid_alive(owner_pid):
            return (
                f"owner root {self.owner_root} already belongs to an active "
                f"dual-link orchestration owner (pid {owner_pid}, lifecycle {lifecycle})"
            )
        for owned in status.get("ownedProcessSet", []):
            pid = owned.get("pid")
            if isinstance(pid, int) and pid != self.orchestrator_pid and self._pid_alive(pid):
                return (
                    f"owner root {self.owner_root} already belongs to an active "
                    f"dual-link orchestration owner (owned pid {pid}, lifecycle {lifecycle})"
                )
        terminal_lifecycles = {"startup_failed", "stopped", "cleanup_failed"}
        if lifecycle in terminal_lifecycles:
            for listener in (status.get("cleanup") or {}).get("listenerChecks", []):
                if not listener.get("closed", False):
                    return (
                        f"owner root {self.owner_root} contains a stale terminal "
                        f"dual-link orchestration status ({lifecycle}) with an open listener "
                        f"({listener.get('label')}:{listener.get('port')})"
                    )
        if lifecycle not in terminal_lifecycles:
            return (
                f"owner root {self.owner_root} contains a stale non-terminal "
                f"dual-link orchestration status (lifecycle {lifecycle}) that cannot be proven clean"
            )
        if lifecycle in terminal_lifecycles:
            return None
        return None

    def _write_json_atomically(self, path: pathlib.Path, payload: dict[str, Any]) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        temp_path = path.with_suffix(f"{path.suffix}.tmp")
        temp_path.write_text(
            json.dumps(payload, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        os.replace(temp_path, path)

    def _startup_claim_payload(self) -> dict[str, Any]:
        return {
            "formalChange": "comm-dual-link-orchestration-v1",
            "orchestratorPid": self.orchestrator_pid,
            "ownerRoot": str(self.owner_root),
            "timestamp": utc_timestamp(),
        }

    def _stale_claim_payload(self) -> dict[str, Any] | None:
        try:
            payload = json.loads(self.startup_claim_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            payload = None
        if payload is None:
            try:
                age_seconds = time.time() - self.startup_claim_path.stat().st_mtime
            except FileNotFoundError:
                return {}
            if age_seconds < 5:
                return None
            return {}
        claim_pid = payload.get("orchestratorPid")
        if isinstance(claim_pid, int) and claim_pid != self.orchestrator_pid and self._pid_alive(claim_pid):
            return None
        return payload

    def acquire_startup_claim(self) -> None:
        self.startup_claim_path.parent.mkdir(parents=True, exist_ok=True)
        while True:
            try:
                fd = os.open(
                    self.startup_claim_path,
                    os.O_CREAT | os.O_EXCL | os.O_WRONLY,
                    0o600,
                )
            except FileExistsError:
                stale_payload = self._stale_claim_payload()
                if stale_payload is None:
                    claim_pid = "unknown"
                    try:
                        live_payload = json.loads(
                            self.startup_claim_path.read_text(encoding="utf-8")
                        )
                    except (OSError, json.JSONDecodeError):
                        live_payload = {}
                    if isinstance(live_payload.get("orchestratorPid"), int):
                        claim_pid = str(live_payload["orchestratorPid"])
                    raise RuntimeError(
                        f"owner root {self.owner_root} already has an active startup claim "
                        f"(pid {claim_pid}) and cannot be reacquired concurrently"
                    )
                try:
                    self.startup_claim_path.unlink()
                except FileNotFoundError:
                    continue
                continue
            with os.fdopen(fd, "w", encoding="utf-8") as handle:
                json.dump(self._startup_claim_payload(), handle, indent=2, sort_keys=True)
                handle.write("\n")
            self.startup_claim_acquired = True
            return

    def release_startup_claim(self) -> None:
        if not self.startup_claim_acquired:
            return
        try:
            self.startup_claim_path.unlink()
        except FileNotFoundError:
            pass
        self.startup_claim_acquired = False

    def preflight(self) -> None:
        self.owner_log_root.mkdir(parents=True, exist_ok=True)
        shutil.rmtree(self.owner_root / "layer1-baseline", ignore_errors=True)
        for check in self._port_checks():
            self._assert_port_available(str(check["host"]), int(check["port"]), str(check["label"]))

    def owner_manifest(self) -> dict[str, Any]:
        return {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "hostedOnly": True,
            "requestedMode": self.requested_mode,
            "ownerRoot": str(self.owner_root),
            "layer1Baseline": {
                "registryEntry": "43B",
                "meaning": "maintained hosted per-band stock ground/operator baseline",
                "stackRoot": str(self.stack.stack_root),
                "manifestPath": str(self.stack.manifest_path),
            },
            "ownedState": {
                "lifecyclePhaseField": "lifecyclePhase",
                "phaseVocabulary": [
                    "preflight",
                    "starting",
                    "ready",
                    "startup_failed",
                    "stopping",
                    "stopped",
                    "cleanup_failed",
                ],
                "requestedMode": self.requested_mode,
                "ownedProcessSetField": "ownedProcessSet",
                "sharedRuntimeStateField": "sharedRuntimeState",
            },
            "ownedTransitions": [
                "preflight->starting",
                "starting->ready",
                "starting->startup_failed",
                "ready->stopping",
                "stopping->stopped",
                "stopping->cleanup_failed",
            ],
            "nonOwnedAdjacentState": [
                "per-band TT&C semantics remain delegated to layer-1 baseline and adjacent COMM proofs",
                "ground_ttc_gateway remains a raw relay and not an orchestration-owned relay engine",
                "stock fprime-gds behavior remains per-band and not an orchestration-owned plugin surface",
                "COMM runtime policy inside OBC remains outside the orchestration owner boundary",
            ],
            "failureBehavior": {
                "startup": "port conflicts or later hosted bring-up failures transition the owner to startup_failed and record the error without claiming COMM semantic failure",
                "cleanup": "owner records stopped when declared listeners close cleanly after teardown; otherwise cleanup_failed records the residual listener state",
            },
            "cleanupBehavior": {
                "layer1Teardown": "owner stops the delegated combined layer-1 stack and then records listener-closure summary for the declared hosted surface",
                "sharedSimulatorBoundary": "shared EPS and ADCS simulator orphan-only cleanup remains a layer-1 baseline responsibility",
            },
            "sharedRuntimeInteraction": {
                "managedBy": "delegated layer-1 combined baseline helper",
                "sharedHostedRuntime": str(self.stack.runtime_root),
                "summary": "owner coordinates one shared hosted TopCcsds runtime through the delegated layer-1 baseline without claiming COMM runtime ownership",
            },
            "nonClaims": [
                "no command authority ownership",
                "no gateway multiplexer behavior",
                "no one stock GDS heterogeneous multi-upstream behavior",
                "no COMM runtime ownership inside OBC",
                "no target-bearing simultaneous dual-link proof",
                "no RF closure",
                "no UHF reliable transfer redesign",
                "no reopening of packet-quiet, beacon-suppress, or formal file/downlink semantics",
            ],
        }

    def status(self, lifecycle_phase: str) -> dict[str, Any]:
        return {
            "formalChange": "comm-dual-link-orchestration-v1",
            "ownerType": "thin-lifecycle-owner",
            "requestedMode": self.requested_mode,
            "lifecyclePhase": lifecycle_phase,
            "orchestratorPid": self.orchestrator_pid,
            "phaseHistory": self.phase_history,
            "ownerRoot": str(self.owner_root),
            "manifestPath": str(self.manifest_path),
            "statusPath": str(self.status_path),
            "layer1Baseline": {
                "registryEntry": "43B",
                "stackRoot": str(self.stack.stack_root),
                "manifestPath": str(self.stack.manifest_path),
                "manifestPresent": self.stack.manifest_path.exists(),
            },
            "ownedProcessSet": [
                {
                    "name": managed.name,
                    "pid": managed.process.pid,
                    "alive": managed.process.poll() is None,
                }
                for managed in self._owned_processes()
            ],
            "sharedRuntimeState": {
                "path": str(self.stack.runtime_root),
                "exists": self.stack.runtime_root.exists(),
                "sharedAcrossBands": True,
                "delegatedToLayer1Baseline": True,
            },
            "failure": self.failure,
            "cleanup": self.cleanup,
        }

    def write_manifest(self) -> None:
        self._write_json_atomically(self.manifest_path, self.owner_manifest())

    def write_status(self, lifecycle_phase: str) -> None:
        self._write_json_atomically(self.status_path, self.status(lifecycle_phase))

    def _listener_summary(self) -> list[dict[str, Any]]:
        results = []
        for check in self._port_checks():
            host = str(check["host"])
            connect_host = "127.0.0.1" if host == "0.0.0.0" else host
            port = int(check["port"])
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.settimeout(0.2)
                open_now = sock.connect_ex((connect_host, port)) == 0
            results.append({"label": check["label"], "host": host, "port": port, "closed": not open_now})
        return results

    def start(self) -> None:
        self.acquire_startup_claim()
        try:
            active_owner_conflict = self._active_owner_conflict()
            if active_owner_conflict is not None:
                raise RuntimeError(active_owner_conflict)
            shutil.rmtree(self.owner_root, ignore_errors=True)
            self.write_manifest()
            if self.startup_claim_delay > 0:
                time.sleep(self.startup_claim_delay)
            self._phase("preflight", "validate owner-declared hosted surface before launching layer-1 baseline")
            try:
                self.preflight()
                self._phase("starting", "launch delegated layer-1 combined baseline")
                self.stack.start()
                self.write_manifest()
                self._phase("ready", "layer-2 orchestration owner ready")
            except Exception as exc:
                self.failure = {
                    "kind": "startup-failed",
                    "message": str(exc),
                    "timestamp": utc_timestamp(),
                }
                try:
                    self.stack.stop()
                finally:
                    self.cleanup = {
                        "summary": "partial layer-1 processes were torn down after startup failure",
                        "listenerChecks": self._listener_summary(),
                    }
                    self._phase("startup_failed", str(exc))
                raise
        finally:
            self.release_startup_claim()

    def stop(self) -> None:
        self._phase("stopping", "tear down delegated layer-1 combined baseline")
        try:
            self.stack.stop()
            listener_checks = self._listener_summary()
            all_closed = all(item["closed"] for item in listener_checks)
            self.cleanup = {
                "summary": "owned listeners closed after delegated layer-1 teardown"
                if all_closed
                else "one or more owned listeners stayed open after delegated layer-1 teardown",
                "listenerChecks": listener_checks,
            }
            self._phase("stopped" if all_closed else "cleanup_failed")
        except Exception as exc:
            self.cleanup = {
                "summary": f"cleanup raised an exception: {exc}",
                "listenerChecks": self._listener_summary(),
            }
            self._phase("cleanup_failed", str(exc))
            raise

    def print_summary(self) -> None:
        print("dual-link-orchestration: READY")
        print(f"owner-type=thin-lifecycle-owner")
        print(f"owner-root={self.owner_root}")
        print(f"manifest-json={self.manifest_path}")
        print(f"status-json={self.status_path}")
        print(f"layer1-manifest={self.stack.manifest_path}")
        print(f"shared-runtime-root={self.stack.runtime_root}")

    def run_until_stopped(self) -> None:
        self.start()
        self.print_summary()
        if self.hold_seconds is not None and self.hold_seconds > 0:
            time.sleep(self.hold_seconds)
            return
        while True:
            time.sleep(1.0)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    root_dir = pathlib.Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(
        description="Run the hosted dual-link thin lifecycle owner above the maintained per-band stock baseline."
    )
    parser.add_argument("--root-dir", default=str(root_dir))
    parser.add_argument("--bin-dir", required=True)
    parser.add_argument("--dictionary-path", required=True)
    parser.add_argument("--cli-path", default=str(root_dir / "fprime-venv/bin/fprime-cli"))
    parser.add_argument("--owner-root", default=str(default_owner_root()))
    parser.add_argument("--runtime-root", default=os.environ.get("DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT"))
    parser.add_argument(
        "--hold-seconds",
        type=float,
        default=(
            float(os.environ["ORCHESTRATION_HOLD_SECS"])
            if os.environ.get("ORCHESTRATION_HOLD_SECS")
            else (
                float(os.environ["STACK_HOLD_SECS"]) if os.environ.get("STACK_HOLD_SECS") else None
            )
        ),
    )
    parser.add_argument("--gds-bind-host", default=os.environ.get("PER_BAND_GDS_BIND_HOST", DEFAULT_GDS_BIND_HOST))
    parser.add_argument("--sband-gds-port", type=int, default=int(os.environ.get("SBAND_GDS_PORT", str(DEFAULT_SBAND_GDS_PORT))))
    parser.add_argument(
        "--sband-gds-tts-port",
        type=int,
        default=int(os.environ.get("SBAND_GDS_TTS_PORT", str(DEFAULT_SBAND_GDS_TTS_PORT))),
    )
    parser.add_argument("--uhf-gds-port", type=int, default=int(os.environ.get("UHF_GDS_PORT", str(DEFAULT_UHF_GDS_PORT))))
    parser.add_argument(
        "--uhf-gds-tts-port",
        type=int,
        default=int(os.environ.get("UHF_GDS_TTS_PORT", str(DEFAULT_UHF_GDS_TTS_PORT))),
    )
    parser.add_argument("--csp-sub-port", type=int, default=int(os.environ.get("CSP_HUB_SUB_PORT", str(DEFAULT_CSP_SUB_PORT))))
    parser.add_argument("--csp-pub-port", type=int, default=int(os.environ.get("CSP_HUB_PUB_PORT", str(DEFAULT_CSP_PUB_PORT))))
    parser.add_argument("--radio-port", type=int, default=int(os.environ.get("RADIO_PORT", str(DEFAULT_RADIO_PORT))))
    parser.add_argument("--sband-tcp-port", type=int, default=int(os.environ.get("SBAND_TCP_PORT", str(DEFAULT_SBAND_TCP_PORT))))
    parser.add_argument("--command-authority-profile", default=os.environ.get("COMMAND_AUTHORITY_PROFILE", "sband-primary"))
    parser.add_argument("--tick-ms", type=int, default=int(os.environ.get("TICK_MS", str(DEFAULT_TICK_MS))))
    parser.add_argument("--auto-ports", action="store_true", default=parse_bool(os.environ.get("PER_BAND_AUTO_PORTS"), False))
    args = parser.parse_args(list(argv))

    if not pathlib.Path(args.bin_dir).exists():
        raise SystemExit(f"bin dir not found: {args.bin_dir}")
    if not pathlib.Path(args.dictionary_path).exists():
        raise SystemExit(f"dictionary not found: {args.dictionary_path}")
    if not args.cli_path:
        args.cli_path = find_fprime_cli(root_dir)
    return args


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    owner = HostedDualLinkOrchestrationOwner(args)
    cleaned = False

    def cleanup() -> None:
        nonlocal cleaned
        if cleaned:
            return
        if not owner.phase_history:
            cleaned = True
            return
        if owner.phase_history and owner.phase_history[-1]["phase"] in {
            "startup_failed",
            "stopped",
            "cleanup_failed",
        }:
            cleaned = True
            return
        cleaned = True
        owner.stop()

    install_signal_cleanup(cleanup)
    try:
        owner.run_until_stopped()
    finally:
        cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
