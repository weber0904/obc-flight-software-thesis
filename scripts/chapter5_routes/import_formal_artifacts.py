#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import shutil
import stat
from dataclasses import dataclass

from check_formal_attempt_artifacts import (
    ATTEMPT_MANIFEST_SCHEMA_VERSION,
    collect_retained_files,
)


TMP_PATH_RE = re.compile(r"(/(?:private/tmp|tmp|var/folders/[^\s\"'`]+)[^\s\"'`)]*)")
SAFE_ATTEMPT_LABEL_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*")
TEXT_SUFFIXES = {
    ".log",
    ".txt",
    ".json",
    ".jsonl",
    ".md",
    ".csv",
    ".tsv",
    ".yaml",
    ".yml",
}


@dataclass(frozen=True)
class CopiedArtifact:
    source: pathlib.Path
    relative_destination: pathlib.Path
    kind: str
    discovered_from: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--destination-root", required=True)
    parser.add_argument("--route", required=True, choices=("route1", "route2", "route3"))
    parser.add_argument("--surface", required=True, choices=("hosted", "target"))
    parser.add_argument(
        "--attempt-label",
        default="",
        help="Optional attempt directory below <route>/<surface>, e.g. attempt-01.",
    )
    parser.add_argument("--wrapper-root", required=True)
    parser.add_argument("--command", required=True)
    parser.add_argument("--verdict", required=True)
    parser.add_argument("--retry-count", type=int, default=0)
    parser.add_argument(
        "--retry-of",
        default="",
        help="Prior attempt label when this import is a fresh retry.",
    )
    parser.add_argument(
        "--failure-class",
        default="",
        help="Classification for a failed or retried attempt.",
    )
    parser.add_argument("--retry-reason", action="append", default=[])
    parser.add_argument("--note", action="append", default=[])
    parser.add_argument("--extra-root", action="append", default=[])
    parser.add_argument("--timestamp", default="")
    return parser.parse_args()


def safe_name(path: pathlib.Path) -> str:
    return re.sub(r"[^A-Za-z0-9._-]+", "_", path.name or "artifact")


def require_safe_attempt_label(value: str) -> None:
    if value and SAFE_ATTEMPT_LABEL_RE.fullmatch(value) is None:
        raise SystemExit(
            "--attempt-label must be one safe path component "
            "using only letters, digits, '.', '_', or '-'"
        )


def read_text_lossy(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def discover_external_paths(wrapper_root: pathlib.Path) -> dict[pathlib.Path, str]:
    discovered: dict[pathlib.Path, str] = {}
    for path in wrapper_root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in TEXT_SUFFIXES and path.name not in {"summary.log", "events.log", "channels.log"}:
            continue
        try:
            text = read_text_lossy(path)
        except OSError:
            continue
        for match in TMP_PATH_RE.finditer(text):
            candidate = pathlib.Path(match.group(1)).resolve()
            if candidate.exists():
                discovered.setdefault(candidate, str(path.relative_to(wrapper_root)))
    return discovered


def ensure_unique_destination(base_dir: pathlib.Path, artifact_name: str) -> pathlib.Path:
    candidate = base_dir / artifact_name
    if not candidate.exists():
        return candidate
    index = 2
    while True:
        candidate = base_dir / f"{artifact_name}-{index}"
        if not candidate.exists():
            return candidate
        index += 1


def is_special_runtime_entry(path: pathlib.Path) -> bool:
    try:
        mode = path.lstat().st_mode
    except OSError:
        return True
    return any(
        (
            stat.S_ISSOCK(mode),
            stat.S_ISFIFO(mode),
            stat.S_ISBLK(mode),
            stat.S_ISCHR(mode),
        )
    )


def copy_path(source: pathlib.Path, destination: pathlib.Path) -> bool:
    if source.is_dir():
        destination.mkdir(parents=True, exist_ok=True)
        copied_any = False
        for child in source.iterdir():
            if is_special_runtime_entry(child):
                continue
            copied_any = copy_path(child, destination / child.name) or copied_any
        return copied_any
    else:
        if is_special_runtime_entry(source):
            return False
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        return True


def is_within(child: pathlib.Path, parent: pathlib.Path) -> bool:
    try:
        child.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def main() -> int:
    args = parse_args()
    require_safe_attempt_label(args.attempt_label)
    destination_root = pathlib.Path(args.destination_root).resolve()
    wrapper_root = pathlib.Path(args.wrapper_root).resolve()
    if not wrapper_root.exists():
        raise SystemExit(f"wrapper root does not exist: {wrapper_root}")

    surface_root = destination_root / args.route / args.surface
    route_root = surface_root / args.attempt_label if args.attempt_label else surface_root
    wrapper_dest_parent = route_root / "wrapper-root"
    external_dest_parent = route_root / "external-roots"
    manifest_path = route_root / "manifest.json"
    wrapper_dest_parent.mkdir(parents=True, exist_ok=True)
    external_dest_parent.mkdir(parents=True, exist_ok=True)

    copied: list[CopiedArtifact] = []

    wrapper_dest = ensure_unique_destination(wrapper_dest_parent, safe_name(wrapper_root))
    copy_path(wrapper_root, wrapper_dest)
    copied.append(
        CopiedArtifact(
            source=wrapper_root,
            relative_destination=wrapper_dest.relative_to(route_root),
            kind="wrapper-root",
            discovered_from="explicit-wrapper-root",
        )
    )

    discovered = discover_external_paths(wrapper_root)
    for extra in args.extra_root:
        extra_path = pathlib.Path(extra).resolve()
        if extra_path.exists():
            discovered.setdefault(extra_path, "explicit-extra-root")

    kept_external_sources: list[pathlib.Path] = []
    for source, discovered_from in sorted(discovered.items(), key=lambda item: (len(item[0].parts), str(item[0]))):
        if is_within(source, wrapper_root):
            continue
        if any(is_within(source, kept_source) for kept_source in kept_external_sources):
            continue
        artifact_name = safe_name(source)
        destination = ensure_unique_destination(external_dest_parent, artifact_name)
        if not copy_path(source, destination):
            continue
        kept_external_sources.append(source)
        copied.append(
            CopiedArtifact(
                source=source,
                relative_destination=destination.relative_to(route_root),
                kind="external-root",
                discovered_from=discovered_from,
            )
        )

    manifest = {
        "schemaVersion": ATTEMPT_MANIFEST_SCHEMA_VERSION,
        "route": args.route,
        "surface": args.surface,
        "attemptLabel": args.attempt_label or None,
        "command": args.command,
        "verdict": args.verdict,
        "retryCount": args.retry_count,
        "retryOf": args.retry_of or None,
        "failureClass": args.failure_class or None,
        "retryReasons": args.retry_reason,
        "timestamp": args.timestamp or "",
        "notes": args.note,
        "artifacts": [
            {
                "kind": artifact.kind,
                "source": str(artifact.source),
                "relativeDestination": str(artifact.relative_destination),
                "discoveredFrom": artifact.discovered_from,
            }
            for artifact in copied
        ],
        "retainedFiles": collect_retained_files(route_root),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(manifest_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
