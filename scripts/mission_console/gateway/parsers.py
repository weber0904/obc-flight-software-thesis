from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any


CHANNEL_REFRESH_COMMANDS: dict[str, list[str]] = {
    "OBCApp.modeManager.MODE_GET": ["SYS_MODE", "SYS_UPTIME_SEC", "SYS_REBOOT_COUNT"],
    "OBCApp.bootManager.BOOT_STATUS": [
        "BOOT_RESET_CAUSE",
        "BOOT_BOOT_COUNT",
        "BOOT_SAFE_FALLBACK_REQUIRED",
    ],
    "OBCApp.ttcPassManager.TTC_GET_STATUS": [
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
    "OBCApp.epsBridge.EPS_GET_STATUS": [
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
    "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": ["ADCS_MODE", "ADCS_Q0", "ADCS_OMEGA_X"],
    "OBCApp.gpsBridge.GPS_GET_STATE": ["GPS_SOURCE_MODE", "GPS_FIX_VALID", "GPS_LAT_DEG"],
    "OBCApp.storageHealthBridge.STORAGE_GET_STATUS": [
        "STORAGE_WARNING_ACTIVE",
        "STORAGE_DATA_PRODUCTS_QUOTA_STATUS",
        "STORAGE_DATA_PRODUCTS_FILE_COUNT",
    ],
    "OBCApp.commController.COMM_GET_STATUS": [
        "COMM_ACTIVE_BAND",
        "COMM_PRIMARY_COMMAND_LINK",
        "COMM_PRIMARY_TELEMETRY_LINK",
        "COMM_PRIMARY_FILE_LINK",
        "COMM_S_BAND_AVAILABLE",
        "COMM_UHF_AVAILABLE",
        "COMM_S_BAND_AVAILABILITY_REASON",
        "COMM_UHF_AVAILABILITY_REASON",
        "COMM_FDIR_FAULT_LATCHED",
        "COMM_FDIR_FAULT_KIND",
    ],
    "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS": [
        "SEQ_CONTEXTS_ACTIVE",
        "SEQ_LAST_CONTEXT_ID",
        "SEQ_LAST_STATE",
        "SEQ_LAST_REASON",
        "SEQ_REJECT_TOTAL",
    ],
}

CHANNEL_REFRESH_VIEWER_FIELDS: dict[str, list[str]] = {
    "OBCApp.modeManager.MODE_GET": ["SYS_MODE", "SYS_UPTIME_SEC", "SYS_REBOOT_COUNT"],
    "OBCApp.bootManager.BOOT_STATUS": [
        "BOOT_ACTIVE_SLOT",
        "BOOT_PENDING_SLOT",
        "BOOT_CONFIRMED",
        "BOOT_UPDATE_PROGRESS",
        "BOOT_LAST_ERROR",
        "BOOT_TRUST_STATUS",
        "BOOT_TRUST_REJECT_REASON",
        "BOOT_STAGED_VERSION",
        "BOOT_LAST_ACCEPTED_VERSION",
        "BOOT_RESET_CAUSE",
        "BOOT_BOOT_COUNT",
        "BOOT_CONSECUTIVE_RESET_COUNT",
        "BOOT_SAFE_FALLBACK_REQUIRED",
        "BOOT_LAST_RECOVERY_SOURCE",
        "BOOT_LAST_RECOVERY_LEVEL",
    ],
    "OBCApp.ttcPassManager.TTC_GET_STATUS": list(CHANNEL_REFRESH_COMMANDS["OBCApp.ttcPassManager.TTC_GET_STATUS"]),
    "OBCApp.epsBridge.EPS_GET_STATUS": list(CHANNEL_REFRESH_COMMANDS["OBCApp.epsBridge.EPS_GET_STATUS"]),
    "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": [
        "ADCS_MODE",
        "ADCS_Q0",
        "ADCS_Q1",
        "ADCS_Q2",
        "ADCS_Q3",
        "ADCS_OMEGA_X",
        "ADCS_OMEGA_Y",
        "ADCS_OMEGA_Z",
        "ADCS_MAG_X",
        "ADCS_MAG_Y",
        "ADCS_MAG_Z",
        "ADCS_POINTING_ERR",
    ],
    "OBCApp.gpsBridge.GPS_GET_STATE": [
        "GPS_SOURCE_MODE",
        "GPS_HAVE_SAMPLE",
        "GPS_FIX_VALID",
        "GPS_SAT_COUNT",
        "GPS_LAT_DEG",
        "GPS_LON_DEG",
        "GPS_ALT_M",
        "GPS_SPEED_MPS",
        "GPS_COURSE_DEG",
        "GPS_HDOP",
    ],
    "OBCApp.storageHealthBridge.STORAGE_GET_STATUS": [
        "STORAGE_WARNING_ACTIVE",
        "STORAGE_DATA_PRODUCTS_QUOTA_STATUS",
        "STORAGE_DATA_PRODUCTS_RETENTION_STATUS",
        "STORAGE_HAVE_SCAN",
        "STORAGE_WARNING_MASK",
        "STORAGE_DEGRADED_MASK",
        "STORAGE_SCAN_COUNT",
        "STORAGE_SCAN_ERRORS",
        "STORAGE_PERSISTENT_FILE_COUNT",
        "STORAGE_PERSISTENT_BYTES",
        "STORAGE_STAGING_FILE_COUNT",
        "STORAGE_STAGING_BYTES",
        "STORAGE_LOG_FILE_COUNT",
        "STORAGE_LOG_BYTES",
        "STORAGE_DATA_PRODUCTS_EXISTS",
        "STORAGE_DATA_PRODUCTS_SCAN_OK",
        "STORAGE_DATA_PRODUCTS_FILE_COUNT",
        "STORAGE_DATA_PRODUCTS_BYTES",
        "STORAGE_DATA_PRODUCTS_ERROR_CODE",
        "STORAGE_DATA_PRODUCTS_QUOTA_BYTES",
        "STORAGE_DATA_PRODUCTS_WATERMARK_BYTES",
    ],
    "OBCApp.commController.COMM_GET_STATUS": list(CHANNEL_REFRESH_COMMANDS["OBCApp.commController.COMM_GET_STATUS"]),
    "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS": list(CHANNEL_REFRESH_COMMANDS["OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS"]),
}
EVENT_BASED_COMMANDS: dict[str, list[str]] = {
    "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS": ["RECOVERY_STATUS"],
    "OBCApp.bootManager.GET_RESET_CAUSE": ["BOOT_RECOVERY_STATUS"],
    "OBCApp.bootManager.GET_BOOT_COUNT": ["BOOT_RECOVERY_STATUS"],
    "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS": ["HW_WATCHDOG_STATUS"],
    "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY": [
        "PERSISTENT_FAULT_HISTORY_STATUS",
        "PERSISTENT_FAULT_HISTORY_RECORD",
    ],
    "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS": ["PAYLOAD_STATUS"],
    "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES": ["PAYLOAD_CAPABILITIES"],
    "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA": ["PAYLOAD_CAPTURE_METADATA"],
}
SINGLE_EVENT_READBACK_COMMANDS = frozenset(
    {
        "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
        "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
        "OBCApp.bootManager.GET_RESET_CAUSE",
        "OBCApp.bootManager.GET_BOOT_COUNT",
        "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
        "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
        "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
    }
)
REJECT_EVENT_NAMES = (
    "COMMAND_AUTHORITY_REJECTED",
    "COMMAND_SEQUENCE_REJECTED",
    "COMMAND_SESSION_REJECTED",
    "SECURE_COMMAND_REJECTED",
)
DASHBOARD_CHANNELS: dict[str, list[str]] = {
    "Satellite Status": [
        "SYS_MODE",
        "ADCS_MODE",
        "EPS_PDU_STATUS",
        "GPS_SOURCE_MODE",
        "PAYLOAD_STATE",
    ],
    "EPS Snapshot": [
        "EPS_VBAT",
        "EPS_IBAT",
        "EPS_SOC",
        "EPS_TEMP_BAT",
        "EPS_PDU_STATUS",
    ],
    "ADCS Snapshot": [
        "ADCS_MODE",
        "ADCS_Q0",
        "ADCS_OMEGA_X",
        "ADCS_OMEGA_Y",
        "ADCS_OMEGA_Z",
    ],
    "Mission State": [
        "SYS_UPTIME_SEC",
        "SYS_REBOOT_COUNT",
        "TTC_POLICY_WINDOW_ACTIVE",
        "TTC_POLICY_TTC_ACTIVE",
    ],
    "Comm State": [
        "COMM_ACTIVE_BAND",
        "COMM_PRIMARY_COMMAND_LINK",
        "COMM_PRIMARY_TELEMETRY_LINK",
        "COMM_PRIMARY_FILE_LINK",
        "COMM_S_BAND_AVAILABLE",
        "COMM_UHF_AVAILABLE",
        "COMM_S_BAND_AVAILABILITY_REASON",
        "COMM_UHF_AVAILABILITY_REASON",
        "COMM_FDIR_FAULT_LATCHED",
        "COMM_FDIR_FAULT_KIND",
    ],
    "Sequence & Ops": [
        "SEQ_CONTEXTS_ACTIVE",
        "SEQ_LAST_CONTEXT_ID",
        "SEQ_LAST_STATE",
        "SEQ_LAST_REASON",
        "SEQ_REJECT_TOTAL",
        "SESSION_LAST_ACCEPTED_SEQUENCE",
    ],
    "Health Snapshot": [
        "SYS_CPU_USAGE",
        "SYS_MEM_RSS_MB",
        "SYS_RESOURCE_DEGRADED",
        "SYS_LOW_MEMORY",
    ],
}
LIVE_TREND_GROUPS: dict[str, list[str]] = {
    "EPS": [
        "EPS_VBAT",
        "EPS_IBAT",
        "EPS_SOC",
        "EPS_TEMP_BAT",
    ],
    "ADCS": [
        "ADCS_Q0",
        "ADCS_Q1",
        "ADCS_Q2",
        "ADCS_Q3",
        "ADCS_OMEGA_X",
        "ADCS_OMEGA_Y",
        "ADCS_OMEGA_Z",
    ],
    "Health": [
        "SYS_CPU_USAGE",
        "SYS_MEM_RSS_MB",
    ],
}
READBACK_VIEWER_TABS: dict[str, list[str]] = {
    "OBC": [
        "OBCApp.modeManager.MODE_GET",
        "OBCApp.ttcPassManager.TTC_GET_STATUS",
    ],
    "EPS": [
        "OBCApp.epsBridge.EPS_GET_STATUS",
    ],
    "ADCS": [
        "OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
    ],
    "Payload": [
        "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS",
        "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES",
        "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA",
    ],
    "Storage": [
        "OBCApp.storageHealthBridge.STORAGE_GET_STATUS",
    ],
    "Boot & Recovery": [
        "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS",
        "OBCApp.bootManager.BOOT_STATUS",
        "OBCApp.bootManager.GET_RESET_CAUSE",
        "OBCApp.bootManager.GET_BOOT_COUNT",
        "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
        "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY",
    ],
    "Sequence": [
        "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS",
    ],
}
EVENT_LINE_RE = re.compile(
    r"^(?P<timestamp>\S+):\s+"
    r"(?P<qualified>[\w.]+)\s+"
    r"\((?P<ident>\d+)\)\s+"
    r"\((?P<context>.*)\)\s+"
    r"(?P<severity>[^:]+)\s+:\s+"
    r"(?P<message>.+)$"
)
EVENT_SIMPLE_RE = re.compile(r"^(?P<timestamp>\S+):\s+(?P<qualified>[\w.]+)\s+:\s+(?P<message>.+)$")
CHANNEL_CSV_RE = re.compile(r"^(?P<timestamp>\S+):\s+(?P<qualified>[\w.]+),(?P<ident>\d+),(?P<value>.+)$")
CHANNEL_TIMESTAMP_PAREN_RE = re.compile(
    r"^(?P<timestamp>\S+):\s+"
    r"(?P<qualified>[\w.]+)\s+"
    r"\((?P<ident>\d+)\)\s+"
    r"\((?P<context>.*)\)\s+"
    r"(?P<value>.+)$"
)
CHANNEL_PAREN_RE = re.compile(r"^(?P<qualified>[\w.]+)\s+\((?P<ident>\d+)\)\s+(?P<value>.+)$")


@dataclass(frozen=True)
class ParsedEvent:
    timestamp: str
    qualified_name: str
    event_name: str
    message: str
    raw: str

    def to_json(self) -> dict[str, Any]:
        return {
            "timestamp": self.timestamp,
            "qualifiedName": self.qualified_name,
            "eventName": self.event_name,
            "message": self.message,
            "raw": self.raw,
        }


@dataclass(frozen=True)
class ParsedChannel:
    timestamp: str | None
    qualified_name: str
    channel_name: str
    channel_id: int
    value: str
    raw: str

    def to_json(self) -> dict[str, Any]:
        return {
            "timestamp": self.timestamp,
            "qualifiedName": self.qualified_name,
            "channelName": self.channel_name,
            "channelId": self.channel_id,
            "value": self.value,
            "raw": self.raw,
        }


def parse_event_line(line: str) -> ParsedEvent | None:
    match = EVENT_LINE_RE.match(line.strip())
    if match:
        qualified = match.group("qualified")
        return ParsedEvent(
            timestamp=match.group("timestamp"),
            qualified_name=qualified,
            event_name=qualified.split(".")[-1],
            message=match.group("message").strip(),
            raw=line.rstrip(),
        )
    match = EVENT_SIMPLE_RE.match(line.strip())
    if match:
        qualified = match.group("qualified")
        return ParsedEvent(
            timestamp=match.group("timestamp"),
            qualified_name=qualified,
            event_name=qualified.split(".")[-1],
            message=match.group("message").strip(),
            raw=line.rstrip(),
        )
    parts = line.strip().split(",", 5)
    if len(parts) == 6 and "." in parts[2]:
        timestamp, _context, qualified, ident_text, _severity, message = parts
        if ident_text.isdigit():
            return ParsedEvent(
                timestamp=timestamp,
                qualified_name=qualified,
                event_name=qualified.split(".")[-1],
                message=message.strip(),
                raw=line.rstrip(),
            )
    return None


def parse_channel_line(line: str) -> ParsedChannel | None:
    stripped = line.strip()
    match = CHANNEL_CSV_RE.match(stripped)
    if match:
        qualified = match.group("qualified")
        return ParsedChannel(
            timestamp=match.group("timestamp"),
            qualified_name=qualified,
            channel_name=qualified.split(".")[-1],
            channel_id=int(match.group("ident")),
            value=match.group("value").strip(),
            raw=line.rstrip(),
        )
    match = CHANNEL_TIMESTAMP_PAREN_RE.match(stripped)
    if match:
        qualified = match.group("qualified")
        return ParsedChannel(
            timestamp=match.group("timestamp"),
            qualified_name=qualified,
            channel_name=qualified.split(".")[-1],
            channel_id=int(match.group("ident")),
            value=match.group("value").strip(),
            raw=line.rstrip(),
        )
    parts = stripped.split(",", 4)
    if len(parts) == 5 and "." in parts[2]:
        timestamp, _context, qualified, ident_text, value = parts
        if ident_text.isdigit():
            return ParsedChannel(
                timestamp=timestamp,
                qualified_name=qualified,
                channel_name=qualified.split(".")[-1],
                channel_id=int(ident_text),
                value=value.strip(),
                raw=line.rstrip(),
            )
    match = CHANNEL_PAREN_RE.match(stripped)
    if match:
        qualified = match.group("qualified")
        return ParsedChannel(
            timestamp=None,
            qualified_name=qualified,
            channel_name=qualified.split(".")[-1],
            channel_id=int(match.group("ident")),
            value=match.group("value").strip(),
            raw=line.rstrip(),
        )
    return None


def classify_readback_command(command_name: str) -> str:
    if command_name in CHANNEL_REFRESH_COMMANDS:
        return "channel-refresh-based"
    if command_name in EVENT_BASED_COMMANDS:
        return "event-based"
    return "generic"


def _extract_named_int(message: str, key: str) -> int | None:
    match = re.search(rf"{re.escape(key)}\s+(\d+)", message)
    return None if match is None else int(match.group(1))


def _safe_search(pattern: str, message: str) -> str | None:
    match = re.search(pattern, message)
    return None if match is None else match.group(1)


def _safe_bool_search(pattern: str, message: str) -> bool | None:
    value = _safe_search(pattern, message)
    if value is None:
        return None
    return value == "True"


def _parse_payload_status(message: str) -> dict[str, Any]:
    match = re.search(
        r"state\s+(?P<state>[A-Z0-9_]+)\s+"
        r"powered\s+(?P<powered>True|False)\s+"
        r"prepared\s+(?P<prepared>True|False)\s+"
        r"result\s+(?P<lastResult>[A-Z0-9_]+)\s+"
        r"capture\s+(?P<captureId>\d+)\s+"
        r"index\s+(?P<captureIndex>\d+)\s+"
        r"preview\s+(?P<previewRelativePath>.*?)\s+"
        r"previewDp\s+(?P<previewDataProductPath>.*?)\s+"
        r"previewPublished\s+(?P<previewPublished>True|False)\s+"
        r"rawPublished\s+(?P<rawPublished>True|False)$",
        message,
    )
    if match is None:
        return {
            "state": _safe_search(r"state\s+([A-Z0-9_]+)", message),
            "powered": _safe_bool_search(r"powered\s+(True|False)", message),
            "prepared": _safe_bool_search(r"prepared\s+(True|False)", message),
            "lastResult": _safe_search(r"result\s+([A-Z0-9_]+)", message),
            "captureId": _extract_named_int(message, "capture"),
            "captureIndex": _extract_named_int(message, "index"),
            "previewRelativePath": None,
            "previewDataProductPath": None,
            "previewPublished": _safe_bool_search(r"previewPublished\s+(True|False)", message),
            "rawPublished": _safe_bool_search(r"rawPublished\s+(True|False)", message),
            "rawMessage": message,
        }
    return {
        "state": match.group("state"),
        "powered": match.group("powered") == "True",
        "prepared": match.group("prepared") == "True",
        "lastResult": match.group("lastResult"),
        "captureId": int(match.group("captureId")),
        "captureIndex": int(match.group("captureIndex")),
        "previewRelativePath": match.group("previewRelativePath"),
        "previewDataProductPath": match.group("previewDataProductPath"),
        "previewPublished": match.group("previewPublished") == "True",
        "rawPublished": match.group("rawPublished") == "True",
        "rawMessage": message,
    }


def _parse_payload_capabilities(message: str) -> dict[str, Any]:
    match = re.search(
        r"backend\s+(?P<backendName>\S+)\s+"
        r"supported\s+(?P<supportedMask>\d+)\s+"
        r"offOnly\s+(?P<offOnlyMask>\d+)\s+"
        r"autoMutable\s+(?P<autoMutableMask>\d+)\s+"
        r"deterministicMutable\s+(?P<deterministicMutableMask>\d+)\s+"
        r"raw\s+(?P<rawRegisterSupported>True|False)\s+"
        r"real\s+(?P<realSensorPath>True|False)$",
        message,
    )
    if match is None:
        return {"rawMessage": message}
    return {
        "backendName": match.group("backendName"),
        "supportedMask": int(match.group("supportedMask")),
        "offOnlyMask": int(match.group("offOnlyMask")),
        "autoMutableMask": int(match.group("autoMutableMask")),
        "deterministicMutableMask": int(match.group("deterministicMutableMask")),
        "rawRegisterSupported": match.group("rawRegisterSupported") == "True",
        "realSensorPath": match.group("realSensorPath") == "True",
        "rawMessage": message,
    }


def _parse_payload_capture_metadata(message: str) -> dict[str, Any]:
    match = re.search(
        r"(?:policy|session)\s+(?P<capturePolicy>[A-Z0-9_]+)\s+"
        r"capture\s+(?P<captureId>\d+)\s+"
        r"index\s+(?P<captureIndex>\d+)\s+"
        r"requested\s+(?P<requestedMask>\d+)\s+"
        r"applied\s+(?P<appliedMask>\d+)\s+"
        r"actualExp\s+(?P<actualExposureUsec>\d+)\s+"
        r"actualGain\s+(?P<actualGainX100>\d+)\s+"
        r"awbValid\s+(?P<actualAwbValid>True|False)\s+"
        r"awbTempK\s+(?P<actualAwbColorTemperatureK>\d+)\s+"
        r"awbRedX1000\s+(?P<actualAwbRedGainX1000>\d+)\s+"
        r"awbBlueX1000\s+(?P<actualAwbBlueGainX1000>\d+)\s+"
        r"raw\s+(?P<rawRelativePath>.*?)\s+"
        r"preview\s+(?P<previewRelativePath>.*?)\s+"
        r"previewDp\s+(?P<previewDataProductPath>.*?)\s+"
        r"rawDp\s+(?P<rawDataProductPath>.*?)\s+"
        r"previewPublished\s+(?P<previewPublished>True|False)\s+"
        r"rawPublished\s+(?P<rawPublished>True|False)$",
        message,
    )
    if match is None:
        return {
            "capturePolicy": _safe_search(r"(?:policy|session)\s+([A-Z0-9_]+)", message),
            "captureId": _extract_named_int(message, "capture"),
            "captureIndex": _extract_named_int(message, "index"),
            "requestedMask": _extract_named_int(message, "requested"),
            "appliedMask": _extract_named_int(message, "applied"),
            "actualExposureUsec": _extract_named_int(message, "actualExp"),
            "actualGainX100": _extract_named_int(message, "actualGain"),
            "actualAwbValid": _safe_bool_search(r"awbValid\s+(True|False)", message),
            "actualAwbColorTemperatureK": _extract_named_int(message, "awbTempK"),
            "actualAwbRedGainX1000": _extract_named_int(message, "awbRedX1000"),
            "actualAwbBlueGainX1000": _extract_named_int(message, "awbBlueX1000"),
            "rawRelativePath": None,
            "previewRelativePath": None,
            "previewDataProductPath": None,
            "rawDataProductPath": None,
            "previewPublished": _safe_bool_search(r"previewPublished\s+(True|False)", message),
            "rawPublished": _safe_bool_search(r"rawPublished\s+(True|False)", message),
            "rawMessage": message,
        }
    return {
        "capturePolicy": match.group("capturePolicy"),
        "captureId": int(match.group("captureId")),
        "captureIndex": int(match.group("captureIndex")),
        "requestedMask": int(match.group("requestedMask")),
        "appliedMask": int(match.group("appliedMask")),
        "actualExposureUsec": int(match.group("actualExposureUsec")),
        "actualGainX100": int(match.group("actualGainX100")),
        "actualAwbValid": match.group("actualAwbValid") == "True",
        "actualAwbColorTemperatureK": int(match.group("actualAwbColorTemperatureK")),
        "actualAwbRedGainX1000": int(match.group("actualAwbRedGainX1000")),
        "actualAwbBlueGainX1000": int(match.group("actualAwbBlueGainX1000")),
        "rawRelativePath": match.group("rawRelativePath"),
        "previewRelativePath": match.group("previewRelativePath"),
        "previewDataProductPath": match.group("previewDataProductPath"),
        "rawDataProductPath": match.group("rawDataProductPath"),
        "previewPublished": match.group("previewPublished") == "True",
        "rawPublished": match.group("rawPublished") == "True",
        "rawMessage": message,
    }


def parse_structured_event(event_name: str, message: str) -> dict[str, Any]:
    if event_name == "RECOVERY_STATUS":
        return {
            "activeIncidentCount": _extract_named_int(message, "activeCount"),
            "activeSource": _safe_search(r"source\s+([A-Z0-9_]+)", message),
            "highestLevel": _safe_search(r"highest\s+([A-Z0-9_]+)", message),
            "lastAction": _safe_search(r"lastAction\s+([A-Z0-9_]+)", message),
            "pendingProcessRestart": _safe_bool_search(r"pendingProcessRestart\s+(True|False)", message),
            "pendingReboot": _safe_bool_search(r"pendingReboot\s+(True|False)", message),
            "relatchCount": _extract_named_int(message, "relatchCount"),
            "rawMessage": message,
        }
    if event_name == "HW_WATCHDOG_STATUS":
        return {
            "enabled": _safe_bool_search(r"enabled\s+(True|False)", message),
            "deviceOpen": _safe_bool_search(r"open\s+(True|False)", message),
            "timeoutSec": _extract_named_int(message, "timeout"),
            "feedCount": _extract_named_int(message, "feedCount"),
            "lastError": _extract_named_int(message, "lastError"),
            "rawMessage": message,
        }
    if event_name == "BOOT_RECOVERY_STATUS":
        return {
            "resetCause": _safe_search(r"cause\s+([A-Z0-9_]+)", message),
            "bootCount": _extract_named_int(message, "bootCount"),
            "consecutive": _extract_named_int(message, "consecutive"),
            "safeFallback": _safe_bool_search(r"safeFallback\s+(True|False)", message),
            "source": _safe_search(r"source\s+([A-Z0-9_]+)", message),
            "level": _safe_search(r"level\s+([A-Z0-9_]+)", message),
            "rawMessage": message,
        }
    if event_name == "PERSISTENT_FAULT_HISTORY_STATUS":
        match = re.search(
            r"total\s+(?P<totalRecords>\d+)\s+"
            r"returned\s+(?P<returnedRecords>\d+)\s+"
            r"activeCopy\s+(?P<activeCopy>[A-Z0-9_]+)\s+"
            r"generation\s+(?P<generation>\d+)$",
            message,
        )
        if match is None:
            return {"rawMessage": message}
        return {
            "totalRecords": int(match.group("totalRecords")),
            "returnedRecords": int(match.group("returnedRecords")),
            "activeCopy": match.group("activeCopy"),
            "generation": int(match.group("generation")),
            "rawMessage": message,
        }
    if event_name == "PERSISTENT_FAULT_HISTORY_RECORD":
        match = re.search(
            r"Persistent fault\[(?P<indexFromLatest>\d+)\]\s+"
            r"kind\s+(?P<kind>[A-Z0-9_]+)\s+"
            r"source\s+(?P<source>[A-Z0-9_]+)\s+"
            r"level\s+(?P<level>[A-Z0-9_]+)\s+"
            r"action\s+(?P<recoveryAction>[A-Z0-9_]+)\s+"
            r"cause\s+(?P<resetCause>[A-Z0-9_]+)\s+"
            r"boot\s+(?P<bootCount>\d+)\s+"
            r"consecutive\s+(?P<consecutiveResetCount>\d+)\s+"
            r"uptime\s+(?P<uptimeSec>\d+)\s+"
            r"time\s+(?P<timestampSec>\d+)\s+"
            r"detail\s+(?P<detail>\d+)\s+"
            r"flags\s+(?P<flags>\d+)$",
            message,
        )
        if match is None:
            return {"rawMessage": message}
        return {
            "indexFromLatest": int(match.group("indexFromLatest")),
            "kind": match.group("kind"),
            "source": match.group("source"),
            "level": match.group("level"),
            "recoveryAction": match.group("recoveryAction"),
            "resetCause": match.group("resetCause"),
            "bootCount": int(match.group("bootCount")),
            "consecutiveResetCount": int(match.group("consecutiveResetCount")),
            "uptimeSec": int(match.group("uptimeSec")),
            "timestampSec": int(match.group("timestampSec")),
            "detail": int(match.group("detail")),
            "flags": int(match.group("flags")),
            "rawMessage": message,
        }
    if event_name == "PAYLOAD_STATUS":
        return _parse_payload_status(message)
    if event_name == "PAYLOAD_CAPABILITIES":
        return _parse_payload_capabilities(message)
    if event_name == "PAYLOAD_CAPTURE_METADATA":
        return _parse_payload_capture_metadata(message)
    if event_name in {"RECOVERY_STATUS", "HW_WATCHDOG_STATUS"}:
        return {"rawMessage": message}
    return {"rawMessage": message}


def structured_event_complete(event_name: str, payload: dict[str, Any]) -> bool:
    required_fields: dict[str, tuple[str, ...]] = {
        "RECOVERY_STATUS": (
            "activeIncidentCount",
            "activeSource",
            "highestLevel",
            "lastAction",
            "pendingProcessRestart",
            "pendingReboot",
            "relatchCount",
        ),
        "HW_WATCHDOG_STATUS": (
            "enabled",
            "deviceOpen",
            "timeoutSec",
            "feedCount",
            "lastError",
        ),
        "BOOT_RECOVERY_STATUS": (
            "resetCause",
            "bootCount",
            "consecutive",
            "safeFallback",
            "source",
            "level",
        ),
        "PERSISTENT_FAULT_HISTORY_STATUS": (
            "totalRecords",
            "returnedRecords",
            "activeCopy",
            "generation",
        ),
        "PERSISTENT_FAULT_HISTORY_RECORD": (
            "indexFromLatest",
            "kind",
            "source",
            "level",
            "recoveryAction",
            "resetCause",
            "bootCount",
            "consecutiveResetCount",
            "uptimeSec",
            "timestampSec",
            "detail",
            "flags",
        ),
        "PAYLOAD_STATUS": (
            "state",
            "powered",
            "prepared",
            "lastResult",
            "captureId",
            "captureIndex",
            "previewRelativePath",
            "previewDataProductPath",
            "previewPublished",
            "rawPublished",
        ),
        "PAYLOAD_CAPABILITIES": (
            "backendName",
            "supportedMask",
            "offOnlyMask",
            "autoMutableMask",
            "deterministicMutableMask",
            "rawRegisterSupported",
            "realSensorPath",
        ),
        "PAYLOAD_CAPTURE_METADATA": (
            "capturePolicy",
            "captureId",
            "captureIndex",
            "requestedMask",
            "appliedMask",
            "actualExposureUsec",
            "actualGainX100",
            "actualAwbValid",
            "actualAwbColorTemperatureK",
            "actualAwbRedGainX1000",
            "actualAwbBlueGainX1000",
            "rawRelativePath",
            "previewRelativePath",
            "previewDataProductPath",
            "rawDataProductPath",
            "previewPublished",
            "rawPublished",
        ),
    }
    expected = required_fields.get(event_name)
    if expected is None:
        return True
    return all(payload.get(field) is not None for field in expected)
