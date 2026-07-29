#!/usr/bin/env python3

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

from check_documentation_governance import run_checks as run_documentation_governance_checks
from generate_reconciliation_matrix_md import render_markdown

ROOT = Path(__file__).resolve().parent.parent
SPECS_DIR = ROOT / "openspec" / "specs"
ARCHIVE_DIR = ROOT / "openspec" / "changes" / "archive"
MATRIX_PATH = ROOT / "docs" / "baseline-reconciliation-matrix.json"
MATRIX_MD_PATH = ROOT / "docs" / "baseline-reconciliation-matrix.md"

PLACEHOLDER_PATTERNS = (
    re.compile(r"\bTBD\b", re.IGNORECASE),
    re.compile(r"placeholder", re.IGNORECASE),
    re.compile(r"Update Purpose after archive", re.IGNORECASE),
    re.compile(r"created by archiving change", re.IGNORECASE),
)


def load_matrix() -> dict:
    try:
        return json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing matrix file: {MATRIX_PATH.relative_to(ROOT)}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {MATRIX_PATH.relative_to(ROOT)}: {exc}")


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def warn(message: str) -> None:
    print(f"WARN: {message}")


def collect_main_specs() -> dict[str, Path]:
    specs: dict[str, Path] = {}
    for spec_dir in sorted(SPECS_DIR.iterdir()):
        spec_path = spec_dir / "spec.md"
        if spec_dir.is_dir() and spec_path.is_file():
            specs[spec_dir.name] = spec_path
    return specs


def collect_archived_changes() -> list[str]:
    changes: list[str] = []
    for change_dir in sorted(ARCHIVE_DIR.iterdir()):
        if change_dir.is_dir() and (change_dir / "tasks.md").is_file():
            changes.append(change_dir.name)
    return changes


def collect_active_changes() -> set[str]:
    active: set[str] = set()
    changes_dir = ROOT / "openspec" / "changes"
    for change_dir in sorted(changes_dir.iterdir()):
        if (
            change_dir.is_dir()
            and change_dir.name != "archive"
            and (change_dir / ".openspec.yaml").is_file()
        ):
            active.add(change_dir.name)
    return active


def extract_purpose_body(spec_path: Path) -> str:
    text = spec_path.read_text(encoding="utf-8")
    match = re.search(r"^## Purpose\s*$", text, re.MULTILINE)
    if not match:
        fail(f"missing '## Purpose' section in {spec_path.relative_to(ROOT)}")
    remainder = text[match.end() :]
    purpose_lines: list[str] = []
    for raw_line in remainder.splitlines():
        line = raw_line.strip()
        if not line:
            if purpose_lines:
                break
            continue
        if line.startswith("## "):
            break
        purpose_lines.append(line)
    if not purpose_lines:
        fail(f"empty Purpose section in {spec_path.relative_to(ROOT)}")
    return " ".join(purpose_lines)


def validate_purposes(specs: dict[str, Path]) -> None:
    for name, spec_path in specs.items():
        purpose = extract_purpose_body(spec_path)
        for pattern in PLACEHOLDER_PATTERNS:
            if pattern.search(purpose):
                fail(
                    f"placeholder Purpose text remains in {spec_path.relative_to(ROOT)} "
                    f"for capability '{name}': {purpose}"
                )


def validate_matrix(
    matrix: dict, specs: dict[str, Path], archived_changes: list[str], active_changes: set[str]
) -> None:
    spec_names = set(specs)

    matrix_capabilities = set(matrix.get("currentCapabilities", []))
    if matrix_capabilities != spec_names:
        missing = sorted(spec_names - matrix_capabilities)
        extra = sorted(matrix_capabilities - spec_names)
        fail(
            "currentCapabilities does not match openspec/specs: "
            f"missing={missing}, extra={extra}"
        )

    entries = matrix.get("entries", [])
    if not isinstance(entries, list) or not entries:
        fail("matrix entries must be a non-empty list")

    entry_by_change: dict[str, dict] = {}
    for entry in entries:
        change = entry.get("archivedChange")
        if not change:
            fail("matrix entry missing archivedChange")
        if change in entry_by_change:
            fail(f"duplicate matrix entry for archived change: {change}")
        entry_by_change[change] = entry

        capabilities = entry.get("capabilities", [])
        if not capabilities:
            fail(f"matrix entry {change} must list at least one capability")
        unknown_capabilities = sorted(set(capabilities) - spec_names)
        if unknown_capabilities:
            fail(f"matrix entry {change} cites unknown capabilities: {unknown_capabilities}")

        evidence_paths = entry.get("evidencePaths", [])
        exception = entry.get("exception")
        if not evidence_paths and not exception:
            fail(f"matrix entry {change} must declare evidencePaths or an exception rationale")

        for rel_path in evidence_paths:
            evidence_path = ROOT / rel_path
            if not evidence_path.is_file():
                fail(f"matrix entry {change} cites missing evidence path: {rel_path}")

    archived_set = set(archived_changes)
    entry_set = set(entry_by_change)
    missing_entries = sorted(archived_set - entry_set)
    extra_entries = sorted(entry_set - archived_set)
    unsupported_extra_entries = [
        entry for entry in extra_entries if normalize_change_name(entry) not in active_changes
    ]
    if missing_entries or unsupported_extra_entries:
        fail(
            "matrix archivedChange coverage mismatch: "
            f"missing_entries={missing_entries}, extra_entries={unsupported_extra_entries}"
        )


def normalize_change_name(archived_dir_name: str) -> str:
    return re.sub(r"^[0-9-]+-", "", archived_dir_name)


def validate_matrix_markdown(matrix: dict) -> None:
    if not MATRIX_MD_PATH.is_file():
        fail(f"missing generated matrix file: {MATRIX_MD_PATH.relative_to(ROOT)}")
    expected = render_markdown(matrix)
    actual = MATRIX_MD_PATH.read_text(encoding="utf-8")
    if actual != expected:
        fail(
            "baseline reconciliation markdown is out of date; "
            "run `python3 scripts/generate_reconciliation_matrix_md.py`"
        )


def main() -> None:
    matrix = load_matrix()
    specs = collect_main_specs()
    archived_changes = collect_archived_changes()
    active_changes = collect_active_changes()
    doc_errors = run_documentation_governance_checks(ROOT)
    if doc_errors:
        fail("documentation governance errors:\n- " + "\n- ".join(doc_errors))
    validate_purposes(specs)
    validate_matrix(matrix, specs, archived_changes, active_changes)
    validate_matrix_markdown(matrix)
    print("PASS: repo consistency checks passed")
    print(f"- main specs checked: {len(specs)}")
    print(f"- archived changes checked: {len(archived_changes)}")
    print(f"- matrix source: {MATRIX_PATH.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
