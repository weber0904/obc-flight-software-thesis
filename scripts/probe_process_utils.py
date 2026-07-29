from __future__ import annotations

import os
import pathlib
import shlex
import signal
import subprocess
import time
from dataclasses import dataclass
from typing import Callable, Iterable, Sequence, TextIO

GDS_PROCESS_MARKERS: tuple[str, ...] = (
    "fprime-gds",
    "fprime_gds.executables.comm",
    "fprime_gds.executables.tcpserver",
    "CustomDataHandlers",
    "fprime-cli",
    "ground_ttc_gateway",
)


@dataclass
class ManagedProcess:
    name: str
    process: subprocess.Popen[str]
    handle: TextIO
    stale_match_groups: tuple[tuple[str, ...], ...] = ()
    stale_match_markers: tuple[str, ...] = GDS_PROCESS_MARKERS
    stale_require_orphan: bool = False


@dataclass(frozen=True)
class ProcessMatch:
    pid: int
    ppid: int
    command: str


def build_fragment_group(*fragments: object) -> tuple[str, ...]:
    return tuple(str(fragment) for fragment in fragments if str(fragment))


def build_gds_stale_match_groups(
    *,
    ip_port: int | str | None = None,
    tts_port: int | str | None = None,
    file_storage_dir: str | pathlib.Path | None = None,
    extra_fragments: Sequence[object] = (),
) -> tuple[tuple[str, ...], ...]:
    base = build_fragment_group(*extra_fragments)
    groups: list[tuple[str, ...]] = []
    if ip_port is not None:
        groups.append(base + build_fragment_group(f"--ip-port {ip_port}"))
    if tts_port is not None:
        groups.append(base + build_fragment_group(f"--tts-port {tts_port}"))
    if file_storage_dir is not None:
        groups.append(base + build_fragment_group(f"--file-storage-directory {file_storage_dir}"))
    return tuple(groups)


def build_gateway_stale_match_groups(
    *,
    gds_port: int | str,
    link_identity: str,
    southbound_fragment: object | None = None,
) -> tuple[tuple[str, ...], ...]:
    southbound = build_fragment_group(southbound_fragment) if southbound_fragment is not None else ()
    return (
        southbound + build_fragment_group(f"--gds-port {gds_port}", f"--link-identity {link_identity}"),
    )


def _iter_process_rows() -> Iterable[tuple[int, int, str]]:
    try:
        result = subprocess.run(
            ["ps", "-ax", "-o", "pid=", "-o", "ppid=", "-o", "command="],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, PermissionError, subprocess.CalledProcessError):
        return
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        parts = stripped.split(None, 2)
        if len(parts) < 3:
            continue
        pid_text, ppid_text, command = parts
        try:
            yield int(pid_text), int(ppid_text), command
        except ValueError:
            continue


def _command_matches_any_group(command: str, match_groups: Sequence[Sequence[str]]) -> bool:
    for group in match_groups:
        if group and all(fragment in command for fragment in group):
            return True
    return False


def _command_matches_markers(command: str, markers: Sequence[str]) -> bool:
    if not markers:
        return True
    return any(marker in command for marker in markers)


def find_matching_processes(
    match_groups: Sequence[Sequence[str]],
    *,
    markers: Sequence[str] = GDS_PROCESS_MARKERS,
    require_orphan: bool = False,
    exclude_pids: Sequence[int] = (),
) -> list[ProcessMatch]:
    if not match_groups:
        return []

    excluded = set(exclude_pids)
    matches: list[ProcessMatch] = []
    for pid, ppid, command in _iter_process_rows():
        if pid in excluded:
            continue
        if require_orphan and ppid != 1:
            continue
        if not _command_matches_markers(command, markers):
            continue
        if not _command_matches_any_group(command, match_groups):
            continue
        matches.append(ProcessMatch(pid=pid, ppid=ppid, command=command))
    return matches


def _kill_pid(pid: int, sig: signal.Signals) -> None:
    try:
        os.kill(pid, sig)
    except (ProcessLookupError, PermissionError):
        return


def _pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def reap_matching_processes(
    match_groups: Sequence[Sequence[str]],
    *,
    markers: Sequence[str] = GDS_PROCESS_MARKERS,
    require_orphan: bool = False,
    exclude_pids: Sequence[int] = (),
    term_wait_sec: float = 1.0,
) -> list[int]:
    if not match_groups:
        return []

    matched_pids = [
        match.pid
        for match in find_matching_processes(
            match_groups,
            markers=markers,
            require_orphan=require_orphan,
            exclude_pids=exclude_pids,
        )
    ]

    for pid in matched_pids:
        _kill_pid(pid, signal.SIGTERM)

    deadline = time.monotonic() + term_wait_sec
    survivors = set(matched_pids)
    while survivors and time.monotonic() < deadline:
        survivors = {pid for pid in survivors if _pid_alive(pid)}
        if survivors:
            time.sleep(0.1)

    for pid in survivors:
        _kill_pid(pid, signal.SIGKILL)

    return matched_pids


def wait_for_no_matching_processes(
    match_groups: Sequence[Sequence[str]],
    *,
    markers: Sequence[str] = GDS_PROCESS_MARKERS,
    require_orphan: bool = False,
    exclude_pids: Sequence[int] = (),
    timeout_sec: float = 5.0,
    poll_interval_sec: float = 0.2,
) -> list[ProcessMatch]:
    deadline = time.monotonic() + timeout_sec
    while True:
        matches = find_matching_processes(
            match_groups,
            markers=markers,
            require_orphan=require_orphan,
            exclude_pids=exclude_pids,
        )
        if not matches:
            return []
        if time.monotonic() >= deadline:
            return matches
        time.sleep(poll_interval_sec)


def start_managed_process(
    name: str,
    args: Sequence[str],
    log_path: pathlib.Path,
    *,
    env: dict[str, str] | None = None,
    cwd: str | pathlib.Path | None = None,
    stdin: int | TextIO | None = None,
    stale_match_groups: Sequence[Sequence[str]] = (),
    stale_match_markers: Sequence[str] = GDS_PROCESS_MARKERS,
    stale_require_orphan: bool = False,
) -> ManagedProcess:
    handle = pathlib.Path(log_path).open("a", encoding="utf-8", buffering=1)
    return start_managed_process_with_handle(
        name,
        args,
        handle,
        env=env,
        cwd=cwd,
        stdin=stdin,
        stale_match_groups=stale_match_groups,
        stale_match_markers=stale_match_markers,
        stale_require_orphan=stale_require_orphan,
    )


def start_managed_process_with_handle(
    name: str,
    args: Sequence[str],
    handle: TextIO,
    *,
    env: dict[str, str] | None = None,
    cwd: str | pathlib.Path | None = None,
    stdin: int | TextIO | None = None,
    stale_match_groups: Sequence[Sequence[str]] = (),
    stale_match_markers: Sequence[str] = GDS_PROCESS_MARKERS,
    stale_require_orphan: bool = False,
) -> ManagedProcess:
    if stale_match_groups:
        reap_matching_processes(
            stale_match_groups,
            markers=stale_match_markers,
            require_orphan=stale_require_orphan,
        )

    handle.write("$ " + shlex.join(str(arg) for arg in args) + "\n")
    handle.flush()
    process = subprocess.Popen(
        [str(arg) for arg in args],
        cwd=(str(cwd) if cwd is not None else None),
        env=env,
        stdin=(stdin if stdin is not None else subprocess.DEVNULL),
        stdout=handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        start_new_session=True,
    )
    return ManagedProcess(
        name=name,
        process=process,
        handle=handle,
        stale_match_groups=tuple(tuple(group) for group in stale_match_groups),
        stale_match_markers=tuple(stale_match_markers),
        stale_require_orphan=stale_require_orphan,
    )


def stop_managed_process(managed: ManagedProcess, *, timeout_sec: float = 5.0) -> None:
    if managed.process.poll() is None:
        try:
            os.killpg(os.getpgid(managed.process.pid), signal.SIGTERM)
        except (ProcessLookupError, PermissionError):
            pass
        try:
            managed.process.wait(timeout=timeout_sec)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            managed.process.wait(timeout=timeout_sec)
    if managed.stale_match_groups:
        reap_matching_processes(
            managed.stale_match_groups,
            markers=managed.stale_match_markers,
            require_orphan=managed.stale_require_orphan,
            exclude_pids=(managed.process.pid,),
        )
    managed.handle.close()


def cleanup_managed_processes(processes: list[ManagedProcess], *, timeout_sec: float = 5.0) -> None:
    managed_processes = list(reversed(processes))
    processes.clear()
    stale_specs = [
        (
            managed.stale_match_groups,
            managed.stale_match_markers,
            managed.stale_require_orphan,
        )
        for managed in managed_processes
        if managed.stale_match_groups
    ]

    for managed in managed_processes:
        if managed.process.poll() is None:
            try:
                os.killpg(os.getpgid(managed.process.pid), signal.SIGTERM)
            except (ProcessLookupError, PermissionError):
                pass

    for managed in managed_processes:
        if managed.process.poll() is None:
            try:
                managed.process.wait(timeout=timeout_sec)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(os.getpgid(managed.process.pid), signal.SIGKILL)
                except (ProcessLookupError, PermissionError):
                    pass
                managed.process.wait(timeout=timeout_sec)
        if managed.stale_match_groups:
            reap_matching_processes(
                managed.stale_match_groups,
                markers=managed.stale_match_markers,
                require_orphan=managed.stale_require_orphan,
                exclude_pids=(managed.process.pid,),
            )
        managed.handle.close()

    # Some GDS subprocesses can outlive the parent briefly and only become visible as
    # stale matches after the managed parent has already exited. Reap a few extra
    # rounds so per-run observers do not survive into the next timing window.
    if stale_specs:
        deadline = time.monotonic() + max(timeout_sec, 2.0)
        while time.monotonic() < deadline:
            matched_total = 0
            for match_groups, markers, require_orphan in stale_specs:
                matched_total += len(
                    reap_matching_processes(
                        match_groups,
                        markers=markers,
                        require_orphan=require_orphan,
                        term_wait_sec=0.2,
                    )
                )
            if matched_total == 0:
                break
            time.sleep(0.2)


def install_signal_cleanup(cleanup_fn: Callable[[], None]) -> None:
    state = {"ran": False}

    def handler(signum: int, _frame: object) -> None:
        if not state["ran"]:
            state["ran"] = True
            cleanup_fn()
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGINT, handler)
