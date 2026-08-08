from __future__ import annotations

import pathlib
import threading
import json
from collections import deque
from dataclasses import dataclass, replace
from datetime import datetime, timezone
from typing import Any

from manual_ops.lib.common import atomic_write_json, ensure_dir

from .parsers import (
    CHANNEL_REFRESH_COMMANDS,
    DASHBOARD_CHANNELS,
    EVENT_BASED_COMMANDS,
    LIVE_TREND_GROUPS,
    READBACK_VIEWER_TABS,
    ParsedChannel,
    ParsedEvent,
    parse_structured_event,
)


@dataclass(frozen=True)
class ChannelSnapshot:
    timestamp: str | None
    qualified_name: str
    channel_name: str
    channel_id: int
    value: str
    raw: str
    generation: int
    observation_source: str
    observation_at: str
    refresh_command: str | None

    @classmethod
    def from_parsed(
        cls,
        parsed: ParsedChannel,
        generation: int,
        *,
        observation_source: str,
        observation_at: str,
        refresh_command: str | None,
    ) -> "ChannelSnapshot":
        return cls(
            timestamp=parsed.timestamp,
            qualified_name=parsed.qualified_name,
            channel_name=parsed.channel_name,
            channel_id=parsed.channel_id,
            value=parsed.value,
            raw=parsed.raw,
            generation=generation,
            observation_source=observation_source,
            observation_at=observation_at,
            refresh_command=refresh_command,
        )

    def to_json(self) -> dict[str, Any]:
        return {
            "timestamp": self.timestamp,
            "qualifiedName": self.qualified_name,
            "channelName": self.channel_name,
            "channelId": self.channel_id,
            "value": self.value,
            "raw": self.raw,
            "generation": self.generation,
            "observationSource": self.observation_source,
            "observationAt": self.observation_at,
            "refreshCommand": self.refresh_command,
        }


def _utc_timestamp() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


class SnapshotStore:
    def __init__(
        self,
        runtime_root: pathlib.Path,
        event_ring_size: int = 200,
        trend_history_size: int = 300,
        beacon_history_size: int = 50,
        *,
        console_session_id: str | None = None,
    ) -> None:
        self.runtime_root = ensure_dir(runtime_root)
        self.events_path = self.runtime_root / "recent-events.json"
        self.channels_path = self.runtime_root / "latest-channels.json"
        self.readback_path = self.runtime_root / "readback-cache.json"
        self.latest_beacon_path = self.runtime_root / "latest-beacon.json"
        self.beacon_history_path = self.runtime_root / "beacon-history.json"
        self.history_path = self.runtime_root / "action-history.jsonl"
        self.packet_lab_history_path = self.runtime_root / "packet-lab-history.jsonl"
        self.console_session_id = console_session_id or "console-session"
        self._event_ring: dict[tuple[str, str], deque[dict[str, Any]]] = {}
        self._channels: dict[tuple[str, str], dict[str, ChannelSnapshot]] = {}
        self._readback_cache: dict[str, dict[str, Any]] = {}
        self._beacon_latest: dict[str, dict[str, Any]] = {}
        self._beacon_history: dict[str, deque[dict[str, Any]]] = {}
        self._last_action: dict[tuple[str, str], dict[str, Any]] = {}
        self._event_sequence: dict[tuple[str, str], int] = {}
        self._channel_generation: dict[tuple[str, str], dict[str, int]] = {}
        self._trend_history: dict[tuple[str, str, str], deque[dict[str, Any]]] = {}
        self._lock = threading.Lock()
        self._event_ring_size = event_ring_size
        self._trend_history_size = trend_history_size
        self._beacon_history_size = beacon_history_size
        self._load_persisted_state()

    def _context_key(self, context_id: str, band: str) -> tuple[str, str]:
        return (context_id, band)

    def append_event(self, context_id: str, band: str, event: ParsedEvent) -> None:
        with self._lock:
            key = self._context_key(context_id, band)
            sequence = self._event_sequence.get(key, 0) + 1
            self._event_sequence[key] = sequence
            ring = self._event_ring.setdefault(key, deque(maxlen=self._event_ring_size))
            payload = event.to_json()
            payload["eventSeq"] = sequence
            ring.append(payload)
            self._flush_events()

    def update_channel(
        self,
        context_id: str,
        band: str,
        channel: ParsedChannel,
        *,
        observation_source: str = "live-update",
        refresh_command: str | None = None,
    ) -> None:
        with self._lock:
            key = self._context_key(context_id, band)
            channel_map = self._channels.setdefault(key, {})
            generations = self._channel_generation.setdefault(key, {})
            generation = generations.get(channel.channel_name, 0) + 1
            generations[channel.channel_name] = generation
            channel_map[channel.channel_name] = ChannelSnapshot.from_parsed(
                channel,
                generation,
                observation_source=observation_source,
                observation_at=_utc_timestamp(),
                refresh_command=refresh_command,
            )
            if channel.channel_name in _LIVE_TREND_CHANNEL_SET:
                self._record_trend_sample_locked(
                    context_id,
                    band,
                    channel.channel_name,
                    channel_map[channel.channel_name],
                )
            self._flush_channels()

    def channel_map(self, context_id: str, band: str) -> dict[str, dict[str, Any]]:
        with self._lock:
            snapshots = self._channels.get(self._context_key(context_id, band), {})
            return {name: snapshot.to_json() for name, snapshot in snapshots.items()}

    def replace_channel_observation(
        self,
        context_id: str,
        band: str,
        channel: ParsedChannel,
        *,
        observation_source: str,
        refresh_command: str | None = None,
    ) -> bool:
        with self._lock:
            key = self._context_key(context_id, band)
            channel_map = self._channels.get(key, {})
            current = channel_map.get(channel.channel_name)
            if current is None:
                return False
            channel_map[channel.channel_name] = replace(
                current,
                timestamp=channel.timestamp,
                qualified_name=channel.qualified_name,
                channel_id=channel.channel_id,
                value=channel.value,
                raw=channel.raw,
                observation_source=observation_source,
                observation_at=_utc_timestamp(),
                refresh_command=refresh_command,
            )
            self._flush_channels()
            return True

    def recent_events(self, context_id: str, band: str, limit: int = 50) -> list[dict[str, Any]]:
        with self._lock:
            ring = list(self._event_ring.get(self._context_key(context_id, band), ()))
        return ring[-limit:]

    def clear_recent_events(self, context_id: str, band: str) -> int:
        with self._lock:
            key = self._context_key(context_id, band)
            ring = self._event_ring.get(key)
            cleared = len(ring or ())
            if ring is not None:
                self._event_ring[key] = deque(maxlen=self._event_ring_size)
            self._flush_events()
            return cleared

    def readback_marker(
        self,
        context_id: str,
        band: str,
        *,
        event_names: list[str] | None,
        channel_names: list[str] | None,
    ) -> dict[str, Any]:
        key = self._context_key(context_id, band)
        with self._lock:
            event_sequence = self._event_sequence.get(key, 0)
            generations = {
                name: self._channels.get(key, {}).get(name).generation if self._channels.get(key, {}).get(name) else 0
                for name in (channel_names or [])
            }
        return {
            "eventSequence": event_sequence,
            "channelGenerations": generations,
            "eventNames": list(event_names or []),
            "channelNames": list(channel_names or []),
        }

    def readback_freshness(self, context_id: str, band: str, marker: dict[str, Any]) -> dict[str, Any]:
        after_events = self.recent_events(context_id, band, limit=self._event_ring_size)
        event_names = set(marker.get("eventNames", ()))
        fresh_events = [
            event
            for event in after_events
            if event.get("eventSeq", 0) > int(marker.get("eventSequence", 0)) and event["eventName"] in event_names
        ]
        reject_events = [
            event
            for event in after_events
            if event.get("eventSeq", 0) > int(marker.get("eventSequence", 0))
            and event["eventName"] in {"COMMAND_AUTHORITY_REJECTED", "COMMAND_SEQUENCE_REJECTED", "COMMAND_SESSION_REJECTED", "SECURE_COMMAND_REJECTED"}
        ]
        current_channels = self.channel_map(context_id, band)
        before_generations = marker.get("channelGenerations", {})
        fresh_channels = {
            name: current_channels.get(name)
            for name in marker.get("channelNames", ())
            if current_channels.get(name) is not None
            and int(current_channels[name].get("generation", 0)) > int(before_generations.get(name, 0))
        }
        return {
            "freshEvents": fresh_events,
            "freshChannels": fresh_channels,
            "rejectEvents": reject_events,
        }

    def set_readback_cache(self, context_id: str, band: str, command_name: str, payload: dict[str, Any]) -> None:
        with self._lock:
            cache_key = f"{context_id}:{band}:{command_name}"
            self._readback_cache[cache_key] = payload
            atomic_write_json(self.readback_path, self._readback_cache)

    def readback_cache(self) -> dict[str, dict[str, Any]]:
        with self._lock:
            return dict(self._readback_cache)

    def set_beacon_snapshot(self, context_id: str, payload: dict[str, Any]) -> None:
        with self._lock:
            previous = self._beacon_latest.get(context_id)
            self._beacon_latest[context_id] = payload
            if self._should_append_beacon_history(previous, payload):
                ring = self._beacon_history.setdefault(context_id, deque(maxlen=self._beacon_history_size))
                ring.append(payload)
            self._flush_beacon()

    def beacon_latest(self, context_id: str) -> dict[str, Any] | None:
        with self._lock:
            payload = self._beacon_latest.get(context_id)
            return None if payload is None else dict(payload)

    def beacon_history(self, context_id: str, limit: int = 20) -> list[dict[str, Any]]:
        with self._lock:
            ring = self._beacon_history.get(context_id, deque())
            return list(ring)[-limit:]

    def trend_history(self, context_id: str, band: str, channel_names: list[str]) -> dict[str, list[dict[str, Any]]]:
        with self._lock:
            payload: dict[str, list[dict[str, Any]]] = {}
            for channel_name in channel_names:
                key = (context_id, band, channel_name)
                payload[channel_name] = list(self._trend_history.get(key, ()))
            return payload

    def readback_viewer(self, context_id: str, band: str) -> dict[str, Any]:
        cache = self.readback_cache()
        refreshable_commands = set(CHANNEL_REFRESH_COMMANDS) | set(EVENT_BASED_COMMANDS)
        latest_actions: dict[str, dict[str, Any]] = {}
        for entry in reversed(self.history(limit=200)):
            request = entry.get("request", {})
            if request.get("kind") != "readback":
                continue
            if request.get("contextId") != context_id or request.get("band") != band:
                continue
            command_name = str(request.get("payload", {}).get("commandName", ""))
            if command_name and command_name not in latest_actions:
                latest_actions[command_name] = entry
        tabs: list[dict[str, Any]] = []
        for tab_name, commands in READBACK_VIEWER_TABS.items():
            cards: list[dict[str, Any]] = []
            for command_name in commands:
                cache_key = f"{context_id}:{band}:{command_name}"
                cached = cache.get(cache_key)
                latest_action = latest_actions.get(command_name)
                refreshable = command_name in refreshable_commands
                state = "empty"
                last_refresh_time = None
                last_refresh_status = "never"
                error = None
                debug_payload = cached
                if latest_action is not None:
                    action_result = latest_action.get("result", {})
                    action_readback = action_result.get("readback") or {}
                    last_refresh_time = action_result.get("finishedAt") or latest_action.get("finishedAt")
                    last_refresh_status = str(action_result.get("status") or "unknown")
                    if action_result.get("status") == "succeeded" and cached is not None:
                        state = "success"
                    elif action_readback.get("incompleteResult") is not None:
                        state = "partial"
                        error = action_result.get("error")
                        debug_payload = action_result.get("readback")
                    elif action_result.get("status") == "failed":
                        state = "failed"
                        error = action_result.get("error")
                        debug_payload = action_result.get("readback")
                elif cached is not None:
                    state = "success"
                    last_refresh_status = "saved"
                cards.append(
                    {
                        "commandName": command_name,
                        "label": command_name.split(".")[-1],
                        "state": state,
                        "lastRefreshTime": last_refresh_time,
                        "lastRefreshStatus": last_refresh_status,
                        "sourceCommand": command_name,
                        "refreshable": refreshable,
                        "refreshCommandName": command_name if refreshable else None,
                        "family": (debug_payload or {}).get("family"),
                        "savedValues": self._viewer_saved_values(cached) if cached else None,
                        "debugPayload": debug_payload,
                        "error": error,
                    }
                )
            tabs.append({"name": tab_name, "cards": cards})
        return {"contextId": context_id, "band": band, "tabs": tabs}

    def clear_band(self, context_id: str, band: str) -> None:
        with self._lock:
            key = self._context_key(context_id, band)
            self._event_ring.pop(key, None)
            self._channels.pop(key, None)
            self._last_action.pop(key, None)
            self._event_sequence.pop(key, None)
            self._channel_generation.pop(key, None)
            for trend_key in [name for name in self._trend_history if name[:2] == key]:
                self._trend_history.pop(trend_key, None)
            self._flush_events()
            self._flush_channels()

    def clear_context_cache(self, context_id: str) -> dict[str, int]:
        with self._lock:
            event_count = 0
            channel_count = 0
            readback_count = 0
            trend_count = 0
            last_action_count = 0
            for key in [key for key in self._event_ring if key[0] == context_id]:
                event_count += len(self._event_ring.get(key, ()))
                self._event_ring.pop(key, None)
                self._event_sequence.pop(key, None)
            for key, channel_map in [item for item in self._channels.items() if item[0][0] == context_id]:
                channel_count += len(channel_map)
                self._channels.pop(key, None)
                self._channel_generation.pop(key, None)
            for key in [key for key in self._last_action if key[0] == context_id]:
                last_action_count += 1
                self._last_action.pop(key, None)
            for trend_key in [key for key in self._trend_history if key[0] == context_id]:
                trend_count += len(self._trend_history.get(trend_key, ()))
                self._trend_history.pop(trend_key, None)
            for cache_key in [key for key in self._readback_cache if key.startswith(f"{context_id}:")]:
                readback_count += 1
                self._readback_cache.pop(cache_key, None)
            if context_id in self._beacon_latest:
                self._beacon_latest.pop(context_id, None)
            if context_id in self._beacon_history:
                self._beacon_history.pop(context_id, None)
            self._flush_events()
            self._flush_channels()
            atomic_write_json(self.readback_path, self._readback_cache)
            self._flush_beacon()
            return {
                "events": event_count,
                "channels": channel_count,
                "readbacks": readback_count,
                "trendSamples": trend_count,
                "lastActions": last_action_count,
            }

    def record_action(self, context_id: str, band: str, payload: dict[str, Any]) -> None:
        with self._lock:
            enriched = {**payload, "consoleSessionId": payload.get("consoleSessionId", self.console_session_id)}
            self._last_action[self._context_key(context_id, band)] = enriched
            with self.history_path.open("a", encoding="utf-8") as handle:
                handle.write(json.dumps(enriched, sort_keys=True) + "\n")

    def record_packet_lab(self, payload: dict[str, Any]) -> None:
        with self._lock:
            enriched = {**payload, "consoleSessionId": payload.get("consoleSessionId", self.console_session_id)}
            with self.packet_lab_history_path.open("a", encoding="utf-8") as handle:
                handle.write(json.dumps(enriched, sort_keys=True) + "\n")

    def history(self, limit: int = 50, *, current_session_only: bool = False) -> list[dict[str, Any]]:
        with self._lock:
            entries = self._read_jsonl_locked(self.history_path)
        if current_session_only:
            entries = [entry for entry in entries if entry.get("consoleSessionId") == self.console_session_id]
        return entries[-limit:]

    def packet_lab_history(self, limit: int = 50, *, current_session_only: bool = False) -> list[dict[str, Any]]:
        with self._lock:
            entries = self._read_jsonl_locked(self.packet_lab_history_path)
        if current_session_only:
            entries = [entry for entry in entries if entry.get("consoleSessionId") == self.console_session_id]
        return entries[-limit:]

    def clear_history(self, *, current_session_only: bool = True) -> int:
        with self._lock:
            return self._clear_jsonl_locked(self.history_path, current_session_only=current_session_only)

    def clear_packet_lab_history(self, *, current_session_only: bool = True) -> int:
        with self._lock:
            return self._clear_jsonl_locked(self.packet_lab_history_path, current_session_only=current_session_only)

    def dashboard_cards(self, context_id: str, band: str, secure_session: dict[str, Any] | None) -> dict[str, Any]:
        channels = self._context_latest_dashboard_channels(context_id)
        cards: dict[str, Any] = {}
        for title, names in DASHBOARD_CHANNELS.items():
            cards[title] = {name: channels.get(name) for name in names}
        cards["Secure Session"] = {
            "selectedBand": band,
            "secureSession": secure_session,
            "lastAction": self._last_action.get(self._context_key(context_id, band)),
        }
        beacon = self._beacon_latest.get(context_id)
        if beacon is not None:
            cards["Beacon"] = {
                "supported": beacon.get("supported", False),
                "available": beacon.get("available", False),
                "lastObservedAt": beacon.get("lastObservedAt"),
                "sequence": beacon.get("sequence"),
                "reason": beacon.get("reason"),
            }
        return cards

    def structured_readback(
        self,
        context_id: str,
        band: str,
        command_name: str,
        event_names: list[str] | None,
        channel_names: list[str] | None,
    ) -> dict[str, Any]:
        payload: dict[str, Any] = {"commandName": command_name}
        if channel_names:
            channels = self.channel_map(context_id, band)
            payload["channels"] = {name: channels.get(name) for name in channel_names}
        if event_names:
            events = [
                event
                for event in self.recent_events(context_id, band, limit=50)
                if event["eventName"] in set(event_names)
            ]
            payload["events"] = [
                {**event, "structured": parse_structured_event(event["eventName"], event["message"])}
                for event in events
            ]
        return payload

    def _flush_events(self) -> None:
        payload = {f"{context}:{band}": list(ring) for (context, band), ring in self._event_ring.items()}
        atomic_write_json(self.events_path, payload)

    def _flush_channels(self) -> None:
        payload = {
            f"{context}:{band}": {name: snapshot.to_json() for name, snapshot in channel_map.items()}
            for (context, band), channel_map in self._channels.items()
        }
        atomic_write_json(self.channels_path, payload)

    def _flush_beacon(self) -> None:
        atomic_write_json(self.latest_beacon_path, self._beacon_latest)
        atomic_write_json(
            self.beacon_history_path,
            {context_id: list(entries) for context_id, entries in self._beacon_history.items()},
        )

    def _load_persisted_state(self) -> None:
        self._load_persisted_channels()
        self._load_persisted_readback_cache()
        self._load_persisted_beacon()
        if not self._readback_cache:
            self._rebuild_readback_cache_from_history()

    def _load_persisted_channels(self) -> None:
        payload = self._read_json_file(self.channels_path)
        if not isinstance(payload, dict):
            return
        for key, channel_map in payload.items():
            if not isinstance(key, str) or ":" not in key or not isinstance(channel_map, dict):
                continue
            context_id, band = key.split(":", 1)
            snapshots: dict[str, ChannelSnapshot] = {}
            generations: dict[str, int] = {}
            for channel_name, snapshot_payload in channel_map.items():
                if not isinstance(snapshot_payload, dict):
                    continue
                try:
                    snapshot = ChannelSnapshot(
                        timestamp=snapshot_payload.get("timestamp"),
                        qualified_name=str(snapshot_payload.get("qualifiedName") or channel_name),
                        channel_name=str(snapshot_payload.get("channelName") or channel_name),
                        channel_id=int(snapshot_payload.get("channelId") or 0),
                        value=str(snapshot_payload.get("value", "")),
                        raw=str(snapshot_payload.get("raw", "")),
                        generation=int(snapshot_payload.get("generation") or 0),
                        observation_source=str(snapshot_payload.get("observationSource") or "restored"),
                        observation_at=str(snapshot_payload.get("observationAt") or _utc_timestamp()),
                        refresh_command=snapshot_payload.get("refreshCommand"),
                    )
                except (TypeError, ValueError):
                    continue
                snapshots[snapshot.channel_name] = snapshot
                generations[snapshot.channel_name] = snapshot.generation
            if snapshots:
                context_key = self._context_key(context_id, band)
                self._channels[context_key] = snapshots
                self._channel_generation[context_key] = generations

    def _load_persisted_readback_cache(self) -> None:
        payload = self._read_json_file(self.readback_path)
        if isinstance(payload, dict):
            self._readback_cache = payload

    def _load_persisted_beacon(self) -> None:
        latest = self._read_json_file(self.latest_beacon_path)
        if isinstance(latest, dict):
            self._beacon_latest = latest
        history = self._read_json_file(self.beacon_history_path)
        if isinstance(history, dict):
            for context_id, entries in history.items():
                if isinstance(entries, list):
                    self._beacon_history[context_id] = deque(entries, maxlen=self._beacon_history_size)

    def _rebuild_readback_cache_from_history(self) -> None:
        entries = self._read_jsonl_locked(self.history_path)
        rebuilt: dict[str, dict[str, Any]] = {}
        for entry in entries:
            request = entry.get("request", {})
            if request.get("kind") != "readback":
                continue
            context_id = request.get("contextId")
            band = request.get("band")
            command_name = request.get("payload", {}).get("commandName")
            result = entry.get("result", {})
            readback = result.get("readback")
            if not context_id or not band or not command_name or not isinstance(readback, dict):
                continue
            if result.get("status") != "succeeded":
                continue
            rebuilt[f"{context_id}:{band}:{command_name}"] = readback
        if rebuilt:
            self._readback_cache = rebuilt
            atomic_write_json(self.readback_path, self._readback_cache)

    def _should_append_beacon_history(self, previous: dict[str, Any] | None, current: dict[str, Any]) -> bool:
        if previous is None:
            return True
        previous_frame_count = int(previous.get("capture", {}).get("frameCount") or 0)
        current_frame_count = int(current.get("capture", {}).get("frameCount") or 0)
        if current_frame_count > previous_frame_count:
            return True
        if previous.get("sequence") != current.get("sequence"):
            return True
        if previous.get("decode", {}).get("status") != current.get("decode", {}).get("status"):
            return True
        return False

    def _context_latest_dashboard_channels(self, context_id: str) -> dict[str, dict[str, Any]]:
        with self._lock:
            selected_names = {channel_name for names in DASHBOARD_CHANNELS.values() for channel_name in names}
            latest: dict[str, tuple[str, ChannelSnapshot]] = {}
            for (candidate_context, band), channel_map in self._channels.items():
                if candidate_context != context_id:
                    continue
                for channel_name, snapshot in channel_map.items():
                    if channel_name not in selected_names:
                        continue
                    current = latest.get(channel_name)
                    if current is None or (snapshot.observation_at or "") >= (current[1].observation_at or ""):
                        latest[channel_name] = (band, snapshot)
            return {
                channel_name: {**snapshot.to_json(), "sourceBand": band}
                for channel_name, (band, snapshot) in latest.items()
            }

    def _record_trend_sample_locked(
        self,
        context_id: str,
        band: str,
        channel_name: str,
        snapshot: ChannelSnapshot,
    ) -> None:
        key = (context_id, band, channel_name)
        history = self._trend_history.setdefault(key, deque(maxlen=self._trend_history_size))
        history.append(
            {
                "channelName": channel_name,
                "flightTimestamp": snapshot.timestamp,
                "gatewayObservedAt": snapshot.observation_at,
                "value": snapshot.value,
                "generation": snapshot.generation,
                "observationSource": snapshot.observation_source,
            }
        )

    def _viewer_saved_values(self, readback: dict[str, Any]) -> dict[str, Any] | None:
        channels = readback.get("channels") or {}
        if channels:
            return {"kind": "channels", "fields": channels}
        main_evidence = readback.get("mainEvidence") or {}
        if main_evidence.get("kind") == "fresh-event":
            event = next(iter(main_evidence.get("events") or []), None)
            if event is not None:
                return {
                    "kind": "event",
                    "eventName": event.get("eventName"),
                    "timestamp": event.get("timestamp"),
                    "fields": {
                        key: value
                        for key, value in (event.get("structured") or {}).items()
                        if key != "rawMessage"
                    },
                    "message": event.get("message"),
                }
        if main_evidence.get("kind") == "fresh-event-group":
            events = list(main_evidence.get("events") or [])
            structured_events = [
                {
                    "eventName": event.get("eventName"),
                    "timestamp": event.get("timestamp"),
                    "fields": {
                        key: value
                        for key, value in (event.get("structured") or {}).items()
                        if key != "rawMessage"
                    },
                    "message": event.get("message"),
                }
                for event in events
            ]
            return {"kind": "event-group", "events": structured_events}
        events = list(readback.get("events") or [])
        if events:
            first = events[-1]
            return {
                "kind": "event",
                "eventName": first.get("eventName"),
                "timestamp": first.get("timestamp"),
                "fields": {
                    key: value
                    for key, value in (first.get("structured") or {}).items()
                    if key != "rawMessage"
                },
                "message": first.get("message"),
            }
        return None

    def _read_jsonl_locked(self, path: pathlib.Path) -> list[dict[str, Any]]:
        if not path.exists():
            return []
        return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line]

    def _read_json_file(self, path: pathlib.Path) -> Any:
        if not path.exists():
            return None
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return None

    def _clear_jsonl_locked(self, path: pathlib.Path, *, current_session_only: bool) -> int:
        entries = self._read_jsonl_locked(path)
        if current_session_only:
            kept = [entry for entry in entries if entry.get("consoleSessionId") != self.console_session_id]
            cleared_count = len(entries) - len(kept)
        else:
            kept = []
            cleared_count = len(entries)
        with path.open("w", encoding="utf-8") as handle:
            for entry in kept:
                handle.write(json.dumps(entry, sort_keys=True) + "\n")
        return cleared_count


_LIVE_TREND_CHANNEL_SET = frozenset(channel for names in LIVE_TREND_GROUPS.values() for channel in names)
