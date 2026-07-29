#!/usr/bin/env python3
"""Deterministically redact personal lab identifiers in preserved history."""

from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path

from build_public_evidence import (
    SOURCE_COMMIT,
    TEXT_EXTENSIONS,
    object_bytes,
    sanitize,
    tracked_paths,
)


def selected(path: str) -> bool:
    candidate = Path(path)
    if candidate.suffix.lower() not in TEXT_EXTENSIONS:
        return False
    if path.startswith("openspec/"):
        return True
    if (
        path.startswith("docs/test-records/")
        and "/artifacts/" not in path
        and candidate.name != "README.md"
    ):
        return True
    return path == "docs/verification-path-registry.md"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    args = parser.parse_args()
    source = args.source.resolve()
    repo_root = args.repo_root.resolve()

    selected_paths = [path for path in tracked_paths(source, ".") if selected(path)]
    totals: Counter[str] = Counter()
    changed = 0
    for path in selected_paths:
        original = object_bytes(source, path)
        public, redactions, treated_as_text = sanitize(source, path, original)
        if not treated_as_text:
            continue
        destination = repo_root / path
        if not destination.is_file():
            raise SystemExit(f"expected preserved history file is missing: {path}")
        destination.write_bytes(public)
        totals.update(redactions)
        changed += bool(redactions)

    print(
        json.dumps(
            {
                "sourceCommit": SOURCE_COMMIT,
                "filesInspected": len(selected_paths),
                "filesRedacted": changed,
                "redactions": dict(sorted(totals.items())),
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
