from __future__ import annotations

import json
import pathlib
import re
import struct
import subprocess
import time
from dataclasses import dataclass
from typing import Any

from manual_ops import manual_secure_ops as secure_ops
from manual_ops.lib.common import find_local_tool, utc_timestamp
from secure_link_auth_lib import (
    COMMAND_DESCRIPTOR,
    FW_PACKET_COMMAND,
    SECURE_COMMAND_V2_AUTH_LABEL,
    SECURE_COMMAND_V2_HEADER_LENGTH,
    SECURE_COMMAND_V2_MAC_LENGTH,
    SECURE_COMMAND_V2_MAGIC,
    SECURE_COMMAND_V2_OPCODE,
    SECURE_COMMAND_V2_VERSION,
    build_secure_command_v2_packet,
    hmac_sha256,
    send_tts_raw_packet,
)

from .locks import BandLockPool
from .parsers import REJECT_EVENT_NAMES, parse_channel_line, parse_event_line, parse_structured_event
from .registry import SurfaceRegistry
from .snapshots import SnapshotStore


PACKET_CASES = {
    "replay-captured-raw": "Replay a previously captured secure packet without rebuilding it.",
    "replay-stale-session": "Send a packet signed with an older cached session key.",
    "duplicate-sequence": "Reuse an already consumed secure sequence number.",
    "tampered-sequence": "Send a packet with an intentionally wrong sequence number.",
    "tampered-mac": "Corrupt only the MAC bytes while preserving the other fields.",
}
CASE_EXPECTED_FAILURE = {
    "replay-captured-raw": "replay-or-stale-session",
    "replay-stale-session": "session-stale",
    "duplicate-sequence": "sequence-reused",
    "tampered-sequence": "sequence-invalid",
    "tampered-mac": "integrity-failure",
}
HEX_ONLY_NON_REUSABLE_CAPTURE_CASES = {"tampered-mac", "replay-stale-session"}
OFFSET_NON_REUSABLE_CAPTURE_CASES = {"duplicate-sequence", "tampered-sequence"}
NON_REUSABLE_CAPTURE_CASES = HEX_ONLY_NON_REUSABLE_CAPTURE_CASES | OFFSET_NON_REUSABLE_CAPTURE_CASES
PACKET_LAB_REJECT_CHANNELS = (
    "SESSION_LAST_ACCEPTED_SEQUENCE",
    "SEQUENCE_REJECT_TOTAL",
    "SEQUENCE_LAST_REJECT_SEQUENCE_NUMBER",
    "SEQUENCE_LAST_REJECT_INNER_OPCODE",
    "SEQUENCE_LAST_REJECT_REASON",
    "SESSION_REJECT_TOTAL",
    "SESSION_LAST_REJECT_SEQUENCE_NUMBER",
    "SESSION_LAST_REJECT_INNER_OPCODE",
    "SESSION_LAST_REJECT_REASON",
    "SECURE_COMMAND_REJECT_TOTAL",
    "SECURE_COMMAND_REJECT_BAD_MAC",
    "SECURE_COMMAND_REJECT_NO_AUTH",
    "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER",
    "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE",
    "SECURE_COMMAND_LAST_REJECT_REASON",
)
PACKET_LAB_CHANNEL_SEARCH_TERMS = ("REJECT", "SESSION_LAST_ACCEPTED_SEQUENCE")
PRIMING_ACCEPT_EVENT_NAMES = ("OpCodeCompleted",)
PRIMING_COMMAND_NAME = "OBCApp.modeManager.MODE_GET"
SECURE_COMMAND_REJECT_REASON_NAMES = {
    0: "NONE",
    1: "BAD_MAGIC",
    2: "UNSUPPORTED_VERSION",
    3: "NONZERO_FLAGS",
    4: "BAD_HEADER_LENGTH",
    5: "NONZERO_RESERVED",
    6: "TRUNCATED_HEADER",
    7: "TRUNCATED_INNER_COMMAND",
    8: "OVERSIZED_INNER_COMMAND",
    9: "BAD_MAC_LENGTH",
    10: "TRUNCATED_AUTH_TAG",
    11: "UNEXPECTED_TRAILING_BYTES",
    12: "INVALID_INNER_COMMAND",
    13: "AUTH_REQUIRED",
    14: "SERVICE_MISMATCH",
    15: "BAD_MAC",
    16: "SEQUENCE_NOT_INCREASING",
    17: "SEQUENCE_WINDOW_FULL",
    18: "INVALID_CONFIG",
}
COMMAND_SEQUENCE_REJECT_REASON_NAMES = {
    0: "NONE",
    1: "NOT_INCREASING",
    2: "WINDOW_FULL",
}
COMMAND_SESSION_REJECT_REASON_NAMES = {
    0: "NONE",
    1: "NOT_OPEN",
    2: "SESSION_MISMATCH",
    3: "ALREADY_OPEN",
    4: "BAD_OPEN_SEQUENCE",
    5: "LEGACY_LIFECYCLE_UNSUPPORTED",
    6: "WINDOW_FULL",
    7: "STALE_REPLAY",
    8: "PERSISTENT_STATE_UNAVAILABLE",
}


@dataclass(frozen=True)
class TransportPacket:
    fw_packet_payload: bytes
    offset: int


def parse_transport_packets(raw_bytes: bytes) -> list[TransportPacket]:
    packets: list[TransportPacket] = []
    offset = 0
    while offset + 8 <= len(raw_bytes):
        descriptor, length = struct.unpack_from(">II", raw_bytes, offset)
        if descriptor != COMMAND_DESCRIPTOR:
            offset += 1
            continue
        start = offset + 8
        end = start + length
        if end > len(raw_bytes):
            break
        packets.append(TransportPacket(fw_packet_payload=raw_bytes[start:end], offset=offset))
        offset = end
    if packets:
        return packets

    # Stock GDS captures can contain a native envelope rather than the
    # synthetic COMMAND_DESCRIPTOR wrapper used by send_tts_raw_packet().
    # Scan for a complete secure F' command packet while retaining its
    # absolute capture offset for the existing replay-exclusion bookkeeping.
    offset = 0
    minimum_header_size = 6 + SECURE_COMMAND_V2_HEADER_LENGTH
    while offset + minimum_header_size <= len(raw_bytes):
        packet_kind, opcode = struct.unpack_from(">HI", raw_bytes, offset)
        if packet_kind != FW_PACKET_COMMAND or opcode != SECURE_COMMAND_V2_OPCODE:
            offset += 1
            continue
        (
            _packet_kind,
            _opcode,
            magic,
            version,
            reserved,
            header_length,
            _sequence_number,
            inner_length,
            mac_length,
            trailer,
        ) = struct.unpack_from(">HIIBBHIHHI", raw_bytes, offset)
        payload_length = 6 + header_length + inner_length + mac_length
        inner_offset = offset + 6 + header_length
        if (
            magic != SECURE_COMMAND_V2_MAGIC
            or version != SECURE_COMMAND_V2_VERSION
            or reserved != 0
            or header_length != SECURE_COMMAND_V2_HEADER_LENGTH
            or inner_length < 6
            or mac_length != SECURE_COMMAND_V2_MAC_LENGTH
            or trailer != 0
            or offset + payload_length > len(raw_bytes)
            or struct.unpack_from(">H", raw_bytes, inner_offset)[0] != FW_PACKET_COMMAND
        ):
            offset += 1
            continue
        packets.append(
            TransportPacket(
                fw_packet_payload=raw_bytes[offset : offset + payload_length],
                offset=offset,
            )
        )
        offset += payload_length
    return packets


def parse_packet_summary(fw_packet_payload: bytes, *, source: str, highlight: str) -> dict[str, Any]:
    summary: dict[str, Any] = {
        "transport": {"payloadLength": len(fw_packet_payload)},
        "fwPacketHeader": {},
        "secureHeader": {},
        "innerCommand": {},
        "authPreview": {},
        "source": source,
        "highlight": highlight,
    }
    if len(fw_packet_payload) >= 6:
        packet_kind, opcode = struct.unpack_from(">HI", fw_packet_payload, 0)
        summary["fwPacketHeader"] = {"packetKind": packet_kind, "opcode": f"0x{opcode:08X}"}
    if len(fw_packet_payload) >= 26 and summary["fwPacketHeader"].get("opcode") == f"0x{SECURE_COMMAND_V2_OPCODE:08X}":
        (
            packet_kind,
            opcode,
            magic,
            version,
            _reserved,
            header_length,
            sequence_number,
            inner_length,
            mac_length,
            _trailer,
        ) = struct.unpack_from(">HIIBBHIHHI", fw_packet_payload, 0)
        inner_start = 26
        inner_end = inner_start + inner_length
        inner_command = fw_packet_payload[inner_start:inner_end]
        auth_tag = fw_packet_payload[inner_end : inner_end + mac_length]
        summary["secureHeader"] = {
            "magic": f"0x{magic:08X}",
            "version": version,
            "headerLength": header_length,
            "secureSequence": sequence_number,
            "innerLength": inner_length,
            "macLength": mac_length,
        }
        if len(inner_command) >= 6:
            inner_kind, inner_opcode = struct.unpack_from(">HI", inner_command, 0)
            summary["innerCommand"] = {
                "packetKind": inner_kind,
                "opcode": f"0x{inner_opcode:08X}",
            }
        summary["authPreview"] = {
            "head": auth_tag[:4].hex(),
            "tail": auth_tag[-4:].hex() if auth_tag else "",
        }
    return summary


def _collect_latest_channels(text: str, channel_names: tuple[str, ...]) -> dict[str, dict[str, Any]]:
    latest: dict[str, dict[str, Any]] = {}
    wanted = set(channel_names)
    for raw_line in text.splitlines():
        parsed = parse_channel_line(raw_line)
        if parsed is not None and parsed.channel_name in wanted:
            latest[parsed.channel_name] = parsed.to_json()
    return latest


def _collect_matching_events(text: str, event_names: tuple[str, ...]) -> list[dict[str, Any]]:
    wanted = set(event_names)
    events: list[dict[str, Any]] = []
    for raw_line in text.splitlines():
        parsed = parse_event_line(raw_line)
        if parsed is not None and parsed.event_name in wanted:
            events.append(
                {
                    **parsed.to_json(),
                    "structured": parse_structured_event(parsed.event_name, parsed.message),
                }
            )
    return events


def _merge_channel_snapshots(
    existing_channels: dict[str, dict[str, Any]],
    additional_channels: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    merged = dict(existing_channels)
    for channel_name, additional in additional_channels.items():
        current = merged.get(channel_name)
        if current is None:
            merged[channel_name] = dict(additional)
            continue
        snapshot = dict(current)
        snapshot.update(additional)
        if "generation" not in additional and "generation" in current:
            snapshot["generation"] = current["generation"]
        merged[channel_name] = snapshot
    return merged


def _mark_channel_fresh_after_marker(
    channel_name: str,
    before_channels: dict[str, dict[str, Any]],
    after_channels: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    snapshot = after_channels.get(channel_name)
    if snapshot is None:
        return after_channels
    merged = dict(after_channels)
    updated = dict(snapshot)
    before_generation = int(before_channels.get(channel_name, {}).get("generation", 0) or 0)
    after_generation = int(updated.get("generation", 0) or 0)
    if after_generation <= before_generation:
        updated["generation"] = before_generation + 1
    merged[channel_name] = updated
    return merged


def _mark_channels_observed_after_marker(
    channel_names: tuple[str, ...],
    after_channels: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    merged = dict(after_channels)
    for channel_name in channel_names:
        snapshot = merged.get(channel_name)
        if snapshot is None:
            continue
        updated = dict(snapshot)
        updated["postMarkerObserved"] = True
        merged[channel_name] = updated
    return merged


class PacketLabService:
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
        self.session_cache_root = runtime_root / "session-cache"
        self.cli_path = find_local_tool("fprime-cli")

    def available_cases(self, context_id: str) -> list[str]:
        if context_id == "target-manual-ground-dual-gds":
            return ["replay-captured-raw", "duplicate-sequence", "tampered-mac"]
        return list(PACKET_CASES)

    def inject(self, *, context_id: str, band: str, case: str, operator_label: str = "mission-console") -> dict[str, Any]:
        try:
            if case not in PACKET_CASES:
                raise RuntimeError(f"unsupported packet-lab case {case}")
            if case not in self.available_cases(context_id):
                raise RuntimeError(f"case {case} is disabled for {context_id}")
            context = self.registry.context(context_id, refresh=True)
            if not context.is_running:
                raise RuntimeError(
                    f"context {context_id} is not running (lifecycleState={context.lifecycle_state})"
                )
            with self.lock_pool.hold(*self._operation_lock_key(context, band)):
                manual_context = secure_ops.resolve_context(
                    env=context.env_name,
                    band=band,
                    manifest_path=context.manifest_path,
                    enforce_target_gates=True,
                )
                if case in {"duplicate-sequence", "tampered-sequence"}:
                    self._prime_duplicate_sequence(context_id, band, manual_context)
                before_events = self.snapshots.recent_events(context_id, band, limit=50)
                before_channels = self.snapshots.channel_map(context_id, band)
                listener_marker = self._listener_log_marker(context_id, band)
                capture_marker = self._capture_length_marker(manual_context)
                fw_packet, source = self._build_case_payload(manual_context, case, context_id=context_id)
                expected = CASE_EXPECTED_FAILURE[case]
                summary = parse_packet_summary(fw_packet, source=source, highlight=expected)
                send_tts_raw_packet(int(manual_context.surface["gdsTtsPort"]), fw_packet)
                time.sleep(2.0)
                attempted_sequence = summary.get("secureHeader", {}).get("secureSequence")
                attempted_opcode = summary.get("innerCommand", {}).get("opcode")
                evidence_deadline = time.time() + 4.0
                after_events, after_channels, observed = self._observe_after_injection(
                    context_id=context_id,
                    band=band,
                    before_events=before_events,
                    before_channels=before_channels,
                    listener_marker=listener_marker,
                    attempted_sequence=attempted_sequence,
                    attempted_opcode=attempted_opcode,
                )
                while observed["kind"] == "inconclusive" and time.time() < evidence_deadline:
                    time.sleep(0.5)
                    after_events, after_channels, observed = self._observe_after_injection(
                        context_id=context_id,
                        band=band,
                        before_events=before_events,
                        before_channels=before_channels,
                        listener_marker=listener_marker,
                        attempted_sequence=attempted_sequence,
                        attempted_opcode=attempted_opcode,
                    )
                if observed["kind"] == "inconclusive":
                    authority_channels = self._bounded_command_ingress_channels(manual_context, timeout_sec=2.0)
                    after_events, after_channels, observed = self._observe_after_injection(
                        context_id=context_id,
                        band=band,
                        before_events=before_events,
                        before_channels=before_channels,
                        listener_marker=listener_marker,
                        attempted_sequence=attempted_sequence,
                        attempted_opcode=attempted_opcode,
                        extra_channels=authority_channels,
                    )
                payload = {
                    "timestamp": utc_timestamp(),
                    "contextId": context_id,
                    "band": band,
                    "case": case,
                    "description": PACKET_CASES[case],
                    "expectedFailureReason": expected,
                    "operatorLabel": operator_label,
                    "source": source,
                    "packetSummary": summary,
                    "rawBytesHex": fw_packet.hex(),
                    "observedEvidence": self._augment_observed_evidence(observed),
                    "faultExplanation": self._fault_explanation(
                        case=case,
                        packet_summary=summary,
                        current_state=secure_ops.load_state(context.manifest, band),
                    ),
                }
                capture_offset = self._locate_captured_payload_offset(
                    manual_context,
                    fw_packet,
                    minimum_offset=capture_marker,
                )
                if capture_offset is not None:
                    payload["capturePacketOffset"] = capture_offset
                self.snapshots.record_packet_lab(payload)
                return payload
        except Exception as exc:
            self.snapshots.record_packet_lab(
                {
                    "timestamp": utc_timestamp(),
                    "contextId": context_id,
                    "band": band,
                    "case": case,
                    "description": PACKET_CASES.get(case),
                    "expectedFailureReason": CASE_EXPECTED_FAILURE.get(case),
                    "operatorLabel": operator_label,
                    "status": "failed",
                    "error": str(exc),
                }
            )
            raise

    def _build_case_payload(
        self,
        context: secure_ops.ManualSurfaceContext,
        case: str,
        *,
        context_id: str | None = None,
    ) -> tuple[bytes, str]:
        if case == "replay-captured-raw":
            if context_id is None:
                raise RuntimeError("replay-captured-raw requires context id")
            return self._latest_captured_secure_packet(context_id, context), "capture-replay"
        current_state = secure_ops.require_active_state(context)
        dictionaries, encoder = secure_ops.load_command_context(context.dictionary_path)
        inner = secure_ops.encode_inner_command(dictionaries, encoder, "OBCApp.modeManager.MODE_GET", [])
        if case == "duplicate-sequence":
            if current_state.next_secure_sequence <= 1:
                raise RuntimeError("duplicate-sequence requires a previously consumed secure sequence")
            sequence_number = current_state.next_secure_sequence - 1
            return (
                build_secure_command_v2_packet(inner, current_state.session_key, sequence_number),
                "live-crafted",
            )
        if case == "tampered-sequence":
            if current_state.next_secure_sequence <= 1:
                raise RuntimeError("tampered-sequence requires a previously consumed secure sequence")
            sequence_number = current_state.next_secure_sequence - 1
            return (
                build_secure_command_v2_packet(inner, current_state.session_key, sequence_number),
                "live-crafted",
            )
        if case == "tampered-mac":
            packet = bytearray(build_secure_command_v2_packet(inner, current_state.session_key, current_state.next_secure_sequence))
            packet[-1] ^= 0xFF
            return bytes(packet), "live-crafted"
        if case == "replay-stale-session":
            if context_id is None:
                raise RuntimeError("replay-stale-session requires context id")
            session_key, sequence_number = self._load_stale_session_material(context_id, context, current_state)
            return (
                build_secure_command_v2_packet(inner, session_key, sequence_number),
                "stale-session-cache",
            )
        raise RuntimeError(f"unsupported case {case}")

    def _latest_captured_secure_packet(self, context_id: str, context: secure_ops.ManualSurfaceContext) -> bytes:
        capture_path = pathlib.Path(context.surface["captures"]["gdsToSouthbound"])
        if not capture_path.exists():
            raise RuntimeError(f"capture not found: {capture_path}")
        packets = parse_transport_packets(capture_path.read_bytes())
        related_bands = self._capture_related_bands(context_id, context.band)
        excluded_offsets: set[int] = set()
        excluded_hex = {
            str(entry.get("rawBytesHex", "")).lower()
            for entry in self.snapshots.packet_lab_history(limit=200)
            if entry.get("contextId") == context_id
            and entry.get("band") in related_bands
            and entry.get("case") in HEX_ONLY_NON_REUSABLE_CAPTURE_CASES
            and (entry.get("source") or entry.get("packetSummary", {}).get("source")) != "capture-replay"
            and entry.get("rawBytesHex")
        }
        for entry in self.snapshots.packet_lab_history(limit=200):
            if (
                entry.get("contextId") != context_id
                or entry.get("band") not in related_bands
                or entry.get("case") not in NON_REUSABLE_CAPTURE_CASES
                or (entry.get("source") or entry.get("packetSummary", {}).get("source")) == "capture-replay"
            ):
                continue
            capture_offset = entry.get("capturePacketOffset")
            if capture_offset is None:
                continue
            try:
                excluded_offsets.add(int(capture_offset))
            except (TypeError, ValueError):
                continue
        for packet in reversed(packets):
            if len(packet.fw_packet_payload) >= 6:
                _kind, opcode = struct.unpack_from(">HI", packet.fw_packet_payload, 0)
                if (
                    opcode == SECURE_COMMAND_V2_OPCODE
                    and packet.offset not in excluded_offsets
                    and packet.fw_packet_payload.hex().lower() not in excluded_hex
                ):
                    return packet.fw_packet_payload
        raise RuntimeError(f"no reusable secure command packet found in capture {capture_path}")

    def _capture_related_bands(self, context_id: str, band: str) -> set[str]:
        try:
            context = self.registry.context(context_id)
            requested_band = context.band(band)
        except Exception:
            return {band}
        return {
            name
            for name, surface in context.bands.items()
            if surface.operation_lock_key == requested_band.operation_lock_key
        } or {band}

    def _load_stale_session_material(
        self,
        context_id: str,
        context: secure_ops.ManualSurfaceContext,
        current_state: secure_ops.SecureSessionState,
    ) -> tuple[bytes, int]:
        candidates: list[pathlib.Path] = []
        seen: set[pathlib.Path] = set()
        for band_name in self._stale_session_related_bands(context_id, context):
            for path in sorted(self.session_cache_root.glob(f"{context_id}-{band_name}-*.json")):
                resolved = path.resolve()
                if resolved in seen:
                    continue
                seen.add(resolved)
                candidates.append(path)
        for path in candidates:
            try:
                payload = json.loads(path.read_text(encoding="utf-8"))
                state = secure_ops.SecureSessionState.from_json(payload)
            except Exception:
                continue
            if state.session_key_hex != current_state.session_key_hex:
                return state.session_key, max(1, state.next_secure_sequence)
        raise RuntimeError("no stale session cache available for replay-stale-session")

    def _stale_session_related_bands(
        self,
        context_id: str,
        context: secure_ops.ManualSurfaceContext,
    ) -> list[str]:
        try:
            registry_context = self.registry.context(context_id)
            requested_band = registry_context.band(context.band)
            related = [
                name
                for name, surface in registry_context.bands.items()
                if surface.operation_lock_key == requested_band.operation_lock_key
            ]
            if related:
                return sorted(set(related))
        except Exception:
            pass
        target_service_id = secure_ops.band_service_id(context.band)
        related: list[str] = []
        seen: set[str] = set()
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

    def _prime_duplicate_sequence(
        self,
        context_id: str,
        band: str,
        context: secure_ops.ManualSurfaceContext,
    ) -> None:
        state = secure_ops.require_active_state(context)
        if state.next_secure_sequence > 1:
            return
        before_channels = self.snapshots.channel_map(context_id, band)
        listener_marker = self._listener_log_marker(context_id, band)
        expected_command_opcode = self._command_opcode_hex(context, PRIMING_COMMAND_NAME, [])
        secure_ops.send_command_result(
            context,
            command_name=PRIMING_COMMAND_NAME,
            command_args=[],
        )
        if not self._wait_for_primed_sequence_acceptance(
            context_id=context_id,
            band=band,
            before_channels=before_channels,
            listener_marker=listener_marker,
            expected_accepted_sequence=state.next_secure_sequence,
            expected_command_opcode=expected_command_opcode,
            timeout_sec=4.0,
        ):
            self._invalidate_priming_session(context)
            raise RuntimeError("duplicate-sequence priming produced no accepted-sequence evidence")

    def _invalidate_priming_session(self, context: secure_ops.ManualSurfaceContext) -> None:
        state = secure_ops.load_state(context.manifest, context.band)
        if state is not None:
            state.invalidated = True
            state.invalidation_reason = "packet-lab-priming-evidence-missing"
            secure_ops.save_state(context.manifest, context.band, state)
        secure_ops.invalidate_sibling_service_states(
            context.manifest,
            current_band=context.band,
            current_service_id=context.service_id,
            reason="packet-lab-priming-evidence-missing",
        )

    def _wait_for_primed_sequence_acceptance(
        self,
        *,
        context_id: str,
        band: str,
        before_channels: dict[str, dict[str, Any]],
        listener_marker: dict[str, int],
        expected_accepted_sequence: int,
        expected_command_opcode: str | None,
        timeout_sec: float,
    ) -> bool:
        deadline = time.time() + timeout_sec
        while time.time() < deadline:
            if self._has_primed_sequence_acceptance(
                context_id=context_id,
                band=band,
                before_channels=before_channels,
                listener_marker=listener_marker,
                expected_accepted_sequence=expected_accepted_sequence,
                expected_command_opcode=expected_command_opcode,
            ):
                return True
            time.sleep(0.5)
        return self._has_primed_sequence_acceptance(
            context_id=context_id,
            band=band,
            before_channels=before_channels,
            listener_marker=listener_marker,
            expected_accepted_sequence=expected_accepted_sequence,
            expected_command_opcode=expected_command_opcode,
        )

    def _has_primed_sequence_acceptance(
        self,
        *,
        context_id: str,
        band: str,
        before_channels: dict[str, dict[str, Any]],
        listener_marker: dict[str, int],
        expected_accepted_sequence: int,
        expected_command_opcode: str | None,
    ) -> bool:
        late_native_events = self._native_event_search_many(
            context_id,
            band,
            PRIMING_ACCEPT_EVENT_NAMES,
            after_offset=int(listener_marker["eventOffset"]),
        )
        if self._has_matching_priming_completion(late_native_events, expected_command_opcode):
            return True
        after_channels = self.snapshots.channel_map(context_id, band)
        late_native_channels = self._native_channel_search_many(
            context_id,
            band,
            ("SESSION_LAST_ACCEPTED_SEQUENCE",),
            after_offset=int(listener_marker["channelOffset"]),
        )
        if late_native_channels:
            after_channels = _merge_channel_snapshots(after_channels, late_native_channels)
            after_channels = _mark_channel_fresh_after_marker(
                "SESSION_LAST_ACCEPTED_SEQUENCE",
                before_channels,
                after_channels,
            )
        before_sequence_snapshot = before_channels.get("SESSION_LAST_ACCEPTED_SEQUENCE") or {}
        after_sequence_snapshot = after_channels.get("SESSION_LAST_ACCEPTED_SEQUENCE") or {}
        before_sequence = self._channel_int(before_sequence_snapshot)
        after_sequence = self._channel_int(after_sequence_snapshot)
        before_generation = int(before_sequence_snapshot.get("generation", 0) or 0)
        after_generation = int(after_sequence_snapshot.get("generation", 0) or 0)
        if after_sequence is None or after_generation <= before_generation:
            return False
        if after_sequence < expected_accepted_sequence:
            return False
        if before_sequence is not None and after_sequence <= before_sequence:
            return False
        return True

    def _has_matching_priming_completion(
        self,
        events: list[dict[str, Any]],
        expected_command_opcode: str | None,
    ) -> bool:
        if not expected_command_opcode:
            return False
        opcode = expected_command_opcode.lower()
        for event in events:
            if str(event.get("eventName")) != "OpCodeCompleted":
                continue
            if opcode in str(event.get("message", "")).lower():
                return True
        return False

    def _command_opcode_hex(
        self,
        context: secure_ops.ManualSurfaceContext,
        command_name: str,
        command_args: list[str],
    ) -> str | None:
        try:
            dictionaries, encoder = secure_ops.load_command_context(context.dictionary_path)
            inner = secure_ops.encode_inner_command(dictionaries, encoder, command_name, command_args)
        except Exception:
            return None
        if len(inner) < 6:
            return None
        return f"0x{int.from_bytes(inner[2:6], 'big'):08x}"

    def _capture_length_marker(self, context: secure_ops.ManualSurfaceContext) -> int | None:
        capture_path = pathlib.Path(context.surface.get("captures", {}).get("gdsToSouthbound", ""))
        if not capture_path.exists():
            return None
        try:
            return capture_path.stat().st_size
        except OSError:
            return None

    def _locate_captured_payload_offset(
        self,
        context: secure_ops.ManualSurfaceContext,
        payload: bytes,
        *,
        minimum_offset: int | None,
    ) -> int | None:
        capture_path = pathlib.Path(context.surface.get("captures", {}).get("gdsToSouthbound", ""))
        if not capture_path.exists():
            return None
        try:
            packets = parse_transport_packets(capture_path.read_bytes())
        except OSError:
            return None
        for packet in reversed(packets):
            if packet.fw_packet_payload != payload:
                continue
            if minimum_offset is not None and packet.offset < minimum_offset:
                continue
            return packet.offset
        return None

    def _classify_observed_result(
        self,
        before_events: list[dict[str, Any]],
        after_events: list[dict[str, Any]],
        before_channels: dict[str, dict[str, Any]],
        after_channels: dict[str, dict[str, Any]],
        *,
        attempted_sequence: int | None = None,
        attempted_opcode: str | None = None,
    ) -> dict[str, Any]:
        if any("eventSeq" in event for event in [*before_events, *after_events]):
            baseline_event_seq = max((int(event.get("eventSeq", 0)) for event in before_events), default=0)
            before_event_keys = {
                (event.get("timestamp"), event.get("eventName"), event.get("message"))
                for event in before_events
            }
            new_events = [
                event
                for event in after_events
                if int(event.get("eventSeq", 0)) > baseline_event_seq
                or (
                    "eventSeq" not in event
                    and (event.get("timestamp"), event.get("eventName"), event.get("message")) not in before_event_keys
                )
            ]
        else:
            new_events = after_events[len(before_events) :]
        reject_events = [event for event in new_events if event["eventName"] in REJECT_EVENT_NAMES]
        completion_events = [
            event
            for event in new_events
            if event["eventName"] in {"OpCodeCompleted", "OpCodeDispatched", "SYS_MODE_CHANGE"}
        ]
        before_sequence_snapshot = before_channels.get("SESSION_LAST_ACCEPTED_SEQUENCE") or {}
        after_sequence_snapshot = after_channels.get("SESSION_LAST_ACCEPTED_SEQUENCE") or {}
        before_sequence = before_sequence_snapshot.get("value")
        after_sequence = after_sequence_snapshot.get("value")
        before_generation = int(before_sequence_snapshot.get("generation", 0) or 0)
        after_generation = int(after_sequence_snapshot.get("generation", 0) or 0)
        has_fresh_sequence_sample = after_sequence is not None and (
            after_generation > before_generation
            or bool(after_sequence_snapshot.get("postMarkerObserved"))
        )
        after_sequence_int = None if after_sequence is None else int(after_sequence)
        if reject_events:
            return {
                "kind": "explicit-reject",
                "events": reject_events,
                "sequenceBefore": before_sequence,
                "sequenceAfter": after_sequence,
            }
        reject_telemetry = self._reject_telemetry_evidence(
            before_channels,
            after_channels,
            attempted_sequence=attempted_sequence,
            attempted_opcode=attempted_opcode,
        )
        if reject_telemetry is not None:
            return {
                "kind": "explicit-reject",
                "telemetry": reject_telemetry,
                "sequenceBefore": before_sequence,
                "sequenceAfter": after_sequence,
            }
        if (
            attempted_sequence is not None
            and after_sequence_int is not None
            and after_sequence_int < attempted_sequence
            and not completion_events
        ):
            if not has_fresh_sequence_sample:
                return {
                    "kind": "inconclusive",
                    "reason": "sequence-sample-did-not-refresh-after-injection",
                    "sequenceBefore": before_sequence,
                    "sequenceAfter": after_sequence,
                    "attemptedSequence": attempted_sequence,
                }
            return {
                "kind": "bounded-no-op",
                "reason": "no-command-completion-and-last-accepted-sequence-remained-below-attempted-sequence",
                "sequenceBefore": before_sequence,
                "sequenceAfter": after_sequence,
                "attemptedSequence": attempted_sequence,
            }
        if before_sequence == after_sequence:
            if before_sequence is None or after_sequence is None or completion_events or not has_fresh_sequence_sample:
                return {
                    "kind": "inconclusive",
                    "reason": "missing-sequence-evidence-or-completion-prevents-bounded-no-op-classification",
                    "sequenceBefore": before_sequence,
                    "sequenceAfter": after_sequence,
                }
            return {
                "kind": "bounded-no-op",
                "reason": "no-explicit-reject-event-and-last-accepted-sequence-did-not-advance",
                "sequenceBefore": before_sequence,
                "sequenceAfter": after_sequence,
            }
        return {
            "kind": "inconclusive",
            "reason": "accepted-sequence-changed-or-no-bounded-negative-evidence",
            "sequenceBefore": before_sequence,
            "sequenceAfter": after_sequence,
        }

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

    def _native_event_search_many(
        self,
        context_id: str,
        band: str,
        event_names: tuple[str, ...],
        *,
        after_offset: int,
    ) -> list[dict[str, Any]]:
        path = self._listener_log_paths(context_id, band)[0]
        if not path.exists():
            return []
        text = path.read_text(encoding="utf-8", errors="replace")
        if after_offset > len(text):
            return _collect_matching_events(text, event_names)
        return _collect_matching_events(text[after_offset:], event_names)

    def _native_channel_search_many(
        self,
        context_id: str,
        band: str,
        channel_names: tuple[str, ...],
        *,
        after_offset: int,
    ) -> dict[str, dict[str, Any]]:
        path = self._listener_log_paths(context_id, band)[1]
        if not path.exists():
            return {}
        text = path.read_text(encoding="utf-8", errors="replace")
        if after_offset > len(text):
            return _collect_latest_channels(text, channel_names)
        return _collect_latest_channels(text[after_offset:], channel_names)

    def _merge_events(
        self,
        existing_events: list[dict[str, Any]],
        additional_events: list[dict[str, Any]],
    ) -> list[dict[str, Any]]:
        merged = list(existing_events)
        seen = {
            (event.get("timestamp"), event.get("eventName"), event.get("message"))
            for event in merged
        }
        for event in additional_events:
            key = (event.get("timestamp"), event.get("eventName"), event.get("message"))
            if key in seen:
                continue
            seen.add(key)
            merged.append(event)
        return merged

    def _observe_after_injection(
        self,
        *,
        context_id: str,
        band: str,
        before_events: list[dict[str, Any]],
        before_channels: dict[str, dict[str, Any]],
        listener_marker: dict[str, int],
        attempted_sequence: int | None,
        attempted_opcode: str | None,
        extra_channels: dict[str, dict[str, Any]] | None = None,
    ) -> tuple[list[dict[str, Any]], dict[str, dict[str, Any]], dict[str, Any]]:
        after_events = self.snapshots.recent_events(context_id, band, limit=50)
        after_channels = self.snapshots.channel_map(context_id, band)
        late_native_events = self._native_event_search_many(
            context_id,
            band,
            REJECT_EVENT_NAMES,
            after_offset=int(listener_marker["eventOffset"]),
        )
        if late_native_events:
            after_events = self._merge_events(after_events, late_native_events)
        late_native_channels = self._native_channel_search_many(
            context_id,
            band,
            PACKET_LAB_REJECT_CHANNELS,
            after_offset=int(listener_marker["channelOffset"]),
        )
        if late_native_channels:
            after_channels = _merge_channel_snapshots(after_channels, late_native_channels)
            after_channels = _mark_channels_observed_after_marker(
                tuple(late_native_channels.keys()),
                after_channels,
            )
            if "SESSION_LAST_ACCEPTED_SEQUENCE" in late_native_channels:
                after_channels = _mark_channel_fresh_after_marker(
                    "SESSION_LAST_ACCEPTED_SEQUENCE",
                    before_channels,
                    after_channels,
                )
        if extra_channels:
            after_channels = _merge_channel_snapshots(after_channels, extra_channels)
            after_channels = _mark_channels_observed_after_marker(
                tuple(extra_channels.keys()),
                after_channels,
            )
            if "SESSION_LAST_ACCEPTED_SEQUENCE" in extra_channels:
                after_channels = _mark_channel_fresh_after_marker(
                    "SESSION_LAST_ACCEPTED_SEQUENCE",
                    before_channels,
                    after_channels,
                )
        observed = self._classify_observed_result(
            before_events,
            after_events,
            before_channels,
            after_channels,
            attempted_sequence=attempted_sequence,
            attempted_opcode=attempted_opcode,
        )
        return after_events, after_channels, observed

    def _bounded_command_ingress_channels(
        self,
        manual_context: secure_ops.ManualSurfaceContext,
        *,
        timeout_sec: float,
    ) -> dict[str, dict[str, Any]]:
        results: dict[str, dict[str, Any]] = {}
        for search_term in PACKET_LAB_CHANNEL_SEARCH_TERMS:
            args = [
                self.cli_path,
                "channels",
                "--dictionary",
                str(manual_context.dictionary_path),
                "--no-zmq",
                "--tts-port",
                str(int(manual_context.surface["gdsTtsPort"])),
                "--search",
                search_term,
                "--timeout",
                str(timeout_sec),
                "--log-to-stdout",
            ]
            try:
                completed = subprocess.run(
                    args,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    check=False,
                    timeout=max(4.0, timeout_sec + 1.0),
                )
                output = completed.stdout or ""
            except subprocess.TimeoutExpired as exc:
                output = "" if exc.stdout is None else str(exc.stdout)
            results.update(_collect_latest_channels(output, PACKET_LAB_REJECT_CHANNELS))
        return results

    def _reject_telemetry_evidence(
        self,
        before_channels: dict[str, dict[str, Any]],
        after_channels: dict[str, dict[str, Any]],
        *,
        attempted_sequence: int | None,
        attempted_opcode: str | None,
    ) -> dict[str, Any] | None:
        if attempted_sequence is None:
            return None
        candidate_prefixes = (
            "SECURE_COMMAND",
            "SEQUENCE",
            "SESSION",
        )
        for prefix in candidate_prefixes:
            sequence_name = f"{prefix}_LAST_REJECT_SEQUENCE_NUMBER"
            reason_name = f"{prefix}_LAST_REJECT_REASON"
            opcode_name = f"{prefix}_LAST_REJECT_INNER_OPCODE"
            total_name = f"{prefix}_REJECT_TOTAL"
            sequence_after = self._channel_int(after_channels.get(sequence_name))
            if sequence_after != attempted_sequence:
                continue
            opcode_after = self._channel_int(after_channels.get(opcode_name))
            if attempted_opcode is not None and opcode_after is not None:
                attempted_opcode_int = int(attempted_opcode, 16)
                if opcode_after != attempted_opcode_int:
                    continue
            reason_after = self._channel_int(after_channels.get(reason_name))
            total_before = self._channel_int(before_channels.get(total_name))
            total_after = self._channel_int(after_channels.get(total_name))
            if total_after is None and reason_after is None:
                continue
            if total_before is not None and total_after is not None and total_after < total_before:
                continue
            total_advanced = (
                total_before is not None
                and total_after is not None
                and total_after > total_before
            )
            generation_advanced = any(
                self._channel_generation(after_channels.get(name)) > self._channel_generation(before_channels.get(name))
                for name in (sequence_name, reason_name, opcode_name, total_name)
            )
            post_marker_observed = any(
                bool(after_channels.get(name, {}).get("postMarkerObserved"))
                for name in (sequence_name, reason_name, opcode_name, total_name)
            )
            if not total_advanced and not generation_advanced:
                if total_before is None and post_marker_observed:
                    pass
                else:
                    continue
            if total_before is None and total_after is not None and total_after <= 0:
                continue
            return {
                "source": "reject-telemetry",
                "prefix": prefix,
                "rejectPath": prefix,
                "sequenceChannel": sequence_name,
                "sequenceValue": sequence_after,
                "reasonChannel": reason_name,
                "reasonValue": reason_after,
                "opcodeChannel": opcode_name,
                "opcodeValue": None if opcode_after is None else f"0x{opcode_after:08X}",
                "totalChannel": total_name,
                "totalBefore": total_before,
                "totalAfter": total_after,
            }
        return None

    def _augment_observed_evidence(self, observed: dict[str, Any]) -> dict[str, Any]:
        payload = dict(observed)
        if payload.get("kind") == "explicit-reject":
            reject_path = str(payload.get("prefix") or payload.get("rejectPath") or "")
            if reject_path:
                payload["rejectPath"] = reject_path
            reason_value = payload.get("reasonValue")
            if isinstance(reason_value, int):
                payload["reasonName"] = self._decode_reject_reason(reject_path, reason_value)
            events = list(payload.get("events") or [])
            if events:
                decoded = self._decode_reject_event(events[-1])
                if decoded is not None:
                    payload.setdefault("rejectPath", decoded.get("rejectPath"))
                    payload["reasonValue"] = decoded.get("reasonValue")
                    payload["reasonName"] = decoded.get("reasonName")
                    payload["eventName"] = decoded.get("eventName")
        return payload

    def _decode_reject_reason(self, reject_path: str, reason_value: int) -> str:
        reject_path = str(reject_path or "").upper()
        if reject_path.startswith("SECURE_COMMAND"):
            return SECURE_COMMAND_REJECT_REASON_NAMES.get(reason_value, f"UNKNOWN_{reason_value}")
        if reject_path.startswith("SEQUENCE"):
            return COMMAND_SEQUENCE_REJECT_REASON_NAMES.get(reason_value, f"UNKNOWN_{reason_value}")
        if reject_path.startswith("SESSION"):
            return COMMAND_SESSION_REJECT_REASON_NAMES.get(reason_value, f"UNKNOWN_{reason_value}")
        return f"UNKNOWN_{reason_value}"

    def _decode_reject_event(self, event: dict[str, Any]) -> dict[str, Any] | None:
        event_name = str(event.get("eventName") or "")
        message = str(event.get("message") or "")
        match = re.search(r"reason\s+(\d+)", message)
        if match is None:
            return None
        reason_value = int(match.group(1))
        if event_name == "SECURE_COMMAND_REJECTED":
            reject_path = "SECURE_COMMAND"
        elif event_name == "COMMAND_SEQUENCE_REJECTED":
            reject_path = "SEQUENCE"
        elif event_name == "COMMAND_SESSION_REJECTED":
            reject_path = "SESSION"
        else:
            return None
        return {
            "eventName": event_name,
            "rejectPath": reject_path,
            "reasonValue": reason_value,
            "reasonName": self._decode_reject_reason(reject_path, reason_value),
        }

    def _fault_explanation(
        self,
        *,
        case: str,
        packet_summary: dict[str, Any],
        current_state: secure_ops.SecureSessionState | None,
    ) -> dict[str, Any]:
        secure_header = packet_summary.get("secureHeader") or {}
        secure_sequence = secure_header.get("secureSequence")
        next_sequence = None if current_state is None else current_state.next_secure_sequence
        explanations: dict[str, dict[str, Any]] = {
            "replay-captured-raw": {
                "faultKind": "replay",
                "affectedField": "captured secure packet bytes",
                "expectedCondition": "a newly built packet for the current session",
                "actualInjectedCondition": "replayed a previously captured secure packet without rebuilding it",
                "whyRejected": "the flight side should reject replayed or stale secure traffic",
            },
            "replay-stale-session": {
                "faultKind": "stale-session",
                "affectedField": "session material",
                "expectedCondition": "packet signed with the current active secure session",
                "actualInjectedCondition": "packet signed with cached older session material",
                "whyRejected": "the session no longer matches the current authorized secure session",
            },
            "duplicate-sequence": {
                "faultKind": "duplicate-sequence",
                "affectedField": "secure sequence",
                "expectedCondition": f"next secure sequence {next_sequence}" if next_sequence is not None else "current next secure sequence",
                "actualInjectedCondition": f"reused secure sequence {secure_sequence}",
                "whyRejected": "the secure sequence was already consumed and is no longer increasing",
            },
            "tampered-sequence": {
                "faultKind": "tampered-sequence",
                "affectedField": "secure sequence",
                "expectedCondition": f"next secure sequence {next_sequence}" if next_sequence is not None else "current next secure sequence",
                "actualInjectedCondition": f"forced incorrect secure sequence {secure_sequence}",
                "whyRejected": "the secure sequence does not satisfy the active window rules",
            },
            "tampered-mac": {
                "faultKind": "tampered-mac",
                "affectedField": "MAC / auth tag",
                "expectedCondition": "valid MAC for the exact packet bytes",
                "actualInjectedCondition": "corrupted MAC bytes while leaving the rest of the packet intact",
                "whyRejected": "the auth tag no longer matches the packet content",
            },
        }
        return explanations.get(case, {"faultKind": case})

    def _channel_int(self, snapshot: dict[str, Any] | None) -> int | None:
        if not snapshot:
            return None
        raw = snapshot.get("value")
        if raw is None:
            return None
        text = str(raw).strip()
        if not text:
            return None
        if text.lower().startswith("0x"):
            try:
                return int(text, 16)
            except ValueError:
                return None
        try:
            return int(text)
        except ValueError:
            return None

    def _channel_generation(self, snapshot: dict[str, Any] | None) -> int:
        if not snapshot:
            return 0
        return int(snapshot.get("generation", 0) or 0)

    def _operation_lock_key(self, context: Any, band: str) -> tuple[Any, ...]:
        surface = context.band(band)
        return (context.context_id, *surface.operation_lock_key)
