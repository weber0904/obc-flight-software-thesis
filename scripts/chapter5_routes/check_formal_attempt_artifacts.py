#!/usr/bin/env python3
"""Validate retained formal-attempt manifests and every imported artifact byte."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import stat
import sys
from typing import Any


ATTEMPT_MANIFEST_SCHEMA_VERSION = 2
SAFE_COMPONENT_CHARS = frozenset(
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-"
)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def valid_sha256(value: Any) -> bool:
    return (
        isinstance(value, str)
        and len(value) == 64
        and all(char in "0123456789abcdef" for char in value.lower())
    )


def safe_component(value: Any) -> bool:
    return (
        isinstance(value, str)
        and bool(value)
        and value[0].isalnum()
        and all(char in SAFE_COMPONENT_CHARS for char in value)
    )


def safe_relative_path(value: Any) -> pathlib.PurePosixPath | None:
    if not isinstance(value, str) or not value:
        return None
    relative = pathlib.PurePosixPath(value)
    if relative.is_absolute() or any(
        part in {"", ".", ".."} for part in relative.parts
    ):
        return None
    return relative


def collect_retained_files(attempt_root: pathlib.Path) -> list[dict[str, Any]]:
    """Return a deterministic manifest for all retained bytes except manifest.json."""

    records: list[dict[str, Any]] = []
    for path in sorted(attempt_root.rglob("*")):
        relative = path.relative_to(attempt_root).as_posix()
        if relative == "manifest.json":
            continue
        metadata = path.lstat()
        if stat.S_ISDIR(metadata.st_mode):
            continue
        if not stat.S_ISREG(metadata.st_mode):
            raise ValueError(f"unsupported retained artifact type: {relative}")
        records.append(
            {
                "path": relative,
                "sizeBytes": metadata.st_size,
                "sha256": sha256_file(path),
            }
        )
    return records


def _load_manifest(
    manifest_path: pathlib.Path,
) -> tuple[dict[str, Any] | None, bytes | None, list[str]]:
    try:
        manifest_bytes = manifest_path.read_bytes()
    except OSError:
        return None, None, ["attempt-manifest-missing"]
    try:
        manifest = json.loads(manifest_bytes.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        return None, manifest_bytes, ["attempt-manifest-invalid"]
    if not isinstance(manifest, dict):
        return None, manifest_bytes, ["attempt-manifest-invalid"]
    return manifest, manifest_bytes, []


def validate_attempt_record(
    evidence_root: pathlib.Path, record: dict[str, Any]
) -> dict[str, Any]:
    failures: list[str] = []
    route = record.get("route", "route1")
    surface = record.get("surface")
    attempt = record.get("attempt")
    if route != "route1" or surface not in {"hosted", "target"}:
        failures.append("attempt-record-scope-invalid")
    if not safe_component(attempt):
        failures.append("attempt-record-label-invalid")

    path_surface = surface if surface in {"hosted", "target"} else "_invalid"
    path_attempt = attempt if safe_component(attempt) else "_invalid"
    relative_manifest = (
        pathlib.PurePosixPath("route1")
        / path_surface
        / str(path_attempt)
        / "manifest.json"
    )
    attempt_root = evidence_root.joinpath(*relative_manifest.parts[:-1])
    manifest_path = attempt_root / "manifest.json"
    manifest, manifest_bytes, load_failures = _load_manifest(manifest_path)
    failures.extend(load_failures)
    actual_manifest_sha256 = (
        hashlib.sha256(manifest_bytes).hexdigest()
        if manifest_bytes is not None
        else None
    )
    if (
        not valid_sha256(record.get("attemptManifestSha256"))
        or record.get("attemptManifestSha256") != actual_manifest_sha256
    ):
        failures.append("attempt-manifest-hash-mismatch")

    if manifest is not None:
        if manifest.get("schemaVersion") != ATTEMPT_MANIFEST_SCHEMA_VERSION:
            failures.append("attempt-manifest-schema-mismatch")
        expected_fields = {
            "route": route,
            "surface": surface,
            "attemptLabel": attempt,
            "command": record.get("command"),
            "verdict": record.get("verdict"),
            "retryCount": record.get("retryCount"),
            "retryOf": record.get("retryOf"),
            "failureClass": record.get("failureClass"),
        }
        if any(manifest.get(key) != value for key, value in expected_fields.items()):
            failures.append("attempt-manifest-record-mismatch")

        artifacts = manifest.get("artifacts")
        if not isinstance(artifacts, list) or not artifacts:
            failures.append("attempt-artifact-roots-invalid")
        else:
            for artifact in artifacts:
                relative = (
                    safe_relative_path(artifact.get("relativeDestination"))
                    if isinstance(artifact, dict)
                    else None
                )
                if relative is None:
                    failures.append("attempt-artifact-roots-invalid")
                    break
                try:
                    destination = attempt_root.joinpath(*relative.parts)
                    metadata = destination.lstat()
                except OSError:
                    failures.append("attempt-artifact-root-missing")
                    break
                if not (
                    stat.S_ISDIR(metadata.st_mode)
                    or stat.S_ISREG(metadata.st_mode)
                ):
                    failures.append("attempt-artifact-root-type-invalid")
                    break

        retained_files = manifest.get("retainedFiles")
        expected_records: dict[str, dict[str, Any]] = {}
        if not isinstance(retained_files, list):
            failures.append("attempt-retained-files-invalid")
        else:
            paths: list[str] = []
            for item in retained_files:
                relative = (
                    safe_relative_path(item.get("path"))
                    if isinstance(item, dict)
                    else None
                )
                if (
                    relative is None
                    or not isinstance(item.get("sizeBytes"), int)
                    or isinstance(item.get("sizeBytes"), bool)
                    or item.get("sizeBytes") < 0
                    or not valid_sha256(item.get("sha256"))
                ):
                    failures.append("attempt-retained-files-invalid")
                    break
                value = relative.as_posix()
                paths.append(value)
                expected_records[value] = item
            if paths != sorted(paths) or len(paths) != len(set(paths)):
                failures.append("attempt-retained-files-invalid")

        try:
            actual_records = {
                item["path"]: item for item in collect_retained_files(attempt_root)
            }
        except (OSError, ValueError):
            actual_records = {}
            failures.append("attempt-artifact-tree-unreadable")
        if set(actual_records) != set(expected_records):
            failures.append("attempt-artifact-file-set-mismatch")
        else:
            if any(
                actual_records[path]["sizeBytes"]
                != expected_records[path]["sizeBytes"]
                for path in expected_records
            ):
                failures.append("attempt-artifact-size-mismatch")
            if any(
                actual_records[path]["sha256"] != expected_records[path]["sha256"]
                for path in expected_records
            ):
                failures.append("attempt-artifact-hash-mismatch")

    if surface == "target" and safe_component(attempt):
        expected_provenance = pathlib.PurePosixPath(
            "deployment",
            f"target-revision-provenance-{attempt}.json",
        )
        recorded_provenance = safe_relative_path(
            record.get("targetRevisionProvenancePath")
        )
        if recorded_provenance != expected_provenance:
            failures.append("attempt-target-provenance-path-mismatch")
        else:
            provenance_path = evidence_root.joinpath(*recorded_provenance.parts)
            try:
                provenance_metadata = provenance_path.lstat()
            except OSError:
                failures.append("attempt-target-provenance-missing")
            else:
                if not stat.S_ISREG(provenance_metadata.st_mode):
                    failures.append("attempt-target-provenance-type-invalid")
                elif (
                    not valid_sha256(
                        record.get("targetRevisionProvenanceSha256")
                    )
                    or sha256_file(provenance_path)
                    != record.get("targetRevisionProvenanceSha256")
                ):
                    failures.append(
                        "attempt-target-provenance-hash-mismatch"
                    )

    unique_failures = list(dict.fromkeys(failures))
    return {
        "verdict": "PASS" if not unique_failures else "FAIL",
        "manifestPath": relative_manifest.as_posix(),
        "manifestSha256": actual_manifest_sha256,
        "failures": unique_failures,
    }


def load_latest_attempt(
    campaign_manifest: pathlib.Path, surface: str, attempt: str
) -> dict[str, Any]:
    try:
        campaign = json.loads(campaign_manifest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"campaign manifest is missing or invalid: {exc}") from exc
    attempts = campaign.get("attempts") if isinstance(campaign, dict) else None
    if not isinstance(attempts, list):
        raise RuntimeError("campaign attempts are missing or invalid")
    matching = [
        item
        for item in attempts
        if isinstance(item, dict) and item.get("surface") == surface
    ]
    if not matching or matching[-1].get("attempt") != attempt:
        raise RuntimeError(
            f"latest {surface} campaign attempt is not {attempt}"
        )
    return matching[-1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--campaign-manifest", required=True)
    parser.add_argument("--surface", required=True, choices=("hosted", "target"))
    parser.add_argument("--attempt", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        record = load_latest_attempt(
            pathlib.Path(args.campaign_manifest).resolve(),
            args.surface,
            args.attempt,
        )
        result = validate_attempt_record(
            pathlib.Path(args.evidence_root).resolve(), record
        )
    except RuntimeError as exc:
        print(f"Formal attempt artifact validation failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    if result["verdict"] != "PASS":
        print(
            "Formal attempt artifact validation failed: "
            + ", ".join(result["failures"]),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
