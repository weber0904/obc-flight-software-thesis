#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import os
import pathlib
import shlex
import sys
import time
from dataclasses import dataclass, field
from typing import Any

ROOT_DIR = pathlib.Path(__file__).resolve().parents[2]
VENV_PYTHON = ROOT_DIR / "fprime-venv" / "bin" / "python"
if __name__ == "__main__" and VENV_PYTHON.exists() and pathlib.Path(sys.executable).resolve() != VENV_PYTHON.resolve():
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), __file__, *sys.argv[1:]])

SCRIPTS_DIR = ROOT_DIR / "scripts"
LIB_DIR = pathlib.Path(__file__).resolve().parent / "lib"
for candidate in (SCRIPTS_DIR, LIB_DIR):
    if str(candidate) not in sys.path:
        sys.path.insert(0, str(candidate))

venv_lib_root = ROOT_DIR / "fprime-venv" / "lib"
if venv_lib_root.exists():
    for python_dir in sorted(venv_lib_root.glob("python*")):
        site_packages = python_dir / "site-packages"
        if site_packages.exists() and str(site_packages) not in sys.path:
            sys.path.insert(0, str(site_packages))

from fprime_gds.common.data_types.cmd_data import CmdData
from fprime_gds.common.encoders.cmd_encoder import CmdEncoder
from fprime_gds.common.models.dictionaries import Dictionaries
from fprime_gds.common.pipeline.standard import StandardPipeline
from fprime_gds.common.testing_fw.api import IntegrationTestAPI
from fprime_gds.common.utils.config_manager import ConfigManager

from common import (
    DEFAULT_SECURE_AUTH_TIMEOUT_SEC,
    ROOT_DIR as REPO_ROOT,
    atomic_write_json,
    read_json,
    service_environment,
    sha256_file,
    ssh_capture,
    systemctl_show,
    remote_journal_since_now,
    utc_timestamp,
    wait_remote_journal_fragments,
)
from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
    SERVICE_ID_SBAND,
    SERVICE_ID_UHF,
    build_req_auth_packet,
    build_response_packet,
    build_secure_command_v2_packet,
    compute_auth_response,
    default_command_auth_keystore_path,
    derive_session_key,
    keystore_entry_for_service_id,
    load_command_auth_keystore,
    load_handshake_messages,
    load_handshake_messages_from_native_packet_log,
    send_tts_raw_packet,
)


SEQ_COMMAND_MAP = {
    "validate": "OBCApp.sequenceAdmissionController.SEQ_VALIDATE",
    "run": "OBCApp.sequenceAdmissionController.SEQ_RUN",
    "prepare-manual": "OBCApp.sequenceAdmissionController.SEQ_PREPARE_MANUAL",
    "start": "OBCApp.sequenceAdmissionController.SEQ_START",
    "step": "OBCApp.sequenceAdmissionController.SEQ_STEP",
    "cancel": "OBCApp.sequenceAdmissionController.SEQ_CANCEL",
}


@dataclass
class SecureSessionState:
    service_id: int
    active_band: str
    next_secure_sequence: int
    last_auth_time: float
    manifest_path: str
    authority_mode: str
    session_key_hex: str
    manifest_owner_pid: int | None
    invalidated: bool = False
    invalidation_reason: str | None = None

    def to_json(self) -> dict[str, Any]:
        return {
            "serviceId": self.service_id,
            "activeBand": self.active_band,
            "nextSecureSequence": self.next_secure_sequence,
            "lastAuthTime": self.last_auth_time,
            "manifestPath": self.manifest_path,
            "authorityMode": self.authority_mode,
            "sessionKeyHex": self.session_key_hex,
            "manifestOwnerPid": self.manifest_owner_pid,
            "invalidated": self.invalidated,
            "invalidationReason": self.invalidation_reason,
        }

    @classmethod
    def from_json(cls, payload: dict[str, Any]) -> "SecureSessionState":
        return cls(
            service_id=int(payload["serviceId"]),
            active_band=str(payload["activeBand"]),
            next_secure_sequence=int(payload["nextSecureSequence"]),
            last_auth_time=float(payload["lastAuthTime"]),
            manifest_path=str(payload["manifestPath"]),
            authority_mode=str(payload["authorityMode"]),
            session_key_hex=str(payload["sessionKeyHex"]),
            manifest_owner_pid=(None if payload.get("manifestOwnerPid") is None else int(payload["manifestOwnerPid"])),
            invalidated=bool(payload.get("invalidated", False)),
            invalidation_reason=(None if payload.get("invalidationReason") is None else str(payload["invalidationReason"])),
        )

    @property
    def session_key(self) -> bytes:
        return bytes.fromhex(self.session_key_hex)


@dataclass(frozen=True)
class ManualSurfaceContext:
    env: str
    band: str
    manifest_path: pathlib.Path
    manifest: dict[str, Any]
    surface: dict[str, Any]
    service_id: int
    dictionary_path: pathlib.Path
    secure_state_path: pathlib.Path


@dataclass
class ManualActionResult:
    status: str
    env: str
    band: str
    timestamp: str = field(default_factory=utc_timestamp)
    details: dict[str, Any] = field(default_factory=dict)

    def to_json(self) -> dict[str, Any]:
        payload = {
            "status": self.status,
            "env": self.env,
            "band": self.band,
            "timestamp": self.timestamp,
        }
        payload.update(self.details)
        return payload


def normalize_sequence_destination(value: str) -> str:
    if value.startswith(".sequence-staging/"):
        leaf = value.split("/", 1)[1]
    else:
        leaf = value
    if "/" in leaf or leaf in {"", ".", ".."}:
        raise RuntimeError("governed staged upload requires one leaf under .sequence-staging/")
    return f".sequence-staging/{leaf}"


def canonical_surface_key(band: str) -> str:
    return "sband" if band == "sband" else "uhf"


def band_service_id(band: str) -> int:
    return SERVICE_ID_SBAND if band == "sband" else SERVICE_ID_UHF


def state_path(manifest: dict[str, Any], band: str) -> pathlib.Path:
    return pathlib.Path(manifest["surfaceRoot"]) / "secure-state" / f"{band}.json"


def load_manifest(path: pathlib.Path) -> dict[str, Any]:
    manifest = read_json(path)
    if "operatorSurfaces" not in manifest:
        raise RuntimeError(f"manifest has no operatorSurfaces: {path}")
    return manifest


def surface_for_band(manifest: dict[str, Any], band: str) -> dict[str, Any]:
    key = canonical_surface_key(band)
    try:
        return manifest["operatorSurfaces"][key]
    except KeyError as exc:
        raise RuntimeError(f"manifest surface missing for band {band}") from exc


def load_state(manifest: dict[str, Any], band: str) -> SecureSessionState | None:
    path = state_path(manifest, band)
    if not path.exists():
        return None
    state = SecureSessionState.from_json(read_json(path))
    return validate_state(manifest, band, state)


def validate_state(manifest: dict[str, Any], band: str, state: SecureSessionState) -> SecureSessionState:
    owner_pid = manifest.get("ownerPid")
    if owner_pid != state.manifest_owner_pid:
        state.invalidated = True
        state.invalidation_reason = "surface-owner-changed"
    if state.active_band != band:
        state.invalidated = True
        state.invalidation_reason = "band-selection-changed"
    if time.time() - state.last_auth_time > DEFAULT_SECURE_AUTH_TIMEOUT_SEC:
        state.invalidated = True
        state.invalidation_reason = "session-timeout"
    return state


def save_state(manifest: dict[str, Any], band: str, state: SecureSessionState) -> pathlib.Path:
    path = state_path(manifest, band)
    atomic_write_json(path, state.to_json())
    os.chmod(path, 0o600)
    return path


def clear_state(manifest: dict[str, Any], band: str) -> None:
    path = state_path(manifest, band)
    try:
        path.unlink()
    except FileNotFoundError:
        return


def _manifest_canonical_bands(manifest: dict[str, Any]) -> list[str]:
    surfaces = manifest.get("operatorSurfaces", {})
    canonical_bands: set[str] = set()
    for surface in surfaces.values():
        for band in surface.get("canonicalBands", []):
            canonical_bands.add(str(band))
    return sorted(canonical_bands)


def invalidate_band_switch_states(
    manifest: dict[str, Any],
    *,
    current_band: str,
    current_state: SecureSessionState,
    reason: str = "band-switch-command-sent",
) -> None:
    for band in sorted({current_band, *_manifest_canonical_bands(manifest)}):
        if band == current_band:
            state = current_state
        else:
            path = state_path(manifest, band)
            if not path.exists():
                continue
            try:
                state = SecureSessionState.from_json(read_json(path))
            except Exception:
                continue
        state.invalidated = True
        state.invalidation_reason = reason
        save_state(manifest, band, state)


def invalidate_sibling_service_states(
    manifest: dict[str, Any],
    *,
    current_band: str,
    current_service_id: int,
    reason: str = "sibling-band-reauthenticated",
) -> None:
    for band in _manifest_canonical_bands(manifest):
        if band == current_band or band_service_id(band) != current_service_id:
            continue
        path = state_path(manifest, band)
        if not path.exists():
            continue
        try:
            state = SecureSessionState.from_json(read_json(path))
        except Exception:
            continue
        state.invalidated = True
        state.invalidation_reason = reason
        save_state(manifest, band, state)


def load_command_context(dictionary_path: pathlib.Path) -> tuple[Dictionaries, CmdEncoder]:
    dictionaries = Dictionaries()
    dictionaries.load_dictionaries(str(dictionary_path), None, None)
    return dictionaries, CmdEncoder()


def encode_inner_command(dictionaries: Dictionaries, encoder: CmdEncoder, command_name: str, args: list[str]) -> bytes:
    template = dictionaries.command_name[command_name]
    encoded = encoder.encode_api(CmdData(tuple(args), template))
    return encoded[8:]


def resolve_context(
    *,
    env: str,
    band: str,
    manifest_path: str | pathlib.Path,
    enforce_target_gates: bool = False,
) -> ManualSurfaceContext:
    manifest_path_obj = pathlib.Path(manifest_path).resolve()
    manifest = load_manifest(manifest_path_obj)
    if enforce_target_gates:
        maybe_run_target_provenance_gate(manifest, env)
        maybe_run_target_manual_auth_preflight_gate(manifest, env)
    dictionary_path = pathlib.Path(manifest["dictionaryPath"])
    if not dictionary_path.exists():
        raise RuntimeError(f"dictionary missing: {dictionary_path}")
    return ManualSurfaceContext(
        env=env,
        band=band,
        manifest_path=manifest_path_obj,
        manifest=manifest,
        surface=surface_for_band(manifest, band),
        service_id=band_service_id(band),
        dictionary_path=dictionary_path,
        secure_state_path=state_path(manifest, band),
    )


def command_request_payload(
    *,
    env: str,
    band: str,
    manifest_path: str | pathlib.Path,
    command_name: str,
    command_args: list[str] | tuple[str, ...],
) -> dict[str, Any]:
    return {
        "env": env,
        "band": band,
        "manifestPath": str(pathlib.Path(manifest_path).resolve()),
        "commandName": command_name,
        "commandArgs": list(command_args),
    }


def _print_result(result: ManualActionResult) -> int:
    print(json.dumps(result.to_json(), indent=2, sort_keys=True))
    return 0


def _native_recv_packet_log() -> pathlib.Path | None:
    value = os.environ.get("MANUAL_SECURE_AUTH_NATIVE_RECV_BIN", "").strip()
    if not value:
        return None
    return pathlib.Path(value).resolve()


def _load_handshake_messages_for_context(
    context: ManualSurfaceContext,
    capture_path: pathlib.Path,
    *,
    message_type: HandshakeMessageType,
    native_packet_log: pathlib.Path | None,
) -> dict[str, list[Any]]:
    capture_kwargs = {
        "scid": int(context.surface.get("gdsScid", 68)),
        "vcid": int(context.surface.get("gdsVcid", 1 if context.service_id == SERVICE_ID_SBAND else 2)),
        "frame_size": int(context.surface.get("gdsFrameSize", 4096)),
        "service_id": context.service_id,
        "message_type": message_type,
    }
    return {
        "wire-capture": load_handshake_messages(capture_path, **capture_kwargs),
        "native-recv": (
            []
            if native_packet_log is None
            else load_handshake_messages_from_native_packet_log(
                native_packet_log,
                service_id=context.service_id,
                message_type=message_type,
            )
        ),
    }


def _handshake_progress(messages_by_source: dict[str, list[Any]]) -> tuple[int, int]:
    return (len(messages_by_source["wire-capture"]), len(messages_by_source["native-recv"]))


def _wait_for_handshake_from_sources(
    context: ManualSurfaceContext,
    capture_path: pathlib.Path,
    *,
    message_type: HandshakeMessageType,
    seen_progress: tuple[int, int],
    timeout: float,
    native_packet_log: pathlib.Path | None,
) -> tuple[Any, str, tuple[int, int]]:
    deadline = time.time() + timeout
    preferred_sources = ("native-recv", "wire-capture")
    while time.time() < deadline:
        messages_by_source = _load_handshake_messages_for_context(
            context,
            capture_path,
            message_type=message_type,
            native_packet_log=native_packet_log,
        )
        progress = _handshake_progress(messages_by_source)
        for source in preferred_sources:
            messages = messages_by_source[source]
            seen_count = seen_progress[0] if source == "wire-capture" else seen_progress[1]
            if len(messages) > seen_count:
                return messages[-1], source, progress
        time.sleep(0.2)
    native_suffix = "" if native_packet_log is None else f" or {native_packet_log}"
    raise TimeoutError(f"timed out waiting for {message_type.name} on {capture_path}{native_suffix}")


def _target_auth_journal_context(context: ManualSurfaceContext) -> dict[str, Any] | None:
    if context.env != "target":
        return None
    baseline = context.manifest.get("targetBaseline", {})
    ingress_port = 0 if context.band == "sband" else 1
    obc_target = str(baseline.get("obcSshTarget", "operator@obc.local"))
    obc_service = str(baseline.get("obcCommServiceName", "obc-comm-csp-stack.service"))
    return {
        "target": obc_target,
        "service": obc_service,
        "ingressPort": ingress_port,
    }


def establish_auth_result(
    context: ManualSurfaceContext,
    *,
    native_packet_log: pathlib.Path | None = None,
) -> ManualActionResult:
    capture_path = pathlib.Path(context.surface["captures"]["southboundToGds"])
    if native_packet_log is None:
        native_packet_log = _native_recv_packet_log()
    challenge_progress = _handshake_progress(
        _load_handshake_messages_for_context(
            context,
            capture_path,
            message_type=HandshakeMessageType.CHALLENGE,
            native_packet_log=native_packet_log,
        )
    )
    target_journal = _target_auth_journal_context(context)
    auth_deadline = time.time() + float(os.environ.get("MANUAL_SECURE_AUTH_ESTABLISH_TIMEOUT_SEC", "45"))
    challenge = None
    challenge_source = "wire-capture"
    last_challenge_error: Exception | None = None
    while time.time() < auth_deadline:
        journal_since = None
        if target_journal is not None:
            journal_since = remote_journal_since_now(target_journal["target"])
        send_tts_raw_packet(int(context.surface["gdsTtsPort"]), build_req_auth_packet(context.service_id))
        try:
            challenge, challenge_source, challenge_progress = _wait_for_handshake_from_sources(
                context,
                capture_path,
                message_type=HandshakeMessageType.CHALLENGE,
                seen_progress=challenge_progress,
                timeout=min(8.0, max(1.0, auth_deadline - time.time())),
                native_packet_log=native_packet_log,
            )
            break
        except TimeoutError as exc:
            last_challenge_error = exc
            if target_journal is not None and journal_since is not None:
                journal_text = ssh_capture(
                    target_journal["target"],
                    f"journalctl -u {shlex.quote(target_journal['service'])} --since {shlex.quote(journal_since)} --no-pager || true",
                    check=False,
                )
                challenge_fragment = (
                    f"Secure auth challenge issued ingress {target_journal['ingressPort']} service {context.service_id}"
                )
                if challenge_fragment in journal_text:
                    try:
                        challenge, challenge_source, challenge_progress = _wait_for_handshake_from_sources(
                            context,
                            capture_path,
                            message_type=HandshakeMessageType.CHALLENGE,
                            seen_progress=challenge_progress,
                            timeout=min(6.0, max(1.0, auth_deadline - time.time())),
                            native_packet_log=native_packet_log,
                        )
                        break
                    except TimeoutError as late_exc:
                        last_challenge_error = late_exc
            challenge_progress = _handshake_progress(
                _load_handshake_messages_for_context(
                    context,
                    capture_path,
                    message_type=HandshakeMessageType.CHALLENGE,
                    native_packet_log=native_packet_log,
                )
            )
            time.sleep(1.0)
    if challenge is None:
        assert last_challenge_error is not None
        raise last_challenge_error
    keystore = load_command_auth_keystore(default_command_auth_keystore_path(REPO_ROOT))
    root_key = keystore_entry_for_service_id(keystore, context.service_id).key_bytes
    session_key = derive_session_key(root_key, context.service_id, challenge.challenge)
    response = compute_auth_response(session_key)
    status_progress = _handshake_progress(
        _load_handshake_messages_for_context(
            context,
            capture_path,
            message_type=HandshakeMessageType.AUTH_STATUS,
            native_packet_log=native_packet_log,
        )
    )
    auth_status = None
    auth_status_source = "wire-capture"
    last_status_error: Exception | None = None
    auth_confirmed_via_journal = False
    while time.time() < auth_deadline:
        journal_since = None
        if target_journal is not None:
            journal_since = remote_journal_since_now(target_journal["target"])
        send_tts_raw_packet(int(context.surface["gdsTtsPort"]), build_response_packet(context.service_id, response))
        try:
            auth_status, auth_status_source, status_progress = _wait_for_handshake_from_sources(
                context,
                capture_path,
                message_type=HandshakeMessageType.AUTH_STATUS,
                seen_progress=status_progress,
                timeout=min(8.0, max(1.0, auth_deadline - time.time())),
                native_packet_log=native_packet_log,
            )
        except TimeoutError as exc:
            last_status_error = exc
            if target_journal is not None and journal_since is not None:
                try:
                    wait_remote_journal_fragments(
                        target_journal["target"],
                        target_journal["service"],
                        (
                            f"Secure auth established ingress {target_journal['ingressPort']} service {context.service_id}",
                            f"Command session opened ingress {target_journal['ingressPort']}",
                        ),
                        4.0,
                        since=journal_since,
                    )
                    auth_confirmed_via_journal = True
                    break
                except Exception:
                    pass
            status_progress = _handshake_progress(
                _load_handshake_messages_for_context(
                    context,
                    capture_path,
                    message_type=HandshakeMessageType.AUTH_STATUS,
                    native_packet_log=native_packet_log,
                )
            )
            time.sleep(1.0)
            continue
        if auth_status.status_code == int(AuthStatusCode.AUTHENTICATED):
            break
        if auth_status.status_code == int(AuthStatusCode.NOT_AUTHENTICATED):
            if target_journal is not None and journal_since is not None:
                try:
                    wait_remote_journal_fragments(
                        target_journal["target"],
                        target_journal["service"],
                        (
                            f"Secure auth established ingress {target_journal['ingressPort']} service {context.service_id}",
                            f"Command session opened ingress {target_journal['ingressPort']}",
                        ),
                        4.0,
                        since=journal_since,
                    )
                    auth_confirmed_via_journal = True
                    break
                except Exception:
                    pass
            last_status_error = RuntimeError(f"auth failed with status code {auth_status.status_code}")
            status_progress = _handshake_progress(
                _load_handshake_messages_for_context(
                    context,
                    capture_path,
                    message_type=HandshakeMessageType.AUTH_STATUS,
                    native_packet_log=native_packet_log,
                )
            )
            time.sleep(1.0)
            continue
        raise RuntimeError(f"unexpected auth status code {auth_status.status_code}")
    if not auth_confirmed_via_journal:
        if auth_status is None:
            assert last_status_error is not None
            raise last_status_error
        if auth_status.status_code != int(AuthStatusCode.AUTHENTICATED):
            if last_status_error is not None:
                raise last_status_error
            raise RuntimeError(f"auth failed with status code {auth_status.status_code}")
    state = SecureSessionState(
        service_id=context.service_id,
        active_band=context.band,
        next_secure_sequence=1,
        last_auth_time=time.time(),
        manifest_path=str(context.manifest_path),
        authority_mode=("sband-primary" if context.service_id == SERVICE_ID_SBAND else context.band),
        session_key_hex=session_key.hex(),
        manifest_owner_pid=context.manifest.get("ownerPid"),
    )
    invalidate_sibling_service_states(
        context.manifest,
        current_band=context.band,
        current_service_id=context.service_id,
    )
    path = save_state(context.manifest, context.band, state)
    return ManualActionResult(
        status="authenticated",
        env=context.env,
        band=context.band,
        details={
            "serviceId": context.service_id,
            "authorityMode": state.authority_mode,
            "nextSecureSequence": state.next_secure_sequence,
            "statePath": str(path),
            "challengeSource": challenge_source,
            "authStatusSource": ("target-journal" if auth_confirmed_via_journal else auth_status_source),
        },
    )


def establish_auth(args: argparse.Namespace) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest, enforce_target_gates=True)
    return _print_result(establish_auth_result(context))


def require_active_state(context: ManualSurfaceContext) -> SecureSessionState:
    state = load_state(context.manifest, context.band)
    if state is None:
        raise RuntimeError(f"no stored session state for {context.band}")
    if state.invalidated:
        save_state(context.manifest, context.band, state)
        raise RuntimeError(f"stored session invalidated: {state.invalidation_reason}")
    return state


def send_command_result(
    context: ManualSurfaceContext,
    *,
    command_name: str,
    command_args: list[str] | tuple[str, ...],
) -> ManualActionResult:
    dictionaries, encoder = load_command_context(context.dictionary_path)
    state = require_active_state(context)
    command_arg_list = list(command_args)
    inner = encode_inner_command(dictionaries, encoder, command_name, command_arg_list)
    payload = build_secure_command_v2_packet(inner, state.session_key, state.next_secure_sequence)
    send_tts_raw_packet(int(context.surface["gdsTtsPort"]), payload)
    result = ManualActionResult(
        status="sent",
        env=context.env,
        band=context.band,
        details={
            "command": command_name,
            "args": command_arg_list,
            "secureSequence": state.next_secure_sequence,
            "manifestOwnerPid": context.manifest.get("ownerPid"),
        },
    )
    state.next_secure_sequence += 1
    state.last_auth_time = time.time()
    if command_name == "OBCApp.commController.COMM_SET_ACTIVE":
        invalidate_band_switch_states(context.manifest, current_band=context.band, current_state=state)
    else:
        save_state(context.manifest, context.band, state)
    return result


def command_send(args: argparse.Namespace) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest, enforce_target_gates=True)
    return _print_result(
        send_command_result(
            context,
            command_name=args.command_name,
            command_args=args.command_args,
        )
    )


def connect_api(context: ManualSurfaceContext) -> tuple[StandardPipeline, IntegrationTestAPI]:
    dictionaries = Dictionaries()
    dictionaries.load_dictionaries(str(context.dictionary_path), None, None)
    pipeline = StandardPipeline()
    surface_root = pathlib.Path(context.manifest["surfaceRoot"])
    store_root = surface_root / "pipeline-store" / context.band
    log_root = surface_root / "pipeline-logs" / context.band
    api_root = surface_root / "test-api" / context.band
    store_root.mkdir(parents=True, exist_ok=True)
    log_root.mkdir(parents=True, exist_ok=True)
    api_root.mkdir(parents=True, exist_ok=True)
    pipeline.setup(
        ConfigManager.get_instance(),
        dictionaries,
        str(store_root),
        logging_prefix=str(log_root),
    )
    pipeline.connect(f"127.0.0.1:{int(context.surface['gdsTtsPort'])}")
    time.sleep(1.0)
    return pipeline, IntegrationTestAPI(pipeline, logpath=str(api_root))


def upload_file_result(
    context: ManualSurfaceContext,
    *,
    local_path: str | pathlib.Path,
    destination_leaf: str,
) -> ManualActionResult:
    require_active_state(context)
    source = pathlib.Path(local_path).resolve()
    if not source.is_file():
        raise RuntimeError(f"local file not found or is a directory: {source}")
    destination = normalize_sequence_destination(destination_leaf)
    pipeline, api = connect_api(context)
    try:
        timeout_seconds = max(1, int(math.ceil(float(os.environ.get("MANUAL_FILE_UPLOAD_TIMEOUT_SEC", "30")))))
        api.uplink_file_and_await_completion(
            str(source),
            destination,
            timeout=timeout_seconds,
        )
    finally:
        try:
            api.teardown()
        except Exception:
            pass
        try:
            pipeline.disconnect()
        except Exception:
            pass
    return ManualActionResult(
        status="uploaded",
        env=context.env,
        band=context.band,
        details={
            "source": str(source),
            "destination": destination,
            "sha256": sha256_file(source),
        },
    )


def upload_file(args: argparse.Namespace) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest, enforce_target_gates=True)
    return _print_result(
        upload_file_result(
            context,
            local_path=args.local_path,
            destination_leaf=args.destination_leaf,
        )
    )


def build_sequence_command(action: str, *, sequence_path: str | None = None, context_id: str | None = None, run_mode: str | None = None) -> tuple[str, list[str]]:
    command_name = SEQ_COMMAND_MAP[action]
    command_args: list[str]
    if action in {"validate", "run", "prepare-manual"}:
        if sequence_path is None:
            raise RuntimeError(f"{action} requires sequence_path")
        command_args = [normalize_sequence_destination(sequence_path)]
        if action == "run" and run_mode:
            command_args.append(run_mode)
    elif action in {"start", "step", "cancel"}:
        if context_id is None:
            raise RuntimeError(f"{action} requires context_id")
        command_args = [context_id]
    else:
        raise RuntimeError(f"unsupported sequence action {action}")
    return command_name, command_args


def sequence_command_result(
    context: ManualSurfaceContext,
    *,
    action: str,
    sequence_path: str | None = None,
    context_id: str | None = None,
    run_mode: str | None = None,
) -> ManualActionResult:
    command_name, command_args = build_sequence_command(
        action,
        sequence_path=sequence_path,
        context_id=context_id,
        run_mode=run_mode,
    )
    result = send_command_result(
        context,
        command_name=command_name,
        command_args=command_args,
    )
    result.details["sequenceAction"] = action
    return result


def sequence_command(args: argparse.Namespace, action: str) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest, enforce_target_gates=True)
    return _print_result(
        sequence_command_result(
            context,
            action=action,
            sequence_path=getattr(args, "sequence_path", None),
            context_id=getattr(args, "context_id", None),
            run_mode=getattr(args, "run_mode", None),
        )
    )


def status_result(context: ManualSurfaceContext) -> ManualActionResult:
    state = load_state(context.manifest, context.band)
    payload = {
        "surfaceType": context.manifest["surfaceType"],
        "surfaceRoot": context.manifest["surfaceRoot"],
        "ownerPid": context.manifest.get("ownerPid"),
        "lifecycleState": context.manifest.get("lifecycleState"),
        "secureSession": None,
    }
    if state is not None:
        payload["secureSession"] = {
            "active": not state.invalidated,
            "serviceId": state.service_id,
            "authorityMode": state.authority_mode,
            "nextSecureSequence": state.next_secure_sequence,
            "lastAuthTime": state.last_auth_time,
            "invalidated": state.invalidated,
            "invalidationReason": state.invalidation_reason,
        }
    return ManualActionResult(status="ok", env=context.env, band=context.band, details=payload)


def status(args: argparse.Namespace) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest)
    return _print_result(status_result(context))


def clear_auth_result(context: ManualSurfaceContext) -> ManualActionResult:
    clear_state(context.manifest, context.band)
    return ManualActionResult(status="cleared", env=context.env, band=context.band)


def clear_auth(args: argparse.Namespace) -> int:
    context = resolve_context(env=args.env, band=args.band, manifest_path=args.manifest)
    return _print_result(clear_auth_result(context))


def maybe_run_target_provenance_gate(manifest: dict[str, Any], env_name: str) -> None:
    if env_name != "target":
        return
    baseline = manifest.get("targetBaseline", {})
    obc_target = str(baseline.get("obcSshTarget", "operator@obc.local"))
    obc_service = str(baseline.get("obcCommServiceName", "obc-comm-csp-stack.service"))
    installed_root = str(baseline.get("installedReleaseRoot", "/home/operator/obc-deploy"))
    repo_keystore = default_command_auth_keystore_path(REPO_ROOT)
    repo_sha = sha256_file(repo_keystore)
    current_link = f"{installed_root}/current"
    remote_keystore = f"{current_link}/config/security/command-auth.ini"
    remote_manifest = f"{current_link}/manifest.json"
    remote_sha = ssh_capture(obc_target, f"sha256sum {remote_keystore} | awk '{{print $1}}'").strip()
    manifest_text = ssh_capture(obc_target, f"cat {remote_manifest}")
    manifest_payload = json.loads(manifest_text)
    manifest_keystore_sha = (
        manifest_payload.get("files", {}).get("config/security/command-auth.ini", {}).get("sha256")
    )
    obc_show = systemctl_show(obc_target, obc_service, ("WorkingDirectory", "ActiveState"))
    current_resolved = ssh_capture(obc_target, f"readlink -f {shlex.quote(current_link)}").strip()
    working_dir = obc_show.get("WorkingDirectory") or current_link
    working_resolved = ssh_capture(obc_target, f"readlink -f {shlex.quote(working_dir)}").strip()
    failures: list[str] = []
    if obc_show.get("ActiveState") != "active":
        failures.append("obc-service-not-active")
    if repo_sha != remote_sha:
        failures.append("repo-keystore-sha-mismatch")
    if manifest_keystore_sha != remote_sha:
        failures.append("installed-manifest-keystore-sha-mismatch")
    if current_resolved != working_resolved:
        failures.append("service-working-directory-not-current-release")
    if failures:
        raise RuntimeError("target provenance gate failed: " + ", ".join(failures))


def maybe_run_target_manual_auth_preflight_gate(manifest: dict[str, Any], env_name: str) -> None:
    if env_name != "target":
        return
    baseline = manifest.get("targetBaseline", {})
    preflight = baseline.get("targetAuthPreflight")
    if not isinstance(preflight, dict) or not preflight.get("applied"):
        raise RuntimeError(
            "target manual baseline is missing the secure-auth preflight overrides; rerun start_target_manual_baseline.sh"
        )
    managed_overrides = preflight.get("managedOverrides", [])
    failures: list[str] = []
    for row in managed_overrides:
        target = str(row.get("target", ""))
        service = str(row.get("service", ""))
        expected_env = {str(key): str(value) for key, value in dict(row.get("env", {})).items()}
        observed_env = service_environment(target, service)
        for key, expected_value in expected_env.items():
            if observed_env.get(key, "") != expected_value:
                failures.append(f"{target}:{service}:{key}:{observed_env.get(key, '')}->{expected_value}")
    if failures:
        raise RuntimeError(
            "target manual baseline secure-auth preflight is stale or missing on the remote services; rerun start_target_manual_baseline.sh: "
            + ", ".join(failures)
        )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Manual secure helper for the maintained dual-GDS operator surface.")
    parser.add_argument("--env", required=True, choices=("hosted", "target"))
    parser.add_argument("--band", required=True, choices=("sband", "uhf-backup", "uhf-primary-after-failover"))
    parser.add_argument("--manifest", required=True)
    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    auth_parser = subparsers.add_parser("auth")
    auth_subparsers = auth_parser.add_subparsers(dest="auth_command", required=True)
    auth_subparsers.add_parser("establish")
    auth_subparsers.add_parser("clear")

    status_parser = subparsers.add_parser("status")
    status_parser.set_defaults(handler=status)

    command_parser = subparsers.add_parser("command")
    command_subparsers = command_parser.add_subparsers(dest="command_mode", required=True)
    command_send_parser = command_subparsers.add_parser("send")
    command_send_parser.add_argument("command_name")
    command_send_parser.add_argument("command_args", nargs="*")

    file_parser = subparsers.add_parser("file")
    file_subparsers = file_parser.add_subparsers(dest="file_mode", required=True)
    file_upload_parser = file_subparsers.add_parser("upload")
    file_upload_parser.add_argument("local_path")
    file_upload_parser.add_argument("destination_leaf")

    seq_parser = subparsers.add_parser("seq")
    seq_subparsers = seq_parser.add_subparsers(dest="seq_mode", required=True)
    for name in ("validate", "run", "prepare-manual"):
        seq_action = seq_subparsers.add_parser(name)
        seq_action.add_argument("sequence_path")
        if name == "run":
            seq_action.add_argument("--run-mode", default="NO_WAIT")
    for name in ("start", "step", "cancel"):
        seq_action = seq_subparsers.add_parser(name)
        seq_action.add_argument("context_id")

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.subcommand == "auth":
        if args.auth_command == "establish":
            return establish_auth(args)
        return clear_auth(args)
    if args.subcommand == "status":
        return status(args)
    if args.subcommand == "command":
        return command_send(args)
    if args.subcommand == "file":
        return upload_file(args)
    if args.subcommand == "seq":
        return sequence_command(args, args.seq_mode)
    raise RuntimeError(f"unsupported subcommand: {args.subcommand}")


if __name__ == "__main__":
    raise SystemExit(main())
