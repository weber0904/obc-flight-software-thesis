#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent

STATUS_RE = re.compile(r"^(?:Status|狀態)[:：]\s*.+$", re.MULTILINE)
FRESHNESS_RE = re.compile(
    r"^(?:Last (?:reconciled|reviewed|refreshed)(?: [^:：]+)?|Updated|更新日期)[:：]\s*.+$",
    re.MULTILINE,
)
MARKDOWN_LINK_RE = re.compile(r"\]\(([^)]+)\)")

CURRENT_DOCS = [
    Path("docs/README.md"),
    Path("docs/architecture/README.md"),
    Path("docs/architecture/current-development-architecture.md"),
    Path("docs/architecture/project-contributions.md"),
    Path("docs/architecture/target-flight-design.md"),
    Path("docs/roadmap/README.md"),
    Path("docs/roadmap/current-baseline.md"),
    Path("docs/roadmap/next-work.md"),
    Path("docs/interfaces.md"),
    Path("docs/operator/mission-console-phase1-runbook.md"),
    Path("docs/operator/target-obc-comm-csp-lab-runbook.md"),
    Path("docs/operator/target-proof-abc-governance.md"),
    Path("docs/operator/thesis-demo-routes.zh-TW.md"),
    Path("docs/verification-path-registry.md"),
    Path("docs/verification-matrix.md"),
    Path("docs/verification-debugging-lessons.md"),
    Path("docs/thesis/claim-evidence-map.zh-TW.md"),
]

ROOT_REQUIRED_LINKS = {
    Path("README.md"): [
        "docs/architecture/current-development-architecture.md",
        "docs/verification-matrix.md",
        "docs/verification-path-registry.md",
        "openspec/specs/",
        "docs/evidence/README.md",
    ],
    Path("CONTRIBUTING.md"): [
        "docs/verification-path-registry.md",
        "scripts/verification-manifest.json",
    ],
}

EXCLUDED_CURRENT_PREFIXES = (
    Path("docs/test-records"),
    Path("docs/roadmap/archive"),
)

FORBIDDEN_CURRENT_REFERENCES = (
    "docs/reporting/",
    "docs/architecture-review/",
    "mission-console-phase1-handoff.md",
    "mission-console-observability-recommendations.md",
    "comm-followup-directions.md",
    "formal-comm-verification-matrix-v1-runbook.md",
    "hosted-official-sequencing-system-resources-runbook.md",
    "thesis-architecture-draft.md",
    "AGENTS.md",
)


def read_text(rel_path: Path) -> str:
    try:
        return (ROOT / rel_path).read_text(encoding="utf-8")
    except FileNotFoundError:
        raise RuntimeError(f"missing file: {rel_path}") from None


def is_current_markdown(rel_path: Path) -> bool:
    if rel_path.suffix.lower() != ".md":
        return False
    return not any(
        rel_path == prefix or prefix in rel_path.parents
        for prefix in EXCLUDED_CURRENT_PREFIXES
    )


def iter_current_markdown() -> list[Path]:
    roots = [ROOT / "README.md", ROOT / "README.zh-TW.md", ROOT / "CONTRIBUTING.md", ROOT / "docs"]
    paths: list[Path] = []
    for candidate in roots:
        if candidate.is_file():
            paths.append(candidate.relative_to(ROOT))
        elif candidate.is_dir():
            paths.extend(
                path.relative_to(ROOT)
                for path in candidate.rglob("*.md")
                if is_current_markdown(path.relative_to(ROOT))
            )
    return sorted(set(paths))


def check_metadata(errors: list[str]) -> None:
    for rel_path in CURRENT_DOCS:
        text = read_text(rel_path)
        if not STATUS_RE.search(text):
            errors.append(f"{rel_path}: missing Status/狀態 metadata")
        if not FRESHNESS_RE.search(text):
            errors.append(f"{rel_path}: missing freshness metadata")


def check_required_links(errors: list[str]) -> None:
    for rel_path, targets in ROOT_REQUIRED_LINKS.items():
        text = read_text(rel_path)
        for target in targets:
            if target not in text:
                errors.append(f"{rel_path}: missing canonical link target '{target}'")


def check_current_references(errors: list[str]) -> None:
    for rel_path in iter_current_markdown():
        text = read_text(rel_path)
        for forbidden in FORBIDDEN_CURRENT_REFERENCES:
            if forbidden in text:
                errors.append(f"{rel_path}: references excluded current path '{forbidden}'")


def check_links(errors: list[str]) -> None:
    for rel_path in iter_current_markdown():
        text = read_text(rel_path)
        for raw_target in MARKDOWN_LINK_RE.findall(text):
            target = unquote(raw_target.strip().split("#", 1)[0])
            if not target or target.startswith(("http://", "https://", "mailto:")):
                continue
            if target.startswith("/"):
                errors.append(f"{rel_path}: absolute local link '{raw_target}'")
                continue
            resolved = (ROOT / rel_path.parent / target).resolve()
            try:
                resolved.relative_to(ROOT.resolve())
            except ValueError:
                errors.append(f"{rel_path}: link escapes repository '{raw_target}'")
                continue
            if not resolved.exists():
                errors.append(f"{rel_path}: broken link '{raw_target}'")


def run_checks(root: Path = ROOT) -> list[str]:
    del root  # retained for compatibility with check_repo_consistency.py
    errors: list[str] = []
    try:
        check_metadata(errors)
        check_required_links(errors)
        check_current_references(errors)
        check_links(errors)
    except RuntimeError as exc:
        errors.append(str(exc))
    return errors


def main() -> None:
    errors = run_checks()
    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
    print("PASS: public documentation governance checks passed")
    print(f"- current metadata documents: {len(CURRENT_DOCS)}")
    print(f"- current Markdown files: {len(iter_current_markdown())}")


if __name__ == "__main__":
    main()
