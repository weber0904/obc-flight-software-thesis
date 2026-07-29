#!/usr/bin/env python3
"""Generate the source-complete executable publication catalog."""

from __future__ import annotations

import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
PUBLICATION_MANIFEST = ROOT / "release/publication-manifest.json"
OUTPUT = ROOT / "scripts/verification-manifest.json"

NEW_PUBLIC_TOOLS = {
    "scripts/bootstrap_dev_config.sh": ("maintained", "configuration"),
    "scripts/build_public_evidence.py": ("support/internal", "evidence"),
    "scripts/check_public_release.py": ("maintained", "governance"),
    "scripts/generate_publication_manifest.py": ("support/internal", "governance"),
    "scripts/generate_verification_manifest.py": ("support/internal", "governance"),
    "scripts/sanitize_public_history.py": ("support/internal", "evidence"),
}


def environment_for(path: str) -> str:
    lowered = path.lower()
    if "hosted" in lowered:
        return "hosted"
    if any(token in lowered for token in ("target", "rpi", "socketcan", "lab_can")):
        return "target/lab"
    if any(token in lowered for token in ("check_", "generate_", "report_", "classify_")):
        return "static"
    return "multi"


def role_for(path: str) -> str:
    name = pathlib.PurePosixPath(path).name
    if name.startswith(("ensure_", "bootstrap_", "install_", "package_", "sync_")):
        return "baseline"
    if name.startswith(("start_", "stop_", "status_", "run_")):
        return "probe-or-operator"
    if name.startswith(("check_", "generate_", "report_", "test_", "classify_")):
        return "governance"
    return "support"


def is_maintained_entrypoint(path: str) -> bool:
    relative = pathlib.PurePosixPath(path).relative_to("scripts")
    name = relative.name
    if len(relative.parts) == 1 and name.startswith(
        (
            "run_",
            "ensure_",
            "bootstrap_",
            "install_",
            "package_",
            "sync_",
            "check_",
            "report_",
        )
    ):
        return True
    if relative.parts[0] in {"manual_ops", "chapter5_routes"} and name.startswith(
        ("run_", "start_", "stop_", "status_", "route")
    ):
        return True
    return False


def main() -> None:
    publication = json.loads(PUBLICATION_MANIFEST.read_text(encoding="utf-8"))
    records = []
    for entry in publication["files"]:
        path = entry["path"]
        if not path.startswith("scripts/") or entry["mode"] != "100755":
            continue
        excluded = entry["disposition"] == "exclude"
        alias = pathlib.PurePosixPath(path).name in {
            "run_ccsds_ground_link_spike_probe.sh",
            "run_gps_hosted_probe.sh",
            "run_storage_health_hosted_probe.sh",
        }
        if excluded:
            status = "removed-alias" if alias else "historical-evidence-only"
            owner = "evidence/verification-path-registry.md (historical entry or successor)"
        else:
            status = "maintained" if is_maintained_entrypoint(path) else "support/internal"
            owner = (
                "evidence/verification-path-registry.md"
                if status == "maintained"
                else "transitive dependency of a maintained entrypoint"
            )
        records.append(
            {
                "path": path,
                "sourceObject": entry["object"],
                "status": status,
                "shipped": not excluded,
                "environment": environment_for(path),
                "role": role_for(path),
                "owner": owner,
                "successor": (
                    "Select the exact current path from evidence/verification-path-registry.md"
                    if excluded
                    else None
                ),
                "reason": entry["reason"],
            }
        )

    for path, (status, role) in NEW_PUBLIC_TOOLS.items():
        records.append(
            {
                "path": path,
                "sourceObject": None,
                "status": status,
                "shipped": True,
                "environment": "static",
                "role": role,
                "owner": "openspec/specs/public-release-profile/spec.md",
                "successor": None,
                "reason": "introduced by the public thesis release",
            }
        )

    counts: dict[str, int] = {}
    for record in records:
        counts[record["status"]] = counts.get(record["status"], 0) + 1
    payload = {
        "schemaVersion": 1,
        "release": "thesis-submission-v1",
        "sourceExecutableCount": sum(1 for record in records if record["sourceObject"]),
        "counts": counts,
        "executables": sorted(records, key=lambda record: record["path"]),
    }
    OUTPUT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
