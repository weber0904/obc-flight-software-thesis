from __future__ import annotations

import pathlib
from datetime import datetime, timezone
from typing import Any

from decode_beacon_v1 import BEACON_MAGIC, BEACON_V1_WIRE_SIZE, DecodeError, decode_beacon_v1

from .registry import SUPPORTED_CANONICAL_BANDS, SurfaceContext
from .snapshots import SnapshotStore


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def _decode_latest_beacon(payload: bytes) -> dict[str, Any]:
    """Return the newest complete BeaconV1 frame, regardless of capture alignment."""
    magic = BEACON_MAGIC.to_bytes(4, byteorder="little")
    offset = len(payload)
    while True:
        offset = payload.rfind(magic, 0, offset)
        if offset < 0:
            raise DecodeError("no complete BeaconV1 frame found")
        frame = payload[offset : offset + BEACON_V1_WIRE_SIZE]
        if len(frame) == BEACON_V1_WIRE_SIZE:
            try:
                return decode_beacon_v1(frame)
            except DecodeError:
                pass


class BeaconManager:
    def __init__(self, snapshots: SnapshotStore) -> None:
        self.snapshots = snapshots

    def poll_contexts(self, contexts: dict[str, SurfaceContext]) -> None:
        for context_id, context in contexts.items():
            snapshot = self._snapshot_for_context(context)
            previous = self.snapshots.beacon_latest(context_id)
            if self._is_same_beacon_frame(previous, snapshot):
                snapshot["lastObservedAt"] = previous.get("lastObservedAt")
            self.snapshots.set_beacon_snapshot(context_id, snapshot)

    @staticmethod
    def _is_same_beacon_frame(previous: dict[str, Any] | None, current: dict[str, Any]) -> bool:
        if previous is None or not previous.get("available") or not current.get("available"):
            return False
        previous_crc = previous.get("decoded", {}).get("crc")
        current_crc = current.get("decoded", {}).get("crc")
        return previous_crc is not None and previous_crc == current_crc

    def _snapshot_for_context(self, context: SurfaceContext) -> dict[str, Any]:
        band_name, surface, beacon_capability = self._select_beacon_surface(context)
        if surface is None or beacon_capability is None:
            return {
                "contextId": context.context_id,
                "supported": False,
                "available": False,
                "reason": "surface-does-not-export-beacon-capability",
                "sourceBand": None,
                "sourceKind": None,
                "capture": {
                    "path": None,
                    "sizeBytes": 0,
                    "frameSize": BEACON_V1_WIRE_SIZE,
                    "frameCount": 0,
                },
                "decode": {
                    "status": "unsupported",
                    "error": None,
                },
                "summary": None,
                "decoded": None,
                "lastObservedAt": None,
                "sequence": None,
            }

        capture_path = pathlib.Path(str(beacon_capability.get("capturePath", ""))).resolve()
        frame_size = int(beacon_capability.get("frameSize") or BEACON_V1_WIRE_SIZE)
        base_payload: dict[str, Any] = {
            "contextId": context.context_id,
            "supported": bool(beacon_capability.get("supported", False)),
            "available": False,
            "reason": None,
            "sourceBand": beacon_capability.get("sourceBand") or band_name,
            "sourceKind": beacon_capability.get("sourceKind"),
            "capture": {
                "path": str(capture_path),
                "sizeBytes": 0,
                "frameSize": frame_size,
                "frameCount": 0,
            },
            "decode": {
                "status": "missing",
                "error": None,
            },
            "summary": None,
            "decoded": None,
            "lastObservedAt": None,
            "sequence": None,
        }
        if not context.is_running:
            base_payload["reason"] = "surface-not-running"
            base_payload["decode"]["status"] = "unavailable"
            return base_payload
        if not capture_path.exists():
            base_payload["reason"] = "capture-missing"
            return base_payload

        payload = capture_path.read_bytes()
        size_bytes = len(payload)
        frame_count = size_bytes // frame_size if frame_size > 0 else 0
        base_payload["capture"]["sizeBytes"] = size_bytes
        base_payload["capture"]["frameCount"] = frame_count
        if frame_count <= 0:
            base_payload["reason"] = "capture-short"
            base_payload["decode"]["status"] = "short-frame"
            return base_payload

        try:
            decoded = _decode_latest_beacon(payload)
        except DecodeError as exc:
            base_payload["reason"] = "decode-failed"
            base_payload["decode"] = {
                "status": "decode-failed",
                "error": str(exc),
            }
            return base_payload

        observed_at = _utc_now()
        time_payload = dict(decoded.get("time", {}))
        summary = {
            "lastBeaconTime": time_payload.get("seconds"),
            "sequence": decoded.get("sequence"),
            "mode": decoded.get("mode"),
            "batterySoc": decoded.get("battery_soc"),
            "adcsMode": decoded.get("adcs_mode"),
        }
        base_payload.update(
            {
                "available": True,
                "reason": None,
                "decode": {
                    "status": "ok",
                    "error": None,
                },
                "summary": summary,
                "decoded": decoded,
                "lastObservedAt": observed_at,
                "sequence": decoded.get("sequence"),
            }
        )
        return base_payload

    def _select_beacon_surface(
        self,
        context: SurfaceContext,
    ) -> tuple[str | None, Any | None, dict[str, Any] | None]:
        preferred_bands = list(SUPPORTED_CANONICAL_BANDS)
        seen = set(preferred_bands)
        preferred_bands.extend(name for name in context.bands if name not in seen)
        for band_name in preferred_bands:
            surface = context.bands.get(band_name)
            if surface is None:
                continue
            if surface.beacon and surface.beacon.get("supported"):
                return band_name, surface, surface.beacon
        return None, None, None
