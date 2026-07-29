#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"

if [[ ! -x "${PYTHON_BIN}" ]]; then
  echo "python interpreter not found at ${PYTHON_BIN}. Build/setup the repo first." >&2
  exit 1
fi

exec "${PYTHON_BIN}" - "$ROOT_DIR" "$@" <<'PY'
from __future__ import annotations

import datetime as dt
import json
import os
import pathlib
import re
import shlex
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from typing import Any


ROOT_DIR = pathlib.Path(sys.argv[1]).resolve()
ARGS = sys.argv[2:]
VALID_MODES = (
    "clean-steady",
    "owner-stop",
    "owner-kill-orphan",
    "ground-disabled-comparator",
    "all",
)
PS_FRAGMENT = "ps -ax -o pid=,ppid=,command="
FOOTPRINT_RE = re.compile(r"Physical footprint:\s+([0-9.]+)([KMGTP])", re.IGNORECASE)

MANUAL_OPS_LIB_DIR = ROOT_DIR / "scripts" / "manual_ops" / "lib"
if str(MANUAL_OPS_LIB_DIR) not in sys.path:
    sys.path.insert(0, str(MANUAL_OPS_LIB_DIR))
SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from surface_owner import ensure_surface_processes_stopped, hosted_reap_specs, reap_surface_specs


def usage() -> int:
    print(
        "usage: bash scripts/run_manual_dual_gds_memory_probe.sh "
        "[clean-steady|owner-stop|owner-kill-orphan|ground-disabled-comparator|all]",
        file=sys.stderr,
    )
    return 2


if len(ARGS) != 1 or ARGS[0] not in VALID_MODES:
    raise SystemExit(usage())


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def local_stamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d-%H%M%S")


def choose_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def run(
    args: list[str],
    *,
    env: dict[str, str] | None = None,
    cwd: pathlib.Path | None = None,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    command = [str(arg) for arg in args]
    try:
        return subprocess.run(
            command,
            cwd=(str(cwd) if cwd is not None else None),
            env=env,
            check=check,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError as exc:
        if check:
            raise
        return subprocess.CompletedProcess(command, -1, "", str(exc))


def shell_join(args: list[str]) -> str:
    return " ".join(shlex.quote(arg) for arg in args)


def append_summary(summary_log: pathlib.Path, line: str) -> None:
    summary_log.parent.mkdir(parents=True, exist_ok=True)
    with summary_log.open("a", encoding="utf-8") as handle:
        handle.write(f"{utc_now()} {line}\n")


def write_json(path: pathlib.Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


@dataclass
class ProcessInfo:
    pid: int
    ppid: int
    command: str


def iter_processes() -> list[ProcessInfo]:
    result = run(["/bin/ps", "-ax", "-o", "pid=", "-o", "ppid=", "-o", "command="], check=True)
    rows: list[ProcessInfo] = []
    for line in result.stdout.splitlines():
        parts = line.strip().split(None, 2)
        if len(parts) != 3:
            continue
        try:
            rows.append(ProcessInfo(pid=int(parts[0]), ppid=int(parts[1]), command=parts[2]))
        except ValueError:
            continue
    return rows


def find_processes(*fragments: str) -> list[ProcessInfo]:
    return [row for row in iter_processes() if all(fragment in row.command for fragment in fragments)]


def first_live_process(*fragments: str, timeout_sec: float = 20.0) -> ProcessInfo | None:
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        matches = find_processes(*fragments)
        if matches:
            return matches[0]
        time.sleep(0.5)
    return None


def kill_pid(pid: int, sig: signal.Signals) -> None:
    try:
        os.kill(pid, sig)
    except OSError:
        return


def pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def wait_pid_exit(pid: int, timeout_sec: float) -> bool:
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        if not pid_alive(pid):
            return True
        time.sleep(0.2)
    return not pid_alive(pid)


def parse_manifest(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def surface_status_payload(surface_root: pathlib.Path) -> dict[str, Any] | None:
    for candidate in (surface_root / "status.json", surface_root / "manifest.json"):
        if candidate.exists():
            return parse_manifest(candidate)
    return None


def surface_stack_manifest_payload(surface_root: pathlib.Path) -> dict[str, Any] | None:
    candidate = surface_root / "stack" / "manifest.json"
    if candidate.exists():
        return parse_manifest(candidate)
    return None


def parse_ps_sample(pid: int) -> tuple[float, float, str, str]:
    result = run(["/bin/ps", "-o", "rss=", "-o", "vsz=", "-o", "etime=", "-o", "command=", "-p", str(pid)], check=True)
    line = result.stdout.strip()
    if not line:
        raise RuntimeError(f"ps returned no row for pid {pid}")
    parts = line.split(None, 3)
    if len(parts) < 4:
        raise RuntimeError(f"unexpected ps output for pid {pid}: {line}")
    rss_kb = float(parts[0])
    vsz_kb = float(parts[1])
    etime = parts[2]
    command = parts[3]
    return rss_kb / 1024.0, vsz_kb / 1024.0, etime, command


def parse_footprint_mb(text: str) -> float | None:
    match = FOOTPRINT_RE.search(text)
    if not match:
        return None
    value = float(match.group(1))
    unit = match.group(2).upper()
    scale = {
        "K": 1.0 / 1024.0,
        "M": 1.0,
        "G": 1024.0,
        "T": 1024.0 * 1024.0,
        "P": 1024.0 * 1024.0 * 1024.0,
    }[unit]
    return value * scale


def capture_vmmap(pid: int, out_path: pathlib.Path) -> float | None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    result = run(["vmmap", str(pid)], check=False)
    filtered: list[str] = []
    for line in result.stdout.splitlines():
        if (
            "Physical footprint:" in line
            or "VM_ALLOCATE" in line
            or line.startswith("REGION TYPE")
            or line.startswith("====")
            or line.startswith("TOTAL")
        ):
            filtered.append(line)
    if result.returncode != 0 and not filtered:
        filtered.append(f"vmmap failed rc={result.returncode}")
        if result.stderr:
            filtered.append(result.stderr.strip())
    out_path.write_text("\n".join(filtered) + "\n", encoding="utf-8")
    return parse_footprint_mb(result.stdout)


def capture_lsof(pid: int, out_path: pathlib.Path) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    result = run(["lsof", "-nP", "-p", str(pid)], check=False)
    body = result.stdout
    if result.returncode != 0 and result.stderr:
        body += ("\n" if body else "") + result.stderr
    out_path.write_text(body, encoding="utf-8")


@dataclass
class SampleState:
    baseline_rss_mb: float | None = None
    baseline_footprint_mb: float | None = None
    latest_rss_mb: float | None = None
    latest_footprint_mb: float | None = None
    stop_reason: str | None = None


class ModeRunner:
    def __init__(self, mode: str, session_root: pathlib.Path) -> None:
        self.mode = mode
        self.session_root = session_root
        self.run_root = session_root / mode
        self.surface_root = self.run_root / "surface"
        self.runtime_root = self.run_root / "runtime-root"
        self.stack_root = self.run_root / "stack"
        self.samples_root = self.run_root / "samples"
        self.summary_log = self.run_root / "summary.log"
        self.ps_log = self.samples_root / "ps.log"
        self.vmmap_root = self.samples_root / "vmmap"
        self.lsof_root = self.samples_root / "lsof"
        self.synthetic_launcher_pid: int | None = None
        self.synthetic_obc_pgid: int | None = None
        self.owner_pid: int | None = None
        self.obc_pid: int | None = None
        self.obc_runtime_fragment: str | None = None
        self.obc_log: pathlib.Path | None = None
        self.state = SampleState()
        self.max_duration_sec = float(os.environ.get("MEMORY_PROBE_MAX_DURATION_SEC", "720"))
        self.ps_sample_sec = float(os.environ.get("MEMORY_PROBE_PS_SAMPLE_SEC", "15"))
        self.vmmap_sample_sec = float(os.environ.get("MEMORY_PROBE_VMMAP_SAMPLE_SEC", "60"))
        self.growth_threshold_mb = float(os.environ.get("MEMORY_PROBE_GROWTH_THRESHOLD_MB", "512"))
        self.uplink_poll_timeout_ms = int(os.environ.get("MEMORY_PROBE_UPLINK_POLL_TIMEOUT_MS", "1000"))

    def prepare(self) -> None:
        self.run_root.mkdir(parents=True, exist_ok=True)
        self.samples_root.mkdir(parents=True, exist_ok=True)
        self.vmmap_root.mkdir(parents=True, exist_ok=True)
        self.lsof_root.mkdir(parents=True, exist_ok=True)
        append_summary(self.summary_log, f"mode={self.mode} run_root={self.run_root}")
        append_summary(
            self.summary_log,
            (
                "config "
                f"max_duration_sec={self.max_duration_sec} ps_sample_sec={self.ps_sample_sec} "
                f"vmmap_sample_sec={self.vmmap_sample_sec} growth_threshold_mb={self.growth_threshold_mb} "
                f"uplink_poll_timeout_ms={self.uplink_poll_timeout_ms}"
            ),
        )

    def start(self) -> None:
        if self.mode == "ground-disabled-comparator":
            self.start_disabled_comparator()
        else:
            self.start_manual_surface()
        self.capture_initial_identity()

    def start_manual_surface(self) -> None:
        env = os.environ.copy()
        env["MANUAL_HOSTED_SURFACE_ROOT"] = str(self.surface_root)
        env["MANUAL_HOSTED_RUNTIME_ROOT"] = str(self.runtime_root)
        env["MANUAL_HOSTED_AUTO_PORTS"] = "1"
        env["GDS_UI_MODE"] = "headless"
        env["MANUAL_HOSTED_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS"] = str(self.uplink_poll_timeout_ms)
        command = ["bash", str(ROOT_DIR / "scripts/manual_ops/hosted/start_hosted_manual_surface.sh")]
        append_summary(self.summary_log, f"start_command={shell_join(command)}")
        result = run(command, env=env, cwd=ROOT_DIR, check=False)
        (self.surface_root / "start.stdout").write_text(result.stdout, encoding="utf-8")
        (self.surface_root / "start.stderr").write_text(result.stderr, encoding="utf-8")
        if result.returncode != 0:
            raise RuntimeError(
                f"manual hosted surface failed rc={result.returncode}; see {self.surface_root / 'launcher.log'}"
            )

        manifest = parse_manifest(self.surface_root / "manifest.json")
        self.owner_pid = int(manifest["ownerPid"])
        self.obc_runtime_fragment = str(manifest["sharedHostedRuntime"])
        self.obc_log = self.surface_root / "stack" / "logs" / "obc.log"
        if not self.stack_root.exists():
            self.stack_root.symlink_to(self.surface_root / "stack", target_is_directory=True)
        append_summary(
            self.summary_log,
            f"manual_surface_ready owner_pid={self.owner_pid} shared_runtime={self.obc_runtime_fragment}",
        )

    def start_disabled_comparator(self) -> None:
        self.surface_root.mkdir(parents=True, exist_ok=True)
        log_root = self.stack_root / "logs"
        log_root.mkdir(parents=True, exist_ok=True)
        runtime_root = self.runtime_root
        persistent_root = runtime_root / "persistent-data"
        staging_root = runtime_root / "staging"
        self.obc_runtime_fragment = str(runtime_root)
        self.obc_log = log_root / "obc.log"
        launcher_log = self.surface_root / "launcher.log"
        if self.obc_log.exists() or self.obc_log.is_symlink():
            self.obc_log.unlink()
        self.obc_log.symlink_to(launcher_log)
        env = os.environ.copy()
        env.update(
            {
                "GROUND_LINK_MODE": "disabled",
                "HEADLESS": "1",
                "GDS_PORT": "0",
                "MANAGE_SBAND_COMM_NODE": "0",
                "MANAGE_GROUND_TTC_GATEWAY": "0",
                "RUNTIME_ROOT": str(runtime_root),
                "PERSISTENT_ROOT": str(persistent_root),
                "STAGING_ROOT": str(staging_root),
                "LOG_ROOT": str(log_root),
                "OBC_CLEANUP_RUNTIME_ROOT": str(runtime_root),
                "CSP_HUB_SUB_PORT": str(choose_free_port()),
                "CSP_HUB_PUB_PORT": str(choose_free_port()),
                "RADIO_PORT": str(choose_free_port()),
            }
        )
        command = ["bash", str(ROOT_DIR / "scripts/run_dev_stack.sh")]
        with launcher_log.open("w", encoding="utf-8") as launcher_handle:
            launcher = subprocess.Popen(
                command,
                cwd=str(ROOT_DIR),
                env=env,
                stdout=launcher_handle,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
        self.synthetic_launcher_pid = launcher.pid
        self.synthetic_obc_pgid = launcher.pid
        append_summary(self.summary_log, f"start_command={shell_join(command)}")
        wait_deadline = time.monotonic() + 30.0
        while time.monotonic() < wait_deadline:
            if launcher_log.exists() and "Runtime mode: headless" in launcher_log.read_text(encoding="utf-8", errors="replace"):
                break
            if launcher.poll() is not None:
                raise RuntimeError(f"disabled comparator launcher exited rc={launcher.returncode}")
            time.sleep(0.5)
        else:
            raise RuntimeError(f"disabled comparator failed to reach headless runtime: {self.obc_log}")

        payload = {
            "formalChange": "manual-dual-gds-memory-probe-local",
            "surfaceRoot": str(self.surface_root),
            "surfaceType": "ground-disabled-comparator",
            "lifecycleState": "running",
            "timestamp": utc_now(),
            "ownerPid": self.synthetic_launcher_pid,
            "sharedHostedRuntime": str(runtime_root),
            "nonClaims": [
                "no manual dual-GDS owner path",
                "no ground_ttc_gateway or COMM node 5 poll path",
            ],
        }
        write_json(self.surface_root / "manifest.json", payload)
        write_json(self.surface_root / "status.json", payload)
        self.owner_pid = self.synthetic_launcher_pid
        append_summary(self.summary_log, f"disabled_comparator_ready launcher_pid={self.synthetic_launcher_pid}")

    def capture_initial_identity(self) -> None:
        assert self.obc_runtime_fragment is not None
        match = first_live_process("/OBC", self.obc_runtime_fragment, timeout_sec=30.0)
        if match is None:
            raise RuntimeError(f"failed to find OBC pid for runtime fragment {self.obc_runtime_fragment}")
        self.obc_pid = match.pid
        append_summary(self.summary_log, f"obc_pid={self.obc_pid} ppid={match.ppid}")
        capture_lsof(self.obc_pid, self.lsof_root / "start.txt")
        self.sample_ps("baseline")
        self.sample_vmmap("baseline")

    def sample_ps(self, label: str) -> None:
        assert self.obc_pid is not None
        rss_mb, vsz_mb, etime, command = parse_ps_sample(self.obc_pid)
        if self.state.baseline_rss_mb is None:
            self.state.baseline_rss_mb = rss_mb
        self.state.latest_rss_mb = rss_mb
        with self.ps_log.open("a", encoding="utf-8") as handle:
            handle.write(
                f"{utc_now()} label={label} pid={self.obc_pid} rss_mb={rss_mb:.2f} "
                f"vsz_mb={vsz_mb:.2f} etime={etime} command={command}\n"
            )
        append_summary(
            self.summary_log,
            f"ps_sample label={label} pid={self.obc_pid} rss_mb={rss_mb:.2f} vsz_mb={vsz_mb:.2f} etime={etime}",
        )

    def sample_vmmap(self, label: str) -> None:
        assert self.obc_pid is not None
        out_path = self.vmmap_root / f"{label}.txt"
        footprint_mb = capture_vmmap(self.obc_pid, out_path)
        if self.state.baseline_footprint_mb is None:
            self.state.baseline_footprint_mb = footprint_mb
        self.state.latest_footprint_mb = footprint_mb
        append_summary(
            self.summary_log,
            f"vmmap_sample label={label} pid={self.obc_pid} physical_footprint_mb={footprint_mb}",
        )

    def observe_until_threshold(self) -> None:
        assert self.obc_pid is not None
        deadline = time.monotonic() + self.max_duration_sec
        next_ps = time.monotonic() + self.ps_sample_sec
        next_vmmap = time.monotonic() + self.vmmap_sample_sec
        while True:
            if not pid_alive(self.obc_pid):
                self.state.stop_reason = "obc-exited"
                return
            now = time.monotonic()
            if now >= deadline:
                self.state.stop_reason = "max-duration"
                return
            if now >= next_ps:
                self.sample_ps(f"t{int(now)}")
                next_ps = now + self.ps_sample_sec
                if (
                    self.state.baseline_rss_mb is not None
                    and self.state.latest_rss_mb is not None
                    and (self.state.latest_rss_mb - self.state.baseline_rss_mb) >= self.growth_threshold_mb
                ):
                    self.state.stop_reason = "rss-growth-threshold"
                    return
            if now >= next_vmmap:
                self.sample_vmmap(f"t{int(now)}")
                next_vmmap = now + self.vmmap_sample_sec
                if (
                    self.state.baseline_footprint_mb is not None
                    and self.state.latest_footprint_mb is not None
                    and (self.state.latest_footprint_mb - self.state.baseline_footprint_mb) >= self.growth_threshold_mb
                ):
                    self.state.stop_reason = "footprint-growth-threshold"
                    return
            time.sleep(1.0)

    def stop_mode_action(self) -> None:
        if self.mode == "owner-stop":
            result = self.run_hosted_surface_stop("stop")
            append_summary(self.summary_log, f"owner_stop rc={result.returncode}")
            if result.returncode != 0:
                self.state.stop_reason = "owner-stop-cleanup-failed"
                raise RuntimeError(
                    f"owner-stop hosted surface cleanup failed rc={result.returncode}; "
                    f"see {self.surface_root / 'stop.stderr'}"
                )
            time.sleep(5.0)
            residue = find_processes("/OBC", self.obc_runtime_fragment or "")
            self.state.stop_reason = "owner-stop-clean" if not residue else "owner-stop-residue"
            return

        if self.mode == "owner-kill-orphan":
            assert self.owner_pid is not None
            kill_pid(self.owner_pid, signal.SIGKILL)
            append_summary(self.summary_log, f"owner_killed pid={self.owner_pid} signal=SIGKILL")
            self.observe_until_threshold()
            if self.state.stop_reason == "obc-exited":
                self.state.stop_reason = "owner-kill-orphan-self-clean"
            elif self.state.stop_reason == "max-duration":
                self.state.stop_reason = "owner-kill-orphan-residue"
            elif self.state.stop_reason == "rss-growth-threshold":
                self.state.stop_reason = "owner-kill-orphan-rss-growth-threshold"
            elif self.state.stop_reason == "footprint-growth-threshold":
                self.state.stop_reason = "owner-kill-orphan-footprint-growth-threshold"
            return

        self.observe_until_threshold()

    def cleanup(self) -> None:
        if self.mode == "ground-disabled-comparator":
            self.cleanup_disabled_comparator()
        else:
            self.cleanup_manual_surface()
        if self.obc_pid is not None and pid_alive(self.obc_pid):
            capture_lsof(self.obc_pid, self.lsof_root / "end.txt")
        else:
            (self.lsof_root / "end.txt").write_text("obc-not-running\n", encoding="utf-8")
        append_summary(self.summary_log, f"stop_reason={self.state.stop_reason}")

    def run_hosted_surface_stop(self, label: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["MANUAL_HOSTED_SURFACE_ROOT"] = str(self.surface_root)
        result = run(
            ["bash", str(ROOT_DIR / "scripts/manual_ops/hosted/stop_hosted_manual_surface.sh")],
            env=env,
            cwd=ROOT_DIR,
            check=False,
        )
        stdout_path = self.surface_root / f"{label}.stdout"
        stderr_path = self.surface_root / f"{label}.stderr"
        if result.stdout:
            stdout_path.write_text(result.stdout, encoding="utf-8")
        if result.stderr:
            stderr_path.write_text(result.stderr, encoding="utf-8")
        return result

    def reap_hosted_surface_payload(self, payload: dict[str, Any]) -> None:
        specs = hosted_reap_specs(payload)
        reap_surface_specs(specs)
        ensure_surface_processes_stopped(specs)

    def hosted_owner_process(self) -> ProcessInfo | None:
        return first_live_process("surface_owner.py", "hosted-owner", str(self.surface_root), timeout_sec=1.0)

    def terminate_hosted_owner_process(self, proc: ProcessInfo | None) -> None:
        if proc is None:
            return
        self.owner_pid = proc.pid
        kill_pid(proc.pid, signal.SIGTERM)
        if not wait_pid_exit(proc.pid, 8.0):
            kill_pid(proc.pid, signal.SIGKILL)
            wait_pid_exit(proc.pid, 2.0)
        append_summary(self.summary_log, f"startup_failure_owner_cleanup pid={proc.pid}")

    def cleanup_no_metadata_start_failure(self) -> None:
        owner_proc = self.hosted_owner_process()
        self.terminate_hosted_owner_process(owner_proc)
        stack_payload = surface_stack_manifest_payload(self.surface_root)
        if stack_payload is not None:
            self.reap_hosted_surface_payload(stack_payload)
            append_summary(self.summary_log, "startup_failure_cleanup=no-metadata-stack-reap")
            return
        residue = find_processes("/OBC", str(self.runtime_root))
        for proc in residue:
            kill_pid(proc.pid, signal.SIGKILL)
        if residue:
            append_summary(
                self.summary_log,
                f"startup_failure_residue_killed={','.join(str(proc.pid) for proc in residue)}",
            )
        else:
            append_summary(self.summary_log, "startup_failure_cleanup=no-metadata-no-residue")

    def cleanup_manual_surface(self) -> None:
        result = self.run_hosted_surface_stop("cleanup")
        append_summary(self.summary_log, f"cleanup_hosted_surface rc={result.returncode}")
        payload = surface_status_payload(self.surface_root)
        if result.returncode != 0 and payload is not None:
            self.reap_hosted_surface_payload(payload)
        if result.returncode != 0:
            raise RuntimeError(
                f"hosted surface cleanup failed rc={result.returncode}; "
                f"see {self.surface_root / 'cleanup.stderr'}"
            )
        if self.obc_runtime_fragment:
            residue = find_processes("/OBC", self.obc_runtime_fragment)
            for proc in residue:
                kill_pid(proc.pid, signal.SIGKILL)
            if residue:
                append_summary(self.summary_log, f"cleanup_residue_killed={','.join(str(proc.pid) for proc in residue)}")

    def cleanup_after_start_failure(self) -> None:
        payload = surface_status_payload(self.surface_root)
        if payload is None:
            if self.mode == "ground-disabled-comparator" and self.synthetic_obc_pgid is not None:
                if self.synthetic_launcher_pid is not None:
                    self.owner_pid = self.synthetic_launcher_pid
                if not self.obc_runtime_fragment:
                    self.obc_runtime_fragment = str(self.runtime_root)
                self.obc_log = self.surface_root / "stack" / "logs" / "obc.log"
                self.cleanup()
                append_summary(self.summary_log, "startup_failure_cleanup=disabled-comparator-no-metadata")
                return
            self.cleanup_no_metadata_start_failure()
            return
        if payload is not None:
            owner_pid = payload.get("ownerPid")
            if isinstance(owner_pid, int):
                self.owner_pid = owner_pid
            runtime_fragment = payload.get("sharedHostedRuntime")
            if isinstance(runtime_fragment, str) and runtime_fragment:
                self.obc_runtime_fragment = runtime_fragment
            self.obc_log = self.surface_root / "stack" / "logs" / "obc.log"
            lifecycle = payload.get("lifecycleState")
            if lifecycle == "startup_failed" and self.owner_pid is not None and not pid_alive(self.owner_pid):
                self.reap_hosted_surface_payload(payload)
                append_summary(self.summary_log, "startup_failure_cleanup=preserved-startup-failed-terminal-state")
                return
        self.cleanup()

    def cleanup_disabled_comparator(self) -> None:
        if self.synthetic_obc_pgid is not None:
            try:
                os.killpg(self.synthetic_obc_pgid, signal.SIGTERM)
            except OSError:
                pass
            time.sleep(3.0)
            try:
                os.killpg(self.synthetic_obc_pgid, signal.SIGKILL)
            except OSError:
                pass
        if self.obc_runtime_fragment:
            residue = find_processes("/OBC", self.obc_runtime_fragment)
            for proc in residue:
                kill_pid(proc.pid, signal.SIGKILL)
            if residue:
                append_summary(self.summary_log, f"cleanup_residue_killed={','.join(str(proc.pid) for proc in residue)}")
        payload = {
            "formalChange": "manual-dual-gds-memory-probe-local",
            "surfaceRoot": str(self.surface_root),
            "surfaceType": "ground-disabled-comparator",
            "lifecycleState": "stopped",
            "timestamp": utc_now(),
            "ownerPid": self.owner_pid,
            "sharedHostedRuntime": str(self.runtime_root),
        }
        write_json(self.surface_root / "status.json", payload)
        write_json(self.surface_root / "manifest.json", payload)

    def finalize(self) -> None:
        if self.obc_pid is not None and pid_alive(self.obc_pid):
            try:
                self.sample_ps("final")
            except Exception:
                pass
            try:
                self.sample_vmmap("final")
            except Exception:
                pass
        final_payload = {
            "mode": self.mode,
            "runRoot": str(self.run_root),
            "surfaceRoot": str(self.surface_root),
            "runtimeRoot": str(self.runtime_root),
            "ownerPid": self.owner_pid,
            "obcPid": self.obc_pid,
            "stopReason": self.state.stop_reason,
            "baselineRssMb": self.state.baseline_rss_mb,
            "latestRssMb": self.state.latest_rss_mb,
            "baselineFootprintMb": self.state.baseline_footprint_mb,
            "latestFootprintMb": self.state.latest_footprint_mb,
            "growthThresholdMb": self.growth_threshold_mb,
            "uplinkPollTimeoutMs": self.uplink_poll_timeout_ms,
            "timestamp": utc_now(),
        }
        write_json(self.run_root / "result.json", final_payload)

    def execute(self) -> dict[str, Any]:
        self.prepare()
        started = False
        cleanup_error: Exception | None = None
        try:
            self.start()
            started = True
            try:
                self.stop_mode_action()
            except KeyboardInterrupt:
                if self.state.stop_reason is None:
                    self.state.stop_reason = "interrupted"
                raise
        except Exception:
            if self.state.stop_reason is None:
                self.state.stop_reason = "startup-failed"
            if not started:
                try:
                    self.cleanup_after_start_failure()
                except Exception as cleanup_exc:
                    append_summary(self.summary_log, f"startup_failure_cleanup_error={cleanup_exc}")
            raise
        finally:
            if started:
                try:
                    self.cleanup()
                except Exception as exc:
                    cleanup_error = exc
                    append_summary(self.summary_log, f"cleanup_error={exc}")
            self.finalize()
            if cleanup_error is not None and sys.exc_info()[0] is None:
                raise cleanup_error
        return json.loads((self.run_root / "result.json").read_text(encoding="utf-8"))


def main() -> int:
    selected = [ARGS[0]] if ARGS[0] != "all" else [
        "clean-steady",
        "owner-stop",
        "owner-kill-orphan",
        "ground-disabled-comparator",
    ]
    session_root = pathlib.Path(os.environ.get("MEMORY_PROBE_ROOT", "/tmp/manual-dual-gds/memory-probe")).resolve() / local_stamp()
    session_root.mkdir(parents=True, exist_ok=True)
    results: list[dict[str, Any]] = []
    interrupted = False
    failed = False
    for mode in selected:
        runner = ModeRunner(mode, session_root)
        try:
            result = runner.execute()
        except KeyboardInterrupt:
            interrupted = True
            result_path = runner.run_root / "result.json"
            if result_path.exists():
                result = json.loads(result_path.read_text(encoding="utf-8"))
            else:
                result = {
                    "mode": mode,
                    "runRoot": str(runner.run_root),
                    "surfaceRoot": str(runner.surface_root),
                    "runtimeRoot": str(runner.runtime_root),
                    "ownerPid": runner.owner_pid,
                    "obcPid": runner.obc_pid,
                    "stopReason": runner.state.stop_reason or "interrupted",
                    "timestamp": utc_now(),
                }
        results.append(result)
        print(f"{mode}: {result['stopReason']} root={result['runRoot']}")
        stop_reason = result.get("stopReason")
        if mode == "clean-steady" and stop_reason != "max-duration":
            failed = True
        elif mode == "ground-disabled-comparator" and stop_reason != "max-duration":
            failed = True
        elif mode == "owner-stop" and stop_reason != "owner-stop-clean":
            failed = True
        if interrupted:
            break
    write_json(session_root / "session-results.json", {"timestamp": utc_now(), "results": results})
    print(f"memory-probe-session={session_root}")
    if interrupted:
        return 130
    return 1 if failed else 0


raise SystemExit(main())
PY
