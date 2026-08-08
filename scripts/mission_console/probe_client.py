#!/usr/bin/env python3
from __future__ import annotations

import argparse
import contextlib
import json
import pathlib
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any


def request_json(method: str, url: str, payload: dict[str, Any] | None = None) -> Any:
    data = None
    headers = {}
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, data=data, method=method, headers=headers)
    with urllib.request.urlopen(request, timeout=15.0) as response:
        return json.loads(response.read().decode("utf-8"))


def get_json(base_url: str, path: str, **query: Any) -> Any:
    if query:
        path = path + "?" + urllib.parse.urlencode(query)
    return request_json("GET", base_url.rstrip("/") + path)


def post_json(base_url: str, path: str, payload: dict[str, Any]) -> Any:
    return request_json("POST", base_url.rstrip("/") + path, payload)


def wait_for_job(base_url: str, job_id: str, timeout_sec: float = 60.0) -> dict[str, Any]:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        payload = get_json(base_url, f"/api/jobs/{job_id}")
        if payload["status"] == "finished":
            return dict(payload["result"])
        if payload["status"] in {"failed", "missing"}:
            raise RuntimeError(json.dumps(payload, indent=2, sort_keys=True))
        time.sleep(1.0)
    raise RuntimeError(f"timed out waiting for job {job_id}")


def submit_job(
    base_url: str,
    path: str,
    payload: dict[str, Any],
    timeout_sec: float = 60.0,
    settle_sec: float = 1.0,
) -> dict[str, Any]:
    queued = post_json(base_url, path, payload)
    result = wait_for_job(base_url, queued["jobId"], timeout_sec=timeout_sec)
    if settle_sec > 0.0:
        time.sleep(settle_sec)
    return result


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


@contextlib.contextmanager
def _native_capture_override(
    hosted_context: dict[str, Any],
    packet: bytes,
) -> Any:
    """Point the isolated hosted manifest at a native-only probe capture."""
    manifest_path = pathlib.Path(hosted_context["manifestPath"])
    band = hosted_context["bands"]["sband"]
    manifest_key = str(band["manifestKey"])
    original_capture_path = str(band["captures"]["gdsToSouthbound"])
    surface_root = pathlib.Path(band["surfaceRoot"])
    native_capture_path = surface_root / "probe-artifacts" / "native-gds-secure-command.bin"
    native_prefix = b"\x20\x44\x04\x4c\x00\x10\x00\xc0\x00\x00\x3f"
    native_capture_path.parent.mkdir(parents=True, exist_ok=True)
    native_capture_path.write_bytes(native_prefix + packet + b"\xac\xa7")

    original_manifest = manifest_path.read_bytes()
    manifest_mode = manifest_path.stat().st_mode & 0o777
    manifest_payload = json.loads(original_manifest.decode("utf-8"))
    configured_capture = str(
        manifest_payload["operatorSurfaces"][manifest_key]["captures"]["gdsToSouthbound"]
    )
    require(
        configured_capture == original_capture_path,
        "hosted native replay probe capture path does not match the active manifest",
    )
    manifest_payload["operatorSurfaces"][manifest_key]["captures"]["gdsToSouthbound"] = str(
        native_capture_path
    )
    temporary_manifest = manifest_path.with_name(f".{manifest_path.name}.mission-console-native-probe.tmp")

    def replace_manifest(content: bytes) -> None:
        temporary_manifest.write_bytes(content)
        temporary_manifest.chmod(manifest_mode)
        temporary_manifest.replace(manifest_path)

    try:
        modified_manifest = json.dumps(manifest_payload, indent=2, sort_keys=True).encode("utf-8") + b"\n"
        replace_manifest(modified_manifest)
        yield native_capture_path, len(native_prefix)
    finally:
        replace_manifest(original_manifest)
        temporary_manifest.unlink(missing_ok=True)


def log(summary: list[str], line: str) -> None:
    print(line)
    summary.append(line)


def wait_for_condition(
    predicate: callable[[], bool],
    *,
    timeout_sec: float,
    interval_sec: float = 1.0,
    message: str,
) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if predicate():
            return
        time.sleep(interval_sec)
    raise RuntimeError(message)


def require_packet_lab_evidence(evidence: dict[str, Any], message: str) -> str:
    kind = evidence["kind"]
    require(kind in {"explicit-reject", "bounded-no-op"}, message)
    return kind


def require_single_event_readback(readback: dict[str, Any], expected_event: str, label: str) -> None:
    require(readback["family"] == "event-based", f"{label} family mismatch")
    evidence = readback.get("mainEvidence")
    require(isinstance(evidence, dict) and evidence.get("kind") == "fresh-event", f"{label} did not close on fresh event")
    events = evidence.get("events") or []
    require(len(events) == 1, f"{label} expected exactly one fresh event")
    require(events[0].get("eventName") == expected_event, f"{label} fresh event mismatch")


def require_persistent_fault_group(readback: dict[str, Any], label: str) -> None:
    require(readback["family"] == "event-based", f"{label} family mismatch")
    evidence = readback.get("mainEvidence")
    require(
        isinstance(evidence, dict) and evidence.get("kind") == "fresh-event-group",
        f"{label} did not close on fresh event group",
    )
    status_event = evidence.get("statusEvent")
    require(isinstance(status_event, dict), f"{label} missing status event")
    require(status_event.get("eventName") == "PERSISTENT_FAULT_HISTORY_STATUS", f"{label} status event mismatch")
    returned_records = int(status_event.get("structured", {}).get("returnedRecords", 0))
    record_events = evidence.get("recordEvents") or []
    if returned_records == 0:
        require(len(record_events) == 0, f"{label} should not include record events for empty result")
    else:
        require(len(record_events) == returned_records, f"{label} record count mismatch")


def run_target_auth_with_retry(base_url: str, summary: list[str], attempts: int = 3) -> dict[str, Any]:
    last_result: dict[str, Any] | None = None
    for attempt in range(1, attempts + 1):
        result = submit_job(
            base_url,
            "/api/auth/ensure",
            {"contextId": "target-manual-ground-dual-gds", "band": "sband"},
            timeout_sec=180.0,
        )
        if result["status"] == "succeeded":
            if attempt > 1:
                log(summary, f"target-auth-sband-retry={attempt}")
            return result
        last_result = result
        log(summary, f"target-auth-sband-attempt-{attempt}=FAILED")
        time.sleep(2.0)
    raise RuntimeError(f"target sband auth failed after {attempts} attempts: {json.dumps(last_result, indent=2)}")


def run_hosted_uhf_auth_with_retry(base_url: str, summary: list[str], attempts: int = 3) -> dict[str, Any]:
    last_result: dict[str, Any] | None = None
    for attempt in range(1, attempts + 1):
        result = submit_job(
            base_url,
            "/api/auth/ensure",
            {"contextId": "hosted-manual-dual-gds", "band": "uhf-primary-after-failover"},
            timeout_sec=120.0,
        )
        if result["status"] == "succeeded":
            return result
        log(summary, f"hosted-auth-uhf-attempt-{attempt}=FAILED")
        last_result = result
    raise RuntimeError(f"hosted UHF re-auth failed after {attempts} attempts: {json.dumps(last_result, indent=2)}")


def run_hosted(base_url: str, sample_sequence: str, summary: list[str]) -> None:
    contexts = get_json(base_url, "/api/contexts")
    hosted = contexts["contexts"]["hosted-manual-dual-gds"]
    require(hosted["lifecycleState"] == "running", "hosted context not running")
    require("sband" in hosted["bands"], "hosted sband surface missing")
    require("uhf-primary-after-failover" in hosted["bands"], "hosted switched UHF surface missing")
    log(summary, "hosted-context=running")

    dashboard = get_json(base_url, "/api/dashboard", contextId="hosted-manual-dual-gds", band="sband")
    require("Mission State" in dashboard["cards"], "dashboard missing Mission State card")
    require("Satellite Status" in dashboard["cards"], "dashboard missing Satellite Status card")
    require("EPS Snapshot" in dashboard["cards"], "dashboard missing EPS Snapshot card")
    require("ADCS Snapshot" in dashboard["cards"], "dashboard missing ADCS Snapshot card")
    log(summary, "hosted-dashboard=PASS")

    auth = submit_job(
        base_url,
        "/api/auth/ensure",
        {"contextId": "hosted-manual-dual-gds", "band": "sband"},
        timeout_sec=90.0,
    )
    require(auth["status"] == "succeeded", "sband auth failed")
    log(summary, "hosted-auth-sband=PASS")

    trends_catalog = get_json(base_url, "/api/trends/catalog", contextId="hosted-manual-dual-gds", band="sband")
    trend_group_names = [group["name"] for group in trends_catalog["groups"]]
    require(trend_group_names == ["EPS", "ADCS", "Health"], f"unexpected trend groups: {trend_group_names}")
    trend_channel_names = {
        group["name"]: [channel["name"] for channel in group["channels"]]
        for group in trends_catalog["groups"]
    }
    require("GPS" not in trend_channel_names, "GPS should not be in first trend tranche")
    require(trend_channel_names["EPS"] == ["EPS_VBAT", "EPS_IBAT", "EPS_SOC", "EPS_TEMP_BAT"], "unexpected EPS trend channels")
    require("ADCS_Q0" in trend_channel_names["ADCS"], "ADCS trend channels missing quaternion")
    require("SYS_CPU_USAGE" in trend_channel_names["Health"], "Health trend channels missing SYS_CPU_USAGE")
    log(summary, "hosted-trends-catalog=PASS")

    def hosted_trend_history_ready() -> bool:
        payload = get_json(
            base_url,
            "/api/trends/history",
            contextId="hosted-manual-dual-gds",
            band="sband",
            channels="EPS_VBAT,ADCS_Q0",
        )
        history = payload["history"]
        return bool(history.get("EPS_VBAT")) and bool(history.get("ADCS_Q0"))

    wait_for_condition(
        hosted_trend_history_ready,
        timeout_sec=30.0,
        message="hosted trend history never populated for EPS_VBAT and ADCS_Q0",
    )
    log(summary, "hosted-trends-history=PASS")

    mode_get = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.modeManager.MODE_GET",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=75.0,
    )
    require(mode_get["status"] == "succeeded", "MODE_GET readback failed")
    require(mode_get["readback"]["family"] == "channel-refresh-based", "MODE_GET family mismatch")
    require(
        mode_get["readback"].get("channelSource") != "snapshot+command-completion",
        "MODE_GET relied on command-completion fallback instead of fresh channels",
    )
    require(
        mode_get["readback"]["channels"].get("SYS_MODE") is not None,
        "MODE_GET returned no SYS_MODE channel value",
    )
    require(
        mode_get["readback"]["channels"].get("SYS_REBOOT_COUNT") is not None,
        "MODE_GET returned no SYS_REBOOT_COUNT channel value",
    )
    log(summary, "hosted-readback-mode-get=PASS")

    gps_state = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.gpsBridge.GPS_GET_STATE",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(gps_state["status"] == "succeeded", "GPS_GET_STATE failed")
    require(gps_state["readback"]["family"] == "channel-refresh-based", "GPS_GET_STATE family mismatch")
    require(
        gps_state["readback"]["channels"].get("GPS_SOURCE_MODE") is not None,
        "GPS_GET_STATE returned no GPS_SOURCE_MODE channel value",
    )
    require(
        gps_state["readback"]["channels"].get("GPS_FIX_VALID") is not None,
        "GPS_GET_STATE returned no GPS_FIX_VALID channel value",
    )
    require(
        gps_state["readback"]["channels"].get("GPS_LAT_DEG") is not None,
        "GPS_GET_STATE returned no GPS_LAT_DEG channel value",
    )
    log(summary, "hosted-readback-gps-state=PASS")

    ttc_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.ttcPassManager.TTC_GET_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(ttc_status["status"] == "succeeded", "TTC_GET_STATUS failed")
    require(ttc_status["readback"]["family"] == "channel-refresh-based", "TTC_GET_STATUS family mismatch")
    require(
        ttc_status["readback"].get("channelSource") != "snapshot+command-completion",
        "TTC_GET_STATUS relied on command-completion fallback instead of fresh channels",
    )
    require(
        ttc_status["readback"]["channels"].get("TTC_POLICY_ENABLED") is not None,
        "TTC_GET_STATUS returned no TTC_POLICY_ENABLED channel value",
    )
    require(
        ttc_status["readback"]["channels"].get("TTC_POLICY_WINDOW_ACTIVE") is not None,
        "TTC_GET_STATUS returned no TTC_POLICY_WINDOW_ACTIVE channel value",
    )
    require(
        ttc_status["readback"]["channels"].get("TTC_POLICY_LOSS_TIMEOUT_SEC") is not None,
        "TTC_GET_STATUS returned no TTC_POLICY_LOSS_TIMEOUT_SEC channel value",
    )
    dashboard_after_ttc = get_json(base_url, "/api/dashboard", contextId="hosted-manual-dual-gds", band="sband")
    require(
        dashboard_after_ttc["cards"]["Mission State"].get("TTC_POLICY_WINDOW_ACTIVE") is not None,
        "dashboard still missing TTC_POLICY_WINDOW_ACTIVE after TTC_GET_STATUS",
    )
    log(summary, "hosted-readback-ttc-status=PASS")

    payload_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(payload_status["status"] == "succeeded", "PAYLOAD_GET_STATUS failed")
    require(payload_status["readback"]["family"] == "event-based", "payload status family mismatch")
    require(
        payload_status["readback"].get("mainEvidence", {}).get("kind") == "fresh-event",
        "PAYLOAD_GET_STATUS did not close on fresh main event evidence",
    )
    log(summary, "hosted-readback-payload-status=PASS")

    payload_capabilities = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(payload_capabilities["status"] == "succeeded", "PAYLOAD_GET_CAPABILITIES failed")
    require(payload_capabilities["readback"]["family"] == "event-based", "payload capabilities family mismatch")
    require(
        payload_capabilities["readback"].get("mainEvidence", {}).get("kind") == "fresh-event",
        "PAYLOAD_GET_CAPABILITIES did not close on fresh main event evidence",
    )
    log(summary, "hosted-readback-payload-capabilities=PASS")

    payload_capture_metadata = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(payload_capture_metadata["status"] == "succeeded", "PAYLOAD_GET_LAST_CAPTURE_METADATA failed")
    require(
        payload_capture_metadata["readback"]["family"] == "event-based",
        "payload capture metadata family mismatch",
    )
    require(
        payload_capture_metadata["readback"].get("mainEvidence", {}).get("kind") == "fresh-event",
        "PAYLOAD_GET_LAST_CAPTURE_METADATA did not close on fresh main event evidence",
    )
    log(summary, "hosted-readback-payload-capture-metadata=PASS")

    recovery_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(recovery_status["status"] == "succeeded", "GET_RECOVERY_STATUS failed")
    require(recovery_status["readback"]["family"] == "event-based", "recovery family mismatch")
    require(
        recovery_status["readback"].get("mainEvidence", {}).get("kind") == "fresh-event",
        "GET_RECOVERY_STATUS did not close on fresh main event evidence",
    )
    log(summary, "hosted-readback-recovery-status=PASS")

    watchdog_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(watchdog_status["status"] == "succeeded", "GET_HW_WATCHDOG_STATUS failed")
    require(watchdog_status["readback"]["family"] == "event-based", "watchdog family mismatch")
    require(
        watchdog_status["readback"].get("mainEvidence", {}).get("kind") == "fresh-event",
        "GET_HW_WATCHDOG_STATUS did not close on fresh main event evidence",
    )
    log(summary, "hosted-readback-watchdog-status=PASS")

    eps_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.epsBridge.EPS_GET_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(eps_status["status"] == "succeeded", "EPS_GET_STATUS failed")
    require(eps_status["readback"]["family"] == "channel-refresh-based", "EPS_GET_STATUS family mismatch")
    require(
        eps_status["readback"]["channels"].get("EPS_HEATER_ENABLED") is not None,
        "EPS_GET_STATUS returned no EPS_HEATER_ENABLED channel value",
    )
    require(
        eps_status["readback"]["channels"].get("EPS_OVERCURRENT_FLAGS") is not None,
        "EPS_GET_STATUS returned no EPS_OVERCURRENT_FLAGS channel value",
    )
    require(
        eps_status["readback"]["channels"].get("EPS_POWER_OUT") is not None,
        "EPS_GET_STATUS returned no EPS_POWER_OUT channel value",
    )
    log(summary, "hosted-readback-eps-status=PASS")

    comm_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.commController.COMM_GET_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(comm_status["status"] == "succeeded", "COMM_GET_STATUS failed")
    require(comm_status["readback"]["family"] == "channel-refresh-based", "COMM_GET_STATUS family mismatch")
    require(
        comm_status["readback"].get("channelSource") != "snapshot+command-completion",
        "COMM_GET_STATUS relied on command-completion fallback instead of fresh channels",
    )
    require(
        comm_status["readback"]["channels"].get("COMM_ACTIVE_BAND") is not None,
        "COMM_GET_STATUS returned no COMM_ACTIVE_BAND channel value",
    )
    require(
        comm_status["readback"]["channels"].get("COMM_S_BAND_AVAILABLE") is not None,
        "COMM_GET_STATUS returned no COMM_S_BAND_AVAILABLE channel value",
    )
    require(
        comm_status["readback"]["channels"].get("COMM_UHF_AVAILABLE") is not None,
        "COMM_GET_STATUS returned no COMM_UHF_AVAILABLE channel value",
    )
    require(
        comm_status["readback"]["channels"].get("COMM_FDIR_FAULT_LATCHED") is not None,
        "COMM_GET_STATUS returned no COMM_FDIR_FAULT_LATCHED channel value",
    )
    dashboard_after_comm = get_json(base_url, "/api/dashboard", contextId="hosted-manual-dual-gds", band="sband")
    require(
        dashboard_after_comm["cards"]["Comm State"].get("COMM_ACTIVE_BAND") is not None,
        "dashboard still missing COMM_ACTIVE_BAND after COMM_GET_STATUS",
    )
    require(
        dashboard_after_comm["cards"]["Comm State"].get("COMM_S_BAND_AVAILABLE") is not None,
        "dashboard still missing COMM_S_BAND_AVAILABLE after COMM_GET_STATUS",
    )
    log(summary, "hosted-readback-comm-status=PASS")

    readback_viewer = get_json(base_url, "/api/readback/viewer", contextId="hosted-manual-dual-gds", band="sband")
    viewer_tabs = {tab["name"]: tab["cards"] for tab in readback_viewer["tabs"]}
    require("OBC" in viewer_tabs, "readback viewer missing OBC tab")
    require("EPS" in viewer_tabs, "readback viewer missing EPS tab")
    mission_cards = {card["commandName"]: card for card in viewer_tabs["OBC"]}
    eps_cards = {card["commandName"]: card for card in viewer_tabs["EPS"]}
    require(mission_cards["OBCApp.modeManager.MODE_GET"]["state"] == "success", "MODE_GET not saved in viewer")
    require(mission_cards["OBCApp.ttcPassManager.TTC_GET_STATUS"]["state"] == "success", "TTC_GET_STATUS not saved in viewer")
    require(eps_cards["OBCApp.epsBridge.EPS_GET_STATUS"]["state"] == "success", "EPS_GET_STATUS not saved in viewer")
    require(
        mission_cards["OBCApp.modeManager.MODE_GET"]["savedValues"] is not None,
        "MODE_GET viewer card missing saved values",
    )
    require(
        mission_cards["OBCApp.modeManager.MODE_GET"]["debugPayload"] is not None,
        "MODE_GET viewer card missing debug payload",
    )
    log(summary, "hosted-readback-viewer=PASS")

    storage_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.storageHealthBridge.STORAGE_GET_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(storage_status["status"] == "succeeded", "STORAGE_GET_STATUS failed")
    require(
        storage_status["readback"]["family"] == "channel-refresh-based",
        "STORAGE_GET_STATUS family mismatch",
    )
    require(
        storage_status["readback"].get("channelSource") != "snapshot+command-completion",
        "STORAGE_GET_STATUS relied on command-completion fallback instead of fresh channels",
    )
    require(
        storage_status["readback"]["channels"].get("STORAGE_WARNING_ACTIVE") is not None,
        "STORAGE_GET_STATUS returned no STORAGE_WARNING_ACTIVE channel value",
    )
    require(
        storage_status["readback"]["channels"].get("STORAGE_DATA_PRODUCTS_QUOTA_STATUS") is not None,
        "STORAGE_GET_STATUS returned no STORAGE_DATA_PRODUCTS_QUOTA_STATUS channel value",
    )
    require(
        storage_status["readback"]["channels"].get("STORAGE_DATA_PRODUCTS_FILE_COUNT") is not None,
        "STORAGE_GET_STATUS returned no STORAGE_DATA_PRODUCTS_FILE_COUNT channel value",
    )
    log(summary, "hosted-readback-storage-status=PASS")

    boot_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.bootManager.BOOT_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(boot_status["status"] == "succeeded", "BOOT_STATUS failed")
    require(boot_status["readback"]["family"] == "channel-refresh-based", "BOOT_STATUS family mismatch")
    require(boot_status["readback"]["channels"].get("BOOT_RESET_CAUSE") is not None, "BOOT_STATUS returned no BOOT_RESET_CAUSE")
    require(boot_status["readback"]["channels"].get("BOOT_BOOT_COUNT") is not None, "BOOT_STATUS returned no BOOT_BOOT_COUNT")
    require(
        boot_status["readback"]["channels"].get("BOOT_SAFE_FALLBACK_REQUIRED") is not None,
        "BOOT_STATUS returned no BOOT_SAFE_FALLBACK_REQUIRED",
    )
    log(summary, "hosted-readback-boot-status=PASS")

    reset_cause = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.bootManager.GET_RESET_CAUSE",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(reset_cause["status"] == "succeeded", "GET_RESET_CAUSE failed")
    require_single_event_readback(reset_cause["readback"], "BOOT_RECOVERY_STATUS", "GET_RESET_CAUSE")
    log(summary, "hosted-readback-reset-cause=PASS")

    boot_count = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.bootManager.GET_BOOT_COUNT",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(boot_count["status"] == "succeeded", "GET_BOOT_COUNT failed")
    require_single_event_readback(boot_count["readback"], "BOOT_RECOVERY_STATUS", "GET_BOOT_COUNT")
    log(summary, "hosted-readback-boot-count=PASS")

    fault_history = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
            "commandArgs": ["4"],
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(fault_history["status"] == "succeeded", "GET_PERSISTENT_FAULT_HISTORY failed")
    require_persistent_fault_group(fault_history["readback"], "GET_PERSISTENT_FAULT_HISTORY")
    log(summary, "hosted-readback-fault-history=PASS")

    adcs_attitude = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=45.0,
    )
    require(adcs_attitude["status"] == "succeeded", "ADCS_GET_ATTITUDE failed")
    require(adcs_attitude["readback"]["family"] == "channel-refresh-based", "ADCS_GET_ATTITUDE family mismatch")
    require(
        adcs_attitude["readback"]["channels"].get("ADCS_MODE") is not None,
        "ADCS_GET_ATTITUDE returned no ADCS_MODE channel value",
    )
    require(
        adcs_attitude["readback"]["channels"].get("ADCS_Q0") is not None,
        "ADCS_GET_ATTITUDE returned no ADCS_Q0 channel value",
    )
    require(
        adcs_attitude["readback"]["channels"].get("ADCS_OMEGA_X") is not None,
        "ADCS_GET_ATTITUDE returned no ADCS_OMEGA_X channel value",
    )
    log(summary, "hosted-readback-adcs-attitude=PASS")

    upload = submit_job(
        base_url,
        "/api/files/upload",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "localPath": sample_sequence,
            "destinationLeaf": "mission-console-sample.bin",
        },
        timeout_sec=90.0,
    )
    require(upload["status"] == "succeeded", "sample upload failed")
    log(summary, "hosted-upload=PASS")

    seq_validate = submit_job(
        base_url,
        "/api/sequences/validate",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "sequencePath": "mission-console-sample.bin",
        },
        timeout_sec=60.0,
    )
    require(seq_validate["status"] == "succeeded", "SEQ_VALIDATE failed")
    log(summary, "hosted-seq-validate=PASS")

    packet_lab = submit_job(
        base_url,
        "/api/packet-lab/inject",
        {"contextId": "hosted-manual-dual-gds", "band": "sband", "case": "tampered-mac"},
        timeout_sec=45.0,
    )
    require(packet_lab["expectedFailureReason"] == "integrity-failure", "packet-lab expected failure mismatch")
    packet_lab_kind = require_packet_lab_evidence(packet_lab["observedEvidence"], "packet-lab evidence missing")
    require(packet_lab.get("faultExplanation", {}).get("faultKind") == "tampered-mac", "packet-lab missing fault explanation kind")
    require(packet_lab.get("faultExplanation", {}).get("affectedField") == "MAC / auth tag", "packet-lab missing affected field")
    if packet_lab_kind == "explicit-reject":
        require(
            packet_lab["observedEvidence"].get("reasonName") not in {None, ""},
            "packet-lab explicit reject did not decode reason name",
        )
    else:
        require(
            packet_lab["observedEvidence"].get("reason") not in {None, ""},
            "packet-lab bounded-no-op evidence missing bounded reason",
        )
    log(summary, f"hosted-packet-lab={packet_lab_kind}")
    log(summary, "hosted-packet-lab-fault-explanation=PASS")

    captured_replay = submit_job(
        base_url,
        "/api/packet-lab/inject",
        {"contextId": "hosted-manual-dual-gds", "band": "sband", "case": "replay-captured-raw"},
        timeout_sec=45.0,
    )
    require(
        captured_replay["expectedFailureReason"] == "replay-or-stale-session",
        "captured replay expected failure mismatch",
    )
    replay_kind = require_packet_lab_evidence(
        captured_replay["observedEvidence"],
        "captured replay evidence missing",
    )
    require(captured_replay.get("source") == "capture-replay", "captured replay did not use a captured packet")
    require(captured_replay.get("rawBytesHex"), "captured replay did not record packet bytes")
    log(summary, f"hosted-packet-lab-captured-replay={replay_kind}")

    captured_packet = bytes.fromhex(str(captured_replay["rawBytesHex"]))
    with _native_capture_override(hosted, captured_packet) as (native_capture_path, native_packet_offset):
        native_replay = submit_job(
            base_url,
            "/api/packet-lab/inject",
            {"contextId": "hosted-manual-dual-gds", "band": "sband", "case": "replay-captured-raw"},
            timeout_sec=45.0,
        )
        require(
            native_replay.get("rawBytesHex") == captured_replay["rawBytesHex"],
            "native capture replay selected different secure packet bytes",
        )
        require(
            native_capture_path.read_bytes()[native_packet_offset : native_packet_offset + len(captured_packet)]
            == captured_packet,
            "native capture replay probe artifact does not contain the selected packet at the asserted offset",
        )
        native_replay_kind = require_packet_lab_evidence(
            native_replay["observedEvidence"],
            "native capture replay evidence missing",
        )
    contexts_after_native_replay = get_json(base_url, "/api/contexts")
    require(
        contexts_after_native_replay["contexts"]["hosted-manual-dual-gds"]["bands"]["sband"]["captures"][
            "gdsToSouthbound"
        ]
        == hosted["bands"]["sband"]["captures"]["gdsToSouthbound"],
        "hosted native replay probe did not restore the active capture path",
    )
    log(summary, f"hosted-packet-lab-native-capture-replay={native_replay_kind}")

    draft_save = post_json(
        base_url,
        "/api/sequences/drafts",
        {
            "contextId": "hosted-manual-dual-gds",
            "title": "mission-console-ui-generated",
            "steps": [
                {
                    "offset": "R00:00:00",
                    "commandName": "OBCApp.modeManager.MODE_GET",
                    "commandArgs": [],
                }
            ],
        },
    )
    require(draft_save["draftId"], "sequence draft save did not return draftId")
    drafts = get_json(base_url, "/api/sequences/drafts", contextId="hosted-manual-dual-gds")
    require(
        any(entry.get("draftId") == draft_save["draftId"] for entry in drafts["drafts"]),
        "saved sequence draft not visible in drafts API",
    )
    log(summary, "hosted-sequence-draft-save=PASS")

    compile_result = post_json(
        base_url,
        "/api/sequences/compile",
        {
            "contextId": "hosted-manual-dual-gds",
            "title": "mission-console-ui-generated",
            "steps": [
                {
                    "offset": "R00:00:00",
                    "commandName": "OBCApp.modeManager.MODE_GET",
                    "commandArgs": [],
                }
            ],
            "draftId": draft_save["draftId"],
        },
    )
    require(compile_result["success"], f"sequence compile failed: {compile_result}")
    compiled_path = pathlib.Path(str(compile_result["compiledPath"]))
    require(compiled_path.exists(), "compiled sequence artifact does not exist")
    log(summary, "hosted-sequence-compile=PASS")

    generated_upload = submit_job(
        base_url,
        "/api/files/upload",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "localPath": str(compiled_path),
            "destinationLeaf": "mission-console-generated.bin",
        },
        timeout_sec=90.0,
    )
    require(generated_upload["status"] == "succeeded", "generated sequence upload failed")
    generated_validate = submit_job(
        base_url,
        "/api/sequences/validate",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "sequencePath": "mission-console-generated.bin",
        },
        timeout_sec=60.0,
    )
    require(generated_validate["status"] == "succeeded", "generated sequence validate failed")
    log(summary, "hosted-sequence-workspace-validate=PASS")

    switch = submit_job(
        base_url,
        "/api/commands/send",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.commController.COMM_SET_ACTIVE",
            "commandArgs": ["UHF"],
            "ensureAuth": True,
        },
        timeout_sec=45.0,
    )
    require(switch["status"] == "succeeded", "COMM_SET_ACTIVE UHF failed")
    log(summary, "hosted-switch-uhf=PASS")

    time.sleep(2.0)
    contexts_after_switch = get_json(base_url, "/api/contexts")
    sband_state = contexts_after_switch["contexts"]["hosted-manual-dual-gds"]["bands"]["sband"]["secureState"]
    require(sband_state is not None and sband_state.get("invalidated"), "sband secure state was not invalidated")
    log(summary, "hosted-sband-invalidated=PASS")

    uhf_auth = run_hosted_uhf_auth_with_retry(base_url, summary)
    require(uhf_auth["status"] == "succeeded", "UHF re-auth failed")
    log(summary, "hosted-auth-uhf-primary=PASS")

    uhf_boot = submit_job(
        base_url,
        "/api/commands/send",
        {
            "contextId": "hosted-manual-dual-gds",
            "band": "uhf-primary-after-failover",
            "commandName": "OBCApp.bootManager.BOOT_STATUS",
            "commandArgs": [],
            "ensureAuth": True,
        },
        timeout_sec=45.0,
    )
    require(uhf_boot["status"] == "succeeded", "UHF BOOT_STATUS command failed")
    log(summary, "hosted-uhf-boot-command=PASS")

    history = get_json(base_url, "/api/history")
    history_entries = history["entries"] if isinstance(history, dict) else history
    require(len(history_entries) >= 5, "hosted history too short")
    readback_cache = get_json(base_url, "/api/readback/cache")
    require(any(key.endswith(":OBCApp.modeManager.MODE_GET") for key in readback_cache), "readback cache missing MODE_GET")
    log(summary, "hosted-history-cache=PASS")


def run_target(base_url: str, summary: list[str]) -> None:
    contexts = get_json(base_url, "/api/contexts")
    target = contexts["contexts"]["target-manual-ground-dual-gds"]
    require(target["lifecycleState"] == "running", "target context not running")
    require(target["targetBaseline"]["manifest"] is not None, "target baseline manifest missing")
    log(summary, "target-context=running")

    auth = run_target_auth_with_retry(base_url, summary)
    require(auth["status"] == "succeeded", "target sband auth failed")
    log(summary, "target-auth-sband=PASS")

    watchdog = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "target-manual-ground-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=60.0,
    )
    require(watchdog["status"] == "succeeded", "GET_HW_WATCHDOG_STATUS failed")
    require(watchdog["readback"]["family"] == "event-based", "watchdog family mismatch")
    log(summary, "target-readback-watchdog=PASS")

    boot_status = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "target-manual-ground-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.bootManager.BOOT_STATUS",
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=60.0,
    )
    require(boot_status["status"] == "succeeded", "target BOOT_STATUS failed")
    require(boot_status["readback"]["family"] == "channel-refresh-based", "target BOOT_STATUS family mismatch")
    require(boot_status["readback"]["channels"].get("BOOT_RESET_CAUSE") is not None, "target BOOT_STATUS missing BOOT_RESET_CAUSE")
    require(boot_status["readback"]["channels"].get("BOOT_BOOT_COUNT") is not None, "target BOOT_STATUS missing BOOT_BOOT_COUNT")
    log(summary, "target-readback-boot-status=PASS")

    fault_history = submit_job(
        base_url,
        "/api/readback/run",
        {
            "contextId": "target-manual-ground-dual-gds",
            "band": "sband",
            "commandName": "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
            "commandArgs": ["4"],
            "searchTimeoutSeconds": 10.0,
        },
        timeout_sec=60.0,
    )
    require(fault_history["status"] == "succeeded", "persistent fault history failed")
    require_persistent_fault_group(fault_history["readback"], "target GET_PERSISTENT_FAULT_HISTORY")
    log(summary, "target-readback-fault-history=PASS")

    packet_lab = submit_job(
        base_url,
        "/api/packet-lab/inject",
        {"contextId": "target-manual-ground-dual-gds", "band": "sband", "case": "tampered-mac"},
        timeout_sec=60.0,
    )
    require(packet_lab["expectedFailureReason"] == "integrity-failure", "target packet-lab failure mismatch")
    packet_lab_kind = require_packet_lab_evidence(packet_lab["observedEvidence"], "target packet-lab evidence missing")
    log(summary, f"target-packet-lab={packet_lab_kind}")

    dashboard = get_json(base_url, "/api/dashboard", contextId="target-manual-ground-dual-gds", band="sband")
    require("Health Snapshot" in dashboard["cards"], "target dashboard missing Health Snapshot")
    log(summary, "target-dashboard=PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description="Mission Console Phase 1 API probe client.")
    parser.add_argument("mode", choices=("hosted", "target"))
    parser.add_argument("--base-url", required=True)
    parser.add_argument("--sample-sequence")
    args = parser.parse_args()

    summary: list[str] = []
    if args.mode == "hosted":
        if not args.sample_sequence:
            raise SystemExit("--sample-sequence is required for hosted mode")
        run_hosted(args.base_url, args.sample_sequence, summary)
    else:
        run_target(args.base_url, summary)
    print("mission-console-probe: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
