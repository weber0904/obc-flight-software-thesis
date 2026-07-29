#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
MARKDOWN_LINK_RE = re.compile(r"\]\(([^)]+)\)")

CURRENT_DOCS = [
    Path("README.md"),
    Path("README.zh-TW.md"),
    Path("CONTRIBUTING.md"),
    Path("SECURITY.md"),
    Path("docs/README.md"),
    Path("docs/architecture.md"),
    Path("docs/interfaces.md"),
    Path("docs/verification.md"),
    Path("docs/operator/hosted.md"),
    Path("docs/operator/mission-console.md"),
    Path("docs/operator/simulator-controls.zh-TW.md"),
    Path("docs/operator/target-lab.md"),
    Path("docs/operator/thesis-demo.zh-TW.md"),
    Path("docs/thesis.md"),
    Path("evidence/README.md"),
    Path("evidence/records/public-thesis-submission-v1/README.md"),
    Path("release/RELEASE_PROVENANCE.md"),
]

ROOT_REQUIRED_LINKS = {
    Path("README.md"): [
        "docs/architecture.md",
        "docs/interfaces.md",
        "docs/verification.md",
        "docs/README.md",
        "evidence/README.md",
        "openspec/specs/",
    ],
    Path("docs/README.md"): [
        "architecture.md",
        "interfaces.md",
        "verification.md",
        "operator/hosted.md",
        "operator/target-lab.md",
        "operator/mission-console.md",
        "thesis.md",
        "../evidence/README.md",
    ],
    Path("CONTRIBUTING.md"): [
        "docs/verification.md",
        "evidence/verification-path-registry.md",
        "scripts/verification-manifest.json",
    ],
}

FORBIDDEN_PORTFOLIO_PATTERNS = (
    re.compile(r"\bcurated public\b", re.IGNORECASE),
    re.compile(r"\bpublication cleanup\b", re.IGNORECASE),
    re.compile(r"\bpreviously demonstrated\b", re.IGNORECASE),
    re.compile(r"\bfresh target\b", re.IGNORECASE),
    re.compile(r"\bnon-claims?\b", re.IGNORECASE),
    re.compile(r"\bnot claimed\b", re.IGNORECASE),
    re.compile(r"\bdoes not claim\b", re.IGNORECASE),
    re.compile(r"公開版"),
    re.compile(r"已從公開"),
    re.compile(r"先前已展示"),
    re.compile(r"不宣稱"),
    re.compile(r"沒有在.*重新執行"),
)


def read_text(rel_path: Path) -> str:
    try:
        return (ROOT / rel_path).read_text(encoding="utf-8")
    except FileNotFoundError:
        raise RuntimeError(f"missing file: {rel_path}") from None


def check_required_links(errors: list[str]) -> None:
    for rel_path, targets in ROOT_REQUIRED_LINKS.items():
        text = read_text(rel_path)
        for target in targets:
            if target not in text:
                errors.append(f"{rel_path}: missing canonical link target '{target}'")


def check_portfolio_language(errors: list[str]) -> None:
    for rel_path in CURRENT_DOCS:
        text = read_text(rel_path)
        for pattern in FORBIDDEN_PORTFOLIO_PATTERNS:
            match = pattern.search(text)
            if match:
                errors.append(
                    f"{rel_path}: portfolio prose contains release-process wording "
                    f"'{match.group(0)}'"
                )


def check_links(errors: list[str]) -> None:
    for rel_path in CURRENT_DOCS:
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


def check_docs_surface(errors: list[str]) -> None:
    allowed = {
        Path("docs/README.md"),
        Path("docs/architecture.md"),
        Path("docs/interfaces.md"),
        Path("docs/verification.md"),
        Path("docs/thesis.md"),
        Path("docs/operator/hosted.md"),
        Path("docs/operator/mission-console.md"),
        Path("docs/operator/simulator-controls.zh-TW.md"),
        Path("docs/operator/target-lab.md"),
        Path("docs/operator/thesis-demo.zh-TW.md"),
    }
    observed = {
        path.relative_to(ROOT)
        for path in (ROOT / "docs").rglob("*.md")
    }
    if observed != allowed:
        errors.append(
            "docs/ Markdown surface is not canonical: "
            f"missing={sorted(str(path) for path in allowed - observed)}, "
            f"extra={sorted(str(path) for path in observed - allowed)}"
        )


def run_checks(root: Path = ROOT) -> list[str]:
    del root
    errors: list[str] = []
    try:
        check_docs_surface(errors)
        check_required_links(errors)
        check_portfolio_language(errors)
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
    print(f"- canonical reader documents: {len(CURRENT_DOCS)}")
    print("- docs Markdown files: 10")


if __name__ == "__main__":
    main()
