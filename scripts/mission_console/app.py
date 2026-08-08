#!/usr/bin/env python3
from __future__ import annotations

import atexit
import os
import pathlib
import signal
import sys
import threading
import time
import traceback
import uuid
from concurrent.futures import Future, ThreadPoolExecutor
from typing import Any

from flask import Flask, jsonify, render_template, request

ROOT_DIR = pathlib.Path(__file__).resolve().parents[2]
VENV_PYTHON = ROOT_DIR / "fprime-venv" / "bin" / "python"
if __name__ == "__main__" and VENV_PYTHON.exists() and pathlib.Path(sys.executable).resolve() != VENV_PYTHON.resolve():
    os.execv(str(VENV_PYTHON), [str(VENV_PYTHON), __file__, *sys.argv[1:]])

SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from manual_ops.lib.common import ensure_dir  # noqa: E402
from mission_console.gateway.actions import GatewayActions, OperationRequest  # noqa: E402
from mission_console.gateway.beacon import BeaconManager  # noqa: E402
from mission_console.gateway.catalog import HIDDEN_GROUPS, VISIBLE_GROUPS, load_command_catalog  # noqa: E402
from mission_console.gateway.listeners import ListenerManager  # noqa: E402
from mission_console.gateway.locks import BandLockPool  # noqa: E402
from mission_console.gateway.packet_lab import PACKET_CASES, PacketLabService  # noqa: E402
from mission_console.gateway.parsers import (  # noqa: E402
    CHANNEL_REFRESH_COMMANDS,
    EVENT_BASED_COMMANDS,
    LIVE_TREND_GROUPS,
    READBACK_VIEWER_TABS,
)
from mission_console.gateway.registry import SUPPORTED_CONTEXT_IDS, SurfaceRegistry  # noqa: E402
from mission_console.gateway.sequence_authoring import SequenceAuthoringService  # noqa: E402
from mission_console.gateway.snapshots import SnapshotStore  # noqa: E402


READBACK_GROUPS: dict[str, list[dict[str, str]]] = {
    "OBC": [
        {"commandName": "OBCApp.modeManager.MODE_GET", "description": "Mission mode and core mode telemetry."},
        {"commandName": "OBCApp.ttcPassManager.TTC_GET_STATUS", "description": "TTC policy and pass-window state."},
    ],
    "Safety & Recovery": [
        {"commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS", "description": "Recovery executor summary."},
        {"commandName": "OBCApp.bootManager.BOOT_STATUS", "description": "Boot status and recovery evidence."},
        {"commandName": "OBCApp.bootManager.GET_RESET_CAUSE", "description": "Reset-cause evidence."},
        {"commandName": "OBCApp.bootManager.GET_BOOT_COUNT", "description": "Boot-count evidence."},
        {"commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS", "description": "Hardware watchdog readback."},
        {
            "commandName": "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
            "description": "Persistent fault history records.",
        },
    ],
    "Payload": [
        {"commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS", "description": "Payload state and last result."},
        {
            "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
            "description": "Payload capability report.",
        },
        {
            "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            "description": "Last payload capture metadata.",
        },
    ],
    "Sequence": [
        {
            "commandName": "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS",
            "description": "Sequence contexts and latest sequence result.",
        },
    ],
    "Subsystems": [
        {"commandName": "OBCApp.commController.COMM_GET_STATUS", "description": "Comm routing and availability snapshot."},
        {"commandName": "OBCApp.epsBridge.EPS_GET_STATUS", "description": "EPS telemetry snapshot."},
        {"commandName": "OBCApp.adcsBridge.ADCS_GET_ATTITUDE", "description": "ADCS mode and attitude snapshot."},
        {"commandName": "OBCApp.gpsBridge.GPS_GET_STATE", "description": "GPS fix and state summary."},
        {"commandName": "OBCApp.storageHealthBridge.STORAGE_GET_STATUS", "description": "Storage health snapshot."},
    ],
}

SEQUENCE_ACTION_FIELDS: dict[str, list[str]] = {
    "validate": ["sequencePath"],
    "prepare-manual": ["sequencePath"],
    "run": ["sequencePath", "runMode"],
    "start": ["sequenceContextId"],
    "step": ["sequenceContextId"],
    "cancel": ["sequenceContextId"],
}


class MissionConsoleRuntime:
    def __init__(self, runtime_root: pathlib.Path) -> None:
        self.runtime_root = ensure_dir(runtime_root)
        self.console_session_id = f"console-{uuid.uuid4().hex[:12]}"
        self.registry = SurfaceRegistry()
        trend_history_size = int(os.environ.get("MISSION_CONSOLE_TREND_HISTORY_SIZE", "300"))
        self.snapshots = SnapshotStore(
            self.runtime_root / "cache",
            trend_history_size=trend_history_size,
            console_session_id=self.console_session_id,
        )
        self.lock_pool = BandLockPool()
        self.listeners = ListenerManager(self.runtime_root / "listeners", self.snapshots)
        self.actions = GatewayActions(self.runtime_root, self.registry, self.snapshots, lock_pool=self.lock_pool)
        self.beacons = BeaconManager(self.snapshots)
        self.packet_lab = PacketLabService(self.runtime_root, self.registry, self.snapshots, lock_pool=self.lock_pool)
        self.sequence_authoring = SequenceAuthoringService(self.runtime_root)
        self.executor = ThreadPoolExecutor(max_workers=4, thread_name_prefix="mission-console")
        self.jobs: dict[str, Future[Any]] = {}
        self._jobs_lock = threading.Lock()
        self._stop = threading.Event()
        self._shutdown_once = threading.Event()
        self._thread = threading.Thread(target=self._background_loop, name="mission-console-refresh", daemon=True)
        self.registry.refresh()
        self._thread.start()

    def submit_job(self, job_id: str, func: Any) -> None:
        with self._jobs_lock:
            self.jobs[job_id] = self.executor.submit(func)

    def job_status(self, job_id: str) -> dict[str, Any]:
        with self._jobs_lock:
            future = self.jobs.get(job_id)
        if future is None:
            return {"jobId": job_id, "status": "missing"}
        if not future.done():
            return {"jobId": job_id, "status": "running"}
        try:
            return {"jobId": job_id, "status": "finished", "result": future.result()}
        except Exception as exc:
            return {"jobId": job_id, "status": "failed", "error": str(exc)}

    def shutdown(self) -> None:
        if self._shutdown_once.is_set():
            return
        self._shutdown_once.set()
        self._stop.set()
        if self._thread.is_alive() and threading.current_thread() is not self._thread:
            self._thread.join(timeout=5.0)
        self.listeners.shutdown()
        self.executor.shutdown(wait=False, cancel_futures=True)

    def _background_loop(self) -> None:
        while not self._stop.is_set():
            try:
                contexts = self.registry.refresh()
                if self._stop.is_set():
                    break
                self.listeners.sync_contexts(contexts)
                if self._stop.is_set():
                    break
                self.listeners.poll()
                self.beacons.poll_contexts(contexts)
            except Exception:
                traceback.print_exc()
            self._stop.wait(1.0)


def create_app() -> Flask:
    app = Flask(__name__, template_folder="templates", static_folder="static")
    runtime_root = pathlib.Path(os.environ.get("MISSION_CONSOLE_ROOT", "/tmp/mission-console-phase1")).resolve()
    runtime = MissionConsoleRuntime(runtime_root)
    app.config["MISSION_CONSOLE_RUNTIME"] = runtime
    atexit.register(runtime.shutdown)

    def _runtime() -> MissionConsoleRuntime:
        return app.config["MISSION_CONSOLE_RUNTIME"]

    def _default_context_band() -> tuple[str, str]:
        contexts = _runtime().registry.contexts()
        for context_id in SUPPORTED_CONTEXT_IDS:
            context = contexts.get(context_id)
            if context is None or not context.bands:
                continue
            return context_id, sorted(context.bands)[0]
        return SUPPORTED_CONTEXT_IDS[0], "sband"

    def _page_model(page_name: str) -> dict[str, Any]:
        context_id, band = _default_context_band()
        static_root = pathlib.Path(app.static_folder or runtime_root)
        asset_version = max(
            int((static_root / "mission-console.css").stat().st_mtime),
            int((static_root / "mission-console.js").stat().st_mtime),
        )
        return {
            "page_name": page_name,
            "initial_context_id": context_id,
            "initial_band": band,
            "asset_version": asset_version,
            "readback_commands": {
                "channelRefresh": sorted(CHANNEL_REFRESH_COMMANDS),
                "eventBased": sorted(EVENT_BASED_COMMANDS),
            },
            "readback_groups": READBACK_GROUPS,
            "packet_lab_cases": PACKET_CASES,
            "sequence_action_fields": SEQUENCE_ACTION_FIELDS,
            "visible_command_groups": list(VISIBLE_GROUPS),
            "hidden_command_groups": list(HIDDEN_GROUPS),
            "trend_groups": LIVE_TREND_GROUPS,
            "readback_viewer_tabs": READBACK_VIEWER_TABS,
            "console_session_id": _runtime().console_session_id,
        }

    @app.route("/")
    def dashboard_page() -> str:
        return render_template("dashboard.html", **_page_model("dashboard"))

    @app.route("/ops")
    def ops_page() -> str:
        return render_template("ops.html", **_page_model("ops"))

    @app.route("/trends")
    def trends_page() -> str:
        return render_template("trends.html", **_page_model("trends"))

    @app.route("/readback")
    def readback_page() -> str:
        return render_template("readback.html", **_page_model("readback"))

    @app.route("/beacon")
    def beacon_page() -> str:
        return render_template("beacon.html", **_page_model("beacon"))

    @app.route("/sequences")
    def sequences_page() -> str:
        return render_template("sequences.html", **_page_model("sequences"))

    @app.route("/surfaces")
    def surfaces_page() -> str:
        return render_template("surfaces.html", **_page_model("surfaces"))

    @app.route("/packet-lab")
    def packet_lab_page() -> str:
        return render_template("packet_lab.html", **_page_model("packet-lab"))

    @app.get("/api/contexts")
    def api_contexts() -> Any:
        return jsonify(_runtime().registry.to_json(refresh=True))

    @app.get("/api/dashboard")
    def api_dashboard() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        context = _runtime().registry.context(context_id)
        secure_session = (context.bands.get(band).secure_state if band in context.bands else None)
        payload = {
            "context": context.to_json(),
            "band": band,
            "cards": _runtime().snapshots.dashboard_cards(context_id, band, secure_session),
            "recentEvents": _runtime().snapshots.recent_events(context_id, band, limit=20),
        }
        return jsonify(payload)

    @app.get("/api/beacon/latest")
    def api_beacon_latest() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        payload = _runtime().snapshots.beacon_latest(context_id) or {
            "contextId": context_id,
            "supported": False,
            "available": False,
            "reason": "no-beacon-snapshot",
        }
        payload["selectedBand"] = band
        return jsonify(payload)

    @app.get("/api/beacon/history")
    def api_beacon_history() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        limit = int(request.args.get("limit") or 20)
        return jsonify(
            {
                "contextId": context_id,
                "selectedBand": band,
                "history": _runtime().snapshots.beacon_history(context_id, limit=limit),
            }
        )

    @app.get("/api/trends/catalog")
    def api_trends_catalog() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        channels = _runtime().snapshots.channel_map(context_id, band)
        groups = []
        for group_name, channel_names in LIVE_TREND_GROUPS.items():
            groups.append(
                {
                    "name": group_name,
                    "channels": [
                        {
                            "name": channel_name,
                            "latest": channels.get(channel_name),
                        }
                        for channel_name in channel_names
                    ],
                }
            )
        return jsonify(
            {
                "contextId": context_id,
                "band": band,
                "groups": groups,
                "historySampleCap": _runtime().snapshots._trend_history_size,
            }
        )

    @app.get("/api/trends/history")
    def api_trends_history() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        channels = [value for value in request.args.get("channels", "").split(",") if value]
        return jsonify(
            {
                "contextId": context_id,
                "band": band,
                "history": _runtime().snapshots.trend_history(context_id, band, channels),
            }
        )

    @app.get("/api/readback/cache")
    def api_readback_cache() -> Any:
        return jsonify(_runtime().snapshots.readback_cache())

    @app.get("/api/readback/viewer")
    def api_readback_viewer() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        return jsonify(_runtime().snapshots.readback_viewer(context_id, band))

    @app.get("/api/command-catalog")
    def api_command_catalog() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        context = _runtime().registry.context(context_id, refresh=True)
        if context.dictionary_path is None:
            return jsonify({"contextId": context_id, "dictionaryPath": None, "commands": []})
        catalog = load_command_catalog(context.dictionary_path)
        return jsonify(
            {
                "contextId": context_id,
                "dictionaryPath": catalog["dictionaryPath"],
                "commands": catalog["commands"],
                "visibleGroups": catalog["visibleGroups"],
                "hiddenGroups": catalog["hiddenGroups"],
            }
        )

    @app.get("/api/events/recent")
    def api_events_recent() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        band = request.args.get("band") or _default_context_band()[1]
        limit = int(request.args.get("limit", "50"))
        return jsonify(_runtime().snapshots.recent_events(context_id, band, limit=limit))

    @app.post("/api/events/clear")
    def api_events_clear() -> Any:
        body = request.get_json(silent=True) or {}
        context_id = body.get("contextId") or _default_context_band()[0]
        band = body.get("band") or _default_context_band()[1]
        cleared = _runtime().snapshots.clear_recent_events(context_id, band)
        return jsonify(
            {
                "contextId": context_id,
                "band": band,
                "clearedCount": cleared,
            }
        )

    @app.post("/api/cache/clear")
    def api_cache_clear() -> Any:
        body = request.get_json(silent=True) or {}
        context_id = body.get("contextId") or _default_context_band()[0]
        cleared = _runtime().snapshots.clear_context_cache(context_id)
        return jsonify(
            {
                "contextId": context_id,
                "cleared": cleared,
            }
        )

    @app.get("/api/history")
    def api_history() -> Any:
        limit = int(request.args.get("limit", "50"))
        show_all = request.args.get("showAll", "").lower() in {"1", "true", "yes", "on"}
        return jsonify(
            {
                "consoleSessionId": _runtime().console_session_id,
                "showAll": show_all,
                "entries": _runtime().snapshots.history(limit=limit, current_session_only=not show_all),
            }
        )

    @app.get("/api/packet-lab/history")
    def api_packet_lab_history() -> Any:
        limit = int(request.args.get("limit", "50"))
        show_all = request.args.get("showAll", "").lower() in {"1", "true", "yes", "on"}
        return jsonify(
            {
                "consoleSessionId": _runtime().console_session_id,
                "showAll": show_all,
                "entries": _runtime().snapshots.packet_lab_history(limit=limit, current_session_only=not show_all),
            }
        )

    @app.post("/api/history/clear")
    def api_history_clear() -> Any:
        body = request.get_json(silent=True) or {}
        clear_all = bool(body.get("clearAll", False))
        cleared = _runtime().snapshots.clear_history(current_session_only=not clear_all)
        return jsonify(
            {
                "consoleSessionId": _runtime().console_session_id,
                "clearAll": clear_all,
                "clearedCount": cleared,
            }
        )

    @app.post("/api/packet-lab/history/clear")
    def api_packet_lab_history_clear() -> Any:
        body = request.get_json(silent=True) or {}
        clear_all = bool(body.get("clearAll", False))
        cleared = _runtime().snapshots.clear_packet_lab_history(current_session_only=not clear_all)
        return jsonify(
            {
                "consoleSessionId": _runtime().console_session_id,
                "clearAll": clear_all,
                "clearedCount": cleared,
            }
        )

    def _submit_operation(payload: dict[str, Any]) -> Any:
        job_id = f"job-{time.time_ns()}"
        operation = OperationRequest(**payload)
        _runtime().submit_job(job_id, lambda: _runtime().actions.execute(operation).to_json())
        return jsonify({"jobId": job_id, "status": "queued"})

    @app.post("/api/auth/ensure")
    def api_auth_ensure() -> Any:
        body = request.get_json(force=True)
        return _submit_operation(
            {
                "contextId": body["contextId"],
                "band": body["band"],
                "kind": "auth-ensure",
                "payload": {},
                "ensureAuth": False,
                "operatorLabel": body.get("operatorLabel", "mission-console"),
            }
        )

    @app.post("/api/commands/send")
    def api_command_send() -> Any:
        body = request.get_json(force=True)
        return _submit_operation(
            {
                "contextId": body["contextId"],
                "band": body["band"],
                "kind": "command",
                "payload": {
                    "commandName": body["commandName"],
                    "commandArgs": body.get("commandArgs", []),
                },
                "ensureAuth": bool(body.get("ensureAuth", True)),
                "operatorLabel": body.get("operatorLabel", "mission-console"),
            }
        )

    @app.post("/api/readback/run")
    def api_readback_run() -> Any:
        body = request.get_json(force=True)
        return _submit_operation(
            {
                "contextId": body["contextId"],
                "band": body["band"],
                "kind": "readback",
                "payload": {
                    "commandName": body["commandName"],
                    "commandArgs": body.get("commandArgs", []),
                    "settleSeconds": body.get("settleSeconds", 2.0),
                    "searchTimeoutSeconds": body.get("searchTimeoutSeconds", 5.0),
                },
                "ensureAuth": bool(body.get("ensureAuth", True)),
                "operatorLabel": body.get("operatorLabel", "mission-console"),
            }
        )

    @app.post("/api/files/upload")
    def api_file_upload() -> Any:
        body = request.get_json(force=True)
        return _submit_operation(
            {
                "contextId": body["contextId"],
                "band": body["band"],
                "kind": "file-upload",
                "payload": {
                    "localPath": body["localPath"],
                    "destinationLeaf": body["destinationLeaf"],
                },
                "ensureAuth": bool(body.get("ensureAuth", True)),
                "operatorLabel": body.get("operatorLabel", "mission-console"),
            }
        )

    @app.post("/api/sequences/<action>")
    def api_sequence_action(action: str) -> Any:
        body = request.get_json(force=True)
        payload = {
            "action": action,
            "sequencePath": body.get("sequencePath"),
            "contextId": body.get("sequenceContextId"),
            "runMode": body.get("runMode"),
        }
        return _submit_operation(
            {
                "contextId": body["contextId"],
                "band": body["band"],
                "kind": "sequence",
                "payload": payload,
                "ensureAuth": bool(body.get("ensureAuth", True)),
                "operatorLabel": body.get("operatorLabel", "mission-console"),
            }
        )

    @app.get("/api/sequences/drafts")
    def api_sequence_drafts() -> Any:
        context_id = request.args.get("contextId") or _default_context_band()[0]
        return jsonify(
            {
                "contextId": context_id,
                "drafts": _runtime().sequence_authoring.list_drafts(context_id),
            }
        )

    @app.post("/api/sequences/drafts")
    def api_sequence_save_draft() -> Any:
        body = request.get_json(force=True)
        payload = _runtime().sequence_authoring.save_draft(
            context_id=body["contextId"],
            title=body.get("title"),
            steps=body.get("steps"),
            raw_source=body.get("rawSource"),
            draft_id=body.get("draftId"),
        )
        return jsonify(payload)

    @app.post("/api/sequences/compile")
    def api_sequence_compile() -> Any:
        body = request.get_json(force=True)
        context = _runtime().registry.context(body["contextId"], refresh=True)
        if context.dictionary_path is None:
            return jsonify({"success": False, "error": "dictionary unavailable", "contextId": body["contextId"]}), 400
        payload = _runtime().sequence_authoring.compile_source(
            context_id=body["contextId"],
            dictionary_path=context.dictionary_path,
            title=body.get("title"),
            steps=body.get("steps"),
            raw_source=body.get("rawSource"),
            draft_id=body.get("draftId"),
        )
        return jsonify(payload)

    @app.post("/api/packet-lab/inject")
    def api_packet_lab_inject() -> Any:
        body = request.get_json(force=True)
        job_id = f"job-{time.time_ns()}"
        _runtime().submit_job(
            job_id,
            lambda: _runtime().packet_lab.inject(
                context_id=body["contextId"],
                band=body["band"],
                case=body["case"],
                operator_label=body.get("operatorLabel", "mission-console"),
            ),
        )
        return jsonify({"jobId": job_id, "status": "queued"})

    @app.get("/api/jobs/<job_id>")
    def api_job_status(job_id: str) -> Any:
        return jsonify(_runtime().job_status(job_id))

    return app


def should_create_global_app(module_name: str | None = None) -> bool:
    return (module_name or __name__) != "__mp_main__"


app = create_app() if should_create_global_app() else None


if __name__ == "__main__":
    if app is None:
        app = create_app()
    runtime = app.config["MISSION_CONSOLE_RUNTIME"]

    def _shutdown_signal_handler(_signum: int, _frame: Any) -> None:
        runtime.shutdown()
        raise SystemExit(0)

    for handled_signal in (signal.SIGINT, signal.SIGTERM):
        signal.signal(handled_signal, _shutdown_signal_handler)
    try:
        app.run(host="127.0.0.1", port=int(os.environ.get("MISSION_CONSOLE_PORT", "5080")), debug=False)
    finally:
        runtime.shutdown()
