#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
JSON_PATH = ROOT / "openspec" / "reconciliation" / "baseline-reconciliation-matrix.json"
MD_PATH = ROOT / "openspec" / "reconciliation" / "baseline-reconciliation-matrix.md"

SECTION_ORDER = (
    ("bootstrap-governance", "Bootstrap Governance"),
    ("initial-baseline-queue", "Initial Baseline Queue"),
    ("later-governed-expansion", "Later Governed Expansions"),
)

TRAIL_CLASSES = {"follow-up-fix", "governance-maintenance"}


def load_matrix(path: Path = JSON_PATH) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def rel_link(rel_path: str) -> str:
    href = (Path("../..") / rel_path).as_posix()
    return f"[`{rel_path}`]({href})"


def render_capabilities(capabilities: list[str]) -> str:
    return ", ".join(f"`{name}`" for name in capabilities) if capabilities else "none"


def render_review_trail(entry: dict) -> str:
    evidence_paths = entry.get("evidencePaths", [])
    pieces = [rel_link(path) for path in evidence_paths]
    exception = entry.get("exception")
    if exception:
        pieces.append(exception)
    return "<br>".join(pieces) if pieces else "none"


def render_table_row(columns: list[str]) -> str:
    escaped = [value.replace("|", "\\|").replace("\n", " ").strip() for value in columns]
    return "| " + " | ".join(escaped) + " |"


def render_section(entries: list[dict], title: str) -> list[str]:
    lines = [f"### {title}", ""]
    if title == "Bootstrap Governance":
        lines.extend(
            [
                "| Archived change | Primary capability coverage | Evidence / review trail |",
                "|---|---|---|",
            ]
        )
        for entry in entries:
            lines.append(
                render_table_row(
                    [
                        f"`{entry['archivedChange']}`",
                        render_capabilities(entry.get("capabilities", [])),
                        render_review_trail(entry),
                    ]
                )
            )
        lines.append("")
        return lines

    lines.extend(
        [
            "| Archived change | Primary capability coverage | Evidence |",
            "|---|---|---|",
        ]
    )
    for entry in entries:
        lines.append(
            render_table_row(
                [
                    f"`{entry['archivedChange']}`",
                    render_capabilities(entry.get("capabilities", [])),
                    render_review_trail(entry),
                ]
            )
        )
    lines.append("")
    return lines


def render_trail_section(entries: list[dict]) -> list[str]:
    lines = [
        "### Follow-up Fixes And Governance Exceptions",
        "",
        "| Archived change | Class | Review trail |",
        "|---|---|---|",
    ]
    for entry in entries:
        classification = entry.get("classification", "other").replace("-", " ")
        lines.append(
            render_table_row(
                [
                    f"`{entry['archivedChange']}`",
                    classification,
                    render_review_trail(entry),
                ]
            )
        )
    lines.append("")
    return lines


def render_markdown(matrix: dict) -> str:
    entry_map: dict[str, list[dict]] = {}
    trail_entries: list[dict] = []
    other_entries: list[dict] = []

    for entry in matrix.get("entries", []):
        classification = entry.get("classification", "other")
        if classification in TRAIL_CLASSES:
            trail_entries.append(entry)
        elif any(classification == key for key, _ in SECTION_ORDER):
            entry_map.setdefault(classification, []).append(entry)
        else:
            other_entries.append(entry)

    lines = [
        "# Baseline Reconciliation Matrix",
        "",
        "This file is generated from [`openspec/reconciliation/baseline-reconciliation-matrix.json`](baseline-reconciliation-matrix.json). Do not edit it by hand.",
        "",
        "This document is the human-readable review surface for the repository's baseline reconciliation data. The JSON file remains the only manually maintained source of truth.",
        "",
        "The purpose of this matrix is to make three facts reviewable in one place:",
        "",
        "1. which archived changes completed the initial baseline queue",
        "2. which later archived changes became governed expansions",
        "3. which archived changes intentionally reuse existing evidence or exist as governance or follow-up exceptions",
        "",
        "## Current Main-Spec Capabilities",
        "",
        "The current formal capability set is:",
        "",
    ]

    for capability in matrix.get("currentCapabilities", []):
        lines.append(f"- `{capability}`")

    queue = matrix.get("initialBaselineQueue", [])
    if queue:
        lines.extend(
            [
                "",
                "## Initial Baseline Queue",
                "",
                "The repository's initial baseline queue was:",
                "",
            ]
        )
        for index, change in enumerate(queue, start=1):
            lines.append(f"{index}. `{change}`")
        lines.extend(
            [
                "",
                "All of those changes remain reviewable through the archived history and the reconciliation entries below.",
            ]
        )

    lines.extend(["", "## Archived Change Classes", ""])

    for key, title in SECTION_ORDER:
        entries = entry_map.get(key, [])
        if entries:
            lines.extend(render_section(entries, title))

    if trail_entries:
        lines.extend(render_trail_section(trail_entries))

    if other_entries:
        lines.extend(
            [
                "### Other Archived Entries",
                "",
                "| Archived change | Class | Review trail |",
                "|---|---|---|",
            ]
        )
        for entry in other_entries:
            lines.append(
                render_table_row(
                    [
                        f"`{entry['archivedChange']}`",
                        entry.get("classification", "other").replace("-", " "),
                        render_review_trail(entry),
                    ]
                )
            )
        lines.append("")

    lines.extend(
        [
            "## Review Rules",
            "",
            "- Every archived change with `tasks.md` must appear in the JSON matrix.",
            "- A change may cite one or more existing evidence paths instead of creating a new `evidence/records/<change>/` directory, but that reuse must be explicit in the matrix.",
            "- A change may omit evidence paths only if its exception rationale is recorded in the matrix.",
            "- Main-spec capabilities are governed by `openspec/specs/`; the matrix is an audit and reconciliation layer, not a second spec system.",
            "",
        ]
    )
    return "\n".join(lines)


def write_markdown(output_path: Path = MD_PATH) -> None:
    matrix = load_matrix()
    output_path.write_text(render_markdown(matrix), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--stdout",
        action="store_true",
        help="write the generated Markdown to stdout instead of updating the checked-in file",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    markdown = render_markdown(load_matrix())
    if args.stdout:
        print(markdown, end="")
        return
    MD_PATH.write_text(markdown, encoding="utf-8")
    print(f"updated {MD_PATH.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
