#!/usr/bin/env python3
"""Generate the exhaustive source-to-public publication manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess

SOURCE_COMMIT = "142683f20ba46f59f894f594f2caf71dfeddf16f"

EXCLUDED_PREFIXES = (
    ".codex/",
    "pending/",
    "docs/architecture/",
    "docs/architecture-review/",
    "docs/operator/",
    "docs/reporting/",
    "docs/roadmap/",
    "scripts/comm_verification/cases/",
    "scripts/comm_verification/env/",
    "scripts/comm_verification/matrix/",
)

EXCLUDED_PATHS = {
    ".github/README.md",
    "AGENTS.md",
    "final_design.md",
    "docs/architecture/comm-followup-directions.md",
    "docs/integrity-and-hashing.md",
    "docs/operator/formal-comm-verification-matrix-v1-runbook.md",
    "docs/operator/hosted-official-sequencing-system-resources-runbook.md",
    "docs/roadmap/mission-console-observability-recommendations.md",
    "docs/roadmap/mission-console-phase1-handoff.md",
    "docs/target-version-metadata.md",
}

TRANSFORMS = {
    "README.md": "README.md",
    "config/security/command-auth.ini": "config/security/command-auth.example.ini",
    "docs/reporting/fprime-native-vs-project-contribution-architecture-v1/README.md":
        "docs/architecture.md",
    "docs/architecture/current-development-architecture.md": "docs/architecture.md",
    "docs/architecture/project-contributions.md": "docs/architecture.md",
    "docs/architecture/target-flight-design.md": "docs/architecture.md",
    "docs/baseline-reconciliation-matrix.json":
        "openspec/reconciliation/baseline-reconciliation-matrix.json",
    "docs/baseline-reconciliation-matrix.md":
        "openspec/reconciliation/baseline-reconciliation-matrix.md",
    "docs/evidence/README.md": "evidence/README.md",
    "docs/integrity-and-hashing.md": "docs/architecture.md",
    "docs/target-version-metadata.md": "docs/interfaces.md",
    "docs/thesis/README.md": "docs/thesis.md",
    "docs/thesis/source-index.md": "docs/thesis.md",
    "docs/thesis/07-verification-and-evidence-map.md": "docs/thesis.md",
    "docs/thesis/claim-evidence-map.zh-TW.md": "docs/thesis.md",
    "docs/verification-debugging-lessons.md": "docs/verification.md",
    "docs/verification-matrix.md": "docs/verification.md",
    "docs/verification-path-registry.md": "evidence/verification-path-registry.md",
    "docs/operator/hosted-dual-link-orchestration-runbook.md":
        "docs/operator/hosted.md",
    "docs/operator/hosted-manual-dual-gds-runbook.md":
        "docs/operator/hosted.md",
    "docs/operator/hosted-per-band-stock-ground-stacks-runbook.md":
        "docs/operator/hosted.md",
    "docs/operator/mission-console-phase1-runbook.md":
        "docs/operator/mission-console.md",
    "docs/operator/simulator-control-reference.zh-TW.md":
        "docs/operator/simulator-controls.zh-TW.md",
    "docs/operator/target-manual-dual-gds-runbook.md":
        "docs/operator/target-lab.md",
    "docs/operator/target-obc-comm-csp-lab-runbook.md":
        "docs/operator/target-lab.md",
    "docs/operator/target-proof-abc-governance.md":
        "docs/operator/target-lab.md",
    "docs/operator/thesis-demo-routes.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/operator/mission-console-phase1-runbook.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/operator/mission-console-beacon-viewer-demo.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/operator/mission-console-target-route1-demo.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/operator/mission-console-target-route2-demo.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/operator/mission-console-target-route3-demo.zh-TW.md":
        "docs/operator/thesis-demo.zh-TW.md",
    "docs/roadmap/current-baseline.md": "docs/architecture.md",
    "docs/roadmap/next-work.md": "docs/architecture.md",
}


def load_script_allowlist(public_root: pathlib.Path) -> tuple[set[str], tuple[str, ...]]:
    allowlist_path = public_root / "scripts/public-allowlist.txt"
    entries = {
        line.strip()
        for line in allowlist_path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }
    exact = {entry for entry in entries if not entry.endswith("/**")}
    prefixes = tuple(entry.removesuffix("**") for entry in entries if entry.endswith("/**"))
    return exact, prefixes


def script_is_allowed(path: str, exact: set[str], prefixes: tuple[str, ...]) -> bool:
    if path.endswith("/.gitkeep") or path == "scripts/manual_ops/examples/sample.bin":
        return False
    return path in exact or any(path.startswith(prefix) for prefix in prefixes)


def git_output(repo: pathlib.Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True)


def git_blob_id(path: pathlib.Path) -> str:
    data = path.read_bytes()
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def classify(
    path: str,
    script_allowlist: set[str],
    script_prefixes: tuple[str, ...],
) -> tuple[str, str | None, str]:
    if path == "config/security/command-auth.ini":
        return "transform", TRANSFORMS[path], "replace tracked credentials with a public example"
    if path.startswith("docs/test-records/"):
        public_path = "evidence/records/" + path.removeprefix("docs/test-records/")
        if "/artifacts/" in path:
            return "externalize", None, "raw evidence belongs in the release asset"
        return (
            "transform",
            public_path,
            "publish the record in the dedicated evidence archive",
        )
    if path in TRANSFORMS:
        return "transform", TRANSFORMS[path], "consolidated into the public canonical layer"
    if path.startswith("docs/thesis/"):
        return "exclude", None, "thesis body-writing and duplicate reference material"
    if path.startswith("scripts/") and not script_is_allowed(
        path, script_allowlist, script_prefixes
    ):
        return "exclude", None, "outside the public workflow allowlist"
    if path in EXCLUDED_PATHS or any(path.startswith(prefix) for prefix in EXCLUDED_PREFIXES):
        return "exclude", None, "development-only, stale snapshot, or retired document family"
    return "include", path, "shipped from the fixed source baseline"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    public_root = args.output.resolve().parents[1]
    script_allowlist, script_prefixes = load_script_allowlist(public_root)

    resolved = git_output(args.source, "rev-parse", SOURCE_COMMIT).strip()
    if resolved != SOURCE_COMMIT:
        raise SystemExit(f"source commit mismatch: expected {SOURCE_COMMIT}, got {resolved}")

    entries = []
    raw = subprocess.check_output(
        ["git", "-C", str(args.source), "ls-tree", "-rz", SOURCE_COMMIT]
    ).decode("utf-8")
    for record in raw.split("\0"):
        if not record:
            continue
        metadata, path = record.split("\t", 1)
        mode, object_type, object_id = metadata.split(" ", 2)
        disposition, public_path, reason = classify(
            path, script_allowlist, script_prefixes
        )
        public_file = public_root / public_path if public_path else None
        if (
            disposition == "include"
            and object_type == "blob"
            and public_file is not None
            and public_file.is_file()
            and git_blob_id(public_file) != object_id
        ):
            disposition = "transform"
            reason = "curated for the public environment or release governance"
        entries.append(
            {
                "path": path,
                "mode": mode,
                "objectType": object_type,
                "object": object_id,
                "disposition": disposition,
                "publicPath": public_path,
                "reason": reason,
            }
        )

    counts: dict[str, int] = {}
    for entry in entries:
        counts[entry["disposition"]] = counts.get(entry["disposition"], 0) + 1

    source_committed_at = git_output(
        args.source, "show", "-s", "--format=%cI", SOURCE_COMMIT
    ).strip()
    source_backed_public_paths = {
        entry["publicPath"] for entry in entries if entry["publicPath"]
    }
    observed_public_paths = {
        path
        for path in git_output(
            public_root,
            "ls-files",
            "-co",
            "--exclude-standard",
        ).splitlines()
        if (public_root / path).exists()
    }
    payload = {
        "schemaVersion": 1,
        "release": "thesis-submission-v1",
        "generatedFromSourceCommittedAt": source_committed_at,
        "source": {
            "repository": "obc-flight-software",
            "commit": SOURCE_COMMIT,
            "trackedPaths": len(entries),
        },
        "submodules": {
            "lib/fprime": "54f02168c676d5b61990d7a48ea9c61b9a8d0b5f",
            "lib/libcsp": "241b756a7fb5af1ba0967183b4a2b5843f77ebdb",
        },
        "counts": counts,
        "publicOnlyPaths": sorted(observed_public_paths - source_backed_public_paths),
        "files": entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
