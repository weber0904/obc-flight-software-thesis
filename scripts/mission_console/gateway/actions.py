from __future__ import annotations

import json
import os
import pathlib
import queue as queue_module
import subprocess
import time
import traceback
import uuid
from multiprocessing import get_context
from dataclasses import dataclass, field
from datetime import datetime
from typing import Any

from manual_ops import manual_secure_ops as secure_ops
from manual_ops.lib.common import ensure_dir, find_local_tool, utc_timestamp

from .locks import BandLockPool
from .parsers import (
    CHANNEL_REFRESH_COMMANDS,
    CHANNEL_REFRESH_VIEWER_FIELDS,
    EVENT_BASED_COMMANDS,
    SINGLE_EVENT_READBACK_COMMANDS,
    ParsedChannel,
    classify_readback_command,
    parse_channel_line,
    parse_event_line,
    parse_structured_event,
    structured_event_complete,
)
from .registry import SurfaceRegistry
from .snapshots import SnapshotStore


@dataclass(frozen=True)
class OperationRequest:
    contextId: str
    band: str
    kind: str
    payload: dict[str, Any] = field(default_factory=dict)
    ensureAuth: bool = False
    operatorLabel: str = "mission-console"


@dataclass
class OperationResult:
    status: str
    startedAt: str
    finishedAt: str | None = None
    commandName: str | None = None
    args: list[str] = field(default_factory=list)
    secureSequence: int | None = None
    artifacts: dict[str, Any] = field(default_factory=dict)
    readback: dict[str, Any] | None = None
    error: str | None = None

    def to_json(self) -> dict[str, Any]:
        return {
            "status": self.status,
            "startedAt": self.startedAt,
            "finishedAt": self.finishedAt,
            "commandName": self.commandName,
            "args": self.args,
            "secureSequence": self.secureSequence,
            "artifacts": self.artifacts,
            "readback": self.readback,
            "error": self.error,
        }


@dataclass
class ArmedChannelSearch:
    channel_name: str
    log_root: pathlib.Path
    process: subprocess.Popen[str]


@dataclass
class ArmedEventSearch:
    event_name: str
    log_root: pathlib.Path
    process: subprocess.Popen[str]


class GatewayActions:
    def __init__(
        self,
        runtime_root: pathlib.Path,
        registry: SurfaceRegistry,
        snapshots: SnapshotStore,
        lock_pool: BandLockPool | None = None,
    ) -> None:
        self.runtime_root = runtime_root
        self.registry = registry
        self.snapshots = snapshots
        self.lock_pool = lock_pool or BandLockPool()
        self.cli_path = find_local_tool("fprime-cli")
        self.session_cache_root = self.runtime_root / "session-cache"
        self.readback_search_root = self.runtime_root / "readback-search"
        self.session_cache_root.mkdir(parents=True, exist_ok=True)
        self.readback_search_root.mkdir(parents=True, exist_ok=True)

    def execute(self, request: OperationRequest) -> OperationResult:
        started_at = utc_timestamp()
        result = OperationResult(status="running", startedAt=started_at)
        history_payload: dict[str, Any] = {
            "request": {
                "contextId": request.contextId,
                "band": request.band,
                "kind": request.kind,
                "payload": request.payload,
                "ensureAuth": request.ensureAuth,
                "operatorLabel": request.operatorLabel,
            },
            "startedAt": started_at,
        }
        try:
            context = self.registry.context(
                request.contextId,
                refresh=request.kind not in {"status", "auth-clear"},
            )
            if request.kind not in {"status", "auth-clear"} and not context.is_running:
                raise RuntimeError(
                    f"context {request.contextId} is not running (lifecycleState={context.lifecycle_state})"
                )
            lock_key = self._operation_lock_key(context, request.band)
            with self.lock_pool.hold(*lock_key):
                manual_context = secure_ops.resolve_context(
                    env=context.env_name,
                    band=request.band,
                    manifest_path=context.manifest_path,
                    enforce_target_gates=request.kind not in {"status", "auth-clear"},
                )
                if request.ensureAuth:
                    ensured = self.ensure_auth(request.contextId, manual_context)
                    history_payload["ensureAuth"] = ensured
                if request.kind == "auth-ensure":
                    ensured = self.ensure_auth(request.contextId, manual_context)
                    result.status = "succeeded"
                    result.artifacts = ensured
                elif request.kind == "auth":
                    self._persist_related_service_session_snapshots(request.contextId, manual_context)
                    action = self._establish_auth(request.contextId, request.band, manual_context)
                    self._persist_session_snapshot(request.contextId, request.band, manual_context.secure_state_path)
                    result.status = "succeeded"
                    result.artifacts = action.to_json()
                elif request.kind == "auth-clear":
                    action = secure_ops.clear_auth_result(manual_context)
                    result.status = "succeeded"
                    result.artifacts = action.to_json()
                elif request.kind == "status":
                    action = secure_ops.status_result(manual_context)
                    result.status = "succeeded"
                    result.artifacts = action.to_json()
                elif request.kind == "command":
                    command_name = str(request.payload["commandName"])
                    command_args = [str(value) for value in request.payload.get("commandArgs", [])]
                    action = secure_ops.send_command_result(manual_context, command_name=command_name, command_args=command_args)
                    result.status = "succeeded"
                    result.commandName = command_name
                    result.args = command_args
                    result.secureSequence = action.details.get("secureSequence")
                    result.artifacts = action.to_json()
                elif request.kind == "readback":
                    result = self._run_readback(request, manual_context, result)
                elif request.kind == "file-upload":
                    action = _run_upload_file_in_subprocess(
                        manual_context,
                        local_path=str(request.payload["localPath"]),
                        destination_leaf=str(request.payload["destinationLeaf"]),
                    )
                    result.status = "succeeded"
                    result.artifacts = action.to_json()
                elif request.kind == "sequence":
                    action = secure_ops.sequence_command_result(
                        manual_context,
                        action=str(request.payload["action"]),
                        sequence_path=request.payload.get("sequencePath"),
                        context_id=request.payload.get("contextId"),
                        run_mode=request.payload.get("runMode"),
                    )
                    result.status = "succeeded"
                    result.commandName = action.details.get("command")
                    result.args = list(action.details.get("args", []))
                    result.secureSequence = action.details.get("secureSequence")
                    result.artifacts = action.to_json()
                else:
                    raise RuntimeError(f"unsupported operation kind {request.kind}")
        except Exception as exc:
            result.status = "failed"
            result.error = f"{exc}\n{traceback.format_exc(limit=8)}"
        result.finishedAt = utc_timestamp()
        history_payload["result"] = result.to_json()
        self.snapshots.record_action(request.contextId, request.band, history_payload)
        return result

    def ensure_auth(self, context_id: str, manual_context: secure_ops.ManualSurfaceContext) -> dict[str, Any]:
        secure_state = secure_ops.load_state(manual_context.manifest, manual_context.band)
        needs_auth = secure_state is None or secure_state.invalidated
        if secure_state is not None and secure_state.manifest_owner_pid != manual_context.manifest.get("ownerPid"):
            needs_auth = True
        if not needs_auth:
            return {"performed": False, "reason": "session-active"}
        previous_state_path = manual_context.secure_state_path
        self._persist_related_service_session_snapshots(context_id, manual_context)
        action = self._establish_auth(context_id, manual_context.band, manual_context)
        self._persist_session_snapshot(context_id, manual_context.band, previous_state_path)
        return {"performed": True, "result": action.to_json()}

    def _run_readback(
        self,
        request: OperationRequest,
        manual_context: secure_ops.ManualSurfaceContext,
        result: OperationResult,
    ) -> OperationResult:
        command_name = str(request.payload["commandName"])
        command_args = [str(value) for value in request.payload.get("commandArgs", [])]
        family = classify_readback_command(command_name)
        channel_names = CHANNEL_REFRESH_COMMANDS.get(command_name)
        viewer_channel_names = CHANNEL_REFRESH_VIEWER_FIELDS.get(command_name, channel_names)
        event_names = EVENT_BASED_COMMANDS.get(command_name)
        strict_single_event = command_name in SINGLE_EVENT_READBACK_COMMANDS
        strict_multi_event = command_name in _MULTI_EVENT_GROUP_COMMANDS
        if family == "generic" or (not channel_names and not event_names):
            raise RuntimeError(f"unsupported structured readback command: {command_name}")
        search_timeout_sec = float(request.payload.get("searchTimeoutSeconds", 5.0))
        native_retry_seconds = float(
            request.payload.get(
                "nativeRetrySeconds",
                min(max(search_timeout_sec, 3.0), 10.0),
            )
        )
        marker = self.snapshots.readback_marker(
            request.contextId,
            request.band,
            event_names=event_names,
            channel_names=channel_names,
        )
        not_before_timestamp = _local_log_timestamp()
        listener_marker = self._listener_log_marker(request.contextId, request.band)
        command_opcode = self._command_opcode_hex(manual_context, command_name, command_args)
        armed_channel_searches = (
            self._arm_channel_searches(
                manual_context,
                channel_names,
                timeout_sec=search_timeout_sec,
            )
            if channel_names
            else []
        )
        armed_event_searches = (
            self._arm_event_searches(
                manual_context,
                event_names,
                timeout_sec=search_timeout_sec,
            )
            if event_names
            else []
        )
        readback: dict[str, Any] | None = None
        action: secure_ops.ManualActionResult | None = None
        try:
            action = secure_ops.send_command_result(manual_context, command_name=command_name, command_args=command_args)
            time.sleep(float(request.payload.get("settleSeconds", 2.0)))
            readback = self.snapshots.structured_readback(
                request.contextId,
                request.band,
                command_name,
                event_names=event_names,
                channel_names=viewer_channel_names,
            )
            readback.setdefault("mainEvidence", None)
            readback.setdefault("fallbackEvidence", None)
            readback.setdefault("incompleteResult", None)
            freshness = self.snapshots.readback_freshness(request.contextId, request.band, marker)
            promotable_channels: dict[str, dict[str, Any]] = {}
            if channel_names and freshness["freshChannels"]:
                self._merge_fresh_channels(readback, freshness, dict(freshness["freshChannels"]))
                self._merge_promotable_channels(promotable_channels, dict(freshness["freshChannels"]))
            native_retry_deadline = time.time() + native_retry_seconds
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]):
                native_channels = self._native_channel_search_many(
                    request.contextId,
                    request.band,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    after_offset=int(listener_marker["channelOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if native_channels:
                    self._merge_fresh_channels(readback, freshness, native_channels)
                    self._merge_promotable_channels(promotable_channels, native_channels)
                    readback["channelSource"] = "native-log"
            while (
                channel_names
                and self._missing_channel_names(channel_names, freshness["freshChannels"])
                and time.time() < native_retry_deadline
            ):
                time.sleep(0.5)
                native_channels = self._native_channel_search_many(
                    request.contextId,
                    request.band,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    after_offset=int(listener_marker["channelOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if native_channels:
                    self._merge_fresh_channels(readback, freshness, native_channels)
                    self._merge_promotable_channels(promotable_channels, native_channels)
                    readback["channelSource"] = "native-log"
            if event_names and self._needs_more_event_evidence(command_name, freshness):
                native_events = self._native_event_search_many(
                    request.contextId,
                    request.band,
                    event_names,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if native_events:
                    self._merge_fresh_events(readback, freshness, native_events)
                    readback["eventSource"] = "native-log"
            while event_names and self._needs_more_event_evidence(command_name, freshness) and time.time() < native_retry_deadline:
                time.sleep(0.5)
                native_events = self._native_event_search_many(
                    request.contextId,
                    request.band,
                    event_names,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if native_events:
                    self._merge_fresh_events(readback, freshness, native_events)
                    readback["eventSource"] = "native-log"
                    if not self._needs_more_event_evidence(command_name, freshness):
                        break
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]) and command_opcode is not None:
                completion = self._native_command_completion(
                    request.contextId,
                    request.band,
                    command_opcode,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                snapshot_channels = self._fresh_snapshot_channels(
                    request.contextId,
                    request.band,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    marker=marker,
                )
                if completion is not None and snapshot_channels:
                    self._merge_fresh_channels(readback, freshness, snapshot_channels)
                    self._merge_promotable_channels(promotable_channels, snapshot_channels)
                    readback["completionEvidence"] = completion
                    readback["channelSource"] = "snapshot+command-completion"
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]):
                late_native_channels = self._native_channel_search_many(
                    request.contextId,
                    request.band,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    after_offset=int(listener_marker["channelOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if late_native_channels:
                    self._merge_fresh_channels(readback, freshness, late_native_channels)
                    self._merge_promotable_channels(promotable_channels, late_native_channels)
                    readback["channelSource"] = "native-log-late"
            if event_names and self._needs_more_event_evidence(command_name, freshness):
                late_native_events = self._native_event_search_many(
                    request.contextId,
                    request.band,
                    event_names,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if late_native_events:
                    self._merge_fresh_events(readback, freshness, late_native_events)
                    readback["eventSource"] = "native-log-late"
            if event_names and self._needs_more_event_evidence(command_name, freshness) and armed_event_searches:
                armed_events = self._collect_armed_event_searches(
                    armed_event_searches,
                    timeout_sec=search_timeout_sec,
                    not_before_timestamp=not_before_timestamp,
                )
                if armed_events:
                    self._merge_fresh_events(readback, freshness, armed_events)
                    readback["eventSource"] = "armed-search"
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]) and armed_channel_searches:
                armed_channels = self._collect_armed_channel_searches(
                    armed_channel_searches,
                    timeout_sec=search_timeout_sec,
                    not_before_timestamp=not_before_timestamp,
                )
                if armed_channels:
                    self._merge_fresh_channels(readback, freshness, armed_channels)
                    self._merge_promotable_channels(promotable_channels, armed_channels)
                    readback["channelSource"] = "armed-search"
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]):
                bounded_channels = self._bounded_channel_search_many(
                    manual_context,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    timeout_sec=search_timeout_sec,
                    not_before_timestamp=not_before_timestamp,
                )
                if bounded_channels:
                    self._merge_fresh_channels(readback, freshness, bounded_channels)
                    self._merge_promotable_channels(promotable_channels, bounded_channels)
                    readback["channelSource"] = "bounded-search"
            if channel_names and self._missing_channel_names(channel_names, freshness["freshChannels"]) and command_opcode is not None:
                completion = self._native_command_completion(
                    request.contextId,
                    request.band,
                    command_opcode,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                snapshot_channels = self._fresh_snapshot_channels(
                    request.contextId,
                    request.band,
                    self._missing_channel_names(channel_names, freshness["freshChannels"]),
                    marker=marker,
                )
                if completion is not None and snapshot_channels:
                    self._merge_fresh_channels(readback, freshness, snapshot_channels)
                    self._merge_promotable_channels(promotable_channels, snapshot_channels)
                    readback["completionEvidence"] = completion
                    readback["channelSource"] = "snapshot+command-completion"
            if event_names and self._needs_more_event_evidence(command_name, freshness):
                if command_name in _COMMAND_COMPLETION_FALLBACKS and command_opcode is not None:
                    completion = self._native_command_completion(
                        request.contextId,
                        request.band,
                        command_opcode,
                        after_offset=int(listener_marker["eventOffset"]),
                        not_before_timestamp=not_before_timestamp,
                    )
                    if completion is not None:
                        # A completion-only fallback proves the command ran, but it does not
                        # prove any older structured event payload still belongs to this command.
                        readback["events"] = []
                        readback["completionEvidence"] = completion
                        readback["eventSource"] = "command-completion-fallback"
                        freshness["freshEvents"] = [completion]
                bounded_events = self._bounded_event_search_many(
                    manual_context,
                    event_names,
                    timeout_sec=search_timeout_sec,
                    not_before_timestamp=not_before_timestamp,
                )
                if bounded_events:
                    self._merge_fresh_events(readback, freshness, bounded_events)
                    readback["eventSource"] = "bounded-search"
            if (
                strict_single_event
                and command_opcode is not None
                and readback.get("completionEvidence") is None
            ):
                completion = self._native_command_completion(
                    request.contextId,
                    request.band,
                    command_opcode,
                    after_offset=int(listener_marker["eventOffset"]),
                    not_before_timestamp=not_before_timestamp,
                )
                if completion is not None:
                    readback["completionEvidence"] = completion
            late_rejects = self._fresh_reject_events(
                request.contextId,
                request.band,
                marker,
                after_offset=int(listener_marker["eventOffset"]),
                not_before_timestamp=not_before_timestamp,
            )
            if late_rejects:
                freshness["rejectEvents"] = late_rejects
            readback["freshness"] = freshness
            if freshness["rejectEvents"]:
                raise RuntimeError(f"readback command rejected: {freshness['rejectEvents'][-1]['message']}")
            if strict_single_event:
                self._classify_single_event_readback(readback, freshness)
            if strict_multi_event:
                self._classify_multi_event_readback(command_name, readback, freshness)
            if strict_single_event and not readback.get("mainEvidence"):
                if readback.get("incompleteResult") is not None:
                    raise RuntimeError(f"readback produced incomplete fresh event payload for {command_name}")
                raise RuntimeError(f"readback produced no fresh events for {command_name}")
            if strict_multi_event and not readback.get("mainEvidence"):
                if readback.get("incompleteResult") is not None:
                    raise RuntimeError(f"readback produced incomplete fresh event group for {command_name}")
                raise RuntimeError(f"readback produced no fresh event group for {command_name}")
            if event_names and not strict_single_event and not strict_multi_event and not freshness["freshEvents"]:
                raise RuntimeError(f"readback produced no fresh events for {command_name}")
            if not strict_single_event and not strict_multi_event and readback.get("incompleteResult") is not None:
                raise RuntimeError(f"readback produced incomplete fresh event payload for {command_name}")
            if channel_names:
                missing_channels = self._missing_channel_names(channel_names, freshness["freshChannels"])
                if missing_channels:
                    raise RuntimeError(
                        f"readback produced no fresh channels for {command_name}: missing {', '.join(missing_channels)}"
                    )
                self._promote_fresh_channels(
                    request.contextId,
                    request.band,
                    promotable_channels,
                    refresh_command=command_name,
                )
            readback["family"] = family
            self.snapshots.set_readback_cache(request.contextId, request.band, command_name, readback)
            result.status = "succeeded"
            result.commandName = command_name
            result.args = command_args
            result.secureSequence = action.details.get("secureSequence")
            result.artifacts = action.to_json()
            result.readback = readback
            return result
        except Exception:
            result.commandName = command_name
            result.args = command_args
            if action is not None:
                result.secureSequence = action.details.get("secureSequence")
                result.artifacts = action.to_json()
            if readback is not None:
                result.readback = readback
            raise
        finally:
            self._stop_armed_channel_searches(armed_channel_searches)
            self._stop_armed_event_searches(armed_event_searches)

    def _classify_single_event_readback(
        self,
        readback: dict[str, Any],
        freshness: dict[str, Any],
    ) -> None:
        fresh_events = self._dedupe_fresh_events(
            [self._hydrate_fresh_event_payload(event) for event in list(freshness.get("freshEvents", []))]
        )
        freshness["freshEvents"] = fresh_events
        if fresh_events:
            complete_events = [
                event
                for event in fresh_events
                if structured_event_complete(event.get("eventName", ""), event.get("structured", {}))
            ]
            if complete_events:
                readback["mainEvidence"] = {
                    "kind": "fresh-event",
                    "source": readback.get("eventSource") or "unknown",
                    "events": complete_events,
                }
                if readback.get("events") and readback["events"] != complete_events:
                    readback["events"] = complete_events
                return
            readback["events"] = fresh_events
            readback["incompleteResult"] = {
                "kind": "fresh-event-incomplete",
                "reason": "structured-event-missing-fields",
                "source": readback.get("eventSource") or "unknown",
                "events": fresh_events,
            }
            completion = readback.get("completionEvidence")
            if completion is not None:
                readback["fallbackEvidence"] = {
                    "kind": "command-completion",
                    "source": "command-completion",
                    "events": [completion],
                }
            return
        completion = readback.get("completionEvidence")
        if completion is not None:
            readback["events"] = []
            readback["fallbackEvidence"] = {
                "kind": "command-completion",
                "source": "command-completion",
                "events": [completion],
            }

    def _hydrate_fresh_event_payload(self, event: dict[str, Any]) -> dict[str, Any]:
        if event.get("structured") is not None:
            return event
        return {
            **event,
            "structured": parse_structured_event(str(event.get("eventName", "")), str(event.get("message", ""))),
        }

    def _dedupe_fresh_events(self, events: list[dict[str, Any]]) -> list[dict[str, Any]]:
        deduped: list[dict[str, Any]] = []
        seen: set[tuple[Any, ...]] = set()
        for event in events:
            structured = event.get("structured")
            structured_key = json.dumps(structured, sort_keys=True, default=str) if structured is not None else None
            key = (
                event.get("timestamp"),
                event.get("eventName"),
                event.get("message"),
                structured_key,
            )
            if key in seen:
                continue
            seen.add(key)
            deduped.append(event)
        return deduped

    def _merge_fresh_events(
        self,
        readback: dict[str, Any],
        freshness: dict[str, Any],
        new_events: list[dict[str, Any]],
    ) -> None:
        combined = list(freshness.get("freshEvents", ()))
        combined.extend(new_events)
        merged = self._dedupe_fresh_events([self._hydrate_fresh_event_payload(event) for event in combined])
        freshness["freshEvents"] = merged
        readback["events"] = merged

    def _needs_more_event_evidence(
        self,
        command_name: str,
        freshness: dict[str, Any],
    ) -> bool:
        fresh_events = list(freshness.get("freshEvents", ()))
        if not fresh_events:
            return True
        if command_name == "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY":
            hydrated = self._dedupe_fresh_events([self._hydrate_fresh_event_payload(event) for event in fresh_events])
            status_events = [
                event
                for event in hydrated
                if event.get("eventName") == "PERSISTENT_FAULT_HISTORY_STATUS"
                and structured_event_complete(event.get("eventName", ""), event.get("structured", {}))
            ]
            if not status_events:
                return True
            expected_records = int(status_events[-1].get("structured", {}).get("returnedRecords", 0))
            if expected_records == 0:
                return False
            observed_records = sum(
                1
                for event in hydrated
                if event.get("eventName") == "PERSISTENT_FAULT_HISTORY_RECORD"
                and structured_event_complete(event.get("eventName", ""), event.get("structured", {}))
            )
            return observed_records < expected_records
        return False

    def _classify_multi_event_readback(
        self,
        command_name: str,
        readback: dict[str, Any],
        freshness: dict[str, Any],
    ) -> None:
        fresh_events = self._dedupe_fresh_events(
            [self._hydrate_fresh_event_payload(event) for event in list(freshness.get("freshEvents", []))]
        )
        freshness["freshEvents"] = fresh_events
        if command_name != "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY":
            return

        status_events = [
            event
            for event in fresh_events
            if event.get("eventName") == "PERSISTENT_FAULT_HISTORY_STATUS"
            and structured_event_complete(event.get("eventName", ""), event.get("structured", {}))
        ]
        record_events = [
            event
            for event in fresh_events
            if event.get("eventName") == "PERSISTENT_FAULT_HISTORY_RECORD"
            and structured_event_complete(event.get("eventName", ""), event.get("structured", {}))
        ]

        completion = readback.get("completionEvidence")
        if completion is not None:
            readback["fallbackEvidence"] = {
                "kind": "command-completion",
                "source": "command-completion",
                "events": [completion],
            }
        if not status_events:
            return

        status_event = status_events[-1]
        expected_records = int(status_event.get("structured", {}).get("returnedRecords", 0))
        if expected_records == 0:
            readback["events"] = [status_event]
            readback["mainEvidence"] = {
                "kind": "fresh-event-group",
                "source": readback.get("eventSource") or "unknown",
                "statusEvent": status_event,
                "recordEvents": [],
                "events": [status_event],
            }
            return

        if len(record_events) >= expected_records:
            complete_group = [status_event, *record_events[:expected_records]]
            readback["events"] = complete_group
            readback["mainEvidence"] = {
                "kind": "fresh-event-group",
                "source": readback.get("eventSource") or "unknown",
                "statusEvent": status_event,
                "recordEvents": record_events[:expected_records],
                "events": complete_group,
            }
            return

        readback["events"] = [status_event, *record_events]
        readback["incompleteResult"] = {
            "kind": "fresh-event-group-incomplete",
            "reason": "missing-record-events",
            "source": readback.get("eventSource") or "unknown",
            "statusEvent": status_event,
            "recordEvents": record_events,
            "expectedRecordCount": expected_records,
            "observedRecordCount": len(record_events),
            "events": [status_event, *record_events],
        }

    def _missing_channel_names(
        self,
        channel_names: list[str],
        fresh_channels: dict[str, dict[str, Any]],
    ) -> list[str]:
        return [name for name in channel_names if fresh_channels.get(name) is None]

    def _merge_fresh_channels(
        self,
        readback: dict[str, Any],
        freshness: dict[str, Any],
        new_channels: dict[str, dict[str, Any]],
    ) -> None:
        readback["channels"] = {
            **dict(readback.get("channels", {})),
            **new_channels,
        }
        freshness["freshChannels"] = {
            **dict(freshness.get("freshChannels", {})),
            **new_channels,
        }

    def _promote_fresh_channels(
        self,
        context_id: str,
        band: str,
        fresh_channels: dict[str, dict[str, Any]],
        *,
        refresh_command: str,
    ) -> None:
        for payload in fresh_channels.values():
            parsed = self._parsed_channel_from_payload(payload)
            if parsed is None:
                continue
            if self.snapshots.replace_channel_observation(
                context_id,
                band,
                parsed,
                observation_source="refresh",
                refresh_command=refresh_command,
            ):
                continue
            self.snapshots.update_channel(
                context_id,
                band,
                parsed,
                observation_source="refresh",
                refresh_command=refresh_command,
            )

    def _merge_promotable_channels(
        self,
        promotable_channels: dict[str, dict[str, Any]],
        new_channels: dict[str, dict[str, Any]],
    ) -> None:
        promotable_channels.update(new_channels)

    def _parsed_channel_from_payload(self, payload: dict[str, Any]) -> ParsedChannel | None:
        raw = payload.get("raw")
        if isinstance(raw, str):
            parsed = parse_channel_line(raw)
            if parsed is not None:
                return parsed
        qualified_name = payload.get("qualifiedName")
        channel_name = payload.get("channelName")
        channel_id = payload.get("channelId")
        value = payload.get("value")
        if not isinstance(qualified_name, str) or not isinstance(channel_name, str):
            return None
        if not isinstance(channel_id, int):
            return None
        return ParsedChannel(
            timestamp=payload.get("timestamp"),
            qualified_name=qualified_name,
            channel_name=channel_name,
            channel_id=channel_id,
            value="" if value is None else str(value),
            raw=raw if isinstance(raw, str) else "",
        )

    def _command_opcode_hex(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        command_name: str,
        command_args: list[str],
    ) -> str | None:
        try:
            dictionaries, encoder = secure_ops.load_command_context(manual_context.dictionary_path)
            inner = secure_ops.encode_inner_command(dictionaries, encoder, command_name, command_args)
        except Exception:
            return None
        if len(inner) < 6:
            return None
        return f"0x{int.from_bytes(inner[2:6], 'big'):08x}"

    def _operation_lock_key(self, context: Any, band: str) -> tuple[Any, ...]:
        surface = context.band(band)
        return (context.context_id, *surface.operation_lock_key)

    def _listener_log_marker(self, context_id: str, band: str) -> dict[str, int]:
        event_log, channel_log = self._listener_log_paths(context_id, band)
        return {
            "eventOffset": (event_log.stat().st_size if event_log.exists() else 0),
            "channelOffset": (channel_log.stat().st_size if channel_log.exists() else 0),
        }

    def _listener_log_paths(self, context_id: str, band: str) -> tuple[pathlib.Path, pathlib.Path]:
        context = self.registry.context(context_id)
        requested_band = context.band(band)
        shared_bands = sorted(
            name for name, surface in context.bands.items() if surface.listener_key == requested_band.listener_key
        )
        listener_band = shared_bands[0]
        root = self.runtime_root / "listeners" / context_id / listener_band / "native-events"
        return root / "event.log", root / "channel.log"

    def _native_channel_search_many(
        self,
        context_id: str,
        band: str,
        channel_names: list[str],
        *,
        after_offset: int,
        not_before_timestamp: str,
    ) -> dict[str, dict[str, Any]]:
        path = self._listener_log_paths(context_id, band)[1]
        text = self._read_log_suffix(path, after_offset=after_offset)
        hits = _collect_latest_channels(text, channel_names, not_before_timestamp=not_before_timestamp)
        missing = [name for name in channel_names if name not in hits]
        if missing and after_offset > 0:
            # Some target-path refresh bursts do not land in a stable per-field order in the
            # native channel log. If the suffix after the readback marker only contains part of
            # the burst, rescan the whole file bounded by command-time to recover the missing
            # fields without accepting older pre-command samples.
            full_text = self._read_log_suffix(path, after_offset=0)
            hits.update(_collect_latest_channels(full_text, missing, not_before_timestamp=not_before_timestamp))
        return hits

    def _native_event_search_many(
        self,
        context_id: str,
        band: str,
        event_names: list[str],
        *,
        after_offset: int,
        not_before_timestamp: str,
    ) -> list[dict[str, Any]]:
        path = self._listener_log_paths(context_id, band)[0]
        text = self._read_log_suffix(path, after_offset=after_offset)
        hits = _collect_matching_events(text, event_names, not_before_timestamp=not_before_timestamp)
        if after_offset > 0:
            # Target-path event bursts can also arrive in a log order that makes a suffix-only
            # read miss earlier events from the same command window. Re-scan the native log
            # bounded by command-time and de-duplicate by event identity so multi-event groups
            # like persistent fault history do not fail just because one record landed before
            # the current suffix marker.
            full_text = self._read_log_suffix(path, after_offset=0)
            hits = _dedupe_event_matches(
                [
                    *_collect_matching_events(full_text, event_names, not_before_timestamp=not_before_timestamp),
                    *hits,
                ]
            )
        return hits

    def _native_command_completion(
        self,
        context_id: str,
        band: str,
        command_opcode: str,
        *,
        after_offset: int,
        not_before_timestamp: str,
    ) -> dict[str, Any] | None:
        path = self._listener_log_paths(context_id, band)[0]
        text = self._read_log_suffix(path, after_offset=after_offset)
        return _find_command_completion(text, command_opcode, not_before_timestamp=not_before_timestamp)

    def _read_log_suffix(self, path: pathlib.Path, *, after_offset: int) -> str:
        if not path.exists():
            return ""
        try:
            file_size = path.stat().st_size
            offset = 0 if after_offset > file_size else max(after_offset, 0)
            with path.open("rb") as stream:
                stream.seek(offset)
                payload = stream.read()
        except OSError:
            return ""
        return payload.decode("utf-8", errors="replace")

    def _fresh_reject_events(
        self,
        context_id: str,
        band: str,
        marker: dict[str, Any],
        *,
        after_offset: int,
        not_before_timestamp: str,
    ) -> list[dict[str, Any]]:
        freshness = self.snapshots.readback_freshness(context_id, band, marker)
        reject_events = list(freshness.get("rejectEvents", ()))
        late_native_rejects = self._native_event_search_many(
            context_id,
            band,
            list(_REJECT_EVENT_NAMES),
            after_offset=after_offset,
            not_before_timestamp=not_before_timestamp,
        )
        seen = {
            (
                event.get("timestamp"),
                event.get("eventName"),
                event.get("message"),
            )
            for event in reject_events
        }
        for event in late_native_rejects:
            key = (event.get("timestamp"), event.get("eventName"), event.get("message"))
            if key in seen:
                continue
            seen.add(key)
            reject_events.append(event)
        return reject_events

    def _persist_session_snapshot(self, context_id: str, band: str, state_path: pathlib.Path) -> None:
        if not state_path.exists():
            return
        cache_path = self.session_cache_root / f"{context_id}-{band}-{uuid.uuid4().hex}.json"
        payload = state_path.read_bytes()
        fd = os.open(cache_path, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
        with os.fdopen(fd, "wb") as handle:
            handle.write(payload)

    def _related_service_bands(self, context: secure_ops.ManualSurfaceContext) -> list[str]:
        seen: set[str] = set()
        related: list[str] = []
        target_service_id = secure_ops.band_service_id(context.band)
        for surface in context.manifest.get("operatorSurfaces", {}).values():
            for band_name in surface.get("canonicalBands", ()):
                band_name = str(band_name)
                if band_name in seen or secure_ops.band_service_id(band_name) != target_service_id:
                    continue
                seen.add(band_name)
                related.append(band_name)
        if context.band not in seen:
            related.insert(0, context.band)
        return related

    def _persist_related_service_session_snapshots(self, context_id: str, context: secure_ops.ManualSurfaceContext) -> None:
        for band_name in self._related_service_bands(context):
            self._persist_session_snapshot(
                context_id,
                band_name,
                secure_ops.state_path(context.manifest, band_name),
            )

    def _current_snapshot_channels(
        self,
        context_id: str,
        band: str,
        channel_names: list[str],
    ) -> dict[str, dict[str, Any]]:
        channels = self.snapshots.channel_map(context_id, band)
        return {
            name: channels[name]
            for name in channel_names
            if channels.get(name) is not None
        }

    def _fresh_snapshot_channels(
        self,
        context_id: str,
        band: str,
        channel_names: list[str],
        *,
        marker: dict[str, Any],
    ) -> dict[str, dict[str, Any]]:
        channels = self._current_snapshot_channels(context_id, band, channel_names)
        before_generations = marker.get("channelGenerations", {})
        return {
            name: channel
            for name, channel in channels.items()
            if int(channel.get("generation", 0)) > int(before_generations.get(name, 0))
        }

    def _establish_auth(
        self,
        context_id: str,
        band: str,
        manual_context: secure_ops.ManualSurfaceContext,
    ) -> secure_ops.ManualActionResult:
        native_recv_bin = self._listener_log_paths(context_id, band)[0].parent / "recv.bin"
        return secure_ops.establish_auth_result(manual_context, native_packet_log=native_recv_bin)

    def _bounded_channel_search_many(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        channel_names: list[str],
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> dict[str, dict[str, Any]]:
        results: dict[str, dict[str, Any]] = {}
        for channel_name in channel_names:
            hit = self._bounded_channel_search(
                manual_context,
                channel_name,
                timeout_sec=timeout_sec,
                not_before_timestamp=not_before_timestamp,
            )
            if hit is not None:
                results[channel_name] = hit
        return results

    def _bounded_channel_search(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        channel_name: str,
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> dict[str, Any] | None:
        log_root = ensure_dir(self.readback_search_root / f"channels-{uuid.uuid4().hex}")
        args = [
            self.cli_path,
            "channels",
            "--dictionary",
            str(manual_context.dictionary_path),
            "--no-zmq",
            "--tts-port",
            str(int(manual_context.surface["gdsTtsPort"])),
            "--search",
            channel_name,
            "--timeout",
            str(timeout_sec),
            "-l",
            str(log_root),
            "--log-directly",
            "--log-to-stdout",
        ]
        combined = _run_bounded_cli_search(args, timeout_sec=timeout_sec)
        channel_log = log_root / "channel.log"
        if channel_log.exists():
            combined += "\n" + channel_log.read_text(encoding="utf-8", errors="replace")
        latest = None
        for raw_line in combined.splitlines():
            parsed = parse_channel_line(raw_line)
            if (
                parsed is not None
                and parsed.channel_name == channel_name
                and _timestamp_at_or_after(parsed.timestamp, not_before_timestamp)
            ):
                latest = parsed.to_json()
        return latest

    def _arm_channel_searches(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        channel_names: list[str],
        *,
        timeout_sec: float,
    ) -> list[ArmedChannelSearch]:
        searches: list[ArmedChannelSearch] = []
        for channel_name in channel_names:
            log_root = ensure_dir(self.readback_search_root / f"armed-channels-{uuid.uuid4().hex}")
            args = [
                self.cli_path,
                "channels",
                "--dictionary",
                str(manual_context.dictionary_path),
                "--no-zmq",
                "--tts-port",
                str(int(manual_context.surface["gdsTtsPort"])),
                "--search",
                channel_name,
                "--timeout",
                str(timeout_sec),
                "-l",
                str(log_root),
                "--log-directly",
                "--log-to-stdout",
            ]
            process = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            searches.append(ArmedChannelSearch(channel_name=channel_name, log_root=log_root, process=process))
        return searches

    def _collect_armed_channel_searches(
        self,
        searches: list[ArmedChannelSearch],
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> dict[str, dict[str, Any]]:
        results: dict[str, dict[str, Any]] = {}
        deadline_at = time.time() + max(15.0, float(timeout_sec) + 10.0)
        for search in searches:
            combined = ""
            try:
                remaining = max(0.1, deadline_at - time.time())
                stdout, _ = search.process.communicate(timeout=remaining)
                combined = stdout or ""
            except subprocess.TimeoutExpired:
                search.process.kill()
                stdout, _ = search.process.communicate()
                combined = stdout or ""
            channel_log = search.log_root / "channel.log"
            if channel_log.exists():
                combined += "\n" + channel_log.read_text(encoding="utf-8", errors="replace")
            latest = None
            for raw_line in combined.splitlines():
                parsed = parse_channel_line(raw_line)
                if (
                    parsed is not None
                    and parsed.channel_name == search.channel_name
                    and _timestamp_at_or_after(parsed.timestamp, not_before_timestamp)
                ):
                    latest = parsed.to_json()
            if latest is not None:
                results[search.channel_name] = latest
        return results

    def _stop_armed_channel_searches(self, searches: list[ArmedChannelSearch]) -> None:
        for search in searches:
            if search.process.poll() is not None:
                continue
            try:
                search.process.terminate()
                search.process.communicate(timeout=2.0)
            except Exception:
                try:
                    search.process.kill()
                    search.process.communicate(timeout=2.0)
                except Exception:
                    pass

    def _arm_event_searches(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        event_names: list[str],
        *,
        timeout_sec: float,
    ) -> list[ArmedEventSearch]:
        searches: list[ArmedEventSearch] = []
        for event_name in event_names:
            log_root = ensure_dir(self.readback_search_root / f"armed-events-{uuid.uuid4().hex}")
            args = [
                self.cli_path,
                "events",
                "--dictionary",
                str(manual_context.dictionary_path),
                "--no-zmq",
                "--tts-port",
                str(int(manual_context.surface["gdsTtsPort"])),
                "--search",
                event_name,
                "--timeout",
                str(timeout_sec),
                "-l",
                str(log_root),
                "--log-directly",
                "--log-to-stdout",
            ]
            process = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            searches.append(ArmedEventSearch(event_name=event_name, log_root=log_root, process=process))
        return searches

    def _collect_armed_event_searches(
        self,
        searches: list[ArmedEventSearch],
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> list[dict[str, Any]]:
        results: list[dict[str, Any]] = []
        deadline_at = time.time() + max(15.0, float(timeout_sec) + 10.0)
        for search in searches:
            combined = ""
            try:
                remaining = max(0.1, deadline_at - time.time())
                stdout, _ = search.process.communicate(timeout=remaining)
                combined = stdout or ""
            except subprocess.TimeoutExpired:
                search.process.kill()
                stdout, _ = search.process.communicate()
                combined = stdout or ""
            event_log = search.log_root / "event.log"
            if event_log.exists():
                combined += "\n" + event_log.read_text(encoding="utf-8", errors="replace")
            results.extend(
                _collect_matching_events(
                    combined,
                    [search.event_name],
                    not_before_timestamp=not_before_timestamp,
                )
            )
        return self._dedupe_fresh_events(results)

    def _stop_armed_event_searches(self, searches: list[ArmedEventSearch]) -> None:
        for search in searches:
            if search.process.poll() is not None:
                continue
            try:
                search.process.terminate()
                search.process.communicate(timeout=2.0)
            except Exception:
                try:
                    search.process.kill()
                    search.process.communicate(timeout=2.0)
                except Exception:
                    pass

    def _bounded_event_search_many(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        event_names: list[str],
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> list[dict[str, Any]]:
        results: list[dict[str, Any]] = []
        for event_name in event_names:
            hits = self._bounded_event_search(
                manual_context,
                event_name,
                timeout_sec=timeout_sec,
                not_before_timestamp=not_before_timestamp,
            )
            results.extend(hits)
        return self._dedupe_fresh_events(results)

    def _bounded_event_search(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        event_name: str,
        *,
        timeout_sec: float,
        not_before_timestamp: str,
    ) -> list[dict[str, Any]]:
        log_root = ensure_dir(self.readback_search_root / f"events-{uuid.uuid4().hex}")
        args = [
            self.cli_path,
            "events",
            "--dictionary",
            str(manual_context.dictionary_path),
            "--no-zmq",
            "--tts-port",
            str(int(manual_context.surface["gdsTtsPort"])),
            "--search",
            event_name,
            "--timeout",
            str(timeout_sec),
            "-l",
            str(log_root),
            "--log-directly",
            "--log-to-stdout",
        ]
        combined = _run_bounded_cli_search(args, timeout_sec=timeout_sec)
        event_log = log_root / "event.log"
        if event_log.exists():
            combined += "\n" + event_log.read_text(encoding="utf-8", errors="replace")
        return _collect_matching_events(
            combined,
            [event_name],
            not_before_timestamp=not_before_timestamp,
        )

def _run_bounded_cli_search(args: list[str], *, timeout_sec: float) -> str:
    try:
        result = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
            timeout=max(15.0, float(timeout_sec) + 10.0),
        )
        return result.stdout
    except subprocess.TimeoutExpired as exc:
        return "" if exc.stdout is None else str(exc.stdout)


_COMMAND_COMPLETION_FALLBACKS = frozenset()

_MULTI_EVENT_GROUP_COMMANDS = frozenset(
    {
        "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
    }
)

_REJECT_EVENT_NAMES = frozenset(
    {
        "COMMAND_AUTHORITY_REJECTED",
        "COMMAND_SEQUENCE_REJECTED",
        "COMMAND_SESSION_REJECTED",
        "SECURE_COMMAND_REJECTED",
    }
)

def _collect_latest_channels(
    text: str,
    channel_names: list[str],
    *,
    not_before_timestamp: str,
) -> dict[str, dict[str, Any]]:
    latest: dict[str, dict[str, Any]] = {}
    wanted = set(channel_names)
    for raw_line in text.splitlines():
        parsed = parse_channel_line(raw_line)
        if (
            parsed is not None
            and parsed.channel_name in wanted
            and _timestamp_at_or_after(parsed.timestamp, not_before_timestamp)
        ):
            latest[parsed.channel_name] = parsed.to_json()
    return latest


def _collect_matching_events(
    text: str,
    event_names: list[str],
    *,
    not_before_timestamp: str,
) -> list[dict[str, Any]]:
    wanted = set(event_names)
    events: list[dict[str, Any]] = []
    for raw_line in text.splitlines():
        parsed = parse_event_line(raw_line)
        if (
            parsed is not None
            and parsed.event_name in wanted
            and _timestamp_at_or_after(parsed.timestamp, not_before_timestamp)
        ):
            events.append(
                {
                    **parsed.to_json(),
                    "structured": parse_structured_event(parsed.event_name, parsed.message),
                }
            )
    return events


def _dedupe_event_matches(events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    deduped: list[dict[str, Any]] = []
    seen: set[tuple[Any, Any, Any]] = set()
    for event in events:
        key = (
            event.get("timestamp"),
            event.get("eventName"),
            event.get("message"),
        )
        if key in seen:
            continue
        seen.add(key)
        deduped.append(event)
    return deduped


def _find_command_completion(
    text: str,
    command_opcode: str,
    *,
    not_before_timestamp: str,
) -> dict[str, Any] | None:
    latest = None
    for raw_line in text.splitlines():
        parsed = parse_event_line(raw_line)
        if parsed is None or parsed.event_name != "OpCodeCompleted":
            continue
        if not _timestamp_at_or_after(parsed.timestamp, not_before_timestamp):
            continue
        if command_opcode.lower() not in parsed.message.lower():
            continue
        latest = {
            **parsed.to_json(),
            "structured": {
                "opcode": command_opcode.lower(),
                "kind": "command-completion",
                "rawMessage": parsed.message,
            },
        }
    return latest


def _local_log_timestamp() -> str:
    return datetime.now().isoformat(timespec="microseconds")


def _timestamp_at_or_after(timestamp: str | None, not_before_timestamp: str) -> bool:
    if timestamp is None:
        return False
    parsed_timestamp = _parse_local_log_timestamp(timestamp)
    parsed_not_before = _parse_local_log_timestamp(not_before_timestamp)
    if parsed_timestamp is None or parsed_not_before is None:
        return timestamp >= not_before_timestamp
    return parsed_timestamp >= parsed_not_before


def _parse_local_log_timestamp(timestamp: str) -> datetime | None:
    try:
        return datetime.fromisoformat(timestamp)
    except ValueError:
        return None


def _upload_file_worker(context: secure_ops.ManualSurfaceContext, local_path: str, destination_leaf: str, queue: Any) -> None:
    try:
        result = secure_ops.upload_file_result(
            context,
            local_path=local_path,
            destination_leaf=destination_leaf,
        )
        queue.put({"ok": True, "result": result.to_json()})
    except Exception as exc:
        queue.put({"ok": False, "error": f"{exc}\n{traceback.format_exc(limit=8)}"})


def _run_upload_file_in_subprocess(
    context: secure_ops.ManualSurfaceContext,
    *,
    local_path: str,
    destination_leaf: str,
) -> secure_ops.ManualActionResult:
    ctx = get_context("spawn")
    queue = ctx.Queue()
    process = ctx.Process(
        target=_upload_file_worker,
        args=(context, local_path, destination_leaf, queue),
    )
    process.start()
    process.join(timeout=120.0)
    if process.is_alive():
        process.terminate()
        process.join(timeout=5.0)
        raise RuntimeError("file upload worker timed out")
    try:
        payload = queue.get(timeout=5.0)
    except queue_module.Empty as exc:
        raise RuntimeError(f"file upload worker exited without result rc={process.exitcode}")
    if not payload["ok"]:
        raise RuntimeError(payload["error"])
    result_payload = dict(payload["result"])
    return secure_ops.ManualActionResult(
        status=str(result_payload.pop("status")),
        env=str(result_payload.pop("env")),
        band=str(result_payload.pop("band")),
        timestamp=str(result_payload.pop("timestamp")),
        details=result_payload,
    )
