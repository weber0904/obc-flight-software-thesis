#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
from typing import Any


EXPECTED_UHF_REFERENCE = {
    "authorityProfile": "uhf-primary",
    "sourceId": 2,
    "keySlot": 2,
    "ingressPort": 1,
    "gdsFramingSelection": "space-packet-space-data-link",
    "vcid": 2,
    "checksumType": "modular",
}


def load_audit(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def compare_expected_fields(label: str, audit: dict[str, Any]) -> dict[str, Any]:
    fields = []
    for field, expected in EXPECTED_UHF_REFERENCE.items():
        fields.append(
            {
                "field": field,
                "matches": audit.get(field) == expected,
                "observed": audit.get(field),
                "expected": expected,
            }
        )
    return {
        "label": label,
        "allExpectedFieldsMatch": all(item["matches"] for item in fields),
        "fieldResults": fields,
    }


def packet_map(audit: dict[str, Any]) -> dict[tuple[str, int], str]:
    return {(packet["packetType"], int(packet["seqID"])): packet["encodedHex"] for packet in audit.get("packets", [])}


def compare_pairwise(left_label: str, left: dict[str, Any], right_label: str, right: dict[str, Any]) -> dict[str, Any]:
    left_packets = packet_map(left)
    right_packets = packet_map(right)
    packet_keys = sorted(set(left_packets.keys()) | set(right_packets.keys()))
    packet_results = []
    for packet_type, seq_id in packet_keys:
        left_hex = left_packets.get((packet_type, seq_id))
        right_hex = right_packets.get((packet_type, seq_id))
        packet_results.append(
            {
                "packetType": packet_type,
                "seqID": seq_id,
                "matches": left_hex == right_hex,
                f"{left_label}Hex": left_hex,
                f"{right_label}Hex": right_hex,
            }
        )
    shared_fields = []
    for field in (
        "compiledSequenceSha256",
        "fileSize",
        "checksumValue",
        "destinationPath",
        "sessionId",
        "sessionOpenSource",
        "switchSource",
        "sourcePath",
        "binaryPath",
    ):
        shared_fields.append(
            {
                "field": field,
                "matches": left.get(field) == right.get(field),
                left_label: left.get(field),
                right_label: right.get(field),
            }
        )
    leaf_fields = []
    for field in ("sourcePath", "binaryPath", "destinationPath"):
        left_value = str(left.get(field, ""))
        right_value = str(right.get(field, ""))
        left_leaf = left_value.split("/")[-1] if left_value else ""
        right_leaf = right_value.split("/")[-1] if right_value else ""
        leaf_fields.append(
            {
                "field": field,
                "matches": left_leaf == right_leaf,
                f"{left_label}Leaf": left_leaf,
                f"{right_label}Leaf": right_leaf,
            }
        )
    return {
        "leftLabel": left_label,
        "rightLabel": right_label,
        "allPacketsMatch": all(item["matches"] for item in packet_results),
        "allSharedFieldsMatch": all(item["matches"] for item in shared_fields),
        "allLeafFieldsMatch": all(item["matches"] for item in leaf_fields),
        "packetResults": packet_results,
        "sharedFieldResults": shared_fields,
        "leafFieldResults": leaf_fields,
    }


def render_pairwise(lines: list[str], title: str, pairwise: dict[str, Any]) -> None:
    left_label = pairwise["leftLabel"]
    right_label = pairwise["rightLabel"]
    lines.extend(
        [
            f"## {title}",
            "",
            f"- all-shared-fields-match: `{str(pairwise['allSharedFieldsMatch']).lower()}`",
            f"- all-leaf-fields-match: `{str(pairwise['allLeafFieldsMatch']).lower()}`",
            f"- all-packets-match: `{str(pairwise['allPacketsMatch']).lower()}`",
            "",
            f"| Field | Match | {left_label} | {right_label} |",
            "|---|---|---|---|",
        ]
    )
    for item in pairwise["sharedFieldResults"]:
        lines.append(f"| {item['field']} | {str(item['matches']).lower()} | `{item[left_label]}` | `{item[right_label]}` |")
    lines.extend(
        [
            "",
            f"| Leaf Field | Match | {left_label} | {right_label} |",
            "|---|---|---|---|",
        ]
    )
    for item in pairwise["leafFieldResults"]:
        lines.append(f"| {item['field']} | {str(item['matches']).lower()} | `{item[f'{left_label}Leaf']}` | `{item[f'{right_label}Leaf']}` |")
    lines.extend(
        [
            "",
            "| Packet | Match |",
            "|---|---|",
        ]
    )
    for item in pairwise["packetResults"]:
        lines.append(f"| {item['packetType']} seq={item['seqID']} | {str(item['matches']).lower()} |")
    lines.append("")


def render_markdown(summary: dict[str, Any]) -> str:
    lines = [
        "# Sequence Packet Audit Diff",
        "",
        "## Expected UHF Fields",
        "",
    ]
    for expected in summary["expectedFieldChecks"]:
        lines.extend(
            [
                f"### {expected['label']}",
                "",
                f"- all-expected-fields-match: `{str(expected['allExpectedFieldsMatch']).lower()}`",
                "",
                "| Field | Match | Observed | Expected |",
                "|---|---|---|---|",
            ]
        )
        for item in expected["fieldResults"]:
            lines.append(f"| {item['field']} | {str(item['matches']).lower()} | `{item['observed']}` | `{item['expected']}` |")
        lines.append("")
    render_pairwise(lines, "Hosted vs Target TCP", summary["hostedVsTargetTcp"])
    if summary.get("targetTcpVsTargetCan"):
        render_pairwise(lines, "Target TCP vs Target CAN", summary["targetTcpVsTargetCan"])
    if summary.get("hostedVsTargetCan"):
        render_pairwise(lines, "Hosted vs Target CAN", summary["hostedVsTargetCan"])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare hosted and target TCP sequence packet audits.")
    parser.add_argument("--hosted", required=True)
    parser.add_argument("--target-tcp", required=True)
    parser.add_argument("--target-can")
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-md")
    args = parser.parse_args()

    hosted = load_audit(pathlib.Path(args.hosted))
    target_tcp = load_audit(pathlib.Path(args.target_tcp))
    target_can = load_audit(pathlib.Path(args.target_can)) if args.target_can else None
    summary = {
        "expectedFieldChecks": [
            compare_expected_fields("hosted-node6-fail", hosted),
            compare_expected_fields("target-tcp-node6-fail", target_tcp),
            *( [compare_expected_fields("target-can-node6-pass", target_can)] if target_can is not None else [] ),
        ],
        "hostedVsTargetTcp": compare_pairwise("hosted", hosted, "targetTcp", target_tcp),
        "targetTcpVsTargetCan": compare_pairwise("targetTcp", target_tcp, "targetCan", target_can) if target_can is not None else None,
        "hostedVsTargetCan": compare_pairwise("hosted", hosted, "targetCan", target_can) if target_can is not None else None,
    }
    output_json = pathlib.Path(args.output_json)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if args.output_md:
        pathlib.Path(args.output_md).write_text(render_markdown(summary) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
