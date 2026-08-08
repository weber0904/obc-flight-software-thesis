#!/usr/bin/env python3
"""Generate the executable catalog for the allowlisted public script surface."""

from __future__ import annotations

import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "scripts/verification-manifest.json"


def git_output(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True)


def tracked_executables() -> list[tuple[str, str]]:
    records: list[tuple[str, str]] = []
    for path in git_output(
        "ls-files", "-co", "--exclude-standard", "scripts"
    ).splitlines():
        absolute = ROOT / path
        if absolute.is_file() and absolute.stat().st_mode & 0o111:
            object_id = git_output("hash-object", path).strip()
            records.append((path, object_id))
    return records


def environment_for(path: str) -> str:
    lowered = path.lower()
    if "hosted" in lowered:
        return "hosted"
    if any(token in lowered for token in ("target", "rpi", "lab_can", "subsystem")):
        return "target/lab"
    if any(token in lowered for token in ("check_", "generate_", "classify_")):
        return "static"
    return "multi"


def role_for(path: str) -> str:
    name = pathlib.PurePosixPath(path).name
    if name.startswith(("ensure_", "bootstrap_", "install_", "package_", "sync_")):
        return "baseline"
    if name.startswith(("run_", "start_", "stop_", "status_", "route")):
        return "operator-or-verification"
    if name.startswith(("check_", "generate_", "test_", "classify_")):
        return "governance"
    return "support"


def is_entrypoint(path: str) -> bool:
    relative = pathlib.PurePosixPath(path).relative_to("scripts")
    name = relative.name
    if len(relative.parts) == 1:
        return name.startswith(
            (
                "run_",
                "ensure_",
                "bootstrap_",
                "install_",
                "package_",
                "sync_",
                "check_",
            )
        )
    return relative.parts[0] in {"manual_ops", "chapter5_routes"} and name.startswith(
        ("run_", "start_", "stop_", "status_", "route")
    )


def main() -> None:
    records = []
    for path, object_id in tracked_executables():
        status = "maintained" if is_entrypoint(path) else "support/internal"
        records.append(
            {
                "path": path,
                "sourceObject": object_id,
                "status": status,
                "shipped": True,
                "environment": environment_for(path),
                "role": role_for(path),
                "owner": (
                    "scripts/README.md"
                    if status == "maintained"
                    else "dependency of an allowlisted public workflow"
                ),
                "successor": None,
                "reason": "allowlisted public workflow",
            }
        )

    counts: dict[str, int] = {}
    for record in records:
        counts[record["status"]] = counts.get(record["status"], 0) + 1
    payload = {
        "schemaVersion": 2,
        "release": "thesis-submission-v1",
        "sourceExecutableCount": len(records),
        "counts": counts,
        "executables": records,
    }
    OUTPUT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
