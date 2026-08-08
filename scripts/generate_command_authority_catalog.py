#!/usr/bin/env python3
"""Generate the command ingress authority opcode catalog from FPP dictionaries."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
import tempfile
from dataclasses import dataclass


CLASS_ENUM = {
    "UNKNOWN": "AuthorityCommandClass::UNKNOWN",
    "READ_STATUS": "AuthorityCommandClass::READ_STATUS",
    "MODE_CHANGE": "AuthorityCommandClass::MODE_CHANGE",
    "SAFETY_EMERGENCY": "AuthorityCommandClass::SAFETY_EMERGENCY",
    "FILE_TRANSFER": "AuthorityCommandClass::FILE_TRANSFER",
    "CONFIG_UPDATE": "AuthorityCommandClass::CONFIG_UPDATE",
    "RESET": "AuthorityCommandClass::RESET",
    "PAYLOAD_CONTROL": "AuthorityCommandClass::PAYLOAD_CONTROL",
    "ADCS_CONTROL": "AuthorityCommandClass::ADCS_CONTROL",
    "POWER_CONTROL": "AuthorityCommandClass::POWER_CONTROL",
    "COMM_CONTROL": "AuthorityCommandClass::COMM_CONTROL",
    "CSP_TRAFFIC": "AuthorityCommandClass::CSP_TRAFFIC",
    "DEV_INTERNAL": "AuthorityCommandClass::DEV_INTERNAL",
    "DATA_PRODUCT": "AuthorityCommandClass::DATA_PRODUCT",
}

RESOURCE_ENUM = {
    "NONE": "AuthorityResourceLabel::NONE",
    "MODE_STATE": "AuthorityResourceLabel::MODE_STATE",
    "EPS": "AuthorityResourceLabel::EPS",
    "ADCS": "AuthorityResourceLabel::ADCS",
    "GPS": "AuthorityResourceLabel::GPS",
    "RADIO": "AuthorityResourceLabel::RADIO",
    "STORAGE": "AuthorityResourceLabel::STORAGE",
    "BOOT": "AuthorityResourceLabel::BOOT",
    "HEALTH": "AuthorityResourceLabel::HEALTH",
    "CSP": "AuthorityResourceLabel::CSP",
    "COMM_LINK": "AuthorityResourceLabel::COMM_LINK",
    "FILE_TRANSFER_SESSION": "AuthorityResourceLabel::FILE_TRANSFER_SESSION",
    "CONFIG_STORE": "AuthorityResourceLabel::CONFIG_STORE",
    "DATA_PRODUCTS": "AuthorityResourceLabel::DATA_PRODUCTS",
    "EVENT_FILTER": "AuthorityResourceLabel::EVENT_FILTER",
    "COMMAND_DISPATCH": "AuthorityResourceLabel::COMMAND_DISPATCH",
    "PAYLOAD": "AuthorityResourceLabel::PAYLOAD",
}


@dataclass(frozen=True)
class Policy:
    name: str
    opcode: int
    command_class: str
    resource: str
    uhf_backup_allowed: bool


def load_commands(dictionary_path: pathlib.Path) -> list[tuple[str, int]]:
    data = json.loads(dictionary_path.read_text())
    commands = data.get("commands", [])
    if not isinstance(commands, list):
        raise ValueError(f"{dictionary_path}: commands must be a list")
    result: list[tuple[str, int]] = []
    for command in commands:
        name = command.get("name")
        opcode = command.get("opcode")
        if not isinstance(name, str) or not isinstance(opcode, int):
            raise ValueError(f"{dictionary_path}: command entries require string name and integer opcode")
        result.append((name, opcode))
    return result


def load_policy(policy_path: pathlib.Path, dictionaries: list[pathlib.Path]) -> list[Policy]:
    policy_data = json.loads(policy_path.read_text())
    policy_commands = policy_data.get("commands", {})
    if not isinstance(policy_commands, dict):
        raise ValueError(f"{policy_path}: commands must be an object keyed by fully qualified command name")

    all_commands: list[tuple[str, int]] = []
    for dictionary in dictionaries:
        all_commands.extend(load_commands(dictionary))

    command_names = {name for name, _ in all_commands}
    policy_names = set(policy_commands.keys())
    missing = sorted(command_names - policy_names)
    stale = sorted(policy_names - command_names)
    if missing:
        raise ValueError("Policy is missing command classifications:\n" + "\n".join(missing))
    if stale:
        raise ValueError("Policy contains stale command classifications:\n" + "\n".join(stale))

    policies: list[Policy] = []
    for name, opcode in all_commands:
        entry = policy_commands[name]
        command_class = entry.get("class")
        resource = entry.get("resource")
        uhf_backup_allowed = entry.get("uhf_backup_allowed")
        if command_class not in CLASS_ENUM:
            raise ValueError(f"{name}: invalid class {command_class!r}")
        if resource not in RESOURCE_ENUM:
            raise ValueError(f"{name}: invalid resource {resource!r}")
        if not isinstance(uhf_backup_allowed, bool):
            raise ValueError(f"{name}: uhf_backup_allowed must be boolean")
        policies.append(Policy(name, opcode, command_class, resource, uhf_backup_allowed))
    return policies


def collapse_by_opcode(policies: list[Policy]) -> list[Policy]:
    by_opcode: dict[int, Policy] = {}
    for policy in policies:
        existing = by_opcode.get(policy.opcode)
        if existing is None:
            by_opcode[policy.opcode] = policy
            continue
        comparable = (policy.command_class, policy.resource, policy.uhf_backup_allowed)
        existing_comparable = (existing.command_class, existing.resource, existing.uhf_backup_allowed)
        if comparable != existing_comparable:
            raise ValueError(
                f"Opcode {policy.opcode} has conflicting classifications: {existing.name} vs {policy.name}"
            )
    return [by_opcode[opcode] for opcode in sorted(by_opcode)]


def render_catalog(entries: list[Policy]) -> str:
    lines = [
        '#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"',
        "",
        "namespace OBC {",
        "",
        "namespace {",
        "",
        "const CommandAuthorityCatalogEntry COMMAND_AUTHORITY_CATALOG[] = {",
    ]
    for entry in entries:
        lines.append(
            f'    {{{entry.opcode}U, "{entry.name}", {CLASS_ENUM[entry.command_class]}, '
            f"{RESOURCE_ENUM[entry.resource]}, {'true' if entry.uhf_backup_allowed else 'false'}}},"
        )
    lines.extend(
        [
            "};",
            "",
            "}  // namespace",
            "",
            "const CommandAuthorityCatalogEntry* findCommandAuthorityCatalogEntry(FwOpcodeType opcode) {",
            "    FwSizeType low = 0;",
            "    FwSizeType high = getCommandAuthorityCatalogEntryCount();",
            "    while (low < high) {",
            "        const FwSizeType mid = low + ((high - low) / 2);",
            "        if (COMMAND_AUTHORITY_CATALOG[mid].opcode < opcode) {",
            "            low = mid + 1;",
            "        } else {",
            "            high = mid;",
            "        }",
            "    }",
            "    if (low < getCommandAuthorityCatalogEntryCount() && COMMAND_AUTHORITY_CATALOG[low].opcode == opcode) {",
            "        return &COMMAND_AUTHORITY_CATALOG[low];",
            "    }",
            "    return nullptr;",
            "}",
            "",
            "FwSizeType getCommandAuthorityCatalogEntryCount() {",
            "    return static_cast<FwSizeType>(sizeof(COMMAND_AUTHORITY_CATALOG) / sizeof(COMMAND_AUTHORITY_CATALOG[0]));",
            "}",
            "",
            "AuthorityDecision evaluateCommandAuthority(const AuthorityConfig& config, FwOpcodeType opcode) {",
            "    AuthorityDecision decision;",
            "    if (!config.valid) {",
            "        decision.reason = AuthorityRejectReason::INVALID_CONFIG;",
            "        decision.response = Fw::CmdResponse::EXECUTION_ERROR;",
            "        return decision;",
            "    }",
            "    const CommandAuthorityCatalogEntry* entry = findCommandAuthorityCatalogEntry(opcode);",
            "    if (entry == nullptr) {",
            "        if (isCommManagedAuthority(config)) {",
            "            decision.reason = AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED;",
            "            decision.response = Fw::CmdResponse::INVALID_OPCODE;",
            "        } else {",
            "            decision.allow = true;",
            "            decision.reason = AuthorityRejectReason::NONE;",
            "            decision.response = Fw::CmdResponse::OK;",
            "        }",
            "        return decision;",
            "    }",
            "    decision.commandClass = entry->commandClass;",
            "    decision.resource = entry->resource;",
            "    if (!isRestrictedAuthority(config) || entry->uhfBackupAllowed) {",
            "        decision.allow = true;",
            "        decision.reason = AuthorityRejectReason::NONE;",
            "        decision.response = Fw::CmdResponse::OK;",
            "    } else {",
            "        decision.reason = AuthorityRejectReason::POLICY_DENIED;",
            "        decision.response = Fw::CmdResponse::VALIDATION_ERROR;",
            "    }",
            "    return decision;",
            "}",
            "",
            "}  // namespace OBC",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--policy", required=True, type=pathlib.Path)
    parser.add_argument("--dictionary", required=True, action="append", type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    try:
        entries = collapse_by_opcode(load_policy(args.policy, args.dictionary))
        generated = render_catalog(entries)
        if args.check:
            with tempfile.NamedTemporaryFile("w", delete=False) as tmp:
                tmp.write(generated)
                tmp_path = pathlib.Path(tmp.name)
            try:
                if args.output.read_text() != tmp_path.read_text():
                    raise ValueError(f"{args.output} is not up to date; rerun {pathlib.Path(__file__).name}")
            finally:
                tmp_path.unlink(missing_ok=True)
        else:
            args.output.write_text(generated)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
