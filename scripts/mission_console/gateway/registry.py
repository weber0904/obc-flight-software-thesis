from __future__ import annotations

import os
import pathlib
import time
from dataclasses import dataclass, field
from typing import Any

from manual_ops.lib.common import read_json
from manual_ops.lib.common import DEFAULT_SECURE_AUTH_TIMEOUT_SEC


SUPPORTED_CONTEXT_IDS = (
    "hosted-manual-dual-gds",
    "target-manual-ground-dual-gds",
)
SUPPORTED_CANONICAL_BANDS = (
    "sband",
    "uhf-backup",
    "uhf-primary-after-failover",
)
DEFAULT_HOSTED_ROOT = pathlib.Path("/tmp/manual-dual-gds/hosted")
DEFAULT_TARGET_GROUND_ROOT = pathlib.Path("/tmp/manual-dual-gds/target-ground")
DEFAULT_TARGET_BASELINE_ROOT = pathlib.Path("/tmp/manual-dual-gds/target-baseline")


@dataclass(frozen=True)
class BandSurface:
    band: str
    manifest_key: str
    surface_root: pathlib.Path
    gds_tts_port: int
    dictionary_path: pathlib.Path
    gui_url: str | None
    captures: dict[str, str]
    logs: dict[str, str]
    southbound: dict[str, Any]
    secure_state: dict[str, Any] | None
    secure_state_path: pathlib.Path
    owner_pid: int | None
    beacon: dict[str, Any] | None = None

    @property
    def listener_key(self) -> tuple[str, int, str, int | None]:
        return (str(self.surface_root), self.gds_tts_port, str(self.dictionary_path), self.owner_pid)

    @property
    def operation_lock_key(self) -> tuple[str, int, str]:
        return (str(self.surface_root), self.gds_tts_port, str(self.dictionary_path))

    def to_json(self) -> dict[str, Any]:
        return {
            "band": self.band,
            "manifestKey": self.manifest_key,
            "surfaceRoot": str(self.surface_root),
            "gdsTtsPort": self.gds_tts_port,
            "dictionaryPath": str(self.dictionary_path),
            "guiUrl": self.gui_url,
            "captures": self.captures,
            "logs": self.logs,
            "southbound": self.southbound,
            "beacon": self.beacon,
            "secureState": self.secure_state,
            "secureStatePath": str(self.secure_state_path),
            "ownerPid": self.owner_pid,
        }


@dataclass(frozen=True)
class SurfaceContext:
    context_id: str
    surface_type: str
    env_name: str
    root: pathlib.Path
    manifest_path: pathlib.Path
    status_path: pathlib.Path
    manifest: dict[str, Any] | None
    status: dict[str, Any] | None
    lifecycle_state: str
    owner_pid: int | None
    dictionary_path: pathlib.Path | None
    owner_alive: bool = True
    bands: dict[str, BandSurface] = field(default_factory=dict)
    target_baseline: dict[str, Any] | None = None
    errors: list[str] = field(default_factory=list)

    @property
    def is_running(self) -> bool:
        return (
            self.lifecycle_state == "running"
            and self.manifest is not None
            and self.owner_pid is not None
            and self.owner_alive
        )

    def band(self, band_name: str) -> BandSurface:
        try:
            return self.bands[band_name]
        except KeyError as exc:
            raise KeyError(f"{self.context_id} has no band {band_name}") from exc

    def to_json(self) -> dict[str, Any]:
        return {
            "contextId": self.context_id,
            "surfaceType": self.surface_type,
            "envName": self.env_name,
            "root": str(self.root),
            "manifestPath": str(self.manifest_path),
            "statusPath": str(self.status_path),
            "lifecycleState": self.lifecycle_state,
            "ownerPid": self.owner_pid,
            "ownerAlive": self.owner_alive,
            "dictionaryPath": (None if self.dictionary_path is None else str(self.dictionary_path)),
            "targetBaseline": self.target_baseline,
            "errors": self.errors,
            "bands": {name: band.to_json() for name, band in self.bands.items()},
        }


def _safe_read_json(path: pathlib.Path) -> dict[str, Any] | None:
    try:
        if path.exists():
            return read_json(path)
    except Exception:
        return None
    return None


def _owner_pid_is_alive(owner_pid: int | None) -> bool:
    if owner_pid is None or owner_pid <= 0:
        return False
    try:
        os.kill(owner_pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    except OSError:
        return False
    return True


def _context_root(context_id: str) -> pathlib.Path:
    if context_id == "hosted-manual-dual-gds":
        return pathlib.Path(os.environ.get("MISSION_CONSOLE_HOSTED_ROOT", str(DEFAULT_HOSTED_ROOT))).resolve()
    if context_id == "target-manual-ground-dual-gds":
        return pathlib.Path(
            os.environ.get("MISSION_CONSOLE_TARGET_GROUND_ROOT", str(DEFAULT_TARGET_GROUND_ROOT))
        ).resolve()
    raise RuntimeError(f"unsupported context {context_id}")


def _baseline_root() -> pathlib.Path:
    return pathlib.Path(
        os.environ.get("MISSION_CONSOLE_TARGET_BASELINE_ROOT", str(DEFAULT_TARGET_BASELINE_ROOT))
    ).resolve()


def _band_name_rows(manifest_key: str, surface_payload: dict[str, Any]) -> list[tuple[str, dict[str, Any]]]:
    rows: list[tuple[str, dict[str, Any]]] = []
    canonical_names = surface_payload.get("canonicalBands", [])
    for canonical in canonical_names:
        rows.append((str(canonical), surface_payload))
    if not rows:
        rows.append((manifest_key, surface_payload))
    return rows


def _public_secure_state(
    payload: dict[str, Any] | None,
    *,
    manifest: dict[str, Any],
    band_name: str,
) -> dict[str, Any] | None:
    if payload is None:
        return None
    invalidated = bool(payload.get("invalidated", False))
    invalidation_reason = payload.get("invalidationReason")
    manifest_owner_pid = payload.get("manifestOwnerPid")
    if manifest_owner_pid != manifest.get("ownerPid"):
        invalidated = True
        invalidation_reason = "surface-owner-changed"
    if payload.get("activeBand") != band_name:
        invalidated = True
        invalidation_reason = "band-selection-changed"
    last_auth_time = payload.get("lastAuthTime")
    if last_auth_time is not None and time.time() - float(last_auth_time) > DEFAULT_SECURE_AUTH_TIMEOUT_SEC:
        invalidated = True
        invalidation_reason = "session-timeout"
    return {
        "active": not invalidated,
        "serviceId": payload.get("serviceId"),
        "authorityMode": payload.get("authorityMode"),
        "nextSecureSequence": payload.get("nextSecureSequence"),
        "lastAuthTime": last_auth_time,
        "invalidated": invalidated,
        "invalidationReason": invalidation_reason,
    }


def _load_band_surfaces(manifest: dict[str, Any]) -> dict[str, BandSurface]:
    bands: dict[str, BandSurface] = {}
    owner_pid = manifest.get("ownerPid")
    surface_root = pathlib.Path(manifest.get("surfaceRoot", ".")).resolve()
    secure_root = surface_root / "secure-state"
    dictionary_path = pathlib.Path(manifest["dictionaryPath"]).resolve()
    for manifest_key, surface_payload in manifest.get("operatorSurfaces", {}).items():
        for band_name, band_payload in _band_name_rows(str(manifest_key), dict(surface_payload)):
            secure_state_path = secure_root / f"{band_name}.json"
            secure_state = _public_secure_state(
                _safe_read_json(secure_state_path),
                manifest=manifest,
                band_name=band_name,
            )
            bands[band_name] = BandSurface(
                band=band_name,
                manifest_key=str(manifest_key),
                surface_root=surface_root,
                gds_tts_port=int(surface_payload["gdsTtsPort"]),
                dictionary_path=dictionary_path,
                gui_url=surface_payload.get("guiUrl"),
                captures={str(key): str(value) for key, value in dict(surface_payload.get("captures", {})).items()},
                logs={str(key): str(value) for key, value in dict(surface_payload.get("logs", {})).items()},
                southbound=dict(surface_payload.get("southbound", {})),
                beacon=(dict(surface_payload.get("beacon", {})) if isinstance(surface_payload.get("beacon"), dict) else None),
                secure_state=secure_state,
                secure_state_path=secure_state_path,
                owner_pid=(None if owner_pid is None else int(owner_pid)),
            )
    return bands


def discover_context(context_id: str) -> SurfaceContext:
    root = _context_root(context_id)
    manifest_path = root / "manifest.json"
    status_path = root / "status.json"
    manifest = _safe_read_json(manifest_path)
    status = _safe_read_json(status_path)
    payload = manifest or status or {}
    errors: list[str] = []
    if manifest is None:
        errors.append("manifest-missing")
    if status is None:
        errors.append("status-missing")
    surface_type = str(payload.get("surfaceType", context_id))
    env_name = "hosted" if context_id == "hosted-manual-dual-gds" else "target"
    lifecycle_state = str(payload.get("lifecycleState", "missing"))
    owner_pid = payload.get("ownerPid")
    if owner_pid is not None:
        owner_pid = int(owner_pid)
    owner_alive = _owner_pid_is_alive(owner_pid)
    if lifecycle_state == "running" and owner_pid is not None and not owner_alive:
        lifecycle_state = "stale-owner-dead"
        errors.append("owner-pid-dead")
    dictionary_path = None
    bands: dict[str, BandSurface] = {}
    if manifest is not None:
        if "dictionaryPath" in manifest:
            dictionary_path = pathlib.Path(manifest["dictionaryPath"]).resolve()
        try:
            bands = _load_band_surfaces(manifest)
        except Exception as exc:
            errors.append(f"band-load-failed:{exc}")
    target_baseline = None
    if context_id == "target-manual-ground-dual-gds":
        baseline_root = _baseline_root()
        target_baseline = {
            "baselineRoot": str(baseline_root),
            "manifest": _safe_read_json(baseline_root / "manifest.json"),
            "status": _safe_read_json(baseline_root / "status.json"),
        }
    return SurfaceContext(
        context_id=context_id,
        surface_type=surface_type,
        env_name=env_name,
        root=root,
        manifest_path=manifest_path,
        status_path=status_path,
        manifest=manifest,
        status=status,
        lifecycle_state=lifecycle_state,
        owner_pid=owner_pid,
        owner_alive=owner_alive,
        dictionary_path=dictionary_path,
        bands=bands,
        target_baseline=target_baseline,
        errors=errors,
    )


class SurfaceRegistry:
    def __init__(self) -> None:
        self._contexts: dict[str, SurfaceContext] = {}
        self._band_drift_tokens: dict[tuple[str, str], tuple[str, int, str, int | None]] = {}
        self._drifted: dict[tuple[str, str], str] = {}

    def refresh(self) -> dict[str, SurfaceContext]:
        next_contexts: dict[str, SurfaceContext] = {}
        drifted: dict[tuple[str, str], str] = {}
        next_tokens: dict[tuple[str, str], tuple[str, int, str, int | None]] = {}
        for context_id in SUPPORTED_CONTEXT_IDS:
            context = discover_context(context_id)
            next_contexts[context_id] = context
            for band_name, band in context.bands.items():
                key = (context_id, band_name)
                next_tokens[key] = band.listener_key
                previous = self._band_drift_tokens.get(key)
                if previous is not None and previous != band.listener_key:
                    drifted[key] = "listener-key-changed"
        self._contexts = next_contexts
        self._band_drift_tokens = next_tokens
        self._drifted = drifted
        return dict(self._contexts)

    def contexts(self) -> dict[str, SurfaceContext]:
        if not self._contexts:
            return self.refresh()
        return dict(self._contexts)

    def context(self, context_id: str, *, refresh: bool = False) -> SurfaceContext:
        if refresh:
            return self.refresh()[context_id]
        return self.contexts()[context_id]

    def drift_reason(self, context_id: str, band: str) -> str | None:
        return self._drifted.get((context_id, band))

    def to_json(self, *, refresh: bool = False) -> dict[str, Any]:
        contexts = self.refresh() if refresh else self.contexts()
        return {
            "contexts": {context_id: context.to_json() for context_id, context in contexts.items()},
            "driftedBands": {
                f"{context_id}:{band}": reason for (context_id, band), reason in sorted(self._drifted.items())
            },
        }
