#!/usr/bin/env python3
from __future__ import annotations

import importlib
import json
import os
import pathlib
import struct
import sys
import tempfile
import threading
import time
import unittest
import subprocess
import zlib
from dataclasses import replace
from unittest import mock
from types import SimpleNamespace

from manual_ops import manual_secure_ops as secure_ops
from mission_console.gateway import catalog as command_catalog
from mission_console.gateway.actions import (
    GatewayActions,
    OperationRequest,
    OperationResult,
    _find_command_completion,
    _run_bounded_cli_search,
    _run_upload_file_in_subprocess,
)
from mission_console.gateway.beacon import BeaconManager
from mission_console.gateway.catalog import load_command_catalog
from mission_console.gateway.listeners import ListenerManager
from mission_console.gateway.locks import BandLockPool
from mission_console.gateway.packet_lab import (
    PacketLabService,
    _mark_channel_fresh_after_marker,
    _merge_channel_snapshots,
    parse_packet_summary,
    parse_transport_packets,
)
from mission_console.gateway.parsers import (
    CHANNEL_REFRESH_COMMANDS,
    EVENT_BASED_COMMANDS,
    LIVE_TREND_GROUPS,
    parse_channel_line,
    parse_event_line,
    parse_structured_event,
    structured_event_complete,
)
from mission_console.gateway.registry import BandSurface, SurfaceContext, SurfaceRegistry, discover_context
from mission_console.gateway.sequence_authoring import SequenceAuthoringService, render_sequence_source
from mission_console.gateway.snapshots import SnapshotStore
from mission_console.probe_client import _native_capture_override
from secure_link_auth_lib import COMMAND_DESCRIPTOR, build_secure_command_v2_packet
from decode_beacon_v1 import BEACON_V1_FORMAT, BEACON_V1_WIRE_SIZE


class MissionConsolePhase1Test(unittest.TestCase):
    def _sample_beacon_frame(self, *, sequence: int = 7, seconds: int = 1234) -> bytes:
        values = [
            0x3143424F,
            2,
            0,
            sequence,
            1,
            0,
            seconds,
            500000,
            3,
            1,
            2,
            99,
            0x10,
            0x20,
            0x30,
            806,
            801,
            34,
            2950,
            250000,
            4,
            1,
            0,
            0,
            12,
            3,
            44,
            5,
            77,
            88,
            99,
            111,
            0,
        ]
        frame = bytearray(struct.pack(BEACON_V1_FORMAT, *values))
        crc = zlib.crc32(frame[:-4]) & 0xFFFFFFFF
        struct.pack_into("<I", frame, len(frame) - 4, crc)
        return bytes(frame)

    def _sample_dictionary_payload(self) -> dict[str, object]:
        return {
            "commands": [
                {
                    "name": "OBCApp.modeManager.MODE_SET",
                    "commandKind": "sync",
                    "opcode": 0x10030000,
                    "formalParams": [
                        {
                            "name": "mode",
                            "type": {"name": "OBC.SatMode", "kind": "qualifiedIdentifier"},
                            "ref": False,
                        }
                    ],
                },
                {
                    "name": "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_RAW",
                    "commandKind": "sync",
                    "opcode": 0x10090000,
                    "formalParams": [
                        {
                            "name": "captureIndex",
                            "type": {"name": "U8", "kind": "integer", "size": 8, "signed": False},
                            "ref": False,
                        },
                        {
                            "name": "tag",
                            "type": {"name": "string", "kind": "string", "size": 64},
                            "ref": False,
                        },
                    ],
                },
                {
                    "name": "OBCApp.sequenceAdmissionController.SEQ_RUN",
                    "commandKind": "sync",
                    "opcode": 0x100A0000,
                    "formalParams": [
                        {
                            "name": "fileName",
                            "type": {"name": "string", "kind": "string", "size": 128},
                            "ref": False,
                        },
                        {
                            "name": "waitMode",
                            "type": {"name": "OBC.WaitModeAlias", "kind": "qualifiedIdentifier"},
                            "ref": False,
                        },
                    ],
                },
                {
                    "name": "CdhCore.cmdDisp.CMD_NO_OP_STRING",
                    "commandKind": "sync",
                    "opcode": 0x10010000,
                    "formalParams": [
                        {
                            "name": "arg1",
                            "type": {"name": "string", "kind": "string", "size": 80},
                            "ref": False,
                        }
                    ],
                },
                {
                    "name": "FileHandling.prmDb.PRM_LOAD_FILE",
                    "commandKind": "sync",
                    "opcode": 0x10020000,
                    "formalParams": [
                        {
                            "name": "fileName",
                            "type": {"name": "string", "kind": "string", "size": 128},
                            "ref": False,
                        },
                        {
                            "name": "merge",
                            "type": {"name": "Svc.PrmDb.Merge", "kind": "qualifiedIdentifier"},
                            "ref": False,
                        },
                    ],
                },
                {
                    "name": "OBCApp.radioController.RADIO_GET_STATUS",
                    "commandKind": "sync",
                    "opcode": 0x100B0000,
                    "formalParams": [],
                },
            ],
            "typeDefinitions": [
                {
                    "kind": "enum",
                    "qualifiedName": "OBC.SatMode",
                    "enumeratedConstants": [
                        {"name": "SAFE", "value": 0},
                        {"name": "IDLE", "value": 1},
                    ],
                    "default": "OBC.SatMode.IDLE",
                },
                {
                    "kind": "alias",
                    "qualifiedName": "OBC.WaitModeAlias",
                    "type": {"name": "Svc.WaitMode", "kind": "qualifiedIdentifier"},
                },
                {
                    "kind": "enum",
                    "qualifiedName": "Svc.WaitMode",
                    "enumeratedConstants": [
                        {"name": "NO_WAIT", "value": 0},
                        {"name": "WAIT", "value": 1},
                    ],
                    "default": "Svc.WaitMode.NO_WAIT",
                },
                {
                    "kind": "enum",
                    "qualifiedName": "Svc.PrmDb.Merge",
                    "enumeratedConstants": [
                        {"name": "MERGE", "value": 0},
                        {"name": "RESET", "value": 1},
                    ],
                    "default": "Svc.PrmDb.Merge.MERGE",
                },
            ],
        }

    def _write_sample_dictionary(self, root: pathlib.Path) -> pathlib.Path:
        path = root / "AppTopologyDictionary.json"
        path.write_text(json.dumps(self._sample_dictionary_payload()), encoding="utf-8")
        return path

    def test_validate_state_invalidates_owner_change_and_timeout(self) -> None:
        manifest = {"ownerPid": 44}
        state = secure_ops.SecureSessionState(
            service_id=1,
            active_band="sband",
            next_secure_sequence=3,
            last_auth_time=time.time() - 999,
            manifest_path="/tmp/example.json",
            authority_mode="sband-primary",
            session_key_hex=("11" * 32),
            manifest_owner_pid=33,
        )
        validated = secure_ops.validate_state(manifest, "sband", state)
        self.assertTrue(validated.invalidated)
        self.assertEqual(validated.invalidation_reason, "session-timeout")

    def test_command_catalog_merges_curated_overlay(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = self._write_sample_dictionary(pathlib.Path(temp_dir))
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        mode_set = commands["OBCApp.modeManager.MODE_SET"]
        self.assertEqual(mode_set["label"], "Set Mission Mode")
        self.assertEqual(mode_set["group"], "OBC")
        self.assertTrue(mode_set["visibleByDefault"])
        self.assertEqual(mode_set["source"], "curated")
        payload_capture = commands["OBCApp.payloadOpsController.PAYLOAD_CAPTURE_RAW"]
        self.assertEqual(payload_capture["label"], "Capture Raw Payload Frame")
        self.assertEqual(payload_capture["args"][0]["placeholder"], "0-255 capture slot")
        self.assertEqual(payload_capture["args"][1]["placeholder"], "Short artifact tag")

    def test_command_catalog_resolves_enum_and_alias_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = self._write_sample_dictionary(pathlib.Path(temp_dir))
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        mode_arg = commands["OBCApp.modeManager.MODE_SET"]["args"][0]
        self.assertEqual(mode_arg["inputKind"], "enum")
        self.assertEqual(mode_arg["defaultValue"], "IDLE")
        self.assertEqual([entry["value"] for entry in mode_arg["options"]], ["SAFE", "IDLE"])
        run_args = commands["OBCApp.sequenceAdmissionController.SEQ_RUN"]["args"]
        wait_mode = [entry for entry in run_args if entry["name"] == "waitMode"][0]
        self.assertEqual(wait_mode["inputKind"], "enum")
        self.assertEqual(wait_mode["defaultValue"], "NO_WAIT")
        self.assertEqual([entry["value"] for entry in wait_mode["options"]], ["NO_WAIT", "WAIT"])

    def test_command_catalog_visibility_keeps_engineering_groups_hidden_by_default(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = self._write_sample_dictionary(pathlib.Path(temp_dir))
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        self.assertTrue(commands["OBCApp.modeManager.MODE_SET"]["visibleByDefault"])
        self.assertFalse(commands["CdhCore.cmdDisp.CMD_NO_OP_STRING"]["visibleByDefault"])
        self.assertEqual(commands["CdhCore.cmdDisp.CMD_NO_OP_STRING"]["group"], "Framework / CDH")
        self.assertEqual(commands["CdhCore.cmdDisp.CMD_NO_OP_STRING"]["source"], "dictionary")
        self.assertFalse(commands["FileHandling.prmDb.PRM_LOAD_FILE"]["visibleByDefault"])
        self.assertEqual(commands["FileHandling.prmDb.PRM_LOAD_FILE"]["group"], "Parameter DB")
        self.assertEqual(commands["OBCApp.modeManager.MODE_SET"]["source"], "curated")

    def test_command_catalog_omits_radio_controller_from_ui(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = self._write_sample_dictionary(pathlib.Path(temp_dir))
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        self.assertNotIn("OBCApp.radioController.RADIO_GET_STATUS", commands)

    def test_command_catalog_missing_dictionary_returns_empty_catalog(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            missing_path = pathlib.Path(temp_dir) / "missing-dictionary.json"
            catalog = load_command_catalog(str(missing_path))
        self.assertEqual(catalog["dictionaryPath"], str(missing_path.resolve()))
        self.assertEqual(catalog["commands"], [])

    def test_command_catalog_parses_hex_opcode_and_boolean_default(self) -> None:
        payload = self._sample_dictionary_payload()
        payload["commands"].append(
            {
                "name": "OBCApp.ttcPassManager.TTC_SET_POLICY",
                "commandKind": "sync",
                "opcode": "0x100C0000",
                "formalParams": [
                    {
                        "name": "policyEnabled",
                        "type": {"name": "bool", "kind": "bool", "default": True},
                        "ref": False,
                    }
                ],
            }
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = pathlib.Path(temp_dir) / "AppTopologyDictionary.json"
            dictionary_path.write_text(json.dumps(payload), encoding="utf-8")
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        policy_command = commands["OBCApp.ttcPassManager.TTC_SET_POLICY"]
        self.assertEqual(policy_command["opcode"], 0x100C0000)
        self.assertEqual(policy_command["args"][0]["defaultValue"], "true")

    def test_command_catalog_resolve_type_stops_on_circular_alias(self) -> None:
        payload = self._sample_dictionary_payload()
        payload["commands"].append(
            {
                "name": "OBCApp.modeManager.MODE_LOOP",
                "commandKind": "sync",
                "opcode": 0x100D0000,
                "formalParams": [
                    {
                        "name": "loopMode",
                        "type": {"name": "LoopAliasA", "kind": "qualifiedIdentifier"},
                        "ref": False,
                    }
                ],
            }
        )
        payload["typeDefinitions"].extend(
            [
                {
                    "kind": "alias",
                    "qualifiedName": "LoopAliasA",
                    "type": {"name": "LoopAliasB", "kind": "qualifiedIdentifier"},
                },
                {
                    "kind": "alias",
                    "qualifiedName": "LoopAliasB",
                    "type": {"name": "LoopAliasA", "kind": "qualifiedIdentifier"},
                },
            ]
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            dictionary_path = pathlib.Path(temp_dir) / "AppTopologyDictionary.json"
            dictionary_path.write_text(json.dumps(payload), encoding="utf-8")
            catalog = load_command_catalog(dictionary_path)
        commands = {entry["name"]: entry for entry in catalog["commands"]}
        loop_mode = commands["OBCApp.modeManager.MODE_LOOP"]["args"][0]
        self.assertEqual(loop_mode["inputKind"], "raw")
        self.assertEqual(loop_mode["typeName"], "LoopAliasA")

    def test_command_catalog_cache_keeps_multiple_dictionary_entries(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            first_root = root / "first"
            first_root.mkdir(parents=True)
            first_path = self._write_sample_dictionary(first_root)
            second_payload = self._sample_dictionary_payload()
            second_payload["commands"][0]["opcode"] = 0x10030001
            second_root = root / "second"
            second_root.mkdir(parents=True)
            second_path = second_root / "AppTopologyDictionary.json"
            second_path.write_text(json.dumps(second_payload), encoding="utf-8")
            with command_catalog._CACHE_LOCK:
                command_catalog._CATALOG_CACHE.clear()
            load_command_catalog(first_path)
            load_command_catalog(second_path)
            with command_catalog._CACHE_LOCK:
                cache_keys = set(command_catalog._CATALOG_CACHE.keys())
                command_catalog._CATALOG_CACHE.clear()
        self.assertEqual(len(cache_keys), 2)

    def test_build_sequence_command(self) -> None:
        command_name, args = secure_ops.build_sequence_command("run", sequence_path="demo.seq", run_mode="NO_WAIT")
        self.assertEqual(command_name, "OBCApp.sequenceAdmissionController.SEQ_RUN")
        self.assertEqual(args, [".sequence-staging/demo.seq", "NO_WAIT"])

    def test_comm_set_active_invalidates_all_cached_band_states(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            secure_state_root = runtime_root / "secure-state"
            secure_state_root.mkdir(parents=True)
            manifest = {
                "surfaceRoot": str(runtime_root),
                "ownerPid": 7,
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"]},
                    "uhf": {"canonicalBands": ["uhf-backup", "uhf-primary-after-failover"]},
                },
            }
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest=manifest,
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=secure_state_root / "sband.json",
            )
            secure_ops.save_state(
                manifest,
                "sband",
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=3,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="sband-primary",
                    session_key_hex=("11" * 32),
                    manifest_owner_pid=7,
                ),
            )
            secure_ops.save_state(
                manifest,
                "uhf-backup",
                secure_ops.SecureSessionState(
                    service_id=2,
                    active_band="uhf-backup",
                    next_secure_sequence=9,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="uhf-backup",
                    session_key_hex=("22" * 32),
                    manifest_owner_pid=7,
                ),
            )
            secure_ops.save_state(
                manifest,
                "uhf-primary-after-failover",
                secure_ops.SecureSessionState(
                    service_id=2,
                    active_band="uhf-primary-after-failover",
                    next_secure_sequence=11,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="uhf-primary",
                    session_key_hex=("33" * 32),
                    manifest_owner_pid=7,
                ),
            )
            with (
                mock.patch.object(secure_ops, "load_command_context", return_value=(object(), object())),
                mock.patch.object(secure_ops, "require_active_state", return_value=secure_ops.load_state(manifest, "sband")),
                mock.patch.object(secure_ops, "encode_inner_command", return_value=b"\x00\x00\x10\x03\x80\x00"),
                mock.patch.object(secure_ops, "build_secure_command_v2_packet", return_value=b"packet"),
                mock.patch.object(secure_ops, "send_tts_raw_packet", return_value=None),
            ):
                secure_ops.send_command_result(
                    context,
                    command_name="OBCApp.commController.COMM_SET_ACTIVE",
                    command_args=["COMM", "UHF"],
                )
            for band in ("sband", "uhf-backup", "uhf-primary-after-failover"):
                state = secure_ops.load_state(manifest, band)
                assert state is not None
                self.assertTrue(state.invalidated)
                self.assertEqual(state.invalidation_reason, "band-switch-command-sent")

    def test_invalidate_sibling_service_states_invalidates_shared_uhf_alias(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            secure_state_root = runtime_root / "secure-state"
            secure_state_root.mkdir(parents=True)
            manifest = {
                "surfaceRoot": str(runtime_root),
                "ownerPid": 7,
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"]},
                    "uhf": {"canonicalBands": ["uhf-backup", "uhf-primary-after-failover"]},
                },
            }
            secure_ops.save_state(
                manifest,
                "uhf-backup",
                secure_ops.SecureSessionState(
                    service_id=2,
                    active_band="uhf-backup",
                    next_secure_sequence=9,
                    last_auth_time=time.time(),
                    manifest_path=str(runtime_root / "manifest.json"),
                    authority_mode="uhf-backup",
                    session_key_hex=("22" * 32),
                    manifest_owner_pid=7,
                ),
            )
            secure_ops.save_state(
                manifest,
                "uhf-primary-after-failover",
                secure_ops.SecureSessionState(
                    service_id=2,
                    active_band="uhf-primary-after-failover",
                    next_secure_sequence=11,
                    last_auth_time=time.time(),
                    manifest_path=str(runtime_root / "manifest.json"),
                    authority_mode="uhf-primary-after-failover",
                    session_key_hex=("33" * 32),
                    manifest_owner_pid=7,
                ),
            )
            secure_ops.invalidate_sibling_service_states(
                manifest,
                current_band="uhf-primary-after-failover",
                current_service_id=secure_ops.SERVICE_ID_UHF,
            )
            sibling = secure_ops.load_state(manifest, "uhf-backup")
            current = secure_ops.load_state(manifest, "uhf-primary-after-failover")
            assert sibling is not None
            assert current is not None
            self.assertTrue(sibling.invalidated)
            self.assertEqual(sibling.invalidation_reason, "sibling-band-reauthenticated")
            self.assertFalse(current.invalidated)

    def test_registry_loads_canonical_bands(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            payload = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(hosted_root / "dict.xml"),
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"], "gdsTtsPort": 50151, "captures": {}, "logs": {}, "southbound": {}},
                    "uhf": {
                        "canonicalBands": ["uhf-backup", "uhf-primary-after-failover"],
                        "gdsTtsPort": 50161,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    },
                },
            }
            (hosted_root / "dict.xml").write_text("<dict />", encoding="utf-8")
            (hosted_root / "manifest.json").write_text(json.dumps(payload), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(payload), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
            ):
                context = discover_context("hosted-manual-dual-gds")
            self.assertIn("sband", context.bands)
            self.assertIn("uhf-backup", context.bands)
            self.assertIn("uhf-primary-after-failover", context.bands)

    def test_registry_exposes_beacon_capability(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            payload = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(hosted_root / "dict.xml"),
                "operatorSurfaces": {
                    "uhf": {
                        "canonicalBands": ["uhf-backup", "uhf-primary-after-failover"],
                        "gdsTtsPort": 50161,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                        "beacon": {
                            "supported": True,
                            "sourceKind": "hosted-pty-side-channel",
                            "sourceBand": "uhf-backup",
                            "capturePath": str(hosted_root / "beacons" / "uhf.bin"),
                            "frameSize": 108,
                        },
                    },
                },
            }
            (hosted_root / "dict.xml").write_text("<dict />", encoding="utf-8")
            (hosted_root / "manifest.json").write_text(json.dumps(payload), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(payload), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
            ):
                context = discover_context("hosted-manual-dual-gds")
            self.assertEqual(context.bands["uhf-backup"].beacon["sourceKind"], "hosted-pty-side-channel")

    def test_snapshot_store_records_bounded_beacon_history(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            store = SnapshotStore(pathlib.Path(temp_dir), beacon_history_size=2)
            first = {
                "contextId": "hosted-manual-dual-gds",
                "sequence": 1,
                "capture": {"frameCount": 1},
                "decode": {"status": "ok"},
            }
            second = {
                "contextId": "hosted-manual-dual-gds",
                "sequence": 2,
                "capture": {"frameCount": 2},
                "decode": {"status": "ok"},
            }
            third = {
                "contextId": "hosted-manual-dual-gds",
                "sequence": 3,
                "capture": {"frameCount": 3},
                "decode": {"status": "ok"},
            }
            store.set_beacon_snapshot("hosted-manual-dual-gds", first)
            store.set_beacon_snapshot("hosted-manual-dual-gds", second)
            store.set_beacon_snapshot("hosted-manual-dual-gds", third)
            latest = store.beacon_latest("hosted-manual-dual-gds")
            history = store.beacon_history("hosted-manual-dual-gds", limit=10)
            assert latest is not None
            self.assertEqual(latest["sequence"], 3)
            self.assertEqual([entry["sequence"] for entry in history], [2, 3])

    def test_beacon_manager_decodes_latest_frame(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            capture_path = root / "uhf-beacon.bin"
            capture_path.write_bytes(self._sample_beacon_frame(sequence=9, seconds=4321))
            store = SnapshotStore(root / "runtime")
            manager = BeaconManager(store)
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=root,
                manifest_path=root / "manifest.json",
                status_path=root / "status.json",
                manifest={},
                status={},
                lifecycle_state="running",
                owner_pid=1,
                dictionary_path=None,
                bands={
                    "uhf-backup": BandSurface(
                        band="uhf-backup",
                        manifest_key="uhf",
                        surface_root=root,
                        gds_tts_port=50161,
                        dictionary_path=root / "dict.json",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        beacon={
                            "supported": True,
                            "sourceKind": "hosted-pty-side-channel",
                            "sourceBand": "uhf-backup",
                            "capturePath": str(capture_path),
                            "frameSize": BEACON_V1_WIRE_SIZE,
                        },
                        secure_state=None,
                        secure_state_path=root / "secure-state.json",
                        owner_pid=1,
                    )
                },
            )
            manager.poll_contexts({context.context_id: context})
            latest = store.beacon_latest(context.context_id)
            assert latest is not None
            self.assertTrue(latest["available"])
            self.assertEqual(latest["sequence"], 9)
            self.assertEqual(latest["decoded"]["time"]["seconds"], 4321)
            manager.poll_contexts(
                {context.context_id: replace(context, lifecycle_state="stopped", owner_alive=False)}
            )
            stopped = store.beacon_latest(context.context_id)
            assert stopped is not None
            self.assertFalse(stopped["available"])
            self.assertEqual(stopped["reason"], "surface-not-running")
            self.assertEqual(stopped["decode"]["status"], "unavailable")

    def test_beacon_manager_resynchronizes_after_partial_leading_frame(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            capture_path = root / "uhf-beacon.bin"
            capture_path.write_bytes(
                b"partial" + self._sample_beacon_frame(sequence=8) + self._sample_beacon_frame(sequence=9)
            )
            store = SnapshotStore(root / "runtime")
            manager = BeaconManager(store)
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=root,
                manifest_path=root / "manifest.json",
                status_path=root / "status.json",
                manifest={},
                status={},
                lifecycle_state="running",
                owner_pid=1,
                dictionary_path=None,
                bands={
                    "uhf-backup": BandSurface(
                        band="uhf-backup",
                        manifest_key="uhf",
                        surface_root=root,
                        gds_tts_port=50161,
                        dictionary_path=root / "dict.json",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        beacon={
                            "supported": True,
                            "sourceKind": "hosted-pty-side-channel",
                            "sourceBand": "uhf-backup",
                            "capturePath": str(capture_path),
                            "frameSize": BEACON_V1_WIRE_SIZE,
                        },
                        secure_state=None,
                        secure_state_path=root / "secure-state.json",
                        owner_pid=1,
                    )
                },
            )
            manager.poll_contexts({context.context_id: context})
            latest = store.beacon_latest(context.context_id)
            assert latest is not None
            self.assertTrue(latest["available"])
            self.assertEqual(latest["sequence"], 9)

    def test_beacon_manager_preserves_observation_time_for_unchanged_frame(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            capture_path = root / "uhf-beacon.bin"
            capture_path.write_bytes(self._sample_beacon_frame(sequence=9))
            store = SnapshotStore(root / "runtime")
            manager = BeaconManager(store)
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=root,
                manifest_path=root / "manifest.json",
                status_path=root / "status.json",
                manifest={},
                status={},
                lifecycle_state="running",
                owner_pid=1,
                dictionary_path=None,
                bands={
                    "uhf-backup": BandSurface(
                        band="uhf-backup",
                        manifest_key="uhf",
                        surface_root=root,
                        gds_tts_port=50161,
                        dictionary_path=root / "dict.json",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        beacon={
                            "supported": True,
                            "sourceKind": "hosted-pty-side-channel",
                            "sourceBand": "uhf-backup",
                            "capturePath": str(capture_path),
                            "frameSize": BEACON_V1_WIRE_SIZE,
                        },
                        secure_state=None,
                        secure_state_path=root / "secure-state.json",
                        owner_pid=1,
                    )
                },
            )
            with mock.patch(
                "mission_console.gateway.beacon._utc_now",
                side_effect=["2026-07-13T00:00:00Z", "2026-07-13T00:01:00Z"],
            ):
                manager.poll_contexts({context.context_id: context})
                manager.poll_contexts({context.context_id: context})
            latest = store.beacon_latest(context.context_id)
            assert latest is not None
            self.assertEqual(latest["lastObservedAt"], "2026-07-13T00:00:00Z")
            self.assertEqual(len(store.beacon_history(context.context_id, limit=10)), 1)

    def test_beacon_manager_rejects_short_frame(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = pathlib.Path(temp_dir)
            capture_path = root / "uhf-beacon.bin"
            capture_path.write_bytes(b"\x00" * 16)
            store = SnapshotStore(root / "runtime")
            manager = BeaconManager(store)
            context = SurfaceContext(
                context_id="target-manual-ground-dual-gds",
                surface_type="target-manual-ground-dual-gds",
                env_name="target",
                root=root,
                manifest_path=root / "manifest.json",
                status_path=root / "status.json",
                manifest={},
                status={},
                lifecycle_state="running",
                owner_pid=1,
                dictionary_path=None,
                bands={
                    "uhf-backup": BandSurface(
                        band="uhf-backup",
                        manifest_key="uhf",
                        surface_root=root,
                        gds_tts_port=50161,
                        dictionary_path=root / "dict.json",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        beacon={
                            "supported": True,
                            "sourceKind": "target-remote-sidecar",
                            "sourceBand": "uhf-backup",
                            "capturePath": str(capture_path),
                            "frameSize": BEACON_V1_WIRE_SIZE,
                        },
                        secure_state=None,
                        secure_state_path=root / "secure-state.json",
                        owner_pid=1,
                    )
                },
            )
            manager.poll_contexts({context.context_id: context})
            latest = store.beacon_latest(context.context_id)
            assert latest is not None
            self.assertFalse(latest["available"])
            self.assertEqual(latest["decode"]["status"], "short-frame")

    def test_discover_context_redacts_secure_session_key_from_band_state(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            secure_root = hosted_root / "secure-state"
            secure_root.mkdir(parents=True)
            payload = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(hosted_root / "dict.xml"),
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"], "gdsTtsPort": 50151, "captures": {}, "logs": {}, "southbound": {}},
                },
            }
            (hosted_root / "dict.xml").write_text("<dict />", encoding="utf-8")
            (hosted_root / "manifest.json").write_text(json.dumps(payload), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(payload), encoding="utf-8")
            (secure_root / "sband.json").write_text(
                json.dumps(
                    {
                        "serviceId": 1,
                        "activeBand": "sband",
                        "nextSecureSequence": 7,
                        "lastAuthTime": time.time(),
                        "manifestPath": str(hosted_root / "manifest.json"),
                        "authorityMode": "sband-primary",
                        "sessionKeyHex": "aa" * 32,
                        "manifestOwnerPid": 123,
                        "invalidated": False,
                        "invalidationReason": None,
                    }
                ),
                encoding="utf-8",
            )
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
            ):
                context = discover_context("hosted-manual-dual-gds")
            secure_state = context.bands["sband"].secure_state
            assert secure_state is not None
            self.assertNotIn("sessionKeyHex", secure_state)
            self.assertTrue(secure_state["active"])
            self.assertEqual(secure_state["nextSecureSequence"], 7)

    def test_discover_context_marks_restarted_surface_secure_state_invalidated(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            secure_root = hosted_root / "secure-state"
            secure_root.mkdir(parents=True)
            payload = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 456,
                "lifecycleState": "running",
                "dictionaryPath": str(hosted_root / "dict.xml"),
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"], "gdsTtsPort": 50151, "captures": {}, "logs": {}, "southbound": {}},
                },
            }
            (hosted_root / "dict.xml").write_text("<dict />", encoding="utf-8")
            (hosted_root / "manifest.json").write_text(json.dumps(payload), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(payload), encoding="utf-8")
            (secure_root / "sband.json").write_text(
                json.dumps(
                    {
                        "serviceId": 1,
                        "activeBand": "sband",
                        "nextSecureSequence": 4,
                        "lastAuthTime": time.time(),
                        "manifestPath": str(hosted_root / "manifest.json"),
                        "authorityMode": "sband-primary",
                        "sessionKeyHex": "bb" * 32,
                        "manifestOwnerPid": 123,
                        "invalidated": False,
                        "invalidationReason": None,
                    }
                ),
                encoding="utf-8",
            )
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
            ):
                context = discover_context("hosted-manual-dual-gds")
            secure_state = context.bands["sband"].secure_state
            assert secure_state is not None
            self.assertTrue(secure_state["invalidated"])
            self.assertEqual(secure_state["invalidationReason"], "surface-owner-changed")
            self.assertFalse(secure_state["active"])

    def test_discover_context_marks_dead_owner_not_running(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            payload = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 456,
                "lifecycleState": "running",
                "dictionaryPath": str(hosted_root / "dict.xml"),
                "operatorSurfaces": {
                    "sband": {"canonicalBands": ["sband"], "gdsTtsPort": 50151, "captures": {}, "logs": {}, "southbound": {}},
                },
            }
            (hosted_root / "dict.xml").write_text("<dict />", encoding="utf-8")
            (hosted_root / "manifest.json").write_text(json.dumps(payload), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(payload), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", side_effect=ProcessLookupError),
            ):
                context = discover_context("hosted-manual-dual-gds")
            self.assertFalse(context.is_running)
            self.assertEqual(context.lifecycle_state, "stale-owner-dead")
            self.assertFalse(context.owner_alive)
            self.assertIn("owner-pid-dead", context.errors)

    def test_parse_event_and_channel_lines(self) -> None:
        event = parse_event_line(
            "2026-05-11T18:49:43.658849: OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE"
        )
        self.assertIsNotNone(event)
        self.assertEqual(event.event_name, "SYS_MODE_CHANGE")
        channel = parse_channel_line("2026-05-09T19:10:14.014608: OBCApp.modeManager.SYS_MODE,268632064,IDLE")
        self.assertIsNotNone(channel)
        self.assertEqual(channel.channel_name, "SYS_MODE")
        current_event = parse_event_line(
            "2026-06-20T04:31:10.316946: OBCApp.secureLinkAuthorizer.SECURE_AUTH_ESTABLISHED (268456706) (2(0)-1781901070:316946) EventSeverity.ACTIVITY_HI : Secure auth established ingress 0 service 1"
        )
        self.assertIsNotNone(current_event)
        self.assertEqual(current_event.event_name, "SECURE_AUTH_ESTABLISHED")
        native_event = parse_event_line(
            "2026-06-20T15:27:38.252940,(2(0)-1781940458:252940),OBCApp.secureLinkAuthorizer.SECURE_AUTH_ESTABLISHED,268456706,EventSeverity.ACTIVITY_HI,Secure auth established ingress 0 service 1"
        )
        self.assertIsNotNone(native_event)
        self.assertEqual(native_event.event_name, "SECURE_AUTH_ESTABLISHED")
        current_channel = parse_channel_line(
            "2026-06-20T04:31:10.076324: OBCApp.watchdogSupervisor.SYS_MEM_RSS_MB (268636161) (2(0)-1781901070:76324) 9.734375"
        )
        self.assertIsNotNone(current_channel)
        self.assertEqual(current_channel.channel_name, "SYS_MEM_RSS_MB")
        self.assertEqual(current_channel.value, "9.734375")
        native_channel = parse_channel_line(
            "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,8"
        )
        self.assertIsNotNone(native_channel)
        self.assertEqual(native_channel.channel_name, "SYS_UPTIME_SEC")
        self.assertEqual(native_channel.value, "8")
        payload = parse_structured_event(
            "BOOT_RECOVERY_STATUS",
            "reset RECOVERY_ADCS_FDIR (3) bootCount 2 consecutive 0 safeFallback 0 source ADCS_POLL_TRANSPORT (6) level R3_RESET_SUBSYSTEM_INTERFACE (3)",
        )
        self.assertEqual(payload["bootCount"], 2)
        self.assertEqual(payload["source"], "ADCS_POLL_TRANSPORT")

    def test_dashboard_cards_surface_operator_first_sections(self) -> None:
        self.assertEqual(
            CHANNEL_REFRESH_COMMANDS["OBCApp.ttcPassManager.TTC_GET_STATUS"],
            [
                "TTC_POLICY_ENABLED",
                "TTC_POLICY_WINDOW_CONFIGURED",
                "TTC_POLICY_WINDOW_ACTIVE",
                "TTC_POLICY_GPS_TIME_VALID",
                "TTC_POLICY_TTC_ACTIVE",
                "TTC_POLICY_LOSS_TIMEOUT_SEC",
                "TTC_POLICY_WINDOW_START_UNIX_SEC",
                "TTC_POLICY_WINDOW_END_UNIX_SEC",
                "TTC_POLICY_CURRENT_GPS_UNIX_SEC",
                "TTC_POLICY_LOSS_TIMER_SEC",
                "TTC_POLICY_LAST_ENTRY_REASON",
                "TTC_POLICY_LAST_EXIT_REASON",
            ],
        )
        self.assertEqual(
            CHANNEL_REFRESH_COMMANDS["OBCApp.gpsBridge.GPS_GET_STATE"],
            ["GPS_SOURCE_MODE", "GPS_FIX_VALID", "GPS_LAT_DEG"],
        )
        self.assertEqual(
            CHANNEL_REFRESH_COMMANDS["OBCApp.epsBridge.EPS_GET_STATUS"],
            [
                "EPS_VBAT",
                "EPS_IBAT",
                "EPS_SOC",
                "EPS_TEMP_BAT",
                "EPS_PDU_STATUS",
                "EPS_HEATER_ENABLED",
                "EPS_OVERCURRENT_FLAGS",
                "EPS_VSOLAR",
                "EPS_ISOLAR",
                "EPS_POWER_OUT",
            ],
        )
        self.assertEqual(
            CHANNEL_REFRESH_COMMANDS["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"],
            ["ADCS_MODE", "ADCS_Q0", "ADCS_OMEGA_X"],
        )
        self.assertEqual(
            CHANNEL_REFRESH_COMMANDS["OBCApp.storageHealthBridge.STORAGE_GET_STATUS"],
            ["STORAGE_WARNING_ACTIVE", "STORAGE_DATA_PRODUCTS_QUOTA_STATUS", "STORAGE_DATA_PRODUCTS_FILE_COUNT"],
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            samples = [
                "2026-06-29T12:00:00.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE",
                "2026-06-29T12:00:03.000000: OBCApp.adcsBridge.ADCS_MODE,19727,DETUMBLE",
                "2026-06-29T12:00:03.100000: OBCApp.adcsBridge.ADCS_Q0,19728,0.91",
                "2026-06-29T12:00:03.200000: OBCApp.adcsBridge.ADCS_OMEGA_X,19729,0.01",
                "2026-06-29T12:00:04.000000: OBCApp.gpsBridge.GPS_SOURCE_MODE,20823,LIVE_UART",
                "2026-06-29T12:00:04.100000: OBCApp.epsBridge.EPS_VBAT,19601,7.5",
                "2026-06-29T12:00:04.200000: OBCApp.epsBridge.EPS_IBAT,19602,0.3",
                "2026-06-29T12:00:04.300000: OBCApp.epsBridge.EPS_SOC,19603,81",
                "2026-06-29T12:00:04.400000: OBCApp.epsBridge.EPS_TEMP_BAT,19604,28",
                "2026-06-29T12:00:05.000000: OBCApp.epsBridge.EPS_PDU_STATUS,19606,0x0F",
                "2026-06-29T12:00:07.000000: OBCApp.modeManager.SYS_UPTIME_SEC,268632065,42",
            ]
            for line in samples:
                parsed = parse_channel_line(line)
                assert parsed is not None
                snapshots.update_channel("hosted-manual-dual-gds", "sband", parsed)
            latest_mode = parse_channel_line("2026-06-29T12:00:08.000000: OBCApp.modeManager.SYS_MODE,268632064,SCIENCE")
            assert latest_mode is not None
            snapshots.update_channel("hosted-manual-dual-gds", "uhf-backup", latest_mode)
            cards = snapshots.dashboard_cards("hosted-manual-dual-gds", "sband", None)
            self.assertIn("Satellite Status", cards)
            self.assertEqual(cards["Satellite Status"]["SYS_MODE"]["value"], "SCIENCE")
            self.assertEqual(cards["Satellite Status"]["SYS_MODE"]["sourceBand"], "uhf-backup")
            self.assertEqual(cards["Satellite Status"]["GPS_SOURCE_MODE"]["value"], "LIVE_UART")
            self.assertEqual(cards["Satellite Status"]["SYS_MODE"]["observationSource"], "live-update")
            self.assertEqual(cards["EPS Snapshot"]["EPS_VBAT"]["value"], "7.5")
            self.assertEqual(cards["ADCS Snapshot"]["ADCS_MODE"]["value"], "DETUMBLE")
            self.assertEqual(cards["ADCS Snapshot"]["ADCS_Q0"]["value"], "0.91")
            self.assertEqual(cards["Mission State"]["SYS_UPTIME_SEC"]["value"], "42")

    def test_trend_history_tracks_curated_channels_with_bounded_retention(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache", trend_history_size=2)
            for value in ("7.1", "7.2", "7.3"):
                parsed = parse_channel_line(f"2026-07-04T12:00:00.000000: OBCApp.epsBridge.EPS_VBAT,19601,{value}")
                assert parsed is not None
                snapshots.update_channel("hosted-manual-dual-gds", "sband", parsed)
            history = snapshots.trend_history("hosted-manual-dual-gds", "sband", ["EPS_VBAT", "SYS_MODE"])
            self.assertEqual([entry["value"] for entry in history["EPS_VBAT"]], ["7.2", "7.3"])
            self.assertEqual(history["SYS_MODE"], [])

    def test_trend_history_separates_context_and_band(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            first = parse_channel_line("2026-07-04T12:00:00.000000: OBCApp.epsBridge.EPS_VBAT,19601,7.5")
            second = parse_channel_line("2026-07-04T12:00:01.000000: OBCApp.epsBridge.EPS_VBAT,19601,8.1")
            assert first is not None and second is not None
            snapshots.update_channel("hosted-manual-dual-gds", "sband", first)
            snapshots.update_channel("target-manual-ground-dual-gds", "uhf-primary-after-failover", second)
            hosted = snapshots.trend_history("hosted-manual-dual-gds", "sband", ["EPS_VBAT"])
            target = snapshots.trend_history("target-manual-ground-dual-gds", "uhf-primary-after-failover", ["EPS_VBAT"])
            self.assertEqual(hosted["EPS_VBAT"][-1]["value"], "7.5")
            self.assertEqual(target["EPS_VBAT"][-1]["value"], "8.1")

    def test_readback_viewer_groups_saved_results_into_tabs(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            snapshots.set_readback_cache(
                "hosted-manual-dual-gds",
                "sband",
                "OBCApp.epsBridge.EPS_GET_STATUS",
                {
                    "family": "channel-refresh-based",
                    "channels": {
                        "EPS_VBAT": {
                            "value": "7.5",
                            "timestamp": "2026-07-04T12:00:00.000000",
                            "observationAt": "2026-07-04T12:00:01Z",
                            "generation": 3,
                            "observationSource": "refresh",
                        }
                    },
                },
            )
            viewer = snapshots.readback_viewer("hosted-manual-dual-gds", "sband")
            self.assertIn("OBC", [tab["name"] for tab in viewer["tabs"]])
            eps_tab = next(tab for tab in viewer["tabs"] if tab["name"] == "EPS")
            eps_card = next(card for card in eps_tab["cards"] if card["commandName"] == "OBCApp.epsBridge.EPS_GET_STATUS")
            self.assertEqual(eps_card["state"], "success")
            self.assertTrue(eps_card["refreshable"])
            self.assertEqual(eps_card["refreshCommandName"], "OBCApp.epsBridge.EPS_GET_STATUS")
            self.assertEqual(eps_card["lastRefreshStatus"], "saved")
            self.assertEqual(eps_card["savedValues"]["kind"], "channels")
            self.assertIn("EPS_VBAT", eps_card["savedValues"]["fields"])

    def test_readback_viewer_marks_quick_refresh_metadata_for_empty_cards(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            viewer = snapshots.readback_viewer("hosted-manual-dual-gds", "sband")
            obc_tab = next(tab for tab in viewer["tabs"] if tab["name"] == "OBC")
            mode_card = next(card for card in obc_tab["cards"] if card["commandName"] == "OBCApp.modeManager.MODE_GET")
            self.assertEqual(mode_card["state"], "empty")
            self.assertTrue(mode_card["refreshable"])
            self.assertEqual(mode_card["refreshCommandName"], "OBCApp.modeManager.MODE_GET")
            self.assertEqual(mode_card["lastRefreshStatus"], "never")

    def test_action_history_filters_and_clears_current_console_session(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache", console_session_id="session-current")
            snapshots.record_action(
                "hosted-manual-dual-gds",
                "sband",
                {
                    "startedAt": "2026-07-05T12:00:00Z",
                    "request": {"contextId": "hosted-manual-dual-gds", "band": "sband", "kind": "command"},
                    "result": {"status": "succeeded", "commandName": "OBCApp.modeManager.MODE_GET"},
                },
            )
            snapshots.record_action(
                "hosted-manual-dual-gds",
                "sband",
                {
                    "consoleSessionId": "session-older",
                    "startedAt": "2026-07-05T11:00:00Z",
                    "request": {"contextId": "hosted-manual-dual-gds", "band": "sband", "kind": "command"},
                    "result": {"status": "succeeded", "commandName": "OBCApp.modeManager.MODE_SET"},
                },
            )
            self.assertEqual(len(snapshots.history(current_session_only=True)), 1)
            self.assertEqual(len(snapshots.history(current_session_only=False)), 2)
            cleared = snapshots.clear_history(current_session_only=True)
            self.assertEqual(cleared, 1)
            remaining = snapshots.history(current_session_only=False)
            self.assertEqual(len(remaining), 1)
            self.assertEqual(remaining[0]["consoleSessionId"], "session-older")

    def test_packet_lab_history_filters_and_clears_current_console_session(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache", console_session_id="session-current")
            snapshots.record_packet_lab({"timestamp": "2026-07-05T12:00:00Z", "case": "tampered-mac"})
            snapshots.record_packet_lab(
                {"consoleSessionId": "session-older", "timestamp": "2026-07-05T11:00:00Z", "case": "replay-stale-session"}
            )
            self.assertEqual(len(snapshots.packet_lab_history(current_session_only=True)), 1)
            self.assertEqual(len(snapshots.packet_lab_history(current_session_only=False)), 2)
            cleared = snapshots.clear_packet_lab_history(current_session_only=True)
            self.assertEqual(cleared, 1)
            remaining = snapshots.packet_lab_history(current_session_only=False)
            self.assertEqual(len(remaining), 1)
            self.assertEqual(remaining[0]["consoleSessionId"], "session-older")

    def test_recent_events_clear_only_resets_selected_ring(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            event = parse_event_line(
                "2026-07-05T12:00:00.000000: OBCApp.bootManager.BOOT_STATUS : boot ok"
            )
            assert event is not None
            snapshots.append_event("hosted-manual-dual-gds", "sband", event)
            snapshots.append_event("hosted-manual-dual-gds", "uhf-backup", event)
            cleared = snapshots.clear_recent_events("hosted-manual-dual-gds", "sband")
            self.assertEqual(cleared, 1)
            self.assertEqual(snapshots.recent_events("hosted-manual-dual-gds", "sband"), [])
            self.assertEqual(len(snapshots.recent_events("hosted-manual-dual-gds", "uhf-backup")), 1)

    def test_clear_context_cache_resets_channels_events_and_readbacks(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            event = parse_event_line(
                "2026-07-05T12:00:00.000000: OBCApp.bootManager.BOOT_STATUS : boot ok"
            )
            channel = parse_channel_line(
                "2026-07-05T12:00:01.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE"
            )
            assert event is not None
            assert channel is not None
            snapshots.append_event("hosted-manual-dual-gds", "sband", event)
            snapshots.append_event("other-context", "sband", event)
            snapshots.update_channel("hosted-manual-dual-gds", "sband", channel)
            snapshots.update_channel("other-context", "sband", channel)
            snapshots.set_readback_cache(
                "hosted-manual-dual-gds",
                "sband",
                "OBCApp.modeManager.MODE_GET",
                {"channels": {"SYS_MODE": {"value": "SAFE"}}},
            )
            snapshots.set_readback_cache(
                "other-context",
                "sband",
                "OBCApp.modeManager.MODE_GET",
                {"channels": {"SYS_MODE": {"value": "SAFE"}}},
            )
            cleared = snapshots.clear_context_cache("hosted-manual-dual-gds")
            self.assertEqual(cleared["events"], 1)
            self.assertEqual(cleared["channels"], 1)
            self.assertEqual(cleared["readbacks"], 1)
            self.assertEqual(snapshots.recent_events("hosted-manual-dual-gds", "sband"), [])
            self.assertEqual(snapshots.channel_map("hosted-manual-dual-gds", "sband"), {})
            self.assertNotIn("hosted-manual-dual-gds:sband:OBCApp.modeManager.MODE_GET", snapshots.readback_cache())
            self.assertEqual(len(snapshots.recent_events("other-context", "sband")), 1)
            self.assertIn("SYS_MODE", snapshots.channel_map("other-context", "sband"))
            self.assertIn("other-context:sband:OBCApp.modeManager.MODE_GET", snapshots.readback_cache())

    def test_promoted_refresh_channels_record_refresh_provenance(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            actions = GatewayActions(runtime_root, SurfaceRegistry(), snapshots)
            parsed = parse_channel_line("2026-07-02T12:00:00.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE")
            assert parsed is not None
            actions._promote_fresh_channels(
                "hosted-manual-dual-gds",
                "sband",
                {
                    "SYS_MODE": {
                        "timestamp": parsed.timestamp,
                        "qualifiedName": parsed.qualified_name,
                        "channelName": parsed.channel_name,
                        "channelId": parsed.channel_id,
                        "value": parsed.value,
                        "raw": parsed.raw,
                    }
                },
                refresh_command="OBCApp.modeManager.MODE_GET",
            )
            promoted = snapshots.channel_map("hosted-manual-dual-gds", "sband")["SYS_MODE"]
            self.assertEqual(promoted["value"], "SAFE")
            self.assertEqual(promoted["observationSource"], "refresh")
            self.assertEqual(promoted["refreshCommand"], "OBCApp.modeManager.MODE_GET")
            self.assertTrue(promoted["observationAt"])

    def test_promoted_refresh_channels_replace_existing_snapshot_payload(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            actions = GatewayActions(runtime_root, SurfaceRegistry(), snapshots)
            first = parse_channel_line("2026-07-02T12:00:00.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE")
            second = parse_channel_line("2026-07-02T12:00:05.000000: OBCApp.modeManager.SYS_MODE,268632064,DETUMBLE")
            assert first is not None
            assert second is not None
            snapshots.update_channel("hosted-manual-dual-gds", "sband", first)
            baseline = snapshots.channel_map("hosted-manual-dual-gds", "sband")["SYS_MODE"]
            actions._promote_fresh_channels(
                "hosted-manual-dual-gds",
                "sband",
                {
                    "SYS_MODE": {
                        "timestamp": second.timestamp,
                        "qualifiedName": second.qualified_name,
                        "channelName": second.channel_name,
                        "channelId": second.channel_id,
                        "value": second.value,
                        "raw": second.raw,
                    }
                },
                refresh_command="OBCApp.modeManager.MODE_GET",
            )
            promoted = snapshots.channel_map("hosted-manual-dual-gds", "sband")["SYS_MODE"]
            self.assertEqual(promoted["value"], "DETUMBLE")
            self.assertEqual(promoted["timestamp"], second.timestamp)
            self.assertEqual(promoted["raw"], second.raw)
            self.assertEqual(promoted["generation"], baseline["generation"])
            self.assertEqual(promoted["observationSource"], "refresh")
            self.assertEqual(promoted["refreshCommand"], "OBCApp.modeManager.MODE_GET")

    def test_channel_refresh_prefers_fresh_gps_channels_over_snapshot_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.gpsBridge.GPS_GET_STATE", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.gpsBridge.GPS_SOURCE_MODE,20823,LIVE_UART",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.gpsBridge.GPS_HAVE_SAMPLE,20824,1",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.gpsBridge.GPS_FIX_VALID,20825,1",
                "2026-07-01T12:00:01.000003,(2(0)-1781940686:4),OBCApp.gpsBridge.GPS_SAT_COUNT,20831,8",
                "2026-07-01T12:00:01.000004,(2(0)-1781940686:5),OBCApp.gpsBridge.GPS_LAT_DEG,20826,48.1173",
                "2026-07-01T12:00:01.000005,(2(0)-1781940686:6),OBCApp.gpsBridge.GPS_LON_DEG,20827,11.516666666666667",
                "2026-07-01T12:00:01.000006,(2(0)-1781940686:7),OBCApp.gpsBridge.GPS_ALT_M,20828,545.4",
                "2026-07-01T12:00:01.000007,(2(0)-1781940686:8),OBCApp.gpsBridge.GPS_SPEED_MPS,20829,0.0",
                "2026-07-01T12:00:01.000008,(2(0)-1781940686:9),OBCApp.gpsBridge.GPS_COURSE_DEG,20830,0.0",
                "2026-07-01T12:00:01.000009,(2(0)-1781940686:10),OBCApp.gpsBridge.GPS_HDOP,20832,0.9",
                "2026-07-01T12:00:01.000010,(2(0)-1781940686:11),OBCApp.gpsBridge.GPS_UTC_SEC_OF_DAY,20833,45319",
                "2026-07-01T12:00:01.000011,(2(0)-1781940686:12),OBCApp.gpsBridge.GPS_UTC_DATE_YMD,20834,230394",
                "2026-07-01T12:00:01.000012,(2(0)-1781940686:13),OBCApp.gpsBridge.GPS_ACCEPTED_SENTENCES,20835,12",
                "2026-07-01T12:00:01.000013,(2(0)-1781940686:14),OBCApp.gpsBridge.GPS_REJECTED_SENTENCES,20836,0",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10070000"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.gpsBridge.GPS_GET_STATE",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.gpsBridge.GPS_GET_STATE"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["GPS_SOURCE_MODE"]["value"], "LIVE_UART")
            self.assertEqual(result.readback["channels"]["GPS_FIX_VALID"]["value"], "1")
            self.assertEqual(result.readback["channels"]["GPS_LAT_DEG"]["value"], "48.1173")
            channel_map = snapshots.channel_map("hosted-manual-dual-gds", "sband")
            self.assertEqual(channel_map["GPS_SOURCE_MODE"]["value"], "LIVE_UART")
            self.assertEqual(channel_map["GPS_FIX_VALID"]["value"], "1")
            self.assertEqual(channel_map["GPS_LAT_DEG"]["value"], "48.1173")

    def test_channel_refresh_prefers_fresh_ttc_channels_over_snapshot_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50152,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50152},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.ttcPassManager.TTC_GET_STATUS", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.ttcPassManager.TTC_POLICY_ENABLED,26625,true",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.ttcPassManager.TTC_POLICY_WINDOW_CONFIGURED,26627,true",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.ttcPassManager.TTC_POLICY_WINDOW_ACTIVE,26630,false",
                "2026-07-01T12:00:01.000003,(2(0)-1781940686:4),OBCApp.ttcPassManager.TTC_POLICY_GPS_TIME_VALID,26631,true",
                "2026-07-01T12:00:01.000004,(2(0)-1781940686:5),OBCApp.ttcPassManager.TTC_POLICY_TTC_ACTIVE,26633,false",
                "2026-07-01T12:00:01.000005,(2(0)-1781940686:6),OBCApp.ttcPassManager.TTC_POLICY_LOSS_TIMEOUT_SEC,26626,4",
                "2026-07-01T12:00:01.000006,(2(0)-1781940686:7),OBCApp.ttcPassManager.TTC_POLICY_WINDOW_START_UNIX_SEC,26628,100",
                "2026-07-01T12:00:01.000007,(2(0)-1781940686:8),OBCApp.ttcPassManager.TTC_POLICY_WINDOW_END_UNIX_SEC,26629,200",
                "2026-07-01T12:00:01.000008,(2(0)-1781940686:9),OBCApp.ttcPassManager.TTC_POLICY_CURRENT_GPS_UNIX_SEC,26632,150",
                "2026-07-01T12:00:01.000009,(2(0)-1781940686:10),OBCApp.ttcPassManager.TTC_POLICY_LOSS_TIMER_SEC,26634,3",
                "2026-07-01T12:00:01.000010,(2(0)-1781940686:11),OBCApp.ttcPassManager.TTC_POLICY_LAST_ENTRY_REASON,26635,2",
                "2026-07-01T12:00:01.000011,(2(0)-1781940686:12),OBCApp.ttcPassManager.TTC_POLICY_LAST_EXIT_REASON,26636,1",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x1001A003"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.ttcPassManager.TTC_GET_STATUS",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.ttcPassManager.TTC_GET_STATUS"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_ENABLED"]["value"], "true")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_WINDOW_CONFIGURED"]["value"], "true")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_WINDOW_ACTIVE"]["value"], "false")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_GPS_TIME_VALID"]["value"], "true")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_TTC_ACTIVE"]["value"], "false")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_LOSS_TIMEOUT_SEC"]["value"], "4")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_WINDOW_START_UNIX_SEC"]["value"], "100")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_WINDOW_END_UNIX_SEC"]["value"], "200")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_CURRENT_GPS_UNIX_SEC"]["value"], "150")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_LOSS_TIMER_SEC"]["value"], "3")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_LAST_ENTRY_REASON"]["value"], "2")
            self.assertEqual(result.readback["channels"]["TTC_POLICY_LAST_EXIT_REASON"]["value"], "1")
            channel_map = snapshots.channel_map("hosted-manual-dual-gds", "sband")
            self.assertEqual(channel_map["TTC_POLICY_ENABLED"]["value"], "true")
            self.assertEqual(channel_map["TTC_POLICY_WINDOW_CONFIGURED"]["value"], "true")
            self.assertEqual(channel_map["TTC_POLICY_WINDOW_ACTIVE"]["value"], "false")
            self.assertEqual(channel_map["TTC_POLICY_GPS_TIME_VALID"]["value"], "true")
            self.assertEqual(channel_map["TTC_POLICY_TTC_ACTIVE"]["value"], "false")
            self.assertEqual(channel_map["TTC_POLICY_LOSS_TIMEOUT_SEC"]["value"], "4")
            self.assertEqual(channel_map["TTC_POLICY_WINDOW_START_UNIX_SEC"]["value"], "100")
            self.assertEqual(channel_map["TTC_POLICY_WINDOW_END_UNIX_SEC"]["value"], "200")
            self.assertEqual(channel_map["TTC_POLICY_CURRENT_GPS_UNIX_SEC"]["value"], "150")
            self.assertEqual(channel_map["TTC_POLICY_LOSS_TIMER_SEC"]["value"], "3")
            self.assertEqual(channel_map["TTC_POLICY_LAST_ENTRY_REASON"]["value"], "2")
            self.assertEqual(channel_map["TTC_POLICY_LAST_EXIT_REASON"]["value"], "1")

    def test_channel_refresh_prefers_fresh_storage_channels_over_snapshot_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50153,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50153},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.storageHealthBridge.STORAGE_GET_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                },
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.storageHealthBridge.STORAGE_WARNING_ACTIVE,26113,1",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.storageHealthBridge.STORAGE_DATA_PRODUCTS_QUOTA_STATUS,26131,2",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.storageHealthBridge.STORAGE_DATA_PRODUCTS_FILE_COUNT,26126,1",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10041000"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.storageHealthBridge.STORAGE_GET_STATUS",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.storageHealthBridge.STORAGE_GET_STATUS"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["STORAGE_WARNING_ACTIVE"]["value"], "1")
            self.assertEqual(result.readback["channels"]["STORAGE_DATA_PRODUCTS_QUOTA_STATUS"]["value"], "2")
            self.assertEqual(result.readback["channels"]["STORAGE_DATA_PRODUCTS_FILE_COUNT"]["value"], "1")
            channel_map = snapshots.channel_map("hosted-manual-dual-gds", "sband")
            self.assertEqual(channel_map["STORAGE_WARNING_ACTIVE"]["value"], "1")
            self.assertEqual(channel_map["STORAGE_DATA_PRODUCTS_QUOTA_STATUS"]["value"], "2")
            self.assertEqual(channel_map["STORAGE_DATA_PRODUCTS_FILE_COUNT"]["value"], "1")

    def test_channel_refresh_prefers_fresh_comm_channels_over_snapshot_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50154,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50154},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.commController.COMM_GET_STATUS", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.commController.COMM_ACTIVE_BAND,21504,SBAND",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.commController.COMM_PRIMARY_COMMAND_LINK,21508,SBAND",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.commController.COMM_PRIMARY_TELEMETRY_LINK,21509,SBAND",
                "2026-07-01T12:00:01.000003,(2(0)-1781940686:4),OBCApp.commController.COMM_PRIMARY_FILE_LINK,21510,SBAND",
                "2026-07-01T12:00:01.000004,(2(0)-1781940686:5),OBCApp.commController.COMM_S_BAND_AVAILABLE,21511,true",
                "2026-07-01T12:00:01.000005,(2(0)-1781940686:6),OBCApp.commController.COMM_UHF_AVAILABLE,21512,true",
                "2026-07-01T12:00:01.000006,(2(0)-1781940686:7),OBCApp.commController.COMM_S_BAND_AVAILABILITY_REASON,21525,0",
                "2026-07-01T12:00:01.000007,(2(0)-1781940686:8),OBCApp.commController.COMM_UHF_AVAILABILITY_REASON,21526,0",
                "2026-07-01T12:00:01.000008,(2(0)-1781940686:9),OBCApp.commController.COMM_FDIR_FAULT_LATCHED,21517,false",
                "2026-07-01T12:00:01.000009,(2(0)-1781940686:10),OBCApp.commController.COMM_FDIR_FAULT_KIND,21518,0",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10078000"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.commController.COMM_GET_STATUS",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.commController.COMM_GET_STATUS"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["COMM_ACTIVE_BAND"]["value"], "SBAND")
            self.assertEqual(result.readback["channels"]["COMM_PRIMARY_COMMAND_LINK"]["value"], "SBAND")
            self.assertEqual(result.readback["channels"]["COMM_PRIMARY_TELEMETRY_LINK"]["value"], "SBAND")
            self.assertEqual(result.readback["channels"]["COMM_PRIMARY_FILE_LINK"]["value"], "SBAND")
            self.assertEqual(result.readback["channels"]["COMM_S_BAND_AVAILABLE"]["value"], "true")
            self.assertEqual(result.readback["channels"]["COMM_UHF_AVAILABLE"]["value"], "true")
            self.assertEqual(result.readback["channels"]["COMM_S_BAND_AVAILABILITY_REASON"]["value"], "0")
            self.assertEqual(result.readback["channels"]["COMM_UHF_AVAILABILITY_REASON"]["value"], "0")
            self.assertEqual(result.readback["channels"]["COMM_FDIR_FAULT_LATCHED"]["value"], "false")
            self.assertEqual(result.readback["channels"]["COMM_FDIR_FAULT_KIND"]["value"], "0")
            channel_map = snapshots.channel_map("hosted-manual-dual-gds", "sband")
            self.assertEqual(channel_map["COMM_ACTIVE_BAND"]["value"], "SBAND")
            self.assertEqual(channel_map["COMM_PRIMARY_COMMAND_LINK"]["value"], "SBAND")
            self.assertEqual(channel_map["COMM_PRIMARY_TELEMETRY_LINK"]["value"], "SBAND")
            self.assertEqual(channel_map["COMM_PRIMARY_FILE_LINK"]["value"], "SBAND")
            self.assertEqual(channel_map["COMM_S_BAND_AVAILABLE"]["value"], "true")
            self.assertEqual(channel_map["COMM_UHF_AVAILABLE"]["value"], "true")
            self.assertEqual(channel_map["COMM_S_BAND_AVAILABILITY_REASON"]["value"], "0")
            self.assertEqual(channel_map["COMM_UHF_AVAILABILITY_REASON"]["value"], "0")
            self.assertEqual(channel_map["COMM_FDIR_FAULT_LATCHED"]["value"], "false")
            self.assertEqual(channel_map["COMM_FDIR_FAULT_KIND"]["value"], "0")

    def test_packet_summary_extracts_secure_fields(self) -> None:
        inner = b"\x00\x00\x10\x03\x00\x01"
        packet = build_secure_command_v2_packet(inner, bytes(range(32)), 7)
        transport = COMMAND_DESCRIPTOR.to_bytes(4, "big") + len(packet).to_bytes(4, "big") + packet
        packets = parse_transport_packets(transport)
        self.assertEqual(len(packets), 1)
        summary = parse_packet_summary(packet, source="unit", highlight="tampered-mac")
        self.assertEqual(summary["secureHeader"]["secureSequence"], 7)
        self.assertEqual(summary["secureHeader"]["magic"], "0x0BC0DE02")
        self.assertEqual(summary["innerCommand"]["opcode"], "0x10030001")

    def test_mission_console_display_formatting_contract(self) -> None:
        test_script = pathlib.Path(__file__).with_name("test_mission_console_display_format.js")
        completed = subprocess.run(
            ["node", str(test_script)],
            check=True,
            capture_output=True,
            text=True,
        )
        self.assertIn("mission-console display formatting: PASS", completed.stdout)

    def test_native_capture_override_is_isolated_and_restores_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            surface_root = pathlib.Path(temp_dir)
            manifest_path = surface_root / "manifest.json"
            original_capture = surface_root / "gds-to-southbound.bin"
            original_capture.write_bytes(b"synthetic-capture")
            original_manifest = (
                json.dumps(
                    {
                        "surfaceRoot": str(surface_root),
                        "operatorSurfaces": {
                            "sband": {
                                "captures": {
                                    "gdsToSouthbound": str(original_capture),
                                }
                            }
                        },
                    },
                    indent=2,
                ).encode("utf-8")
                + b"\n"
            )
            manifest_path.write_bytes(original_manifest)
            hosted_context = {
                "manifestPath": str(manifest_path),
                "bands": {
                    "sband": {
                        "manifestKey": "sband",
                        "surfaceRoot": str(surface_root),
                        "captures": {
                            "gdsToSouthbound": str(original_capture),
                        },
                    }
                },
            }
            packet = b"secure-command-packet"

            with _native_capture_override(hosted_context, packet) as (native_capture, packet_offset):
                modified_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
                self.assertEqual(
                    modified_manifest["operatorSurfaces"]["sband"]["captures"]["gdsToSouthbound"],
                    str(native_capture),
                )
                self.assertEqual(
                    native_capture.read_bytes()[packet_offset : packet_offset + len(packet)],
                    packet,
                )
                self.assertEqual(original_capture.read_bytes(), b"synthetic-capture")

            self.assertEqual(manifest_path.read_bytes(), original_manifest)
            self.assertEqual(original_capture.read_bytes(), b"synthetic-capture")

            with self.assertRaisesRegex(RuntimeError, "probe interruption"):
                with _native_capture_override(hosted_context, packet):
                    raise RuntimeError("probe interruption")
            self.assertEqual(manifest_path.read_bytes(), original_manifest)
            self.assertEqual(original_capture.read_bytes(), b"synthetic-capture")

    def test_transport_parser_accepts_native_gds_capture_with_noise_and_trailer(self) -> None:
        inner = b"\x00\x00\x10\x03\x00\x01"
        packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
        native_gds_envelope = b"\x20\x44\x04\x4c\x00\x10\x00\xc0\x00\x00\x3f"
        capture = b"GDS-noise" + native_gds_envelope + packet + b"\xac\xa7"

        packets = parse_transport_packets(capture)

        self.assertEqual(len(packets), 1)
        self.assertEqual(packets[0].fw_packet_payload, packet)
        self.assertEqual(packets[0].offset, len(b"GDS-noise" + native_gds_envelope))

    def test_transport_parser_skips_malformed_native_candidates(self) -> None:
        inner = b"\x00\x00\x10\x03\x00\x01"
        valid_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
        mutations = {
            "magic": (6, b"\x00\x00\x00\x00"),
            "version": (10, b"\x7f"),
            "reserved": (11, b"\x01"),
            "header-length": (12, b"\x00\x13"),
            "inner-length": (18, b"\x00\x05"),
            "mac-length": (20, b"\x00\x1f"),
            "trailer": (22, b"\x00\x00\x00\x01"),
            "inner-command-kind": (26, b"\x00\x01"),
        }
        for label, (start, replacement) in mutations.items():
            with self.subTest(label=label):
                malformed = bytearray(valid_packet)
                malformed[start : start + len(replacement)] = replacement
                capture = b"prefix" + bytes(malformed) + b"\xff" + valid_packet + b"suffix"

                packets = parse_transport_packets(capture)

                self.assertEqual(len(packets), 1)
                self.assertEqual(packets[0].fw_packet_payload, valid_packet)
                self.assertEqual(packets[0].offset, len(b"prefix") + len(malformed) + 1)

    def test_transport_parser_rejects_truncated_native_candidate(self) -> None:
        inner = b"\x00\x00\x10\x03\x00\x01"
        packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)

        packets = parse_transport_packets(b"noise" + packet[:-1])

        self.assertEqual(packets, [])

    def test_transport_parser_prefers_synthetic_descriptor_envelope(self) -> None:
        inner = b"\x00\x00\x10\x03\x00\x01"
        native_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
        synthetic_packet = build_secure_command_v2_packet(inner, bytes([0x22]) * 32, 8)
        synthetic_offset = len(native_packet) + len(b"native-prefix")
        capture = (
            b"native-prefix"
            + native_packet
            + COMMAND_DESCRIPTOR.to_bytes(4, "big")
            + len(synthetic_packet).to_bytes(4, "big")
            + synthetic_packet
        )

        packets = parse_transport_packets(capture)

        self.assertEqual(len(packets), 1)
        self.assertEqual(packets[0].fw_packet_payload, synthetic_packet)
        self.assertEqual(packets[0].offset, synthetic_offset)

    def test_replay_captured_raw_rejects_capture_without_valid_packet(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            capture_path.write_bytes(b"GDS noise without a secure command")
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )

            with self.assertRaisesRegex(RuntimeError, "no reusable secure command packet found"):
                service._latest_captured_secure_packet("hosted-manual-dual-gds", context)

    def test_packet_lab_classifier(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            snapshots = SnapshotStore(pathlib.Path(temp_dir) / "cache")
            service = PacketLabService(pathlib.Path(temp_dir), SurfaceRegistry(), snapshots)
            explicit = service._classify_observed_result(
                before_events=[],
                after_events=[{"eventName": "COMMAND_SEQUENCE_REJECTED", "message": "bad"}],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                attempted_sequence=2,
            )
            self.assertEqual(explicit["kind"], "explicit-reject")
            bounded = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 2}},
                attempted_sequence=2,
            )
            self.assertEqual(bounded["kind"], "bounded-no-op")
            bounded_attempt = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "2", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "3", "generation": 2}},
                attempted_sequence=4,
            )
            self.assertEqual(bounded_attempt["kind"], "bounded-no-op")
            explicit_with_full_window = service._classify_observed_result(
                before_events=[{"eventSeq": 100, "eventName": "SOMETHING", "message": "old"}],
                after_events=[
                    {"eventSeq": 100, "eventName": "SOMETHING", "message": "old"},
                    {"eventSeq": 101, "eventName": "COMMAND_SEQUENCE_REJECTED", "message": "bad"},
                ],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                attempted_sequence=2,
            )
            self.assertEqual(explicit_with_full_window["kind"], "explicit-reject")
            explicit_from_late_native_event = service._classify_observed_result(
                before_events=[{"eventSeq": 100, "eventName": "SOMETHING", "message": "old"}],
                after_events=[
                    {"eventSeq": 100, "eventName": "SOMETHING", "message": "old"},
                    {"timestamp": "2026-06-21T16:38:44Z", "eventName": "SECURE_COMMAND_REJECTED", "message": "late"},
                ],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                attempted_sequence=4,
            )
            self.assertEqual(explicit_from_late_native_event["kind"], "explicit-reject")
            explicit_from_reject_telemetry = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={},
                after_channels={
                    "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER": {"value": "4", "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE": {"value": str(0x10030001), "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_REASON": {"value": "15", "generation": 2},
                    "SECURE_COMMAND_REJECT_TOTAL": {"value": "1", "generation": 2},
                },
                attempted_sequence=4,
                attempted_opcode="0x10030001",
            )
            self.assertEqual(explicit_from_reject_telemetry["kind"], "explicit-reject")
            self.assertEqual(explicit_from_reject_telemetry["telemetry"]["prefix"], "SECURE_COMMAND")
            explicit_from_first_post_marker_reject_telemetry = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={},
                after_channels={
                    "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER": {
                        "value": "4",
                        "postMarkerObserved": True,
                    },
                    "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE": {
                        "value": str(0x10030001),
                        "postMarkerObserved": True,
                    },
                    "SECURE_COMMAND_LAST_REJECT_REASON": {
                        "value": "15",
                        "postMarkerObserved": True,
                    },
                    "SECURE_COMMAND_REJECT_TOTAL": {
                        "value": "1",
                        "postMarkerObserved": True,
                    },
                },
                attempted_sequence=4,
                attempted_opcode="0x10030001",
            )
            self.assertEqual(explicit_from_first_post_marker_reject_telemetry["kind"], "explicit-reject")
            bounded_from_post_marker_sequence_observation = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 4}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 4, "postMarkerObserved": True}},
                attempted_sequence=4,
            )
            self.assertEqual(bounded_from_post_marker_sequence_observation["kind"], "bounded-no-op")
            stale_reject_telemetry = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={
                    "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER": {"value": "4", "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE": {"value": str(0x10030001), "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_REASON": {"value": "15", "generation": 2},
                    "SECURE_COMMAND_REJECT_TOTAL": {"value": "1", "generation": 2},
                    "SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "3", "generation": 2},
                },
                after_channels={
                    "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER": {"value": "4", "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE": {"value": str(0x10030001), "generation": 2},
                    "SECURE_COMMAND_LAST_REJECT_REASON": {"value": "15", "generation": 2},
                    "SECURE_COMMAND_REJECT_TOTAL": {"value": "1", "generation": 2},
                    "SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "3", "generation": 2},
                },
                attempted_sequence=4,
                attempted_opcode="0x10030001",
            )
            self.assertEqual(stale_reject_telemetry["kind"], "inconclusive")
            inconclusive_missing_sequence = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={},
                after_channels={},
                attempted_sequence=2,
            )
            self.assertEqual(inconclusive_missing_sequence["kind"], "inconclusive")
            inconclusive_stale_sequence_sample = service._classify_observed_result(
                before_events=[],
                after_events=[],
                before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                after_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "1", "generation": 1}},
                attempted_sequence=2,
            )
            self.assertEqual(inconclusive_stale_sequence_sample["kind"], "inconclusive")

    def test_packet_lab_augments_reject_reason_and_fault_explanation(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            observed = service._augment_observed_evidence(
                {
                    "kind": "explicit-reject",
                    "prefix": "SEQUENCE",
                    "reasonValue": 1,
                }
            )
            self.assertEqual(observed["rejectPath"], "SEQUENCE")
            self.assertEqual(observed["reasonName"], "NOT_INCREASING")
            explanation = service._fault_explanation(
                case="tampered-sequence",
                packet_summary={"secureHeader": {"secureSequence": 7}},
                current_state=secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=8,
                    last_auth_time=1_720_094_400.0,
                    manifest_path="/tmp/manifest.json",
                    authority_mode="sband-primary",
                    session_key_hex="00" * 32,
                    manifest_owner_pid=11,
                ),
            )
            self.assertEqual(explanation["faultKind"], "tampered-sequence")
            self.assertEqual(explanation["affectedField"], "secure sequence")
            self.assertIn("next secure sequence 8", explanation["expectedCondition"])
            self.assertIn("forced incorrect secure sequence 7", explanation["actualInjectedCondition"])

    def test_packet_lab_decodes_reject_event_reason_name(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            service = PacketLabService(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            decoded = service._decode_reject_event(
                {
                    "eventName": "COMMAND_SESSION_REJECTED",
                    "message": "Command session rejected reason 1 sequence 7 opcode 0x10030001",
                }
            )
            self.assertIsNotNone(decoded)
            assert decoded is not None
            self.assertEqual(decoded["rejectPath"], "SESSION")
            self.assertEqual(decoded["reasonValue"], 1)
            self.assertEqual(decoded["reasonName"], "NOT_OPEN")

    def test_merge_channel_snapshots_preserves_existing_generation(self) -> None:
        merged = _merge_channel_snapshots(
            {
                "SESSION_LAST_ACCEPTED_SEQUENCE": {
                    "value": "3",
                    "generation": 7,
                    "timestamp": "before",
                }
            },
            {
                "SESSION_LAST_ACCEPTED_SEQUENCE": {
                    "value": "3",
                    "timestamp": "after",
                }
            },
        )
        self.assertEqual(merged["SESSION_LAST_ACCEPTED_SEQUENCE"]["generation"], 7)
        self.assertEqual(merged["SESSION_LAST_ACCEPTED_SEQUENCE"]["timestamp"], "after")

    def test_mark_channel_fresh_after_marker_bumps_generation(self) -> None:
        marked = _mark_channel_fresh_after_marker(
            "SESSION_LAST_ACCEPTED_SEQUENCE",
            {
                "SESSION_LAST_ACCEPTED_SEQUENCE": {
                    "value": "3",
                    "generation": 4,
                }
            },
            {
                "SESSION_LAST_ACCEPTED_SEQUENCE": {
                    "value": "3",
                    "generation": 4,
                }
            },
        )
        self.assertEqual(marked["SESSION_LAST_ACCEPTED_SEQUENCE"]["generation"], 5)

    def test_observe_after_injection_does_not_promote_stale_reject_telemetry(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            before_channels = {
                "SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER": {"value": "4", "generation": 2},
                "SECURE_COMMAND_LAST_REJECT_INNER_OPCODE": {"value": str(0x10030001), "generation": 2},
                "SECURE_COMMAND_LAST_REJECT_REASON": {"value": "15", "generation": 2},
                "SECURE_COMMAND_REJECT_TOTAL": {"value": "1", "generation": 2},
                "SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "3", "generation": 2},
            }
            with (
                mock.patch.object(service.snapshots, "recent_events", return_value=[]),
                mock.patch.object(service.snapshots, "channel_map", return_value=dict(before_channels)),
                mock.patch.object(service, "_native_event_search_many", return_value=[]),
                mock.patch.object(
                    service,
                    "_native_channel_search_many",
                    return_value={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "3"}},
                ),
            ):
                _, after_channels, observed = service._observe_after_injection(
                    context_id="hosted-manual-dual-gds",
                    band="sband",
                    before_events=[],
                    before_channels=before_channels,
                    listener_marker={"eventOffset": 0, "channelOffset": 0},
                    attempted_sequence=4,
                    attempted_opcode="0x10030001",
                )
            self.assertEqual(observed["kind"], "bounded-no-op")
            self.assertFalse(
                after_channels["SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER"].get("postMarkerObserved", False)
            )
            self.assertTrue(after_channels["SESSION_LAST_ACCEPTED_SEQUENCE"].get("generation", 0) > 2)

    def test_packet_lab_skips_corrupt_stale_session_cache(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            service.session_cache_root.mkdir(parents=True, exist_ok=True)
            (service.session_cache_root / "hosted-manual-dual-gds-sband-bad.json").write_text("{not-json", encoding="utf-8")
            stale_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=4,
                last_auth_time=time.time(),
                manifest_path="/tmp/stale.json",
                authority_mode="sband-primary",
                session_key_hex="44" * 32,
                manifest_owner_pid=12,
            )
            (service.session_cache_root / "hosted-manual-dual-gds-sband-good.json").write_text(
                json.dumps(stale_state.to_json()),
                encoding="utf-8",
            )
            current_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=6,
                last_auth_time=time.time(),
                manifest_path="/tmp/current.json",
                authority_mode="sband-primary",
                session_key_hex="55" * 32,
                manifest_owner_pid=12,
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            session_key, sequence = service._load_stale_session_material("hosted-manual-dual-gds", context, current_state)
            self.assertEqual(session_key, stale_state.session_key)
            self.assertEqual(sequence, stale_state.next_secure_sequence)

    def test_packet_lab_stale_session_cache_is_scoped_to_context(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            service.session_cache_root.mkdir(parents=True, exist_ok=True)
            hosted_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=4,
                last_auth_time=time.time(),
                manifest_path="/tmp/hosted-stale.json",
                authority_mode="sband-primary",
                session_key_hex="44" * 32,
                manifest_owner_pid=12,
            )
            target_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=9,
                last_auth_time=time.time(),
                manifest_path="/tmp/target-stale.json",
                authority_mode="sband-primary",
                session_key_hex="66" * 32,
                manifest_owner_pid=13,
            )
            (service.session_cache_root / "hosted-manual-dual-gds-sband-a.json").write_text(
                json.dumps(hosted_state.to_json()),
                encoding="utf-8",
            )
            (service.session_cache_root / "target-manual-ground-dual-gds-sband-b.json").write_text(
                json.dumps(target_state.to_json()),
                encoding="utf-8",
            )
            current_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=12,
                last_auth_time=time.time(),
                manifest_path="/tmp/current.json",
                authority_mode="sband-primary",
                session_key_hex="77" * 32,
                manifest_owner_pid=14,
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            session_key, sequence = service._load_stale_session_material("hosted-manual-dual-gds", context, current_state)
            self.assertEqual(session_key, hosted_state.session_key)
            self.assertEqual(sequence, hosted_state.next_secure_sequence)

    def test_packet_lab_stale_session_cache_includes_uhf_sibling_alias(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            service = PacketLabService(runtime_root, registry, snapshots)
            service.session_cache_root.mkdir(parents=True, exist_ok=True)
            stale_state = secure_ops.SecureSessionState(
                service_id=2,
                active_band="uhf-backup",
                next_secure_sequence=5,
                last_auth_time=time.time(),
                manifest_path="/tmp/uhf-stale.json",
                authority_mode="uhf-backup",
                session_key_hex="44" * 32,
                manifest_owner_pid=12,
            )
            (service.session_cache_root / "hosted-manual-dual-gds-uhf-backup-a.json").write_text(
                json.dumps(stale_state.to_json()),
                encoding="utf-8",
            )
            current_state = secure_ops.SecureSessionState(
                service_id=2,
                active_band="uhf-primary-after-failover",
                next_secure_sequence=8,
                last_auth_time=time.time(),
                manifest_path="/tmp/current-uhf.json",
                authority_mode="uhf-primary-after-failover",
                session_key_hex="55" * 32,
                manifest_owner_pid=12,
            )
            manifest = {
                "surfaceRoot": str(runtime_root),
                "ownerPid": 12,
                "operatorSurfaces": {
                    "uhf": {
                        "canonicalBands": ["uhf-backup", "uhf-primary-after-failover"],
                        "gdsTtsPort": 50161,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="uhf-primary-after-failover",
                manifest_path=runtime_root / "manifest.json",
                manifest=manifest,
                surface={"gdsTtsPort": 50161},
                service_id=2,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            session_key, sequence = service._load_stale_session_material("hosted-manual-dual-gds", context, current_state)
            self.assertEqual(session_key, stale_state.session_key)
            self.assertEqual(sequence, stale_state.next_secure_sequence)

    def test_replay_captured_raw_skips_prior_packet_lab_injections(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            good_inner = b"\x00\x00\x10\x03\x00\x01"
            good_packet = build_secure_command_v2_packet(good_inner, bytes([0x11]) * 32, 7)
            bad_packet = bytearray(build_secure_command_v2_packet(good_inner, bytes([0x22]) * 32, 8))
            bad_packet[-1] ^= 0xFF
            capture_path.write_bytes(
                COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(good_packet).to_bytes(4, "big")
                + good_packet
                + COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(bad_packet).to_bytes(4, "big")
                + bytes(bad_packet)
            )
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "sband",
                    "case": "tampered-mac",
                    "rawBytesHex": bytes(bad_packet).hex(),
                    "packetSummary": {"source": "live-crafted"},
                }
            )
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "sband",
                    "case": "replay-captured-raw",
                    "source": "capture-replay",
                    "rawBytesHex": good_packet.hex(),
                }
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)
            self.assertEqual(replay_packet, good_packet)

    def test_replay_captured_raw_accepts_native_gds_capture_envelope(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            inner = b"\x00\x00\x10\x03\x00\x01"
            captured_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
            native_gds_envelope = b"\x20\x44\x04\x4c\x00\x10\x00\xc0\x00\x00\x3f"
            capture_path.write_bytes(b"GDS-noise" + native_gds_envelope + captured_packet + b"\xac\xa7")
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )

            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)

            self.assertEqual(replay_packet, captured_packet)

    def test_replay_captured_raw_keeps_valid_capture_when_duplicate_case_matches_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            inner = b"\x00\x00\x10\x03\x00\x01"
            good_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
            capture_path.write_bytes(
                COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(good_packet).to_bytes(4, "big")
                + good_packet
            )
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "sband",
                    "case": "duplicate-sequence",
                    "source": "live-crafted",
                    "rawBytesHex": good_packet.hex(),
                }
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)
            self.assertEqual(replay_packet, good_packet)

    def test_replay_captured_raw_skips_live_crafted_sequence_artifact_by_offset(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            inner = b"\x00\x00\x10\x03\x00\x01"
            good_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
            duplicate_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 9)
            good_transport = COMMAND_DESCRIPTOR.to_bytes(4, "big") + len(good_packet).to_bytes(4, "big") + good_packet
            duplicate_transport = (
                COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(duplicate_packet).to_bytes(4, "big")
                + duplicate_packet
            )
            capture_path.write_bytes(good_transport + duplicate_transport)
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "sband",
                    "case": "duplicate-sequence",
                    "source": "live-crafted",
                    "rawBytesHex": duplicate_packet.hex(),
                    "capturePacketOffset": len(good_transport),
                }
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)
            self.assertEqual(replay_packet, good_packet)

    def test_replay_captured_raw_skips_prior_stale_session_packet(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            service = PacketLabService(runtime_root, SurfaceRegistry(), snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            inner = b"\x00\x00\x10\x03\x00\x01"
            good_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
            stale_packet = build_secure_command_v2_packet(inner, bytes([0x22]) * 32, 8)
            capture_path.write_bytes(
                COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(good_packet).to_bytes(4, "big")
                + good_packet
                + COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(stale_packet).to_bytes(4, "big")
                + stale_packet
            )
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "sband",
                    "case": "replay-stale-session",
                    "source": "stale-session-cache",
                    "rawBytesHex": stale_packet.hex(),
                }
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)
            self.assertEqual(replay_packet, good_packet)

    def test_replay_captured_raw_skips_alias_band_packet_lab_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            service = PacketLabService(runtime_root, registry, snapshots)
            capture_path = runtime_root / "gds-to-southbound.bin"
            inner = b"\x00\x00\x10\x03\x00\x01"
            good_packet = build_secure_command_v2_packet(inner, bytes([0x11]) * 32, 7)
            bad_packet = bytearray(build_secure_command_v2_packet(inner, bytes([0x22]) * 32, 8))
            bad_packet[-1] ^= 0xFF
            capture_path.write_bytes(
                COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(good_packet).to_bytes(4, "big")
                + good_packet
                + COMMAND_DESCRIPTOR.to_bytes(4, "big")
                + len(bad_packet).to_bytes(4, "big")
                + bytes(bad_packet)
            )
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            alias_surface = BandSurface(
                band="uhf-backup",
                manifest_key="uhf",
                surface_root=runtime_root / "surface",
                gds_tts_port=50152,
                dictionary_path=dictionary,
                gui_url=None,
                captures={"gdsToSouthbound": str(capture_path)},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state" / "uhf-backup.json",
                owner_pid=1,
            )
            primary_surface = BandSurface(
                band="uhf-primary-after-failover",
                manifest_key="uhf",
                surface_root=runtime_root / "surface",
                gds_tts_port=50152,
                dictionary_path=dictionary,
                gui_url=None,
                captures={"gdsToSouthbound": str(capture_path)},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state" / "uhf-primary-after-failover.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={
                        "uhf-backup": alias_surface,
                        "uhf-primary-after-failover": primary_surface,
                    },
                )
            }
            snapshots.record_packet_lab(
                {
                    "contextId": "hosted-manual-dual-gds",
                    "band": "uhf-backup",
                    "case": "tampered-mac",
                    "source": "live-crafted",
                    "rawBytesHex": bytes(bad_packet).hex(),
                }
            )
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="uhf-primary-after-failover",
                manifest_path=runtime_root / "manifest.json",
                manifest={},
                surface={"captures": {"gdsToSouthbound": str(capture_path)}, "gdsTtsPort": 50152},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            replay_packet = service._latest_captured_secure_packet("hosted-manual-dual-gds", context)
            self.assertEqual(replay_packet, good_packet)

    def test_gateway_ensure_auth(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"ownerPid": 11, "surfaceRoot": str(runtime_root)},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=context.manifest_path,
                    status_path=runtime_root / "status.json",
                    manifest=context.manifest,
                    status=context.manifest,
                    lifecycle_state="running",
                    owner_pid=11,
                    dictionary_path=context.dictionary_path,
                    bands={
                        "sband": BandSurface(
                            band="sband",
                            manifest_key="sband",
                            surface_root=runtime_root,
                            gds_tts_port=50151,
                            dictionary_path=context.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=context.secure_state_path,
                            owner_pid=11,
                        )
                    },
                )
            }
            active_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=4,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="22" * 32,
                manifest_owner_pid=11,
            )
            with mock.patch.object(secure_ops, "load_state", return_value=active_state):
                result = actions.ensure_auth("hosted-manual-dual-gds", context)
                self.assertFalse(result["performed"])
            invalid_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=1,
                last_auth_time=time.time() - 999,
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="33" * 32,
                manifest_owner_pid=11,
                invalidated=True,
                invalidation_reason="session-timeout",
            )
            with mock.patch.object(secure_ops, "load_state", return_value=invalid_state), mock.patch.object(
                secure_ops,
                "establish_auth_result",
                return_value=secure_ops.ManualActionResult(status="authenticated", env="hosted", band="sband"),
            ):
                result = actions.ensure_auth("hosted-manual-dual-gds", context)
                self.assertTrue(result["performed"])

    def test_execute_auth_ensure_is_idempotent_when_session_active(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"ownerPid": 11, "surfaceRoot": str(runtime_root)},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=context.manifest_path,
                    status_path=runtime_root / "status.json",
                    manifest=context.manifest,
                    status=context.manifest,
                    lifecycle_state="running",
                    owner_pid=11,
                    dictionary_path=context.dictionary_path,
                    bands={
                        "sband": BandSurface(
                            band="sband",
                            manifest_key="sband",
                            surface_root=runtime_root,
                            gds_tts_port=50151,
                            dictionary_path=context.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=context.secure_state_path,
                            owner_pid=11,
                        )
                    },
                )
            }
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="auth-ensure",
                payload={},
                ensureAuth=False,
            )
            with (
                mock.patch.object(registry, "refresh", return_value=registry._contexts),
                mock.patch.object(secure_ops, "resolve_context", return_value=context),
                mock.patch.object(actions, "ensure_auth", return_value={"performed": False, "reason": "session-active"}) as ensure_mock,
                mock.patch.object(actions, "_establish_auth") as establish_mock,
            ):
                result = actions.execute(request)
            self.assertEqual(result.status, "succeeded")
            self.assertEqual(result.artifacts, {"performed": False, "reason": "session-active"})
            ensure_mock.assert_called_once_with("hosted-manual-dual-gds", context)
            establish_mock.assert_not_called()

    def test_persist_session_snapshot_keeps_private_permissions(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            actions = GatewayActions(runtime_root, SurfaceRegistry(), snapshots)
            manifest = {"surfaceRoot": str(runtime_root), "ownerPid": 9}
            state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=5,
                last_auth_time=time.time(),
                manifest_path=str(runtime_root / "manifest.json"),
                authority_mode="sband-primary",
                session_key_hex="66" * 32,
                manifest_owner_pid=9,
            )
            state_path = secure_ops.save_state(manifest, "sband", state)
            actions._persist_session_snapshot("hosted-manual-dual-gds", "sband", state_path)
            cached = list(actions.session_cache_root.glob("hosted-manual-dual-gds-sband-*.json"))
            self.assertEqual(len(cached), 1)
            self.assertEqual(cached[0].read_text(encoding="utf-8"), state_path.read_text(encoding="utf-8"))
            self.assertEqual(cached[0].stat().st_mode & 0o777, 0o600)

    def test_execute_auth_snapshots_old_session_before_reauth(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"ownerPid": 11, "surfaceRoot": str(runtime_root)},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=context.manifest_path,
                    status_path=runtime_root / "status.json",
                    manifest=context.manifest,
                    status=context.manifest,
                    lifecycle_state="running",
                    owner_pid=11,
                    dictionary_path=context.dictionary_path,
                    bands={
                        "sband": BandSurface(
                            band="sband",
                            manifest_key="sband",
                            surface_root=runtime_root,
                            gds_tts_port=50151,
                            dictionary_path=context.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=context.secure_state_path,
                            owner_pid=11,
                        )
                    },
                )
            }
            old_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=4,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="22" * 32,
                manifest_owner_pid=11,
            )
            secure_ops.save_state(context.manifest, "sband", old_state)
            new_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=1,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="33" * 32,
                manifest_owner_pid=11,
            )

            def overwrite_state(*args: object, **kwargs: object) -> secure_ops.ManualActionResult:
                secure_ops.save_state(context.manifest, "sband", new_state)
                return secure_ops.ManualActionResult(status="authenticated", env="hosted", band="sband")

            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="auth",
                payload={},
                ensureAuth=False,
            )
            with (
                mock.patch.object(registry, "refresh", return_value=registry._contexts),
                mock.patch.object(secure_ops, "resolve_context", return_value=context),
                mock.patch.object(actions, "_establish_auth", side_effect=overwrite_state),
            ):
                result = actions.execute(request)
            self.assertEqual(result.status, "succeeded")
            cached = sorted(actions.session_cache_root.glob("hosted-manual-dual-gds-sband-*.json"))
            self.assertEqual(len(cached), 2)
            session_keys = {json.loads(path.read_text(encoding="utf-8"))["sessionKeyHex"] for path in cached}
            self.assertEqual(session_keys, {old_state.session_key_hex, new_state.session_key_hex})

    def test_execute_auth_snapshots_uhf_sibling_alias_before_reauth(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            manifest = {
                "ownerPid": 11,
                "surfaceRoot": str(runtime_root),
                "operatorSurfaces": {
                    "uhf": {
                        "canonicalBands": ["uhf-backup", "uhf-primary-after-failover"],
                        "gdsTtsPort": 50161,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="uhf-primary-after-failover",
                manifest_path=runtime_root / "manifest.json",
                manifest=manifest,
                surface={"gdsTtsPort": 50161},
                service_id=2,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "uhf-primary-after-failover.json",
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=context.manifest_path,
                    status_path=runtime_root / "status.json",
                    manifest=manifest,
                    status=manifest,
                    lifecycle_state="running",
                    owner_pid=11,
                    dictionary_path=context.dictionary_path,
                    bands={
                        "uhf-primary-after-failover": BandSurface(
                            band="uhf-primary-after-failover",
                            manifest_key="uhf",
                            surface_root=runtime_root,
                            gds_tts_port=50161,
                            dictionary_path=context.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=context.secure_state_path,
                            owner_pid=11,
                        )
                    },
                )
            }
            sibling_state = secure_ops.SecureSessionState(
                service_id=2,
                active_band="uhf-backup",
                next_secure_sequence=4,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="uhf-backup",
                session_key_hex="22" * 32,
                manifest_owner_pid=11,
            )
            new_state = secure_ops.SecureSessionState(
                service_id=2,
                active_band="uhf-primary-after-failover",
                next_secure_sequence=1,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="uhf-primary-after-failover",
                session_key_hex="33" * 32,
                manifest_owner_pid=11,
            )
            secure_ops.save_state(manifest, "uhf-backup", sibling_state)

            def overwrite_state(*args: object, **kwargs: object) -> secure_ops.ManualActionResult:
                secure_ops.save_state(manifest, "uhf-primary-after-failover", new_state)
                return secure_ops.ManualActionResult(status="authenticated", env="hosted", band="uhf-primary-after-failover")

            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="uhf-primary-after-failover",
                kind="auth",
                payload={},
                ensureAuth=False,
            )
            with (
                mock.patch.object(registry, "refresh", return_value=registry._contexts),
                mock.patch.object(secure_ops, "resolve_context", return_value=context),
                mock.patch.object(actions, "_establish_auth", side_effect=overwrite_state),
            ):
                result = actions.execute(request)
            self.assertEqual(result.status, "succeeded")
            sibling_cached = sorted(actions.session_cache_root.glob("hosted-manual-dual-gds-uhf-backup-*.json"))
            self.assertEqual(len(sibling_cached), 1)
            self.assertEqual(
                json.loads(sibling_cached[0].read_text(encoding="utf-8"))["sessionKeyHex"],
                sibling_state.session_key_hex,
            )

    def test_execute_rejects_non_status_operation_for_stopped_context(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root,
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                status={"lifecycleState": "stopped"},
                lifecycle_state="stopped",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands={
                    "sband": BandSurface(
                        band="sband",
                        manifest_key="sband",
                        surface_root=runtime_root,
                        gds_tts_port=50151,
                        dictionary_path=runtime_root / "dict.xml",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        secure_state=None,
                        secure_state_path=runtime_root / "secure-state.json",
                        owner_pid=11,
                    )
                },
            )
            registry._contexts = {"hosted-manual-dual-gds": context}
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="command",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []},
            )
            with (
                mock.patch.object(registry, "refresh", return_value={"hosted-manual-dual-gds": context}) as refresh_mock,
                mock.patch.object(secure_ops, "resolve_context") as resolve_context,
            ):
                result = actions.execute(request)
            self.assertEqual(result.status, "failed")
            assert result.error is not None
            self.assertIn("context hosted-manual-dual-gds is not running", result.error)
            refresh_mock.assert_called_once()
            resolve_context.assert_not_called()

    def test_execute_refreshes_context_before_non_status_dispatch(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            cached_running = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root,
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                status={"lifecycleState": "running"},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands={
                    "sband": BandSurface(
                        band="sband",
                        manifest_key="sband",
                        surface_root=runtime_root,
                        gds_tts_port=50151,
                        dictionary_path=runtime_root / "dict.xml",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        secure_state=None,
                        secure_state_path=runtime_root / "secure-state.json",
                        owner_pid=11,
                    )
                },
            )
            refreshed_stopped = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root,
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                status={"lifecycleState": "stopped"},
                lifecycle_state="stopped",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands=cached_running.bands,
            )
            registry._contexts = {"hosted-manual-dual-gds": cached_running}
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="command",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []},
            )
            with (
                mock.patch.object(registry, "refresh", return_value={"hosted-manual-dual-gds": refreshed_stopped}) as refresh_mock,
                mock.patch.object(secure_ops, "resolve_context") as resolve_context,
            ):
                result = actions.execute(request)
            self.assertEqual(result.status, "failed")
            assert result.error is not None
            self.assertIn("context hosted-manual-dual-gds is not running", result.error)
            refresh_mock.assert_called_once()
            resolve_context.assert_not_called()

    def test_establish_auth_result_uses_explicit_native_packet_log(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            recv_path = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events" / "recv.bin"
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 55},
                surface={"captures": {"southboundToGds": str(runtime_root / "challenge.bin")}, "gdsTtsPort": 51901},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            challenge = SimpleNamespace(challenge=(b"\x01" * 16), packet_offset=12)
            authenticated = SimpleNamespace(status_code=1, packet_offset=18)
            wait_calls = [
                (challenge, "native-recv", (0, 1)),
                (authenticated, "native-recv", (0, 1)),
            ]
            with (
                mock.patch.object(secure_ops, "_load_handshake_messages_for_context", return_value={"wire-capture": [], "native-recv": []}),
                mock.patch.object(secure_ops, "_handshake_progress", return_value=(0, 0)),
                mock.patch.object(secure_ops, "_wait_for_handshake_from_sources", side_effect=wait_calls) as wait_mock,
                mock.patch.object(secure_ops, "send_tts_raw_packet", return_value=None),
                mock.patch.object(secure_ops, "load_command_auth_keystore", return_value=object()),
                mock.patch.object(secure_ops, "keystore_entry_for_service_id", return_value=SimpleNamespace(key_bytes=(b"\x02" * 32))),
                mock.patch.object(secure_ops, "derive_session_key", return_value=(b"\x03" * 32)),
                mock.patch.object(secure_ops, "compute_auth_response", return_value=(b"\x04" * 32)),
            ):
                result = secure_ops.establish_auth_result(context, native_packet_log=recv_path)
            self.assertEqual(result.status, "authenticated")
            first_call = wait_mock.call_args_list[0]
            self.assertEqual(first_call.kwargs["native_packet_log"], recv_path)

    def test_band_lock_pool_serializes_same_band(self) -> None:
        pool = BandLockPool()
        order: list[str] = []

        def worker(name: str, delay: float) -> None:
            with pool.hold("ctx", "sband"):
                order.append(f"{name}-enter")
                time.sleep(delay)
                order.append(f"{name}-exit")

        first = threading.Thread(target=worker, args=("first", 0.05))
        second = threading.Thread(target=worker, args=("second", 0.0))
        first.start()
        time.sleep(0.01)
        second.start()
        first.join()
        second.join()
        self.assertEqual(order, ["first-enter", "first-exit", "second-enter", "second-exit"])

    def test_runtime_shutdown_joins_refresh_thread_before_listener_cleanup(self) -> None:
        with (
            mock.patch("threading.Thread.start", return_value=None),
            mock.patch("atexit.register", return_value=None),
        ):
            sys.modules.pop("mission_console.app", None)
            app_module = importlib.import_module("mission_console.app")
        runtime = object.__new__(app_module.MissionConsoleRuntime)
        runtime._shutdown_once = threading.Event()
        runtime._stop = threading.Event()
        order: list[str] = []

        class FakeThread:
            def is_alive(self) -> bool:
                return True

            def join(self, timeout: float | None = None) -> None:
                order.append(f"join:{timeout}")

        class FakeListeners:
            def shutdown(self) -> None:
                order.append("listeners.shutdown")

        class FakeExecutor:
            def shutdown(self, wait: bool = False, cancel_futures: bool = False) -> None:
                order.append(f"executor.shutdown:{wait}:{cancel_futures}")

        runtime._thread = FakeThread()
        runtime.listeners = FakeListeners()
        runtime.executor = FakeExecutor()
        runtime.shutdown()
        self.assertTrue(runtime._stop.is_set())
        self.assertEqual(
            order,
            [
                "join:5.0",
                "listeners.shutdown",
                "executor.shutdown:False:True",
            ],
        )
        app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_app_module_skips_global_runtime_bootstrap_in_spawn_child(self) -> None:
        with (
            mock.patch("threading.Thread.start", return_value=None),
            mock.patch("atexit.register", return_value=None),
        ):
            sys.modules.pop("mission_console.app", None)
            app_module = importlib.import_module("mission_console.app")
        self.assertFalse(app_module.should_create_global_app("__mp_main__"))
        self.assertTrue(app_module.should_create_global_app("mission_console.app"))
        app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_command_catalog_route_returns_context_scoped_catalog(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            console_root = pathlib.Path(temp_dir) / "console-root"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "guiUrl": "http://127.0.0.1:5000",
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(console_root),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                client = app_module.app.test_client()
                response = client.get("/api/command-catalog?contextId=hosted-manual-dual-gds")
                payload = response.get_json()
            assert payload is not None
            self.assertEqual(response.status_code, 200)
            self.assertEqual(payload["contextId"], "hosted-manual-dual-gds")
            self.assertEqual(payload["dictionaryPath"], str(dictionary_path.resolve()))
            commands = {entry["name"]: entry for entry in payload["commands"]}
            self.assertEqual(commands["OBCApp.modeManager.MODE_SET"]["label"], "Set Mission Mode")
            self.assertEqual(commands["CdhCore.cmdDisp.CMD_NO_OP_STRING"]["group"], "Framework / CDH")
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_command_catalog_route_returns_empty_catalog_when_dictionary_is_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            missing_dictionary = hosted_root / "missing-dictionary.json"
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(missing_dictionary),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "guiUrl": "http://127.0.0.1:5000",
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(pathlib.Path(temp_dir) / "console"),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                client = app_module.app.test_client()
                response = client.get("/api/command-catalog?contextId=hosted-manual-dual-gds")
                payload = response.get_json()
            assert payload is not None
            self.assertEqual(response.status_code, 200)
            self.assertEqual(payload["dictionaryPath"], str(missing_dictionary.resolve()))
            self.assertEqual(payload["commands"], [])
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_trends_catalog_route_only_exposes_eps_adcs_health(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(pathlib.Path(temp_dir) / "console"),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                client = app_module.app.test_client()
                response = client.get("/api/trends/catalog?contextId=hosted-manual-dual-gds&band=sband")
                payload = response.get_json()
            assert payload is not None
            self.assertEqual(response.status_code, 200)
            self.assertEqual([group["name"] for group in payload["groups"]], ["EPS", "ADCS", "Health"])
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_dashboard_route_aggregates_context_latest_cards_and_band_scoped_secure_session(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            secure_state_root = hosted_root / "secure-state"
            secure_state_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    },
                    "uhf": {
                        "canonicalBands": ["uhf-backup"],
                        "gdsTtsPort": 50161,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    },
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            secure_ops.save_state(
                manifest,
                "sband",
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=9,
                    last_auth_time=time.time(),
                    manifest_path=str(hosted_root / "manifest.json"),
                    authority_mode="sband-primary",
                    session_key_hex=("11" * 32),
                    manifest_owner_pid=123,
                ),
            )
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                for line in (
                    "2026-07-05T12:00:00.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE",
                    "2026-07-05T12:00:01.000000: OBCApp.watchdogSupervisor.SYS_CPU_USAGE,268636160,0.7",
                    "2026-07-05T12:00:02.000000: OBCApp.epsBridge.EPS_VBAT,19601,7.8",
                ):
                    parsed = parse_channel_line(line)
                    assert parsed is not None
                    runtime.snapshots.update_channel("hosted-manual-dual-gds", "sband", parsed)
                latest_mode = parse_channel_line(
                    "2026-07-05T12:00:03.000000: OBCApp.modeManager.SYS_MODE,268632064,SCIENCE"
                )
                assert latest_mode is not None
                runtime.snapshots.update_channel("hosted-manual-dual-gds", "uhf-backup", latest_mode)
                client = app_module.app.test_client()
                payload = client.get("/api/dashboard?contextId=hosted-manual-dual-gds&band=sband").get_json()
            assert payload is not None
            cards = payload["cards"]
            self.assertEqual(cards["Satellite Status"]["SYS_MODE"]["value"], "SCIENCE")
            self.assertEqual(cards["Satellite Status"]["SYS_MODE"]["sourceBand"], "uhf-backup")
            self.assertEqual(cards["Secure Session"]["selectedBand"], "sband")
            self.assertEqual(cards["Secure Session"]["secureSession"]["nextSecureSequence"], 9)
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_dashboard_route_keeps_beacon_card_minimal(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                runtime.snapshots.set_beacon_snapshot(
                    "hosted-manual-dual-gds",
                    {
                        "supported": True,
                        "available": True,
                        "lastObservedAt": "2026-07-09T13:57:12Z",
                        "sequence": 17,
                        "sourceBand": "uhf-backup",
                        "sourceKind": "hosted-pty-side-channel",
                        "reason": None,
                    },
                )
                client = app_module.app.test_client()
                payload = client.get("/api/dashboard?contextId=hosted-manual-dual-gds&band=sband").get_json()
            assert payload is not None
            beacon = payload["cards"]["Beacon"]
            self.assertEqual(beacon["lastObservedAt"], "2026-07-09T13:57:12Z")
            self.assertEqual(beacon["sequence"], 17)
            self.assertNotIn("sourceBand", beacon)
            self.assertNotIn("sourceKind", beacon)
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_beacon_routes_expose_detail_and_history(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                runtime.snapshots.set_beacon_snapshot(
                    "hosted-manual-dual-gds",
                    {
                        "contextId": "hosted-manual-dual-gds",
                        "supported": True,
                        "available": True,
                        "lastObservedAt": "2026-07-09T13:57:12Z",
                        "sequence": 23,
                        "sourceBand": "uhf-backup",
                        "sourceKind": "hosted-pty-side-channel",
                        "capture": {"path": "/tmp/hosted-uhf.bin", "sizeBytes": 108, "frameSize": 108, "frameCount": 1},
                        "decode": {"status": "ok", "error": None},
                        "summary": {"lastBeaconTime": 1783604180, "sequence": 23},
                        "decoded": {"type": "BeaconV1", "sequence": 23},
                        "reason": None,
                    },
                )
                client = app_module.app.test_client()
                latest = client.get("/api/beacon/latest?contextId=hosted-manual-dual-gds").get_json()
                history = client.get("/api/beacon/history?contextId=hosted-manual-dual-gds").get_json()
            assert latest is not None
            assert history is not None
            self.assertEqual(latest["sourceKind"], "hosted-pty-side-channel")
            self.assertEqual(latest["capture"]["frameSize"], 108)
            self.assertEqual(latest["decoded"]["sequence"], 23)
            self.assertIn(23, [entry.get("sequence") for entry in history["history"]])
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_readback_viewer_route_exposes_quick_refresh_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(pathlib.Path(temp_dir) / "console"),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                runtime.snapshots.set_readback_cache(
                    "hosted-manual-dual-gds",
                    "sband",
                    "OBCApp.modeManager.MODE_GET",
                    {
                        "family": "channel-refresh-based",
                        "channels": {
                            "SYS_MODE": {
                                "value": "SAFE",
                                "timestamp": "2026-07-05T12:00:00.000000",
                                "observationAt": "2026-07-05T12:00:01Z",
                                "generation": 1,
                                "observationSource": "refresh",
                            }
                        },
                    },
                )
                client = app_module.app.test_client()
                payload = client.get("/api/readback/viewer?contextId=hosted-manual-dual-gds&band=sband").get_json()
            assert payload is not None
            obc_tab = next(tab for tab in payload["tabs"] if tab["name"] == "OBC")
            mode_card = next(card for card in obc_tab["cards"] if card["commandName"] == "OBCApp.modeManager.MODE_GET")
            self.assertTrue(mode_card["refreshable"])
            self.assertEqual(mode_card["refreshCommandName"], "OBCApp.modeManager.MODE_GET")
            self.assertEqual(mode_card["sourceCommand"], "OBCApp.modeManager.MODE_GET")
            self.assertEqual(mode_card["lastRefreshStatus"], "saved")
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_events_clear_route_resets_selected_context_band_ring(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    },
                    "uhf": {
                        "canonicalBands": ["uhf-backup"],
                        "gdsTtsPort": 50152,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    },
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(pathlib.Path(temp_dir) / "console"),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                event = parse_event_line(
                    "2026-07-05T12:00:00.000000: OBCApp.bootManager.BOOT_STATUS : boot ok"
                )
                assert event is not None
                runtime.snapshots.append_event("hosted-manual-dual-gds", "sband", event)
                runtime.snapshots.append_event("hosted-manual-dual-gds", "uhf-backup", event)
                client = app_module.app.test_client()
                payload = client.post(
                    "/api/events/clear",
                    json={"contextId": "hosted-manual-dual-gds", "band": "sband"},
                ).get_json()
                remaining_sband = runtime.snapshots.recent_events("hosted-manual-dual-gds", "sband")
                remaining_uhf = runtime.snapshots.recent_events("hosted-manual-dual-gds", "uhf-backup")
            assert payload is not None
            self.assertEqual(payload["clearedCount"], 1)
            self.assertEqual(remaining_sband, [])
            self.assertEqual(len(remaining_uhf), 1)
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_cache_clear_route_resets_selected_context_cache_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(pathlib.Path(temp_dir) / "console"),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                event = parse_event_line(
                    "2026-07-05T12:00:00.000000: OBCApp.bootManager.BOOT_STATUS : boot ok"
                )
                channel = parse_channel_line(
                    "2026-07-05T12:00:01.000000: OBCApp.modeManager.SYS_MODE,268632064,SAFE"
                )
                assert event is not None
                assert channel is not None
                runtime.snapshots.append_event("hosted-manual-dual-gds", "sband", event)
                runtime.snapshots.append_event("other-context", "sband", event)
                runtime.snapshots.update_channel("hosted-manual-dual-gds", "sband", channel)
                runtime.snapshots.update_channel("other-context", "sband", channel)
                runtime.snapshots.set_readback_cache(
                    "hosted-manual-dual-gds",
                    "sband",
                    "OBCApp.modeManager.MODE_GET",
                    {"channels": {"SYS_MODE": {"value": "SAFE"}}},
                )
                runtime.snapshots.set_readback_cache(
                    "other-context",
                    "sband",
                    "OBCApp.modeManager.MODE_GET",
                    {"channels": {"SYS_MODE": {"value": "SAFE"}}},
                )
                client = app_module.app.test_client()
                payload = client.post(
                    "/api/cache/clear",
                    json={"contextId": "hosted-manual-dual-gds"},
                ).get_json()
            assert payload is not None
            self.assertEqual(payload["cleared"]["events"], 1)
            self.assertEqual(payload["cleared"]["channels"], 1)
            self.assertEqual(payload["cleared"]["readbacks"], 1)
            self.assertEqual(runtime.snapshots.recent_events("hosted-manual-dual-gds", "sband"), [])
            self.assertEqual(runtime.snapshots.channel_map("hosted-manual-dual-gds", "sband"), {})
            self.assertNotIn("hosted-manual-dual-gds:sband:OBCApp.modeManager.MODE_GET", runtime.snapshots.readback_cache())
            self.assertEqual(len(runtime.snapshots.recent_events("other-context", "sband")), 1)
            self.assertIn("other-context:sband:OBCApp.modeManager.MODE_GET", runtime.snapshots.readback_cache())
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_sequence_source_renderer_builds_relative_time_lines(self) -> None:
        source = render_sequence_source(
            [
                {"offset": "R00:00:00", "commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []},
                {"offset": "R00:00:03", "commandName": "OBCApp.epsBridge.EPS_GET_STATUS", "commandArgs": []},
            ]
        )
        self.assertIn("R00:00:00 OBCApp.modeManager.MODE_GET", source)
        self.assertIn("R00:00:03 OBCApp.epsBridge.EPS_GET_STATUS", source)

    def test_sequence_authoring_compile_uses_active_dictionary_and_persists_runtime_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            dictionary_path = self._write_sample_dictionary(runtime_root)
            service = SequenceAuthoringService(runtime_root)
            with mock.patch("mission_console.gateway.sequence_authoring.subprocess.run") as run_mock:
                def _write_output(args, **_kwargs):
                    pathlib.Path(args[-1]).write_bytes(b"compiled-seq")
                    return SimpleNamespace(returncode=0, stdout="compiled ok")
                run_mock.side_effect = _write_output
                payload = service.compile_source(
                    context_id="hosted-manual-dual-gds",
                    dictionary_path=dictionary_path,
                    title="demo-sequence",
                    steps=[{"offset": "R00:00:00", "commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []}],
                    raw_source=None,
                )
            self.assertTrue(payload["success"])
            self.assertTrue(payload["sourcePath"].endswith(".seq"))
            self.assertTrue(payload["compiledPath"].endswith(".bin"))
            self.assertIn("sequence-drafts", payload["sourcePath"])

    def test_sequence_authoring_sanitizes_untrusted_draft_id(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = SequenceAuthoringService(runtime_root)
            payload = service.save_draft(
                context_id="hosted-manual-dual-gds",
                title="demo",
                steps=[],
                raw_source="",
                draft_id="../../outside",
            )
            self.assertEqual(payload["draftId"], "outside")
            self.assertTrue((runtime_root / "sequence-drafts" / "outside.json").exists())
            self.assertFalse((runtime_root.parent / "outside.json").exists())

    def test_sequence_authoring_list_drafts_ignores_non_dict_json(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = SequenceAuthoringService(runtime_root)
            (service.drafts_root / "bad.json").write_text('["not-a-dict"]\n', encoding="utf-8")
            payload = service.list_drafts("hosted-manual-dual-gds")
            self.assertEqual(payload, [])

    def test_sequence_authoring_compile_handles_missing_seqgen(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            dictionary_path = self._write_sample_dictionary(runtime_root)
            service = SequenceAuthoringService(runtime_root)
            service.seqgen_path = None
            with mock.patch("mission_console.gateway.sequence_authoring._find_seqgen_tool", return_value=None):
                payload = service.compile_source(
                    context_id="hosted-manual-dual-gds",
                    dictionary_path=dictionary_path,
                    title="demo-sequence",
                    steps=[{"offset": "R00:00:00", "commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []}],
                    raw_source=None,
                )
            self.assertFalse(payload["success"])
            self.assertEqual(payload["returnCode"], -1)
            self.assertIn("fprime-seqgen compiler tool not found", payload["diagnostics"])

    def test_sequence_authoring_init_does_not_require_seqgen(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            with mock.patch("mission_console.gateway.sequence_authoring._find_seqgen_tool", side_effect=AssertionError("should not be called")):
                service = SequenceAuthoringService(runtime_root)
            self.assertIsNone(service.seqgen_path)

    def test_sequence_authoring_compile_handles_subprocess_execution_error(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            dictionary_path = self._write_sample_dictionary(runtime_root)
            service = SequenceAuthoringService(runtime_root)
            with mock.patch("mission_console.gateway.sequence_authoring.subprocess.run", side_effect=PermissionError("denied")):
                payload = service.compile_source(
                    context_id="hosted-manual-dual-gds",
                    dictionary_path=dictionary_path,
                    title="demo-sequence",
                    steps=[{"offset": "R00:00:00", "commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": []}],
                    raw_source=None,
                )
            self.assertFalse(payload["success"])
            self.assertEqual(payload["returnCode"], -1)
            self.assertIn("Compilation failed to execute", payload["diagnostics"])

    def test_contexts_route_refreshes_secure_state_before_serializing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            secure_state_root = hosted_root / "secure-state"
            secure_state_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            status = {
                "surfaceType": "hosted-manual-dual-gds",
                "ownerPid": 123,
                "lifecycleState": "running",
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(status), encoding="utf-8")
            secure_ops.save_state(
                manifest,
                "sband",
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=4,
                    last_auth_time=time.time(),
                    manifest_path=str(hosted_root / "manifest.json"),
                    authority_mode="sband-primary",
                    session_key_hex=("11" * 32),
                    manifest_owner_pid=123,
                ),
            )
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                client = app_module.app.test_client()
                initial = client.get("/api/contexts").get_json()
                assert initial is not None
                self.assertFalse(
                    initial["contexts"]["hosted-manual-dual-gds"]["bands"]["sband"]["secureState"]["invalidated"]
                )

                invalidated_state = secure_ops.load_state(manifest, "sband")
                assert invalidated_state is not None
                invalidated_state.invalidated = True
                invalidated_state.invalidation_reason = "band-switch-command-sent"
                secure_ops.save_state(manifest, "sband", invalidated_state)

                refreshed = client.get("/api/contexts").get_json()
                assert refreshed is not None
                self.assertTrue(
                    refreshed["contexts"]["hosted-manual-dual-gds"]["bands"]["sband"]["secureState"]["invalidated"]
                )
                self.assertEqual(
                    refreshed["contexts"]["hosted-manual-dual-gds"]["bands"]["sband"]["secureState"][
                        "invalidationReason"
                    ],
                    "band-switch-command-sent",
                )
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_history_routes_filter_and_clear_current_console_session(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            console_root = pathlib.Path(temp_dir) / "console-root"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root),
                        "MISSION_CONSOLE_ROOT": str(console_root),
                    },
                    clear=False,
                ),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                runtime = app_module.app.config["MISSION_CONSOLE_RUNTIME"]
                runtime.snapshots.record_action(
                    "hosted-manual-dual-gds",
                    "sband",
                    {
                        "timestamp": "2026-07-05T10:00:00Z",
                        "contextId": "hosted-manual-dual-gds",
                        "band": "sband",
                        "kind": "readback",
                        "status": "succeeded",
                        "commandName": "OBCApp.modeManager.MODE_GET",
                    }
                )
                runtime.snapshots.record_action(
                    "hosted-manual-dual-gds",
                    "sband",
                    {
                        "consoleSessionId": "older-session",
                        "timestamp": "2026-07-05T09:00:00Z",
                        "contextId": "hosted-manual-dual-gds",
                        "band": "sband",
                        "kind": "readback",
                        "status": "succeeded",
                        "commandName": "OBCApp.modeManager.MODE_SET",
                    }
                )
                runtime.snapshots.record_packet_lab(
                    {
                        "timestamp": "2026-07-05T10:30:00Z",
                        "contextId": "hosted-manual-dual-gds",
                        "band": "sband",
                        "case": "tampered-mac",
                        "status": "succeeded",
                    }
                )
                runtime.snapshots.record_packet_lab(
                    {
                        "consoleSessionId": "older-session",
                        "timestamp": "2026-07-05T09:30:00Z",
                        "contextId": "hosted-manual-dual-gds",
                        "band": "sband",
                        "case": "replay-stale-session",
                        "status": "succeeded",
                    }
                )
                client = app_module.app.test_client()
                action_payload = client.get("/api/history").get_json()
                packet_payload = client.get("/api/packet-lab/history").get_json()
                assert action_payload is not None
                assert packet_payload is not None
                self.assertEqual(len(action_payload["entries"]), 1)
                self.assertEqual(len(packet_payload["entries"]), 1)
                self.assertFalse(action_payload["showAll"])
                self.assertFalse(packet_payload["showAll"])
                action_all = client.get("/api/history?showAll=1").get_json()
                packet_all = client.get("/api/packet-lab/history?showAll=true").get_json()
                assert action_all is not None
                assert packet_all is not None
                self.assertEqual(len(action_all["entries"]), 2)
                self.assertEqual(len(packet_all["entries"]), 2)
                cleared = client.post("/api/history/clear", json={}).get_json()
                packet_cleared = client.post("/api/packet-lab/history/clear", json={}).get_json()
                assert cleared is not None
                assert packet_cleared is not None
                self.assertEqual(cleared["clearedCount"], 1)
                self.assertEqual(packet_cleared["clearedCount"], 1)
                remaining_actions = client.get("/api/history?showAll=1").get_json()
                remaining_packets = client.get("/api/packet-lab/history?showAll=1").get_json()
                assert remaining_actions is not None
                assert remaining_packets is not None
                self.assertEqual(len(remaining_actions["entries"]), 1)
                self.assertEqual(len(remaining_packets["entries"]), 1)
                self.assertEqual(remaining_actions["entries"][0]["consoleSessionId"], "older-session")
                self.assertEqual(remaining_packets["entries"][0]["consoleSessionId"], "older-session")
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_ops_page_boot_includes_sequence_and_readback_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            hosted_root = pathlib.Path(temp_dir) / "hosted"
            hosted_root.mkdir(parents=True)
            dictionary_path = self._write_sample_dictionary(hosted_root)
            manifest = {
                "surfaceType": "hosted-manual-dual-gds",
                "surfaceRoot": str(hosted_root),
                "ownerPid": 123,
                "lifecycleState": "running",
                "dictionaryPath": str(dictionary_path),
                "operatorSurfaces": {
                    "sband": {
                        "canonicalBands": ["sband"],
                        "gdsTtsPort": 50151,
                        "captures": {},
                        "logs": {},
                        "southbound": {},
                    }
                },
            }
            (hosted_root / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
            (hosted_root / "status.json").write_text(json.dumps(manifest), encoding="utf-8")
            with (
                mock.patch.dict(os.environ, {"MISSION_CONSOLE_HOSTED_ROOT": str(hosted_root)}, clear=False),
                mock.patch("mission_console.gateway.registry.os.kill", return_value=None),
                mock.patch("threading.Thread.start", return_value=None),
                mock.patch("atexit.register", return_value=None),
            ):
                sys.modules.pop("mission_console.app", None)
                app_module = importlib.import_module("mission_console.app")
                client = app_module.app.test_client()
                response = client.get("/ops")
                html = response.get_data(as_text=True)
                normalized_html = html.replace(" ", "")
            self.assertEqual(response.status_code, 200)
            self.assertIn('"sequenceActionFields"', html)
            self.assertIn('"run":["sequencePath","runMode"]', normalized_html)
            self.assertIn('"readbackGroups"', html)
            self.assertIn('"OBCApp.commController.COMM_GET_STATUS"', html)
            app_module.app.config["MISSION_CONSOLE_RUNTIME"].shutdown()

    def test_gateway_operation_lock_key_collapses_uhf_aliases(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="uhf-backup",
                manifest_key="uhf",
                surface_root=runtime_root / "surface",
                gds_tts_port=51911,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=77,
            )
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 77},
                status={"ownerPid": 77},
                lifecycle_state="running",
                owner_pid=77,
                dictionary_path=dictionary,
                bands={
                    "uhf-backup": band,
                    "uhf-primary-after-failover": BandSurface(
                        band="uhf-primary-after-failover",
                        manifest_key="uhf",
                        surface_root=band.surface_root,
                        gds_tts_port=band.gds_tts_port,
                        dictionary_path=band.dictionary_path,
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        secure_state=None,
                        secure_state_path=runtime_root / "secure-state-primary.json",
                        owner_pid=77,
                    ),
                },
            )
            self.assertEqual(
                actions._operation_lock_key(context, "uhf-backup"),
                actions._operation_lock_key(context, "uhf-primary-after-failover"),
            )

    def test_gateway_operation_lock_key_ignores_owner_pid_for_same_surface_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            first_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=51901,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=77,
            )
            second_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=first_band.surface_root,
                gds_tts_port=first_band.gds_tts_port,
                dictionary_path=first_band.dictionary_path,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state-restarted.json",
                owner_pid=88,
            )
            first_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 77},
                status={"ownerPid": 77},
                lifecycle_state="running",
                owner_pid=77,
                dictionary_path=dictionary,
                bands={"sband": first_band},
            )
            second_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 88},
                status={"ownerPid": 88},
                lifecycle_state="running",
                owner_pid=88,
                dictionary_path=dictionary,
                bands={"sband": second_band},
            )
            self.assertEqual(
                actions._operation_lock_key(first_context, "sband"),
                actions._operation_lock_key(second_context, "sband"),
            )

    def test_listener_poll_log_reads_only_appended_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            manager = ListenerManager(pathlib.Path(temp_dir), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            log_path = pathlib.Path(temp_dir) / "event.log"
            log_path.write_text("old\nnew\n", encoding="utf-8")
            seen: list[str] = []
            offset = manager._poll_log(log_path, 4, seen.append)
            self.assertEqual(seen, ["new"])
            self.assertEqual(offset, log_path.stat().st_size)

    def test_parse_structured_event_handles_missing_fields(self) -> None:
        payload = parse_structured_event("PAYLOAD_STATUS", "capture 7 path /tmp/out.bin")
        self.assertIsNone(payload["state"])
        self.assertEqual(payload["captureId"], 7)

    def test_parse_structured_payload_status_event(self) -> None:
        payload = parse_structured_event(
            "PAYLOAD_STATUS",
            "Payload status state PSTATE_READY powered True prepared False result PRESULT_OK capture 7 index 3 "
            "preview persistent-data/payload/camera/PIC07.jpg previewDp data-products/demo.fdp "
            "previewPublished True rawPublished False",
        )
        self.assertEqual(payload["state"], "PSTATE_READY")
        self.assertTrue(payload["powered"])
        self.assertFalse(payload["prepared"])
        self.assertEqual(payload["lastResult"], "PRESULT_OK")
        self.assertEqual(payload["captureId"], 7)
        self.assertEqual(payload["captureIndex"], 3)
        self.assertEqual(payload["previewRelativePath"], "persistent-data/payload/camera/PIC07.jpg")
        self.assertEqual(payload["previewDataProductPath"], "data-products/demo.fdp")
        self.assertTrue(payload["previewPublished"])
        self.assertFalse(payload["rawPublished"])
        self.assertTrue(structured_event_complete("PAYLOAD_STATUS", payload))

    def test_parse_structured_payload_status_event_accepts_empty_paths(self) -> None:
        payload = parse_structured_event(
            "PAYLOAD_STATUS",
            "Payload status state PSTATE_OFF powered False prepared False result PRESULT_NONE capture 0 index 0 "
            "preview  previewDp  previewPublished False rawPublished False",
        )
        self.assertEqual(payload["previewRelativePath"], "")
        self.assertEqual(payload["previewDataProductPath"], "")
        self.assertTrue(structured_event_complete("PAYLOAD_STATUS", payload))

    def test_parse_structured_payload_capabilities_event(self) -> None:
        payload = parse_structured_event(
            "PAYLOAD_CAPABILITIES",
            "Payload capabilities backend libcamera supported 63 offOnly 15 autoMutable 0 deterministicMutable 48 raw False real True",
        )
        self.assertEqual(payload["backendName"], "libcamera")
        self.assertEqual(payload["supportedMask"], 63)
        self.assertEqual(payload["offOnlyMask"], 15)
        self.assertEqual(payload["autoMutableMask"], 0)
        self.assertEqual(payload["deterministicMutableMask"], 48)
        self.assertFalse(payload["rawRegisterSupported"])
        self.assertTrue(payload["realSensorPath"])
        self.assertTrue(structured_event_complete("PAYLOAD_CAPABILITIES", payload))

    def test_parse_structured_payload_capture_metadata_event_accepts_empty_raw_dp(self) -> None:
        payload = parse_structured_event(
            "PAYLOAD_CAPTURE_METADATA",
            "Payload capture metadata policy CAPTURE_AUTO capture 1 index 16 requested 0 applied 0 "
            "actualExp 23123 actualGain 187 awbValid True awbTempK 5032 awbRedX1000 1710 awbBlueX1000 1385 "
            "raw persistent-data/payload/camera/PIC10.bin preview persistent-data/payload/camera/PIC10.jpg "
            "previewDp data-products/demo.fdp rawDp  previewPublished True rawPublished False",
        )
        self.assertEqual(payload["capturePolicy"], "CAPTURE_AUTO")
        self.assertEqual(payload["captureId"], 1)
        self.assertEqual(payload["captureIndex"], 16)
        self.assertEqual(payload["requestedMask"], 0)
        self.assertEqual(payload["appliedMask"], 0)
        self.assertEqual(payload["actualExposureUsec"], 23123)
        self.assertEqual(payload["actualGainX100"], 187)
        self.assertTrue(payload["actualAwbValid"])
        self.assertEqual(payload["actualAwbColorTemperatureK"], 5032)
        self.assertEqual(payload["actualAwbRedGainX1000"], 1710)
        self.assertEqual(payload["actualAwbBlueGainX1000"], 1385)
        self.assertEqual(payload["rawRelativePath"], "persistent-data/payload/camera/PIC10.bin")
        self.assertEqual(payload["previewRelativePath"], "persistent-data/payload/camera/PIC10.jpg")
        self.assertEqual(payload["previewDataProductPath"], "data-products/demo.fdp")
        self.assertEqual(payload["rawDataProductPath"], "")
        self.assertTrue(payload["previewPublished"])
        self.assertFalse(payload["rawPublished"])
        self.assertTrue(structured_event_complete("PAYLOAD_CAPTURE_METADATA", payload))

    def test_payload_structured_event_completeness_rejects_missing_fields(self) -> None:
        self.assertFalse(structured_event_complete("PAYLOAD_CAPABILITIES", {"rawMessage": "payload"}))

    def test_parse_structured_recovery_status_event(self) -> None:
        payload = parse_structured_event(
            "RECOVERY_STATUS",
            "Recovery status activeCount 2 source ADCS_POLL_FRESHNESS highest R5_PROCESS_RESTART "
            "lastAction PROCESS_RESTART pendingProcessRestart True pendingReboot False relatchCount 4",
        )
        self.assertEqual(payload["activeIncidentCount"], 2)
        self.assertEqual(payload["activeSource"], "ADCS_POLL_FRESHNESS")
        self.assertEqual(payload["highestLevel"], "R5_PROCESS_RESTART")
        self.assertEqual(payload["lastAction"], "PROCESS_RESTART")
        self.assertTrue(payload["pendingProcessRestart"])
        self.assertFalse(payload["pendingReboot"])
        self.assertEqual(payload["relatchCount"], 4)
        self.assertTrue(structured_event_complete("RECOVERY_STATUS", payload))

    def test_parse_structured_watchdog_status_event(self) -> None:
        payload = parse_structured_event(
            "HW_WATCHDOG_STATUS",
            "Hardware watchdog status enabled False open True timeout 15 feedCount 7 lastError 0",
        )
        self.assertFalse(payload["enabled"])
        self.assertTrue(payload["deviceOpen"])
        self.assertEqual(payload["timeoutSec"], 15)
        self.assertEqual(payload["feedCount"], 7)
        self.assertEqual(payload["lastError"], 0)
        self.assertTrue(structured_event_complete("HW_WATCHDOG_STATUS", payload))

    def test_upload_file_result_rejects_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            with mock.patch.object(secure_ops, "require_active_state", return_value=object()):
                with self.assertRaisesRegex(RuntimeError, "directory"):
                    secure_ops.upload_file_result(context, local_path=runtime_root, destination_leaf="demo.bin")

    def test_bounded_cli_search_timeout_returns_partial_output(self) -> None:
        expired = subprocess.TimeoutExpired(
            cmd=["fprime-cli", "events"],
            timeout=7.0,
            output="partial output",
        )
        with mock.patch("mission_console.gateway.actions.subprocess.run", side_effect=expired):
            combined = _run_bounded_cli_search(["fprime-cli", "events"], timeout_sec=5.0)
        self.assertEqual(combined, "partial output")

    def test_native_event_search_reads_appended_listener_chunk(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            event_log = native_root / "event.log"
            prefix = "old-line\n"
            payload_line = (
                "2026-06-20T15:47:03.147396,(2(0)-1781941623:147396),"
                "OBCApp.payloadOpsController.PAYLOAD_STATUS,268673028,"
                "EventSeverity.ACTIVITY_LO,"
                "Payload status state PSTATE_OFF powered False prepared False result PRESULT_NONE capture 0 index 0"
            )
            event_log.write_text(prefix + payload_line + "\n", encoding="utf-8")
            hits = actions._native_event_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["PAYLOAD_STATUS"],
                after_offset=len(prefix),
                not_before_timestamp="2026-06-20T15:47:03.000000",
            )
            self.assertEqual(len(hits), 1)
            self.assertEqual(hits[0]["eventName"], "PAYLOAD_STATUS")

    def test_native_channel_search_does_not_return_pre_marker_stale_hit(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            channel_log = native_root / "channel.log"
            stale_line = (
                "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),"
                "OBCApp.modeManager.SYS_UPTIME_SEC,268632065,8"
            )
            prefix = stale_line + "\n"
            channel_log.write_text(prefix, encoding="utf-8")
            hits = actions._native_channel_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["SYS_UPTIME_SEC"],
                after_offset=len(prefix),
                not_before_timestamp="2026-06-20T15:31:26.000000",
            )
            self.assertEqual(hits, {})

    def test_native_channel_search_uses_byte_offsets(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            channel_log = native_root / "channel.log"
            prefix = "前置行\n"
            payload_line = (
                "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),"
                "OBCApp.modeManager.SYS_MODE,268632064,SAFE"
            )
            channel_log.write_text(prefix + payload_line + "\n", encoding="utf-8")
            hits = actions._native_channel_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["SYS_MODE"],
                after_offset=len(prefix.encode("utf-8")),
                not_before_timestamp="2026-06-20T15:31:25.000000",
            )
            self.assertEqual(list(hits), ["SYS_MODE"])
            self.assertEqual(hits["SYS_MODE"]["value"], "SAFE")

    def test_native_channel_search_backfills_missing_fields_from_full_log(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            channel_log = native_root / "channel.log"
            boot_count = (
                "2026-07-03T21:29:09.846038,(2(0)-1783085349:846038),"
                "OBCApp.bootManager.BOOT_BOOT_COUNT,268664842,100945\n"
            )
            reset_cause = (
                "2026-07-03T21:29:09.846038,(2(0)-1783085349:846038),"
                "OBCApp.bootManager.BOOT_RESET_CAUSE,268664841,UNKNOWN\n"
            )
            safe_fallback = (
                "2026-07-03T21:29:09.846038,(2(0)-1783085349:846038),"
                "OBCApp.bootManager.BOOT_SAFE_FALLBACK_REQUIRED,268664844,False\n"
            )
            channel_log.write_text(boot_count + reset_cause + safe_fallback, encoding="utf-8")
            hits = actions._native_channel_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["BOOT_RESET_CAUSE", "BOOT_BOOT_COUNT", "BOOT_SAFE_FALLBACK_REQUIRED"],
                after_offset=len(boot_count.encode("utf-8")),
                not_before_timestamp="2026-07-03T21:29:09.000000",
            )
            self.assertEqual(set(hits), {"BOOT_RESET_CAUSE", "BOOT_BOOT_COUNT", "BOOT_SAFE_FALLBACK_REQUIRED"})
            self.assertEqual(hits["BOOT_BOOT_COUNT"]["value"], "100945")

    def test_native_event_search_uses_byte_offsets(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            event_log = native_root / "event.log"
            prefix = "前置行\n"
            payload_line = (
                "2026-06-20T15:47:03.147396,(2(0)-1781941623:147396),"
                "OBCApp.payloadOpsController.PAYLOAD_STATUS,268673028,"
                "EventSeverity.ACTIVITY_LO,"
                "Payload status state PSTATE_OFF powered False prepared False result PRESULT_NONE capture 0 index 0"
            )
            event_log.write_text(prefix + payload_line + "\n", encoding="utf-8")
            hits = actions._native_event_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["PAYLOAD_STATUS"],
                after_offset=len(prefix.encode("utf-8")),
                not_before_timestamp="2026-06-20T15:47:03.000000",
            )
            self.assertEqual(len(hits), 1)
            self.assertEqual(hits[0]["eventName"], "PAYLOAD_STATUS")

    def test_native_event_search_backfills_multi_event_group_from_full_log(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.json"
            dictionary.write_text("{}", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            event_log = native_root / "event.log"
            status_line = (
                "2026-07-03T22:00:51.358905,(2(0)-1783087251:358905),"
                "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS,268666112,"
                "EventSeverity.ACTIVITY_HI,"
                "Persistent fault history total 64 returned 2 activeCopy COPY_A generation 1086111\n"
            )
            record0 = (
                "2026-07-03T22:00:51.359007,(2(0)-1783087251:359007),"
                "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD,268666113,"
                "EventSeverity.ACTIVITY_HI,"
                "Persistent fault[0] kind BOOT_OBSERVED source ADCS_POLL_TRANSPORT level R6_OBC_REBOOT "
                "action NONE cause UNKNOWN boot 100949 consecutive 0 uptime 0 time 1783087154 detail 0 flags 0\n"
            )
            record1 = (
                "2026-07-03T22:00:51.359070,(2(0)-1783087251:359070),"
                "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD,268666113,"
                "EventSeverity.ACTIVITY_HI,"
                "Persistent fault[1] kind BOOT_OBSERVED source ADCS_POLL_TRANSPORT level R6_OBC_REBOOT "
                "action NONE cause UNKNOWN boot 100948 consecutive 0 uptime 0 time 1783086544 detail 0 flags 0\n"
            )
            event_log.write_text(status_line + record0 + record1, encoding="utf-8")
            hits = actions._native_event_search_many(
                "hosted-manual-dual-gds",
                "sband",
                ["PERSISTENT_FAULT_HISTORY_STATUS", "PERSISTENT_FAULT_HISTORY_RECORD"],
                after_offset=len((status_line + record0).encode("utf-8")),
                not_before_timestamp="2026-07-03T22:00:51.000000",
            )
            self.assertEqual([event["eventName"] for event in hits], [
                "PERSISTENT_FAULT_HISTORY_STATUS",
                "PERSISTENT_FAULT_HISTORY_RECORD",
                "PERSISTENT_FAULT_HISTORY_RECORD",
            ])

    def test_command_completion_fallback_extracts_matching_opcode(self) -> None:
        text = (
            "2026-06-20T16:02:35.007729,(2(0)-1781942555:7729),"
            "CdhCore.cmdDisp.OpCodeDispatched,16777217,EventSeverity.COMMAND,"
            "Opcode 0x10038000 dispatched to port 9\n"
            "2026-06-20T16:02:35.007853,(2(0)-1781942555:7853),"
            "CdhCore.cmdDisp.OpCodeCompleted,16777218,EventSeverity.COMMAND,"
            "Opcode 0x10038000 completed\n"
        )
        completion = _find_command_completion(
            text,
            "0x10038000",
            not_before_timestamp="2026-06-20T16:02:35.000000",
        )
        self.assertIsNotNone(completion)
        assert completion is not None
        self.assertEqual(completion["eventName"], "OpCodeCompleted")
        self.assertEqual(completion["structured"]["opcode"], "0x10038000")

    def test_command_completion_fallback_rejects_pre_marker_opcode(self) -> None:
        text = (
            "2026-06-20T16:02:34.500000,(2(0)-1781942554:500000),"
            "CdhCore.cmdDisp.OpCodeCompleted,16777218,EventSeverity.COMMAND,"
            "Opcode 0x10038000 completed\n"
        )
        completion = _find_command_completion(
            text,
            "0x10038000",
            not_before_timestamp="2026-06-20T16:02:35.000000",
        )
        self.assertIsNone(completion)

    def test_channel_refresh_prefers_fresh_mode_channels_over_completion_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_mode = parse_channel_line(
                "2026-06-20T15:31:26.232548,(2(0)-1781940686:232548),OBCApp.modeManager.SYS_MODE,268632064,SAFE"
            )
            late_uptime = parse_channel_line(
                "2026-06-20T15:31:26.232549,(2(0)-1781940686:232549),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,9"
            )
            late_reboot = parse_channel_line(
                "2026-06-20T15:31:26.232550,(2(0)-1781940686:232550),OBCApp.modeManager.SYS_REBOOT_COUNT,268632066,1"
            )
            assert late_mode is not None
            assert late_uptime is not None
            assert late_reboot is not None
            late_channels = {
                late_mode.channel_name: late_mode.to_json(),
                late_uptime.channel_name: late_uptime.to_json(),
                late_reboot.channel_name: late_reboot.to_json(),
            }
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value={"eventName": "OpCodeCompleted"}),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10030001"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.modeManager.MODE_GET",
                        "channels": {
                            "SYS_MODE": None,
                            "SYS_UPTIME_SEC": None,
                            "SYS_REBOOT_COUNT": None,
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["SYS_MODE"]["value"], "SAFE")
            self.assertEqual(result.readback["channels"]["SYS_REBOOT_COUNT"]["value"], "1")

    def test_channel_refresh_prefers_fresh_eps_channels_over_completion_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.epsBridge.EPS_GET_STATUS", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.epsBridge.EPS_VBAT,19600,8.1",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.epsBridge.EPS_IBAT,19601,-0.3",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.epsBridge.EPS_SOC,19602,76.0",
                "2026-07-01T12:00:01.000003,(2(0)-1781940686:4),OBCApp.epsBridge.EPS_TEMP_BAT,19605,28.0",
                "2026-07-01T12:00:01.000004,(2(0)-1781940686:5),OBCApp.epsBridge.EPS_PDU_STATUS,19606,0x03",
                "2026-07-01T12:00:01.000005,(2(0)-1781940686:6),OBCApp.epsBridge.EPS_HEATER_ENABLED,19608,true",
                "2026-07-01T12:00:01.000006,(2(0)-1781940686:7),OBCApp.epsBridge.EPS_OVERCURRENT_FLAGS,19609,0x02",
                "2026-07-01T12:00:01.000007,(2(0)-1781940686:8),OBCApp.epsBridge.EPS_VSOLAR,19603,5.3",
                "2026-07-01T12:00:01.000008,(2(0)-1781940686:9),OBCApp.epsBridge.EPS_ISOLAR,19604,0.6",
                "2026-07-01T12:00:01.000009,(2(0)-1781940686:10),OBCApp.epsBridge.EPS_POWER_OUT,19607,7.5",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10040000"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.epsBridge.EPS_GET_STATUS",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.epsBridge.EPS_GET_STATUS"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["EPS_HEATER_ENABLED"]["value"], "true")
            self.assertEqual(result.readback["channels"]["EPS_OVERCURRENT_FLAGS"]["value"], "0x02")
            self.assertEqual(result.readback["channels"]["EPS_POWER_OUT"]["value"], "7.5")

    def test_channel_refresh_prefers_fresh_adcs_channels_over_completion_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.adcsBridge.ADCS_GET_ATTITUDE", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            late_lines = [
                "2026-07-01T12:00:01.000000,(2(0)-1781940686:1),OBCApp.adcsBridge.ADCS_MODE,19727,POINTING",
                "2026-07-01T12:00:01.000001,(2(0)-1781940686:2),OBCApp.adcsBridge.ADCS_Q0,19717,0.9238795",
                "2026-07-01T12:00:01.000002,(2(0)-1781940686:3),OBCApp.adcsBridge.ADCS_Q1,19718,0.0",
                "2026-07-01T12:00:01.000003,(2(0)-1781940686:4),OBCApp.adcsBridge.ADCS_Q2,19719,0.0",
                "2026-07-01T12:00:01.000004,(2(0)-1781940686:5),OBCApp.adcsBridge.ADCS_Q3,19720,0.3826834",
                "2026-07-01T12:00:01.000005,(2(0)-1781940686:6),OBCApp.adcsBridge.ADCS_OMEGA_X,19721,0.03",
                "2026-07-01T12:00:01.000006,(2(0)-1781940686:7),OBCApp.adcsBridge.ADCS_OMEGA_Y,19722,0.0",
                "2026-07-01T12:00:01.000007,(2(0)-1781940686:8),OBCApp.adcsBridge.ADCS_OMEGA_Z,19723,0.0",
                "2026-07-01T12:00:01.000008,(2(0)-1781940686:9),OBCApp.adcsBridge.ADCS_MAG_X,19724,0.2",
                "2026-07-01T12:00:01.000009,(2(0)-1781940686:10),OBCApp.adcsBridge.ADCS_MAG_Y,19725,-0.1",
                "2026-07-01T12:00:01.000010,(2(0)-1781940686:11),OBCApp.adcsBridge.ADCS_MAG_Z,19726,0.4",
                "2026-07-01T12:00:01.000011,(2(0)-1781940686:12),OBCApp.adcsBridge.ADCS_POINTING_ERR,19728,4.0",
            ]
            late_channels = {}
            for raw in late_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                late_channels[parsed.channel_name] = parsed.to_json()
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value=late_channels),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10030002"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
                        "channels": {
                            name: None for name in CHANNEL_REFRESH_COMMANDS["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "native-log")
            self.assertEqual(result.readback["channels"]["ADCS_MODE"]["value"], "POINTING")
            self.assertEqual(result.readback["channels"]["ADCS_Q0"]["value"], "0.9238795")
            self.assertEqual(result.readback["channels"]["ADCS_OMEGA_X"]["value"], "0.03")

    def test_channel_refresh_can_fall_back_to_late_snapshot_plus_completion(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            seed_lines = [
                "2026-06-20T15:31:25.232547,(2(0)-1781940685:232547),OBCApp.modeManager.SYS_MODE,268632064,SAFE",
                "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,8",
                "2026-06-20T15:31:25.232549,(2(0)-1781940685:232549),OBCApp.modeManager.SYS_REBOOT_COUNT,268632066,1",
            ]
            for raw in seed_lines:
                parsed = parse_channel_line(raw)
                assert parsed is not None
                snapshots.update_channel("hosted-manual-dual-gds", "sband", parsed)

            def completion_side_effect(*args: object, **kwargs: object) -> dict[str, str]:
                late_lines = [
                    "2026-06-20T15:31:26.232547,(2(0)-1781940686:232547),OBCApp.modeManager.SYS_MODE,268632064,SAFE",
                    "2026-06-20T15:31:26.232548,(2(0)-1781940686:232548),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,9",
                    "2026-06-20T15:31:26.232549,(2(0)-1781940686:232549),OBCApp.modeManager.SYS_REBOOT_COUNT,268632066,1",
                ]
                for raw in late_lines:
                    late = parse_channel_line(raw)
                    assert late is not None
                    snapshots.update_channel("hosted-manual-dual-gds", "sband", late)
                return {"eventName": "OpCodeCompleted"}

            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value={}),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", side_effect=completion_side_effect),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10030001"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.modeManager.MODE_GET",
                        "channels": {
                            "SYS_MODE": None,
                            "SYS_UPTIME_SEC": None,
                            "SYS_REBOOT_COUNT": None,
                        },
                    },
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["channelSource"], "snapshot+command-completion")
            self.assertEqual(result.readback["channels"]["SYS_REBOOT_COUNT"]["value"], "1")
            channel_map = snapshots.channel_map("hosted-manual-dual-gds", "sband")
            self.assertEqual(channel_map["SYS_MODE"]["generation"], 2)
            self.assertEqual(channel_map["SYS_UPTIME_SEC"]["generation"], 2)
            self.assertEqual(channel_map["SYS_REBOOT_COUNT"]["generation"], 2)

    def test_channel_refresh_continues_until_missing_mode_channels_are_filled(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            fresh_mode = parse_channel_line(
                "2026-06-20T15:31:26.232548,(2(0)-1781940686:232548),OBCApp.modeManager.SYS_MODE,268632064,SAFE"
            )
            missing_fill = {
                "SYS_UPTIME_SEC": {
                    "timestamp": "2026-06-20T15:31:26.232549",
                    "qualifiedName": "OBCApp.modeManager.SYS_UPTIME_SEC",
                    "channelName": "SYS_UPTIME_SEC",
                    "channelId": 268632065,
                    "value": "9",
                    "raw": "2026-06-20T15:31:26.232549,(2(0)-1781940686:232549),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,9",
                    "generation": 2,
                },
                "SYS_REBOOT_COUNT": {
                    "timestamp": "2026-06-20T15:31:26.232550",
                    "qualifiedName": "OBCApp.modeManager.SYS_REBOOT_COUNT",
                    "channelName": "SYS_REBOOT_COUNT",
                    "channelId": 268632066,
                    "value": "1",
                    "raw": "2026-06-20T15:31:26.232550,(2(0)-1781940686:232550),OBCApp.modeManager.SYS_REBOOT_COUNT,268632066,1",
                    "generation": 2,
                },
            }
            assert fresh_mode is not None
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(
                        status="sent",
                        env="hosted",
                        band="sband",
                        details={"secureSequence": 1},
                    ),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value={}),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10030001"),
                mock.patch.object(actions, "_native_command_completion", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.modeManager.MODE_GET",
                        "channels": {
                            "SYS_MODE": None,
                            "SYS_UPTIME_SEC": None,
                            "SYS_REBOOT_COUNT": None,
                        },
                    },
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={
                        "freshEvents": [],
                        "freshChannels": {"SYS_MODE": fresh_mode.to_json()},
                        "rejectEvents": [],
                    },
                ),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value=missing_fill) as bounded_mock,
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            bounded_mock.assert_called_once()
            missing_names = bounded_mock.call_args.args[1]
            self.assertEqual(missing_names, ["SYS_UPTIME_SEC", "SYS_REBOOT_COUNT"])
            assert result.readback is not None
            self.assertEqual(result.readback["channels"]["SYS_MODE"]["value"], "SAFE")
            self.assertEqual(result.readback["channels"]["SYS_UPTIME_SEC"]["value"], "9")
            self.assertEqual(result.readback["channels"]["SYS_REBOOT_COUNT"]["value"], "1")

    def test_channel_refresh_rejects_stale_snapshot_even_with_completion(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={"commandName": "OBCApp.modeManager.MODE_GET", "commandArgs": [], "settleSeconds": 0},
                ensureAuth=True,
            )
            stale = parse_channel_line(
                "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,8"
            )
            assert stale is not None
            snapshots.update_channel("hosted-manual-dual-gds", "sband", stale)
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_channel_search_many", return_value={}),
                mock.patch.object(actions, "_bounded_channel_search_many", return_value={}),
                mock.patch.object(actions, "_native_command_completion", return_value={"eventName": "OpCodeCompleted"}),
                mock.patch.object(actions, "_arm_channel_searches", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10030001"),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.modeManager.MODE_GET",
                        "channels": {
                            "SYS_MODE": None,
                            "SYS_UPTIME_SEC": None,
                            "SYS_REBOOT_COUNT": None,
                        },
                    },
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "no fresh channels"):
                    actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))

    def test_event_readback_can_fall_back_to_late_native_log_after_bounded_search(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "target-manual-ground-dual-gds": SurfaceContext(
                    context_id="target-manual-ground-dual-gds",
                    surface_type="target-manual-ground-dual-gds",
                    env_name="target",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="target-manual-ground-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            late_event = {
                "timestamp": "2026-06-21T00:40:20.327158",
                "qualifiedName": "OBCApp.linuxWatchdogSink.HW_WATCHDOG_STATUS",
                "eventName": "HW_WATCHDOG_STATUS",
                "message": "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                "raw": "raw",
                "structured": parse_structured_event(
                    "HW_WATCHDOG_STATUS",
                    "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                ),
            }
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="target", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", side_effect=[[], [late_event]]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["eventSource"], "native-log-late")
            self.assertEqual(result.readback["events"], [late_event])

    def test_event_readback_rechecks_rejects_before_accepting_fallback_result(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "target-manual-ground-dual-gds": SurfaceContext(
                    context_id="target-manual-ground-dual-gds",
                    surface_type="target-manual-ground-dual-gds",
                    env_name="target",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="target-manual-ground-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            late_event = {
                "timestamp": "2026-06-21T00:40:20.327158",
                "qualifiedName": "OBCApp.linuxWatchdogSink.HW_WATCHDOG_STATUS",
                "eventName": "HW_WATCHDOG_STATUS",
                "message": "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                "raw": "raw",
                "structured": parse_structured_event(
                    "HW_WATCHDOG_STATUS",
                    "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                ),
            }
            late_reject = {
                "timestamp": "2026-06-21T00:40:20.527158",
                "qualifiedName": "OBCApp.cmdSeq.COMMAND_SEQUENCE_REJECTED",
                "eventName": "COMMAND_SEQUENCE_REJECTED",
                "message": "duplicate sequence",
                "raw": "raw",
                "structured": {"rawMessage": "duplicate sequence"},
            }
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="target", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", side_effect=[[], [late_event]]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[late_reject]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "readback command rejected: duplicate sequence"):
                    actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(snapshots.readback_cache(), {})

    def test_readback_rejects_unsupported_command_before_send(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.modeManager.MODE_SET",
                    "commandArgs": ["SAFE"],
                    "settleSeconds": 0,
                },
                ensureAuth=True,
            )
            with mock.patch.object(secure_ops, "send_command_result") as send_command:
                with self.assertRaisesRegex(RuntimeError, "unsupported structured readback command"):
                    actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            send_command.assert_not_called()

    def test_completion_only_event_fallback_clears_stale_structured_events(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "target-manual-ground-dual-gds": SurfaceContext(
                    context_id="target-manual-ground-dual-gds",
                    surface_type="target-manual-ground-dual-gds",
                    env_name="target",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="target-manual-ground-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.bootManager.BOOT_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                    "searchTimeoutSeconds": 0,
                },
                ensureAuth=True,
            )
            completion = {
                "timestamp": "2026-06-21T00:40:20.327158",
                "qualifiedName": "OBCApp.cmdDisp.OpCodeCompleted",
                "eventName": "OpCodeCompleted",
                "message": "completed",
                "raw": "raw",
                "structured": {"opcode": "0x10038000"},
            }
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="target", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch.object(actions, "_native_command_completion", return_value=completion),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x10038000"),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.bootManager.BOOT_STATUS", "channels": {}},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={
                        "freshEvents": [],
                        "freshChannels": {"BOOT_RESET_CAUSE": {"channelName": "BOOT_RESET_CAUSE"}},
                        "rejectEvents": [],
                    },
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "missing BOOT_BOOT_COUNT, BOOT_SAFE_FALLBACK_REQUIRED"):
                    actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))

    def test_boot_status_is_channel_refresh_based(self) -> None:
        self.assertIn("OBCApp.bootManager.BOOT_STATUS", CHANNEL_REFRESH_COMMANDS)
        self.assertNotIn("OBCApp.bootManager.BOOT_STATUS", EVENT_BASED_COMMANDS)

    def test_parse_persistent_fault_history_events(self) -> None:
        status = parse_structured_event(
            "PERSISTENT_FAULT_HISTORY_STATUS",
            "Persistent fault history total 2 returned 2 activeCopy COPY_B generation 7",
        )
        record = parse_structured_event(
            "PERSISTENT_FAULT_HISTORY_RECORD",
            "Persistent fault[0] kind INCIDENT_OPENED source COMM_PRIMARY_UNAVAILABLE level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR boot 12 consecutive 3 uptime 44 time 1700 detail 99 flags 1",
        )
        self.assertEqual(status["totalRecords"], 2)
        self.assertEqual(status["returnedRecords"], 2)
        self.assertEqual(status["activeCopy"], "COPY_B")
        self.assertEqual(record["indexFromLatest"], 0)
        self.assertEqual(record["recoveryAction"], "OBC_REBOOT")
        self.assertEqual(record["bootCount"], 12)
        self.assertTrue(structured_event_complete("PERSISTENT_FAULT_HISTORY_STATUS", status))
        self.assertTrue(structured_event_complete("PERSISTENT_FAULT_HISTORY_RECORD", record))

    def test_parse_boot_recovery_status_event(self) -> None:
        payload = parse_structured_event(
            "BOOT_RECOVERY_STATUS",
            "Boot recovery status cause RECOVERY_WATCHDOG bootCount 4 consecutive 1 safeFallback False source WATCHDOG_EPS_BRIDGE level R6_OBC_REBOOT",
        )
        self.assertEqual(payload["resetCause"], "RECOVERY_WATCHDOG")
        self.assertEqual(payload["bootCount"], 4)
        self.assertFalse(payload["safeFallback"])
        self.assertTrue(structured_event_complete("BOOT_RECOVERY_STATUS", payload))

    def test_single_event_readback_records_completion_as_fallback_but_still_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                    "searchTimeoutSeconds": 0,
                },
                ensureAuth=True,
            )
            completion = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.cmdDisp.OpCodeCompleted",
                "eventName": "OpCodeCompleted",
                "message": "completed 0x100d0000",
                "raw": "raw",
                "structured": {"opcode": "0x100d0000"},
            }
            stale_event = {
                "timestamp": "2026-07-03T10:19:00.000000",
                "qualifiedName": "OBCApp.linuxWatchdogSink.HW_WATCHDOG_STATUS",
                "eventName": "HW_WATCHDOG_STATUS",
                "message": "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                "raw": "raw",
                "structured": parse_structured_event(
                    "HW_WATCHDOG_STATUS",
                    "Hardware watchdog status enabled False open False timeout 15 feedCount 0 lastError 0",
                ),
            }
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch.object(actions, "_native_command_completion", return_value=completion),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x100d0000"),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={
                        "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
                        "events": [stale_event],
                    },
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "produced no fresh events"):
                    actions._run_readback(request, manual_context, operation)
            assert operation.readback is not None
            self.assertEqual(operation.readback["events"], [])
            self.assertEqual(operation.readback["completionEvidence"], completion)
            self.assertEqual(
                operation.readback["fallbackEvidence"],
                {
                    "kind": "command-completion",
                    "source": "command-completion",
                    "events": [completion],
                },
            )

    def test_persistent_fault_history_group_accepts_empty_fresh_status(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            actions = GatewayActions(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            status_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS",
                "eventName": "PERSISTENT_FAULT_HISTORY_STATUS",
                "message": "Persistent fault history total 0 returned 0 activeCopy NONE generation 0",
                "raw": "raw",
            }
            readback = {"eventSource": "native-log"}
            freshness = {"freshEvents": [status_event]}
            actions._classify_multi_event_readback(
                "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                readback,
                freshness,
            )
            self.assertEqual(readback["mainEvidence"]["kind"], "fresh-event-group")
            self.assertEqual(readback["mainEvidence"]["recordEvents"], [])

    def test_persistent_fault_history_group_requires_matching_records(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            actions = GatewayActions(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            status_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS",
                "eventName": "PERSISTENT_FAULT_HISTORY_STATUS",
                "message": "Persistent fault history total 2 returned 2 activeCopy COPY_B generation 7",
                "raw": "raw",
            }
            record_event = {
                "timestamp": "2026-07-03T10:20:31.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD",
                "eventName": "PERSISTENT_FAULT_HISTORY_RECORD",
                "message": "Persistent fault[0] kind INCIDENT_OPENED source COMM_PRIMARY_UNAVAILABLE level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR boot 12 consecutive 3 uptime 44 time 1700 detail 99 flags 1",
                "raw": "raw",
            }
            readback = {"eventSource": "native-log"}
            freshness = {"freshEvents": [status_event, record_event]}
            actions._classify_multi_event_readback(
                "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                readback,
                freshness,
            )
            self.assertIsNone(readback.get("mainEvidence"))
            self.assertEqual(readback["incompleteResult"]["kind"], "fresh-event-group-incomplete")
            self.assertEqual(readback["incompleteResult"]["expectedRecordCount"], 2)
            self.assertEqual(readback["incompleteResult"]["observedRecordCount"], 1)

    def test_persistent_fault_history_group_accepts_complete_group(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            actions = GatewayActions(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            status_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS",
                "eventName": "PERSISTENT_FAULT_HISTORY_STATUS",
                "message": "Persistent fault history total 2 returned 2 activeCopy COPY_B generation 7",
                "raw": "raw",
            }
            record_zero = {
                "timestamp": "2026-07-03T10:20:31.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD",
                "eventName": "PERSISTENT_FAULT_HISTORY_RECORD",
                "message": "Persistent fault[0] kind INCIDENT_OPENED source COMM_PRIMARY_UNAVAILABLE level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR boot 12 consecutive 3 uptime 44 time 1700 detail 99 flags 1",
                "raw": "raw",
            }
            record_one = {
                "timestamp": "2026-07-03T10:20:32.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD",
                "eventName": "PERSISTENT_FAULT_HISTORY_RECORD",
                "message": "Persistent fault[1] kind BOOT_OBSERVED source COMM_PRIMARY_UNAVAILABLE level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR boot 11 consecutive 2 uptime 43 time 1690 detail 77 flags 0",
                "raw": "raw",
            }
            readback = {"eventSource": "native-log"}
            freshness = {"freshEvents": [status_event, record_zero, record_one]}
            actions._classify_multi_event_readback(
                "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                readback,
                freshness,
            )
            self.assertEqual(readback["mainEvidence"]["kind"], "fresh-event-group")
            self.assertEqual(len(readback["mainEvidence"]["recordEvents"]), 2)

    def test_persistent_fault_history_group_dedupes_identical_events(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            actions = GatewayActions(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            status_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS",
                "eventName": "PERSISTENT_FAULT_HISTORY_STATUS",
                "message": "Persistent fault history total 1 returned 1 activeCopy COPY_B generation 7",
                "raw": "raw",
            }
            record_event = {
                "timestamp": "2026-07-03T10:20:31.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD",
                "eventName": "PERSISTENT_FAULT_HISTORY_RECORD",
                "message": "Persistent fault[0] kind INCIDENT_OPENED source COMM_PRIMARY_UNAVAILABLE level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR boot 12 consecutive 3 uptime 44 time 1700 detail 99 flags 1",
                "raw": "raw",
            }
            readback = {"eventSource": "native-log"}
            freshness = {"freshEvents": [status_event, status_event.copy(), record_event, record_event.copy()]}
            actions._classify_multi_event_readback(
                "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                readback,
                freshness,
            )
            self.assertEqual(readback["mainEvidence"]["kind"], "fresh-event-group")
            self.assertEqual(readback["mainEvidence"]["statusEvent"]["eventName"], "PERSISTENT_FAULT_HISTORY_STATUS")
            self.assertEqual(len(readback["mainEvidence"]["recordEvents"]), 1)

    def test_persistent_fault_history_readback_collects_late_records_from_bounded_search(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "target-manual-ground-dual-gds": SurfaceContext(
                    context_id="target-manual-ground-dual-gds",
                    surface_type="target-manual-ground-dual-gds",
                    env_name="target",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="target-manual-ground-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
                    "commandArgs": ["4"],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                    "searchTimeoutSeconds": 1,
                },
                ensureAuth=True,
            )
            operation = OperationResult(status="running", startedAt="start")
            status_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_STATUS",
                "eventName": "PERSISTENT_FAULT_HISTORY_STATUS",
                "message": "Persistent fault history total 4 returned 4 activeCopy COPY_B generation 7",
                "raw": "raw",
            }
            record_events = [
                {
                    "timestamp": f"2026-07-03T10:20:3{index + 1}.000000",
                    "qualifiedName": "OBCApp.persistentFaultManager.PERSISTENT_FAULT_HISTORY_RECORD",
                    "eventName": "PERSISTENT_FAULT_HISTORY_RECORD",
                    "message": (
                        f"Persistent fault[{index}] kind INCIDENT_OPENED source COMM_PRIMARY_UNAVAILABLE "
                        f"level R6_OBC_REBOOT action OBC_REBOOT cause RECOVERY_COMM_FDIR "
                        f"boot {12 - index} consecutive {3 - index} uptime {44 - index} time {1700 - index} "
                        f"detail {99 - index} flags {index}"
                    ),
                    "raw": "raw",
                }
                for index in range(4)
            ]
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="target", band="sband", details={"secureSequence": 3}),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[status_event, *record_events[:2]]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=record_events[2:]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                result = actions._run_readback(request, manual_context, operation)
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["family"], "event-based")
            self.assertEqual(result.readback["mainEvidence"]["kind"], "fresh-event-group")
            self.assertEqual(len(result.readback["mainEvidence"]["recordEvents"]), 4)
            self.assertIsNone(result.readback["incompleteResult"])

    def test_single_event_readback_dedupes_identical_fresh_events(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            actions = GatewayActions(pathlib.Path(temp_dir), SurfaceRegistry(), SnapshotStore(pathlib.Path(temp_dir) / "cache"))
            readback = {"eventSource": "native-log"}
            event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.bootManager.BOOT_RECOVERY_STATUS",
                "eventName": "BOOT_RECOVERY_STATUS",
                "message": "Boot recovery status cause UNKNOWN bootCount 1 consecutive 0 safeFallback False source NONE level R0_RECORD_ONLY",
                "raw": "raw",
            }
            freshness = {"freshEvents": [event, event.copy()]}
            actions._classify_single_event_readback(readback, freshness)
            self.assertEqual(readback["mainEvidence"]["kind"], "fresh-event")
            self.assertEqual(len(readback["mainEvidence"]["events"]), 1)
            self.assertEqual(len(freshness["freshEvents"]), 1)

    def test_single_event_readback_accepts_fresh_ring_event_without_preparsed_structure(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            ring_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.recoveryExecutor.RECOVERY_STATUS",
                "eventName": "RECOVERY_STATUS",
                "message": "Recovery status activeCount 0 source NONE highest R0_RECORD_ONLY lastAction NONE pendingProcessRestart False pendingReboot False relatchCount 0",
                "raw": "raw",
            }
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [ring_event], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                result = actions._run_readback(request, manual_context, OperationResult(status="running", startedAt="start"))
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["mainEvidence"]["kind"], "fresh-event")
            self.assertEqual(result.readback["mainEvidence"]["events"][0]["structured"]["activeIncidentCount"], 0)

    def test_single_event_readback_marks_incomplete_payload(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            incomplete_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.recoveryExecutor.RECOVERY_STATUS",
                "eventName": "RECOVERY_STATUS",
                "message": "Recovery status activeCount 2 source ADCS_POLL_FRESHNESS highest R5_PROCESS_RESTART",
                "raw": "raw",
                "structured": parse_structured_event(
                    "RECOVERY_STATUS",
                    "Recovery status activeCount 2 source ADCS_POLL_FRESHNESS highest R5_PROCESS_RESTART",
                ),
            }
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1})),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[incomplete_event]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "incomplete fresh event payload"):
                    actions._run_readback(request, manual_context, operation)
            assert operation.readback is not None
            self.assertEqual(
                operation.readback["incompleteResult"],
                {
                    "kind": "fresh-event-incomplete",
                    "reason": "structured-event-missing-fields",
                    "source": "native-log",
                    "events": [incomplete_event],
                },
            )

    def test_payload_status_readback_requires_complete_fresh_event(self) -> None:
        self._assert_payload_single_event_readback_success(
            "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
            {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.payloadOpsController.PAYLOAD_STATUS",
                "eventName": "PAYLOAD_STATUS",
                "message": "Payload status state PSTATE_READY powered True prepared True result PRESULT_OK capture 1 index 16 preview persistent-data/payload/camera/PIC10.jpg previewDp data-products/demo.fdp previewPublished True rawPublished False",
                "raw": "raw",
            },
        )

    def test_payload_capabilities_readback_requires_complete_fresh_event(self) -> None:
        self._assert_payload_single_event_readback_success(
            "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
            {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.payloadOpsController.PAYLOAD_CAPABILITIES",
                "eventName": "PAYLOAD_CAPABILITIES",
                "message": "Payload capabilities backend libcamera supported 63 offOnly 15 autoMutable 0 deterministicMutable 48 raw False real True",
                "raw": "raw",
            },
        )

    def test_payload_capture_metadata_readback_requires_complete_fresh_event(self) -> None:
        self._assert_payload_single_event_readback_success(
            "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_METADATA",
                "eventName": "PAYLOAD_CAPTURE_METADATA",
                "message": "Payload capture metadata policy CAPTURE_AUTO capture 1 index 16 requested 0 applied 0 actualExp 23123 actualGain 187 awbValid True awbTempK 5032 awbRedX1000 1710 awbBlueX1000 1385 raw persistent-data/payload/camera/PIC10.bin preview persistent-data/payload/camera/PIC10.jpg previewDp data-products/demo.fdp rawDp  previewPublished True rawPublished False",
                "raw": "raw",
            },
        )

    def test_payload_single_event_readback_completion_fallback_does_not_succeed(self) -> None:
        self._assert_payload_single_event_completion_only_fails("OBCApp.payloadOpsController.PAYLOAD_GET_STATUS")

    def test_payload_single_event_readback_marks_incomplete_payload(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            incomplete_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.payloadOpsController.PAYLOAD_STATUS",
                "eventName": "PAYLOAD_STATUS",
                "message": "Payload status capture 1 index 16 preview persistent-data/payload/camera/PIC10.jpg",
                "raw": "raw",
                "structured": parse_structured_event(
                    "PAYLOAD_STATUS",
                    "Payload status capture 1 index 16 preview persistent-data/payload/camera/PIC10.jpg",
                ),
            }
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1}),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[incomplete_event]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "incomplete fresh event payload"):
                    actions._run_readback(request, manual_context, operation)
            assert operation.readback is not None
            self.assertEqual(operation.readback["incompleteResult"]["kind"], "fresh-event-incomplete")
            self.assertEqual(operation.readback["incompleteResult"]["events"], [incomplete_event])

    def test_payload_single_event_readback_keeps_completion_fallback_when_incomplete(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            incomplete_event = {
                "timestamp": "2026-07-03T10:20:30.000000",
                "qualifiedName": "OBCApp.payloadOpsController.PAYLOAD_STATUS",
                "eventName": "PAYLOAD_STATUS",
                "message": "Payload status capture 1 index 16 preview persistent-data/payload/camera/PIC10.jpg",
                "raw": "raw",
                "structured": parse_structured_event(
                    "PAYLOAD_STATUS",
                    "Payload status capture 1 index 16 preview persistent-data/payload/camera/PIC10.jpg",
                ),
            }
            completion = {
                "timestamp": "2026-07-03T10:20:31.000000",
                "qualifiedName": "CdhCore.cmdDisp.OpCodeCompleted",
                "eventName": "OpCodeCompleted",
                "message": "Opcode 0x1003a005 completed",
                "raw": "raw",
            }
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1}),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[incomplete_event]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x1003a005"),
                mock.patch.object(actions, "_native_command_completion", return_value=completion),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS", "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "incomplete fresh event payload"):
                    actions._run_readback(request, manual_context, operation)
            assert operation.readback is not None
            self.assertEqual(operation.readback["incompleteResult"]["kind"], "fresh-event-incomplete")
            self.assertEqual(
                operation.readback["fallbackEvidence"],
                {
                    "kind": "command-completion",
                    "source": "command-completion",
                    "events": [completion],
                },
            )

    def _assert_payload_single_event_readback_success(self, command_name: str, event: dict[str, Any]) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": command_name,
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1}),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[event]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": command_name, "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                result = actions._run_readback(request, manual_context, operation)
            self.assertEqual(result.status, "succeeded")
            assert result.readback is not None
            self.assertEqual(result.readback["family"], "event-based")
            self.assertEqual(result.readback["mainEvidence"]["kind"], "fresh-event")
            self.assertEqual(result.readback["mainEvidence"]["events"][0]["eventName"], event["eventName"])
            self.assertIsNone(result.readback["incompleteResult"])

    def _assert_payload_single_event_completion_only_fails(self, command_name: str) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=1,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={},
                    status={},
                    lifecycle_state="running",
                    owner_pid=1,
                    dictionary_path=dictionary,
                    bands={"sband": band},
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 1},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state.json",
            )
            request = OperationRequest(
                contextId="hosted-manual-dual-gds",
                band="sband",
                kind="readback",
                payload={
                    "commandName": command_name,
                    "commandArgs": [],
                    "settleSeconds": 0,
                    "nativeRetrySeconds": 0,
                },
                ensureAuth=True,
            )
            operation = OperationResult(status="running", startedAt="start")
            with (
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband", details={"secureSequence": 1}),
                ),
                mock.patch.object(actions, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(actions, "_native_event_search_many", return_value=[]),
                mock.patch.object(actions, "_bounded_event_search_many", return_value=[]),
                mock.patch.object(actions, "_arm_event_searches", return_value=[]),
                mock.patch.object(actions, "_fresh_reject_events", return_value=[]),
                mock.patch.object(actions, "_command_opcode_hex", return_value="0x1003a005"),
                mock.patch.object(
                    actions,
                    "_native_command_completion",
                    return_value={
                        "timestamp": "2026-07-03T10:20:31.000000",
                        "qualifiedName": "CdhCore.cmdDisp.OpCodeCompleted",
                        "eventName": "OpCodeCompleted",
                        "message": "Opcode 0x1003a005 completed",
                        "raw": "raw",
                    },
                ),
                mock.patch("mission_console.gateway.actions.time.sleep", return_value=None),
                mock.patch.object(
                    snapshots,
                    "structured_readback",
                    return_value={"commandName": command_name, "events": []},
                ),
                mock.patch.object(
                    snapshots,
                    "readback_freshness",
                    return_value={"freshEvents": [], "freshChannels": {}, "rejectEvents": []},
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "produced no fresh events"):
                    actions._run_readback(request, manual_context, operation)
            assert operation.readback is not None
            self.assertEqual(
                operation.readback["fallbackEvidence"],
                {
                    "kind": "command-completion",
                    "source": "command-completion",
                    "events": [
                        {
                            "timestamp": "2026-07-03T10:20:31.000000",
                            "qualifiedName": "CdhCore.cmdDisp.OpCodeCompleted",
                            "eventName": "OpCodeCompleted",
                            "message": "Opcode 0x1003a005 completed",
                            "raw": "raw",
                        }
                    ],
                },
            )
            self.assertIsNone(operation.readback["mainEvidence"])

    def test_target_auth_accepts_journal_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            manifest = {
                "surfaceRoot": str(runtime_root),
                "ownerPid": 55,
                "targetBaseline": {
                    "obcSshTarget": "operator@obc.local",
                    "obcCommServiceName": "obc-comm-csp-stack.service",
                },
            }
            context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest=manifest,
                surface={"captures": {"southboundToGds": str(runtime_root / "challenge.bin")}, "gdsTtsPort": 51901},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            challenge = SimpleNamespace(challenge=(b"\x01" * 16), packet_offset=12)
            wait_calls = [
                (challenge, "native-recv", (0, 1)),
                TimeoutError("status missing on capture"),
            ]
            with (
                mock.patch.object(secure_ops, "_load_handshake_messages_for_context", return_value={"wire-capture": [], "native-recv": []}),
                mock.patch.object(secure_ops, "_handshake_progress", return_value=(0, 0)),
                mock.patch.object(secure_ops, "_wait_for_handshake_from_sources", side_effect=wait_calls),
                mock.patch.object(secure_ops, "remote_journal_since_now", return_value="2026-06-20 12:00:00"),
                mock.patch.object(secure_ops, "wait_remote_journal_fragments", return_value=None),
                mock.patch.object(secure_ops, "ssh_capture", return_value="2026-06-20 12:00:00"),
                mock.patch.object(secure_ops, "send_tts_raw_packet", return_value=None),
                mock.patch.object(secure_ops, "load_command_auth_keystore", return_value=object()),
                mock.patch.object(secure_ops, "keystore_entry_for_service_id", return_value=SimpleNamespace(key_bytes=(b"\x02" * 32))),
                mock.patch.object(secure_ops, "derive_session_key", return_value=(b"\x03" * 32)),
                mock.patch.object(secure_ops, "compute_auth_response", return_value=(b"\x04" * 32)),
            ):
                result = secure_ops.establish_auth_result(context)
            self.assertEqual(result.status, "authenticated")
            self.assertEqual(result.details["challengeSource"], "native-recv")
            self.assertEqual(result.details["authStatusSource"], "target-journal")

    def test_target_auth_accepts_not_authenticated_wire_when_journal_confirms(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            manifest = {
                "surfaceRoot": str(runtime_root),
                "ownerPid": 55,
                "targetBaseline": {
                    "obcSshTarget": "operator@obc.local",
                    "obcCommServiceName": "obc-comm-csp-stack.service",
                },
            }
            context = secure_ops.ManualSurfaceContext(
                env="target",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest=manifest,
                surface={"captures": {"southboundToGds": str(runtime_root / "challenge.bin")}, "gdsTtsPort": 51901},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            challenge = SimpleNamespace(challenge=(b"\x01" * 16), packet_offset=12)
            not_authenticated = SimpleNamespace(status_code=2, packet_offset=18)
            wait_calls = [
                (challenge, "native-recv", (0, 1)),
                (not_authenticated, "wire-capture", (1, 1)),
            ]
            with (
                mock.patch.object(secure_ops, "_load_handshake_messages_for_context", return_value={"wire-capture": [], "native-recv": []}),
                mock.patch.object(secure_ops, "_handshake_progress", return_value=(0, 0)),
                mock.patch.object(secure_ops, "_wait_for_handshake_from_sources", side_effect=wait_calls),
                mock.patch.object(secure_ops, "remote_journal_since_now", return_value="2026-06-20 12:00:00"),
                mock.patch.object(secure_ops, "wait_remote_journal_fragments", return_value=None),
                mock.patch.object(secure_ops, "ssh_capture", return_value="2026-06-20 12:00:00"),
                mock.patch.object(secure_ops, "send_tts_raw_packet", return_value=None),
                mock.patch.object(secure_ops, "load_command_auth_keystore", return_value=object()),
                mock.patch.object(secure_ops, "keystore_entry_for_service_id", return_value=SimpleNamespace(key_bytes=(b"\x02" * 32))),
                mock.patch.object(secure_ops, "derive_session_key", return_value=(b"\x03" * 32)),
                mock.patch.object(secure_ops, "compute_auth_response", return_value=(b"\x04" * 32)),
            ):
                result = secure_ops.establish_auth_result(context)
            self.assertEqual(result.status, "authenticated")
            self.assertEqual(result.details["authStatusSource"], "target-journal")

    def test_hosted_auth_rejects_not_authenticated_status_without_journal(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 55},
                surface={"captures": {"southboundToGds": str(runtime_root / "challenge.bin")}, "gdsTtsPort": 51901},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state" / "sband.json",
            )
            challenge = SimpleNamespace(challenge=(b"\x01" * 16), packet_offset=12)
            not_authenticated = SimpleNamespace(status_code=2, packet_offset=18)
            wait_calls = [
                (challenge, "wire-capture", (1, 0)),
                (not_authenticated, "wire-capture", (2, 0)),
                (not_authenticated, "wire-capture", (3, 0)),
                (not_authenticated, "wire-capture", (4, 0)),
            ]
            fake_time = iter((0.0, 0.1, 0.2, 0.3, 0.4, 100.0, 100.0, 100.0))
            with (
                mock.patch.object(secure_ops, "_load_handshake_messages_for_context", return_value={"wire-capture": [], "native-recv": []}),
                mock.patch.object(secure_ops, "_handshake_progress", return_value=(0, 0)),
                mock.patch.object(secure_ops, "_wait_for_handshake_from_sources", side_effect=wait_calls),
                mock.patch.object(secure_ops, "send_tts_raw_packet", return_value=None),
                mock.patch.object(secure_ops, "load_command_auth_keystore", return_value=object()),
                mock.patch.object(secure_ops, "keystore_entry_for_service_id", return_value=SimpleNamespace(key_bytes=(b"\x02" * 32))),
                mock.patch.object(secure_ops, "derive_session_key", return_value=(b"\x03" * 32)),
                mock.patch.object(secure_ops, "compute_auth_response", return_value=(b"\x04" * 32)),
                mock.patch("manual_ops.manual_secure_ops.time.time", side_effect=lambda: next(fake_time)),
                mock.patch("manual_ops.manual_secure_ops.time.sleep", return_value=None),
            ):
                with self.assertRaises(RuntimeError):
                    secure_ops.establish_auth_result(context)

    def test_listener_manager_groups_alias_bands_under_one_handle(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            manager = ListenerManager(runtime_root / "listeners", snapshots)
            band = BandSurface(
                band="uhf-backup",
                manifest_key="uhf",
                surface_root=runtime_root / "surface",
                gds_tts_port=51911,
                dictionary_path=runtime_root / "dict.xml",
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=77,
            )
            context = SurfaceContext(
                context_id="target-manual-ground-dual-gds",
                surface_type="target-manual-ground-dual-gds",
                env_name="target",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 77},
                status={"ownerPid": 77},
                lifecycle_state="running",
                owner_pid=77,
                dictionary_path=runtime_root / "dict.xml",
                bands={
                    "uhf-backup": band,
                    "uhf-primary-after-failover": BandSurface(
                        band="uhf-primary-after-failover",
                        manifest_key="uhf",
                        surface_root=band.surface_root,
                        gds_tts_port=band.gds_tts_port,
                        dictionary_path=band.dictionary_path,
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        secure_state=None,
                        secure_state_path=runtime_root / "secure-state-primary.json",
                        owner_pid=77,
                    ),
                },
            )
            fake_process = SimpleNamespace(process=SimpleNamespace(poll=lambda: None))
            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch("mission_console.gateway.listeners.start_managed_process", return_value=fake_process),
            ):
                manager = ListenerManager(runtime_root / "listeners", snapshots)
                manager.sync_contexts({"target-manual-ground-dual-gds": context})
            self.assertEqual(len(manager._handles), 1)
            handle = next(iter(manager._handles.values()))
            self.assertEqual(handle.bands, ("uhf-backup", "uhf-primary-after-failover"))

    def test_listener_restart_clears_cached_snapshot_state(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            manager = ListenerManager(runtime_root / "listeners", snapshots)
            initial_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=runtime_root / "dict.xml",
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=11,
            )
            updated_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=runtime_root / "dict.xml",
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=22,
            )
            first_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands={"sband": initial_band},
            )
            second_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 22},
                status={"ownerPid": 22},
                lifecycle_state="running",
                owner_pid=22,
                dictionary_path=runtime_root / "dict.xml",
                bands={"sband": updated_band},
            )
            event = parse_event_line(
                "2026-06-20T16:02:35.007853: OBCApp.cmdSeq.COMMAND_AUTHORITY_REJECTED : stale session"
            )
            channel = parse_channel_line(
                "2026-06-20T15:31:25.232548,(2(0)-1781940685:232548),OBCApp.modeManager.SYS_UPTIME_SEC,268632065,8"
            )
            assert event is not None
            assert channel is not None
            snapshots.append_event("hosted-manual-dual-gds", "sband", event)
            snapshots.update_channel("hosted-manual-dual-gds", "sband", channel)
            snapshots.set_readback_cache(
                "hosted-manual-dual-gds",
                "sband",
                "OBCApp.modeManager.MODE_GET",
                {"status": "stale"},
            )
            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch("mission_console.gateway.listeners.start_managed_process", return_value=SimpleNamespace(process=SimpleNamespace(poll=lambda: None))),
                mock.patch("mission_console.gateway.listeners.cleanup_managed_processes", return_value=None),
            ):
                manager.sync_contexts({"hosted-manual-dual-gds": first_context})
                manager.sync_contexts({"hosted-manual-dual-gds": second_context})
            self.assertEqual(snapshots.channel_map("hosted-manual-dual-gds", "sband"), {})
            self.assertEqual(snapshots.recent_events("hosted-manual-dual-gds", "sband"), [])
            self.assertEqual(
                snapshots.readback_cache(),
                {
                    "hosted-manual-dual-gds:sband:OBCApp.modeManager.MODE_GET": {"status": "stale"},
                },
            )

    def test_listener_restart_starts_from_existing_native_log_tail(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=11,
            )
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=dictionary,
                bands={"sband": band},
            )
            native_root = runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events"
            native_root.mkdir(parents=True, exist_ok=True)
            stale_event_log = native_root / "event.log"
            stale_channel_log = native_root / "channel.log"
            stale_event_log.write_text("old-event\n", encoding="utf-8")
            stale_channel_log.write_text("old-channel\n", encoding="utf-8")
            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch(
                    "mission_console.gateway.listeners.start_managed_process",
                    return_value=SimpleNamespace(process=SimpleNamespace(poll=lambda: None)),
                ),
            ):
                manager = ListenerManager(runtime_root / "listeners", snapshots)
                manager.sync_contexts({"hosted-manual-dual-gds": context})
            handle = next(iter(manager._handles.values()))
            self.assertEqual(handle.events_offset, len("old-event\n"))
            self.assertEqual(handle.channels_offset, len("old-channel\n"))

    def test_listener_manager_restarts_dead_child_without_surface_drift(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            manager = ListenerManager(runtime_root / "listeners", snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=11,
            )
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=dictionary,
                bands={"sband": band},
            )
            first_events = SimpleNamespace(process=mock.Mock())
            first_events.process.poll.return_value = None
            first_channels = SimpleNamespace(process=mock.Mock())
            first_channels.process.poll.return_value = None
            replacement_events = SimpleNamespace(process=mock.Mock())
            replacement_events.process.poll.return_value = None
            replacement_channels = SimpleNamespace(process=mock.Mock())
            replacement_channels.process.poll.return_value = None
            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch(
                    "mission_console.gateway.listeners.start_managed_process",
                    side_effect=[first_events, first_channels, replacement_events, replacement_channels],
                ) as start_mock,
                mock.patch("mission_console.gateway.listeners.cleanup_managed_processes", return_value=None) as cleanup_mock,
            ):
                manager.sync_contexts({"hosted-manual-dual-gds": context})
                first_events.process.poll.return_value = 1
                manager.sync_contexts({"hosted-manual-dual-gds": context})
            self.assertEqual(start_mock.call_count, 4)
            self.assertEqual(cleanup_mock.call_count, 1)

    def test_listener_stale_reap_matches_native_log_root_only(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            manager = ListenerManager(runtime_root / "listeners", snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=11,
            )
            context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=dictionary,
                bands={"sband": band},
            )
            start_calls: list[dict[str, object]] = []

            def fake_start(name: str, args: object, log_path: object, **kwargs: object) -> SimpleNamespace:
                start_calls.append(
                    {
                        "name": name,
                        "args": args,
                        "log_path": log_path,
                        "stale_match_groups": kwargs.get("stale_match_groups"),
                    }
                )
                return SimpleNamespace(process=SimpleNamespace(poll=lambda: None))

            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch("mission_console.gateway.listeners.start_managed_process", side_effect=fake_start),
            ):
                manager.sync_contexts({"hosted-manual-dual-gds": context})

            self.assertEqual(len(start_calls), 2)
            expected_root = str(runtime_root / "listeners" / "hosted-manual-dual-gds" / "sband" / "native-events")
            event_call = next(call for call in start_calls if call["name"] == "hosted-manual-dual-gds-sband-events")
            channel_call = next(call for call in start_calls if call["name"] == "hosted-manual-dual-gds-sband-channels")
            self.assertEqual(
                event_call["stale_match_groups"],
                (("events", "--tts-port 50151", expected_root),),
            )
            self.assertEqual(
                channel_call["stale_match_groups"],
                (("channels", "--tts-port 50151", expected_root),),
            )

    def test_listener_manager_stops_drifted_handle_before_starting_replacement(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            manager = ListenerManager(runtime_root / "listeners", snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            first_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=11,
            )
            second_band = BandSurface(
                band="sband",
                manifest_key="sband",
                surface_root=runtime_root / "surface",
                gds_tts_port=50151,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=22,
            )
            first_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=dictionary,
                bands={"sband": first_band},
            )
            second_context = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root / "surface",
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 22},
                status={"ownerPid": 22},
                lifecycle_state="running",
                owner_pid=22,
                dictionary_path=dictionary,
                bands={"sband": second_band},
            )
            order: list[str] = []

            def fake_process(name: str) -> SimpleNamespace:
                return SimpleNamespace(
                    name=name,
                    process=SimpleNamespace(poll=lambda: None),
                    stale_match_groups=(),
                    stale_match_markers=(),
                    stale_require_orphan=False,
                    handle=SimpleNamespace(close=lambda: None),
                )

            def start_side_effect(name: str, *args: object, **kwargs: object) -> SimpleNamespace:
                order.append(f"start:{name}")
                return fake_process(name)

            def cleanup_side_effect(_processes: list[object], *, timeout_sec: float = 5.0) -> None:
                order.append("cleanup")

            with (
                mock.patch("mission_console.gateway.listeners.find_local_tool", return_value="fprime-cli"),
                mock.patch("mission_console.gateway.listeners.start_managed_process", side_effect=start_side_effect),
                mock.patch("mission_console.gateway.listeners.cleanup_managed_processes", side_effect=cleanup_side_effect),
            ):
                manager.sync_contexts({"hosted-manual-dual-gds": first_context})
                order.append("second-sync")
                manager.sync_contexts({"hosted-manual-dual-gds": second_context})
            second_sync_order = order[order.index("second-sync") + 1 :]
            self.assertEqual(
                second_sync_order[:3],
                [
                    "cleanup",
                    "start:hosted-manual-dual-gds-sband-events",
                    "start:hosted-manual-dual-gds-sband-channels",
                ],
            )

    def test_registry_marks_owner_pid_change_as_listener_drift(self) -> None:
        registry = SurfaceRegistry()
        first_band = BandSurface(
            band="sband",
            manifest_key="sband",
            surface_root=pathlib.Path("/tmp/surface"),
            gds_tts_port=50151,
            dictionary_path=pathlib.Path("/tmp/dict.json"),
            gui_url=None,
            captures={},
            logs={},
            southbound={},
            secure_state=None,
            secure_state_path=pathlib.Path("/tmp/secure.json"),
            owner_pid=11,
        )
        second_band = BandSurface(
            band="sband",
            manifest_key="sband",
            surface_root=pathlib.Path("/tmp/surface"),
            gds_tts_port=50151,
            dictionary_path=pathlib.Path("/tmp/dict.json"),
            gui_url=None,
            captures={},
            logs={},
            southbound={},
            secure_state=None,
            secure_state_path=pathlib.Path("/tmp/secure.json"),
            owner_pid=22,
        )
        first_context = SurfaceContext(
            context_id="hosted-manual-dual-gds",
            surface_type="hosted-manual-dual-gds",
            env_name="hosted",
            root=pathlib.Path("/tmp/surface"),
            manifest_path=pathlib.Path("/tmp/manifest.json"),
            status_path=pathlib.Path("/tmp/status.json"),
            manifest={"ownerPid": 11},
            status={"ownerPid": 11},
            lifecycle_state="running",
            owner_pid=11,
            dictionary_path=pathlib.Path("/tmp/dict.json"),
            bands={"sband": first_band},
        )
        second_context = SurfaceContext(
            context_id="hosted-manual-dual-gds",
            surface_type="hosted-manual-dual-gds",
            env_name="hosted",
            root=pathlib.Path("/tmp/surface"),
            manifest_path=pathlib.Path("/tmp/manifest.json"),
            status_path=pathlib.Path("/tmp/status.json"),
            manifest={"ownerPid": 22},
            status={"ownerPid": 22},
            lifecycle_state="running",
            owner_pid=22,
            dictionary_path=pathlib.Path("/tmp/dict.json"),
            bands={"sband": second_band},
        )
        target_context = SurfaceContext(
            context_id="target-manual-ground-dual-gds",
            surface_type="target-manual-ground-dual-gds",
            env_name="target",
            root=pathlib.Path("/tmp/target"),
            manifest_path=pathlib.Path("/tmp/target-manifest.json"),
            status_path=pathlib.Path("/tmp/target-status.json"),
            manifest=None,
            status=None,
            lifecycle_state="stopped",
            owner_pid=None,
            dictionary_path=None,
            bands={},
        )
        with mock.patch(
            "mission_console.gateway.registry.discover_context",
            side_effect=[first_context, target_context, second_context, target_context],
        ):
            registry.refresh()
            registry.refresh()
        self.assertEqual(registry.drift_reason("hosted-manual-dual-gds", "sband"), "listener-key-changed")

    def test_establish_auth_uses_alias_listener_root_for_native_recv_bin(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            actions = GatewayActions(runtime_root, registry, snapshots)
            dictionary = runtime_root / "dict.xml"
            dictionary.write_text("<dict />", encoding="utf-8")
            band = BandSurface(
                band="uhf-backup",
                manifest_key="uhf",
                surface_root=runtime_root / "surface",
                gds_tts_port=51911,
                dictionary_path=dictionary,
                gui_url=None,
                captures={},
                logs={},
                southbound={},
                secure_state=None,
                secure_state_path=runtime_root / "secure-state.json",
                owner_pid=77,
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root / "surface",
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={"ownerPid": 77},
                    status={"ownerPid": 77},
                    lifecycle_state="running",
                    owner_pid=77,
                    dictionary_path=dictionary,
                    bands={
                        "uhf-backup": band,
                        "uhf-primary-after-failover": BandSurface(
                            band="uhf-primary-after-failover",
                            manifest_key="uhf",
                            surface_root=band.surface_root,
                            gds_tts_port=band.gds_tts_port,
                            dictionary_path=band.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=runtime_root / "secure-state-primary.json",
                            owner_pid=77,
                        ),
                    },
                )
            }
            manual_context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="uhf-primary-after-failover",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root / "surface"), "ownerPid": 77},
                surface={},
                service_id=2,
                dictionary_path=dictionary,
                secure_state_path=runtime_root / "secure-state-primary.json",
            )
            expected = runtime_root / "listeners" / "hosted-manual-dual-gds" / "uhf-backup" / "native-events" / "recv.bin"
            with mock.patch.object(
                secure_ops,
                "establish_auth_result",
                return_value=secure_ops.ManualActionResult(status="authenticated", env="hosted", band="uhf-primary-after-failover"),
            ) as establish_mock:
                actions._establish_auth("hosted-manual-dual-gds", "uhf-primary-after-failover", manual_context)
            self.assertEqual(establish_mock.call_args.kwargs["native_packet_log"], expected)

    def test_duplicate_sequence_is_primed_before_building_negative_packet(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                surface={},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            states = [
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=1,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="sband-primary",
                    session_key_hex="11" * 32,
                    manifest_owner_pid=11,
                ),
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=2,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="sband-primary",
                    session_key_hex="11" * 32,
                    manifest_owner_pid=11,
                ),
            ]
            with (
                mock.patch.object(secure_ops, "require_active_state", side_effect=states),
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband")) as send_mock,
                mock.patch.object(service, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(service, "_wait_for_primed_sequence_acceptance", return_value=True),
                mock.patch.object(secure_ops, "load_command_context", return_value=(mock.Mock(), mock.Mock())),
                mock.patch.object(secure_ops, "encode_inner_command", return_value=b"\x00\x00\x10\x03\x00\x01"),
                mock.patch("mission_console.gateway.packet_lab.build_secure_command_v2_packet", return_value=b"packet") as build_mock,
            ):
                service._prime_duplicate_sequence("hosted-manual-dual-gds", "sband", context)
                payload, source = service._build_case_payload(context, "duplicate-sequence")
            self.assertEqual(payload, b"packet")
            self.assertEqual(source, "live-crafted")
            send_mock.assert_called_once()
            self.assertEqual(build_mock.call_args.args[2], 1)

    def test_tampered_sequence_primes_and_reuses_non_increasing_sequence(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                surface={},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            states = [
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=1,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="sband-primary",
                    session_key_hex="11" * 32,
                    manifest_owner_pid=11,
                ),
                secure_ops.SecureSessionState(
                    service_id=1,
                    active_band="sband",
                    next_secure_sequence=2,
                    last_auth_time=time.time(),
                    manifest_path=str(context.manifest_path),
                    authority_mode="sband-primary",
                    session_key_hex="11" * 32,
                    manifest_owner_pid=11,
                ),
            ]
            with (
                mock.patch.object(secure_ops, "require_active_state", side_effect=states),
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband")) as send_mock,
                mock.patch.object(service, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(service, "_wait_for_primed_sequence_acceptance", return_value=True),
                mock.patch.object(secure_ops, "load_command_context", return_value=(mock.Mock(), mock.Mock())),
                mock.patch.object(secure_ops, "encode_inner_command", return_value=b"\x00\x00\x10\x03\x00\x01"),
                mock.patch("mission_console.gateway.packet_lab.build_secure_command_v2_packet", return_value=b"packet") as build_mock,
            ):
                service._prime_duplicate_sequence("hosted-manual-dual-gds", "sband", context)
                payload, source = service._build_case_payload(context, "tampered-sequence")
            self.assertEqual(payload, b"packet")
            self.assertEqual(source, "live-crafted")
            send_mock.assert_called_once()
            self.assertEqual(build_mock.call_args.args[2], 1)

    def test_duplicate_sequence_priming_requires_acceptance_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                surface={},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=1,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="11" * 32,
                manifest_owner_pid=11,
            )
            with (
                mock.patch.object(secure_ops, "require_active_state", return_value=state),
                mock.patch.object(secure_ops, "send_command_result", return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband")),
                mock.patch.object(service, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(service, "_wait_for_primed_sequence_acceptance", return_value=False),
            ):
                with self.assertRaisesRegex(RuntimeError, "accepted-sequence evidence"):
                    service._prime_duplicate_sequence("hosted-manual-dual-gds", "sband", context)

    def test_duplicate_sequence_priming_invalidates_session_when_evidence_missing(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            secure_state_dir = runtime_root / "secure-state"
            secure_state_dir.mkdir(parents=True)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={
                    "surfaceRoot": str(runtime_root),
                    "ownerPid": 11,
                    "operatorSurfaces": {"sband": {"canonicalBands": ["sband"]}},
                },
                surface={},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=secure_state_dir / "sband.json",
            )
            primed_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=2,
                last_auth_time=time.time(),
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="11" * 32,
                manifest_owner_pid=11,
            )
            secure_ops.save_state(context.manifest, "sband", primed_state)
            original_state = secure_ops.SecureSessionState(
                service_id=1,
                active_band="sband",
                next_secure_sequence=1,
                last_auth_time=primed_state.last_auth_time,
                manifest_path=str(context.manifest_path),
                authority_mode="sband-primary",
                session_key_hex="11" * 32,
                manifest_owner_pid=11,
            )
            with (
                mock.patch.object(secure_ops, "require_active_state", return_value=original_state),
                mock.patch.object(
                    secure_ops,
                    "send_command_result",
                    return_value=secure_ops.ManualActionResult(status="sent", env="hosted", band="sband"),
                ),
                mock.patch.object(service, "_listener_log_marker", return_value={"eventOffset": 0, "channelOffset": 0}),
                mock.patch.object(service, "_wait_for_primed_sequence_acceptance", return_value=False),
            ):
                with self.assertRaisesRegex(RuntimeError, "accepted-sequence evidence"):
                    service._prime_duplicate_sequence("hosted-manual-dual-gds", "sband", context)
            state_after = secure_ops.load_state(context.manifest, "sband")
            self.assertIsNotNone(state_after)
            assert state_after is not None
            self.assertTrue(state_after.invalidated)
            self.assertEqual(state_after.invalidation_reason, "packet-lab-priming-evidence-missing")
            self.assertEqual(state_after.next_secure_sequence, 2)

    def test_primed_sequence_acceptance_requires_sequence_value_to_advance(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            with (
                mock.patch.object(service, "_native_event_search_many", return_value=[]),
                mock.patch.object(service, "_native_channel_search_many", return_value={}),
                mock.patch.object(
                    service.snapshots,
                    "channel_map",
                    return_value={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "0", "generation": 2}},
                ),
            ):
                accepted = service._has_primed_sequence_acceptance(
                    context_id="hosted-manual-dual-gds",
                    band="sband",
                    before_channels={"SESSION_LAST_ACCEPTED_SEQUENCE": {"value": "0", "generation": 1}},
                    listener_marker={"eventOffset": 0, "channelOffset": 0},
                    expected_accepted_sequence=1,
                    expected_command_opcode="0x10038000",
                )
            self.assertFalse(accepted)

    def test_primed_sequence_acceptance_ignores_foreign_completion_opcode(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            service = PacketLabService(runtime_root, SurfaceRegistry(), SnapshotStore(runtime_root / "cache"))
            with (
                mock.patch.object(
                    service,
                    "_native_event_search_many",
                    return_value=[
                        {
                            "eventName": "OpCodeCompleted",
                            "message": "Opcode OPCODE 0x20000001 completed",
                        }
                    ],
                ),
                mock.patch.object(service, "_native_channel_search_many", return_value={}),
                mock.patch.object(service.snapshots, "channel_map", return_value={}),
            ):
                accepted = service._has_primed_sequence_acceptance(
                    context_id="hosted-manual-dual-gds",
                    band="sband",
                    before_channels={},
                    listener_marker={"eventOffset": 0, "channelOffset": 0},
                    expected_accepted_sequence=1,
                    expected_command_opcode="0x10038000",
                )
            self.assertFalse(accepted)

    def test_inject_tampered_sequence_primes_before_evidence_baseline(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            service = PacketLabService(runtime_root, registry, snapshots)
            context = secure_ops.ManualSurfaceContext(
                env="hosted",
                band="sband",
                manifest_path=runtime_root / "manifest.json",
                manifest={"surfaceRoot": str(runtime_root), "ownerPid": 11},
                surface={"gdsTtsPort": 50151},
                service_id=1,
                dictionary_path=runtime_root / "dict.xml",
                secure_state_path=runtime_root / "secure-state.json",
            )
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=context.manifest_path,
                    status_path=runtime_root / "status.json",
                    manifest=context.manifest,
                    status=context.manifest,
                    lifecycle_state="running",
                    owner_pid=11,
                    dictionary_path=context.dictionary_path,
                    bands={
                        "sband": BandSurface(
                            band="sband",
                            manifest_key="sband",
                            surface_root=runtime_root,
                            gds_tts_port=50151,
                            dictionary_path=context.dictionary_path,
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=context.secure_state_path,
                            owner_pid=11,
                        )
                    },
                )
            }
            order: list[str] = []

            def record_recent_events(*args: object, **kwargs: object) -> list[dict[str, object]]:
                order.append("recent_events")
                return []

            def record_channel_map(*args: object, **kwargs: object) -> dict[str, dict[str, object]]:
                order.append("channel_map")
                return {}

            with (
                mock.patch.object(registry, "refresh", return_value=registry._contexts),
                mock.patch.object(secure_ops, "resolve_context", return_value=context),
                mock.patch.object(service, "_prime_duplicate_sequence", side_effect=lambda *args: order.append("prime")),
                mock.patch.object(snapshots, "recent_events", side_effect=record_recent_events),
                mock.patch.object(snapshots, "channel_map", side_effect=record_channel_map),
                mock.patch.object(service, "_build_case_payload", return_value=(b"packet", "live-crafted")),
                mock.patch.object(service, "_classify_observed_result", return_value={"kind": "bounded-no-op"}),
                mock.patch("mission_console.gateway.packet_lab.parse_packet_summary", return_value={"secureHeader": {"secureSequence": 1}}),
                mock.patch("mission_console.gateway.packet_lab.send_tts_raw_packet"),
                mock.patch("mission_console.gateway.packet_lab.time.sleep", return_value=None),
                mock.patch.object(snapshots, "record_packet_lab"),
            ):
                payload = service.inject(context_id="hosted-manual-dual-gds", band="sband", case="tampered-sequence")

            self.assertEqual(payload["case"], "tampered-sequence")
            self.assertEqual(order[:3], ["prime", "recent_events", "channel_map"])

    def test_packet_lab_rejects_stopped_context_before_resolve_or_send(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            service = PacketLabService(runtime_root, registry, snapshots)
            registry._contexts = {
                "hosted-manual-dual-gds": SurfaceContext(
                    context_id="hosted-manual-dual-gds",
                    surface_type="hosted-manual-dual-gds",
                    env_name="hosted",
                    root=runtime_root,
                    manifest_path=runtime_root / "manifest.json",
                    status_path=runtime_root / "status.json",
                    manifest={"ownerPid": 11},
                    status={"ownerPid": 11},
                    lifecycle_state="stopped",
                    owner_pid=11,
                    dictionary_path=runtime_root / "dict.xml",
                    bands={
                        "sband": BandSurface(
                            band="sband",
                            manifest_key="sband",
                            surface_root=runtime_root,
                            gds_tts_port=50151,
                            dictionary_path=runtime_root / "dict.xml",
                            gui_url=None,
                            captures={},
                            logs={},
                            southbound={},
                            secure_state=None,
                            secure_state_path=runtime_root / "secure-state.json",
                            owner_pid=11,
                        )
                    },
                )
            }
            with (
                mock.patch.object(registry, "refresh", return_value=registry._contexts) as refresh_mock,
                mock.patch.object(secure_ops, "resolve_context") as resolve_mock,
                mock.patch("mission_console.gateway.packet_lab.send_tts_raw_packet") as send_mock,
                mock.patch.object(snapshots, "record_packet_lab") as record_mock,
            ):
                with self.assertRaisesRegex(RuntimeError, "is not running"):
                    service.inject(context_id="hosted-manual-dual-gds", band="sband", case="tampered-mac")
            refresh_mock.assert_called_once()
            resolve_mock.assert_not_called()
            send_mock.assert_not_called()
            record_mock.assert_called_once()
            failure_payload = record_mock.call_args.args[0]
            self.assertEqual(failure_payload["status"], "failed")
            self.assertEqual(failure_payload["case"], "tampered-mac")
            self.assertIn("is not running", failure_payload["error"])

    def test_packet_lab_refreshes_context_before_send(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            runtime_root = pathlib.Path(temp_dir)
            snapshots = SnapshotStore(runtime_root / "cache")
            registry = SurfaceRegistry()
            service = PacketLabService(runtime_root, registry, snapshots)
            cached_running = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root,
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="running",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands={
                    "sband": BandSurface(
                        band="sband",
                        manifest_key="sband",
                        surface_root=runtime_root,
                        gds_tts_port=50151,
                        dictionary_path=runtime_root / "dict.xml",
                        gui_url=None,
                        captures={},
                        logs={},
                        southbound={},
                        secure_state=None,
                        secure_state_path=runtime_root / "secure-state.json",
                        owner_pid=11,
                    )
                },
            )
            refreshed_stopped = SurfaceContext(
                context_id="hosted-manual-dual-gds",
                surface_type="hosted-manual-dual-gds",
                env_name="hosted",
                root=runtime_root,
                manifest_path=runtime_root / "manifest.json",
                status_path=runtime_root / "status.json",
                manifest={"ownerPid": 11},
                status={"ownerPid": 11},
                lifecycle_state="stopped",
                owner_pid=11,
                dictionary_path=runtime_root / "dict.xml",
                bands=cached_running.bands,
            )
            registry._contexts = {"hosted-manual-dual-gds": cached_running}
            with (
                mock.patch.object(registry, "refresh", return_value={"hosted-manual-dual-gds": refreshed_stopped}) as refresh_mock,
                mock.patch.object(secure_ops, "resolve_context") as resolve_mock,
                mock.patch("mission_console.gateway.packet_lab.send_tts_raw_packet") as send_mock,
                mock.patch.object(snapshots, "record_packet_lab") as record_mock,
            ):
                with self.assertRaisesRegex(RuntimeError, "is not running"):
                    service.inject(context_id="hosted-manual-dual-gds", band="sband", case="tampered-mac")
            refresh_mock.assert_called_once()
            resolve_mock.assert_not_called()
            send_mock.assert_not_called()
            record_mock.assert_called_once()

    def test_upload_worker_reads_result_with_bounded_get_after_join(self) -> None:
        class FakeQueue:
            def __init__(self) -> None:
                self.get_timeout: float | None = None
                self.empty_calls = 0

            def empty(self) -> bool:
                self.empty_calls += 1
                return True

            def get(self, timeout: float | None = None) -> dict[str, Any]:
                self.get_timeout = timeout
                return {
                    "ok": True,
                    "result": {
                        "status": "uploaded",
                        "env": "hosted",
                        "band": "sband",
                        "timestamp": "2026-06-20T20:00:00Z",
                        "leaf": "demo.bin",
                    },
                }

        class FakeProcess:
            exitcode = 0

            def start(self) -> None:
                return None

            def join(self, timeout: float | None = None) -> None:
                return None

            def is_alive(self) -> bool:
                return False

            def terminate(self) -> None:
                raise AssertionError("terminate should not be called for a completed worker")

        class FakeContext:
            def __init__(self) -> None:
                self.queue = FakeQueue()
                self.process = FakeProcess()

            def Queue(self) -> FakeQueue:
                return self.queue

            def Process(self, target: Any, args: tuple[Any, ...]) -> FakeProcess:
                return self.process

        fake_context = FakeContext()
        manual_context = secure_ops.ManualSurfaceContext(
            env="hosted",
            band="sband",
            manifest_path=pathlib.Path("/tmp/manifest.json"),
            manifest={"surfaceRoot": "/tmp", "ownerPid": 11},
            surface={"gdsTtsPort": 50151},
            service_id=1,
            dictionary_path=pathlib.Path("/tmp/dict.xml"),
            secure_state_path=pathlib.Path("/tmp/secure-state.json"),
        )
        with mock.patch("mission_console.gateway.actions.get_context", return_value=fake_context):
            result = _run_upload_file_in_subprocess(
                manual_context,
                local_path="/tmp/demo.bin",
                destination_leaf="demo.bin",
            )
        self.assertEqual(result.status, "uploaded")
        self.assertEqual(result.details["leaf"], "demo.bin")
        self.assertEqual(fake_context.queue.get_timeout, 5.0)
        self.assertEqual(fake_context.queue.empty_calls, 0)


if __name__ == "__main__":
    unittest.main()
