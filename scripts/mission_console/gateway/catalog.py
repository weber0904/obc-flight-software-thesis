from __future__ import annotations

import json
import pathlib
import threading
from typing import Any


VISIBLE_GROUPS = (
    "OBC",
    "Comm",
    "TTC",
    "Payload",
    "Sequence",
    "Recovery & Boot",
    "EPS",
    "ADCS",
    "GPS",
    "Storage",
)

HIDDEN_GROUPS = (
    "Framework / CDH",
    "Raw Sequencer",
    "FileHandling low-level",
    "Parameter DB",
    "CSP/raw transport",
    "Event/health filters",
    "Other engineering commands",
)

GROUP_RULES: tuple[dict[str, Any], ...] = (
    {
        "prefixes": ("OBCApp.modeManager.",),
        "group": "OBC",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.commController.",),
        "group": "Comm",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.ttcPassManager.",),
        "group": "TTC",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.payloadOpsController.",),
        "group": "Payload",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.sequenceAdmissionController.",),
        "group": "Sequence",
        "visibleByDefault": True,
    },
    {
        "prefixes": (
            "OBCApp.recoveryExecutor.",
            "OBCApp.bootManager.",
            "OBCApp.linuxWatchdogSink.",
            "OBCApp.persistentFaultManager.",
        ),
        "group": "Recovery & Boot",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.epsBridge.",),
        "group": "EPS",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.adcsBridge.",),
        "group": "ADCS",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.gpsBridge.",),
        "group": "GPS",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.storageHealthBridge.",),
        "group": "Storage",
        "visibleByDefault": True,
    },
    {
        "prefixes": ("OBCApp.cmdSeqA.", "OBCApp.cmdSeqB."),
        "group": "Raw Sequencer",
        "visibleByDefault": False,
    },
    {
        "prefixes": ("FileHandling.fileDownlink.", "FileHandling.fileManager."),
        "group": "FileHandling low-level",
        "visibleByDefault": False,
    },
    {
        "prefixes": ("FileHandling.prmDb.",),
        "group": "Parameter DB",
        "visibleByDefault": False,
    },
    {
        "prefixes": ("OBCApp.cspBridge.",),
        "group": "CSP/raw transport",
        "visibleByDefault": False,
    },
    {
        "prefixes": ("CdhCore.events.", "CdhCore.health."),
        "group": "Event/health filters",
        "visibleByDefault": False,
    },
    {
        "prefixes": ("CdhCore.",),
        "group": "Framework / CDH",
        "visibleByDefault": False,
    },
)

CURATED_COMMANDS: dict[str, dict[str, Any]] = {
    "OBCApp.modeManager.MODE_GET": {
        "label": "Get Mission Mode",
        "description": "Read the current mission mode and core mode telemetry.",
    },
    "OBCApp.modeManager.MODE_SET": {
        "label": "Set Mission Mode",
        "description": "Request a mission mode transition through the governed mode manager.",
        "args": {
            "mode": {
                "placeholder": "Select a target mode",
            }
        },
    },
    "OBCApp.commController.COMM_SET_ACTIVE": {
        "label": "Set Active Command Band",
        "description": "Explicitly switch the active operator band. The previous band session will be invalidated.",
        "args": {
            "band": {
                "placeholder": "Select SBAND or UHF",
                "options": [
                    {"value": "SBAND", "label": "S-band"},
                    {"value": "UHF", "label": "UHF"},
                ],
            }
        },
    },
    "OBCApp.commController.COMM_START_PASS": {
        "label": "Start Pass Window",
        "description": "Mark the command link as in-pass for a bounded duration.",
    },
    "OBCApp.commController.COMM_STOP_PASS": {
        "label": "Stop Pass Window",
        "description": "Close the active pass window and return the link policy to steady state.",
    },
    "OBCApp.ttcPassManager.TTC_SET_POLICY": {
        "label": "Set TTC Policy",
        "description": "Enable or disable the TTC policy and update the loss-of-lock timeout.",
    },
    "OBCApp.ttcPassManager.TTC_SET_PASS_WINDOW": {
        "label": "Set TTC Pass Window",
        "description": "Program the current TTC window using Unix timestamps.",
    },
    "OBCApp.ttcPassManager.TTC_GET_STATUS": {
        "label": "Get TTC Status",
        "description": "Read the current TTC window and policy state.",
    },
    "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_RAW": {
        "label": "Capture Raw Payload Frame",
        "description": "Trigger a bounded raw payload capture with a numeric capture slot and operator tag.",
        "args": {
            "captureIndex": {
                "placeholder": "0-255 capture slot",
            },
            "tag": {
                "placeholder": "Short artifact tag",
            },
        },
    },
    "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO": {
        "label": "Capture Auto Exposure",
        "description": "Capture using the auto session and per-capture override mask.",
    },
    "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC": {
        "label": "Capture Deterministic Exposure",
        "description": "Capture using deterministic exposure and gain settings.",
    },
    "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS": {
        "label": "Get Payload Status",
        "description": "Read payload session state, result state, and latest payload evidence.",
    },
    "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA": {
        "label": "Get Last Capture Metadata",
        "description": "Read the most recent payload capture metadata record.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_VALIDATE": {
        "label": "Validate Sequence",
        "description": "Run the governed sequence validator against a staged sequence file.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_RUN": {
        "label": "Run Sequence",
        "description": "Execute a staged governed sequence using the sequence admission controller.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_PREPARE_MANUAL": {
        "label": "Prepare Manual Sequence",
        "description": "Prepare a governed sequence for later manual stepping.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_START": {
        "label": "Start Manual Sequence",
        "description": "Start a prepared manual sequence context.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_STEP": {
        "label": "Step Manual Sequence",
        "description": "Advance a prepared manual sequence context by one step.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_CANCEL": {
        "label": "Cancel Sequence Context",
        "description": "Cancel a governed sequence context by id.",
    },
    "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS": {
        "label": "Get Sequence Status",
        "description": "Read the sequence admission controller status and last context outcome.",
    },
    "OBCApp.bootManager.BOOT_STATUS": {
        "label": "Get Boot Status",
        "description": "Read boot and recovery state from the boot manager.",
    },
    "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS": {
        "label": "Get Recovery Status",
        "description": "Read the current recovery-executor state and bounded recovery evidence.",
    },
    "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS": {
        "label": "Get Hardware Watchdog Status",
        "description": "Read the hardware watchdog sink status surface.",
    },
}

_CATALOG_CACHE: dict[tuple[str, int], dict[str, Any]] = {}
_CACHE_LOCK = threading.Lock()
UI_EXCLUDED_PREFIXES = ("OBCApp.radioController.",)


def load_command_catalog(dictionary_path: pathlib.Path | str) -> dict[str, Any]:
    resolved_path = pathlib.Path(dictionary_path).resolve()
    empty_catalog = {
        "dictionaryPath": str(resolved_path),
        "commands": [],
        "visibleGroups": list(VISIBLE_GROUPS),
        "hiddenGroups": list(HIDDEN_GROUPS),
    }
    if not resolved_path.is_file():
        return empty_catalog
    try:
        stat = resolved_path.stat()
    except FileNotFoundError:
        return empty_catalog
    cache_key = (str(resolved_path), stat.st_mtime_ns)
    with _CACHE_LOCK:
        cached = _CATALOG_CACHE.get(cache_key)
        if cached is not None:
            return cached
    payload = json.loads(resolved_path.read_text(encoding="utf-8"))
    type_index = _build_type_index(payload.get("typeDefinitions", []))
    commands = [
        _catalog_command(entry, type_index)
        for entry in payload.get("commands", [])
        if not str(entry.get("name", "")).startswith(UI_EXCLUDED_PREFIXES)
    ]
    catalog = {
        "dictionaryPath": str(resolved_path),
        "commands": sorted(commands, key=lambda row: (not row["visibleByDefault"], row["group"], row["name"])),
        "visibleGroups": list(VISIBLE_GROUPS),
        "hiddenGroups": list(HIDDEN_GROUPS),
    }
    with _CACHE_LOCK:
        _CATALOG_CACHE[cache_key] = catalog
    return catalog


def _build_type_index(type_definitions: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    index: dict[str, dict[str, Any]] = {}
    for definition in type_definitions:
        key = definition.get("qualifiedName") or definition.get("name")
        if key:
            index[str(key)] = dict(definition)
    return index


def _catalog_command(command: dict[str, Any], type_index: dict[str, dict[str, Any]]) -> dict[str, Any]:
    name = str(command["name"])
    rule = _group_rule_for(name)
    curated = CURATED_COMMANDS.get(name, {})
    args = [
        _catalog_arg(argument, type_index, (curated.get("args") or {}).get(str(argument["name"]), {}))
        for argument in command.get("formalParams", [])
    ]
    label = curated.get("label") or _default_label(name)
    description = curated.get("description") or _default_description(name, rule["group"])
    return {
        "name": name,
        "label": label,
        "group": rule["group"],
        "source": ("curated" if curated else "dictionary"),
        "visibleByDefault": bool(curated.get("visibleByDefault", rule["visibleByDefault"])),
        "description": description,
        "opcode": int(str(command["opcode"]), 0),
        "commandKind": str(command.get("commandKind", "unknown")),
        "args": args,
    }


def _catalog_arg(argument: dict[str, Any], type_index: dict[str, dict[str, Any]], override: dict[str, Any]) -> dict[str, Any]:
    type_spec = dict(argument.get("type", {}))
    resolved = _resolve_type(type_spec, type_index)
    input_kind = override.get("inputKind") or _input_kind_for(resolved)
    default_value = override.get("defaultValue")
    options = override.get("options")
    if default_value is None:
        default_value = _default_value_for(resolved)
    if options is None:
        options = _options_for(resolved)
    return {
        "name": str(argument["name"]),
        "required": True,
        "inputKind": input_kind,
        "typeName": _display_type_name(type_spec),
        "annotation": override.get("annotation") or _annotation_for(type_spec, resolved),
        "options": options or [],
        "defaultValue": default_value,
        "placeholder": override.get("placeholder") or _placeholder_for(argument, resolved),
    }


def _group_rule_for(command_name: str) -> dict[str, Any]:
    for rule in GROUP_RULES:
        prefixes = tuple(str(prefix) for prefix in rule["prefixes"])
        if command_name.startswith(prefixes):
            return {
                "group": str(rule["group"]),
                "visibleByDefault": bool(rule["visibleByDefault"]),
            }
    return {
        "group": "Other engineering commands",
        "visibleByDefault": False,
    }


def _default_label(command_name: str) -> str:
    suffix = command_name.split(".")[-1]
    return suffix.replace("_", " ").title()


def _default_description(command_name: str, group_name: str) -> str:
    component = ".".join(command_name.split(".")[:-1])
    return f"Dictionary-backed {group_name.lower()} command from {component}."


def _resolve_type(
    type_spec: dict[str, Any],
    type_index: dict[str, dict[str, Any]],
    visited: set[str] | None = None,
) -> dict[str, Any]:
    visited = set() if visited is None else set(visited)
    kind = str(type_spec.get("kind", "raw"))
    if kind != "qualifiedIdentifier":
        return dict(type_spec)
    name = str(type_spec.get("name", ""))
    if not name or name in visited:
        return dict(type_spec)
    visited.add(name)
    target = type_index.get(name)
    if target is None:
        return dict(type_spec)
    target_kind = str(target.get("kind", "raw"))
    if target_kind == "alias":
        aliased = dict(target.get("type", {}))
        if aliased.get("kind") == "qualifiedIdentifier":
            return _resolve_type(aliased, type_index, visited)
        aliased.setdefault("qualifiedName", target.get("qualifiedName"))
        return aliased
    if target_kind == "qualifiedIdentifier":
        return _resolve_type(dict(target), type_index, visited)
    target = dict(target)
    target.setdefault("qualifiedName", target.get("qualifiedName") or type_spec.get("name"))
    return target


def _input_kind_for(resolved: dict[str, Any]) -> str:
    kind = str(resolved.get("kind", "raw"))
    if kind == "integer":
        return "integer"
    if kind == "float":
        return "float"
    if kind == "string":
        return "string"
    if kind == "bool":
        return "bool"
    if kind == "enum":
        return "enum"
    if kind == "qualifiedIdentifier":
        return "raw"
    return "raw"


def _display_type_name(type_spec: dict[str, Any]) -> str:
    return str(type_spec.get("name") or type_spec.get("qualifiedName") or type_spec.get("kind", "raw"))


def _annotation_for(type_spec: dict[str, Any], resolved: dict[str, Any]) -> str:
    kind = str(resolved.get("kind", "raw"))
    if kind == "integer":
        signed = "signed" if resolved.get("signed") else "unsigned"
        size = resolved.get("size")
        return f"{signed} {size}-bit integer" if size is not None else signed
    if kind == "float":
        size = resolved.get("size")
        return f"{size}-bit float" if size is not None else "floating-point"
    if kind == "string":
        size = resolved.get("size")
        return f"string ({size} chars max)" if size is not None else "string"
    if kind == "bool":
        return "boolean"
    if kind == "enum":
        return str(resolved.get("qualifiedName") or type_spec.get("name") or "enum")
    return str(type_spec.get("name") or type_spec.get("kind", "raw"))


def _options_for(resolved: dict[str, Any]) -> list[dict[str, Any]]:
    kind = str(resolved.get("kind", "raw"))
    if kind == "bool":
        return [
            {"value": "true", "label": "True"},
            {"value": "false", "label": "False"},
        ]
    if kind != "enum":
        return []
    options: list[dict[str, Any]] = []
    for constant in resolved.get("enumeratedConstants", []):
        label = str(constant.get("name"))
        annotation = constant.get("annotation")
        if annotation:
            label = f"{label} — {annotation}"
        options.append(
            {
                "value": str(constant.get("name")),
                "label": label,
                "annotation": annotation,
            }
        )
    return options


def _default_value_for(resolved: dict[str, Any]) -> Any:
    kind = str(resolved.get("kind", "raw"))
    default = resolved.get("default")
    if kind == "bool":
        if default is not None:
            if isinstance(default, bool):
                return "true" if default else "false"
            return "true" if str(default).strip().lower() in ("true", "1") else "false"
        return "false"
    if kind == "enum" and default is not None:
        return str(default).split(".")[-1]
    return default


def _placeholder_for(argument: dict[str, Any], resolved: dict[str, Any]) -> str:
    name = str(argument.get("name", "value"))
    kind = str(resolved.get("kind", "raw"))
    if kind == "integer":
        return f"Enter {name}"
    if kind == "float":
        return f"Enter {name}"
    if kind == "string":
        return f"Enter {name}"
    if kind == "enum":
        return f"Select {name}"
    if kind == "bool":
        return f"Select {name}"
    return f"Enter {name}"
