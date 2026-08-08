from __future__ import annotations

import pathlib
from dataclasses import dataclass
from typing import Any

from manual_ops.lib.common import ensure_dir, find_local_tool
from probe_process_utils import ManagedProcess, cleanup_managed_processes, start_managed_process

from .parsers import parse_channel_line, parse_event_line
from .registry import SurfaceContext
from .snapshots import SnapshotStore


@dataclass
class ListenerHandle:
    context_id: str
    bands: tuple[str, ...]
    root: pathlib.Path
    events_log: pathlib.Path
    channels_log: pathlib.Path
    native_event_log: pathlib.Path
    native_channel_log: pathlib.Path
    events_process: ManagedProcess
    channels_process: ManagedProcess | None
    listener_key: tuple[str, int, str, int | None]
    channels_enabled: bool
    events_offset: int = 0
    channels_offset: int = 0


class ListenerManager:
    def __init__(self, runtime_root: pathlib.Path, snapshots: SnapshotStore) -> None:
        self.runtime_root = ensure_dir(runtime_root)
        self.snapshots = snapshots
        self.cli_path = find_local_tool("fprime-cli")
        self._handles: dict[tuple[str, tuple[str, int, str, int | None]], ListenerHandle] = {}

    def sync_contexts(self, contexts: dict[str, SurfaceContext]) -> None:
        wanted: set[tuple[str, tuple[str, int, str]]] = set()
        for context_id, context in contexts.items():
            if not context.is_running:
                continue
            grouped: dict[tuple[str, int, str, int | None], list[str]] = {}
            for band_name, band in context.bands.items():
                grouped.setdefault(band.listener_key, []).append(band_name)
            for listener_key, band_names in grouped.items():
                key = (context_id, listener_key)
                wanted.add(key)
                handle = self._handles.get(key)
                normalized_bands = tuple(sorted(band_names))
                channels_enabled = self._channels_enabled(context, normalized_bands)
                if handle is not None and not self._listener_processes_running(handle):
                    self._stop_listener(key)
                    handle = None
                if (
                    handle is None
                    or handle.bands != normalized_bands
                    or handle.channels_enabled != channels_enabled
                ):
                    for stale_key in self._conflicting_listener_keys(context_id, listener_key):
                        self._stop_listener(stale_key)
                    self._restart_listener(context_id, normalized_bands, listener_key, context, channels_enabled)
        for key in sorted(set(self._handles) - wanted):
            self._stop_listener(key)

    def poll(self) -> None:
        for handle in list(self._handles.values()):
            self._poll_events(handle)
            self._poll_channels(handle)

    def shutdown(self) -> None:
        for key in list(self._handles):
            self._stop_listener(key)

    def _restart_listener(
        self,
        context_id: str,
        band_names: tuple[str, ...],
        listener_key: tuple[str, int, str, int | None],
        context: SurfaceContext,
        channels_enabled: bool,
    ) -> None:
        self._stop_listener((context_id, listener_key))
        for band_name in band_names:
            self.snapshots.clear_band(context_id, band_name)
        primary_band = band_names[0]
        band = context.band(primary_band)
        root = ensure_dir(self.runtime_root / context_id / primary_band)
        events_log = root / "events.log"
        channels_log = root / "channels.log"
        native_log_root = ensure_dir(root / "native-events")
        native_event_log = native_log_root / "event.log"
        native_channel_log = native_log_root / "channel.log"
        initial_event_offset = native_event_log.stat().st_size if native_event_log.exists() else 0
        initial_channel_offset = native_channel_log.stat().st_size if native_channel_log.exists() else 0
        common_args = [
            "--dictionary",
            str(band.dictionary_path),
            "--no-zmq",
            "-l",
            str(native_log_root),
            "--log-directly",
            "--tts-port",
            str(band.gds_tts_port),
        ]
        events_process = start_managed_process(
            f"{context_id}-{primary_band}-events",
            [self.cli_path, "events", *common_args],
            events_log,
            stale_match_groups=(("events", f"--tts-port {band.gds_tts_port}", str(native_log_root)),),
        )
        channels_process = None
        if channels_enabled:
            channels_process = start_managed_process(
                f"{context_id}-{primary_band}-channels",
                [self.cli_path, "channels", *common_args],
                channels_log,
                stale_match_groups=(("channels", f"--tts-port {band.gds_tts_port}", str(native_log_root)),),
            )
        self._handles[(context_id, listener_key)] = ListenerHandle(
            context_id=context_id,
            bands=band_names,
            root=root,
            events_log=events_log,
            channels_log=channels_log,
            native_event_log=native_event_log,
            native_channel_log=native_channel_log,
            events_process=events_process,
            channels_process=channels_process,
            listener_key=listener_key,
            channels_enabled=channels_enabled,
            events_offset=initial_event_offset,
            channels_offset=initial_channel_offset,
        )

    def _stop_listener(self, key: tuple[str, tuple[str, int, str, int | None]]) -> None:
        handle = self._handles.pop(key, None)
        if handle is None:
            return
        for band_name in handle.bands:
            self.snapshots.clear_band(handle.context_id, band_name)
        processes = [handle.events_process]
        if handle.channels_process is not None:
            processes.insert(0, handle.channels_process)
        cleanup_managed_processes(processes, timeout_sec=5.0)

    def _poll_events(self, handle: ListenerHandle) -> None:
        handle.events_offset = self._poll_log(
            handle.native_event_log,
            handle.events_offset,
            lambda line: self._handle_event_line(handle.context_id, handle.bands, line),
        )

    def _poll_channels(self, handle: ListenerHandle) -> None:
        if not handle.channels_enabled or handle.channels_process is None:
            return
        handle.channels_offset = self._poll_log(
            handle.native_channel_log,
            handle.channels_offset,
            lambda line: self._handle_channel_line(handle.context_id, handle.bands, line),
        )

    def _channels_enabled(self, context: SurfaceContext, band_names: tuple[str, ...]) -> bool:
        if context.context_id != "target-manual-ground-dual-gds":
            return True
        for band_name in band_names:
            secure_state = context.band(band_name).secure_state
            if secure_state and not bool(secure_state.get("invalidated", False)):
                return True
        return False

    def _listener_processes_running(self, handle: ListenerHandle) -> bool:
        if handle.events_process.process.poll() is not None:
            return False
        if handle.channels_enabled:
            if handle.channels_process is None:
                return False
            if handle.channels_process.process.poll() is not None:
                return False
        return True

    def _conflicting_listener_keys(
        self,
        context_id: str,
        listener_key: tuple[str, int, str, int | None],
    ) -> list[tuple[str, tuple[str, int, str, int | None]]]:
        identity = listener_key[:3]
        return [
            key
            for key, handle in self._handles.items()
            if key[0] == context_id and handle.listener_key[:3] == identity and handle.listener_key != listener_key
        ]

    def _poll_log(self, path: pathlib.Path, offset: int, line_handler: Any) -> int:
        if not path.exists():
            return offset
        try:
            file_size = path.stat().st_size
            if offset > file_size:
                offset = 0
            with path.open("rb") as stream:
                stream.seek(offset)
                chunk = stream.read()
                next_offset = stream.tell()
        except OSError:
            return offset
        for line in chunk.decode("utf-8", errors="replace").splitlines():
            line_handler(line)
        return next_offset

    def _handle_event_line(self, context_id: str, bands: tuple[str, ...], line: str) -> None:
        event = parse_event_line(line)
        if event is None:
            return
        for band in bands:
            self.snapshots.append_event(context_id, band, event)

    def _handle_channel_line(self, context_id: str, bands: tuple[str, ...], line: str) -> None:
        channel = parse_channel_line(line)
        if channel is None:
            return
        for band in bands:
            self.snapshots.update_channel(context_id, band, channel)
