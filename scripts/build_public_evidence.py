#!/usr/bin/env python3
"""Build the sanitized, deterministic thesis-submission evidence asset.

The public repository keeps readable test-record summaries in Git. Bulky raw
artifacts are read from the frozen private source commit, sanitized where they
are text, and written to a versioned GitHub Release asset. This script also
creates the in-repository evidence catalog and per-record artifact descriptors.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
import json
import re
import subprocess
import tarfile
from collections import Counter, defaultdict
from pathlib import Path


SOURCE_COMMIT = "142683f20ba46f59f894f594f2caf71dfeddf16f"
RELEASE_TAG = "thesis-submission-v1"
ASSET_NAME = f"obc-flight-software-thesis-evidence-{RELEASE_TAG}.tar.gz"
ASSET_ROOT = f"obc-flight-software-thesis-evidence-{RELEASE_TAG}"
DOWNLOAD_URL = (
    "https://github.com/weber0904/obc-flight-software-thesis/releases/"
    f"download/{RELEASE_TAG}/{ASSET_NAME}"
)
TEXT_EXTENSIONS = {
    ".csv",
    ".ini",
    ".json",
    ".jsonl",
    ".log",
    ".md",
    ".nmea",
    ".out",
    ".seq",
    ".tsv",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git(source: Path, *args: str, text: bool = False) -> bytes | str:
    result = subprocess.run(
        ["git", "-C", str(source), *args],
        check=True,
        capture_output=True,
        text=text,
    )
    return result.stdout


def tracked_paths(source: Path, scope: str = "docs/test-records") -> list[str]:
    raw = git(
        source,
        "ls-tree",
        "-r",
        "-z",
        "--name-only",
        SOURCE_COMMIT,
        "--",
        scope,
    )
    assert isinstance(raw, bytes)
    return sorted(path.decode("utf-8") for path in raw.rstrip(b"\0").split(b"\0"))


def object_bytes(source: Path, path: str) -> bytes:
    raw = git(source, "show", f"{SOURCE_COMMIT}:{path}")
    assert isinstance(raw, bytes)
    return raw


def replacement_rules(source: Path) -> list[tuple[str, re.Pattern[str], str]]:
    exact_source = re.escape(str(source.resolve()))
    source_user = source.resolve().parts[2] if len(source.resolve().parts) > 2 else ""
    legacy_target_user = "you" + "jun"
    user_alternation = "|".join(
        re.escape(value)
        for value in sorted({source_user, legacy_target_user})
        if value
    )
    serial_identifier = "BG03" + "OJK3"
    return [
        ("source-repository-path", re.compile(exact_source), "$REPO_ROOT"),
        (
            "macos-user-path",
            re.compile(rf"/Users/(?:{user_alternation})(?=/|\b)"),
            "$HOME",
        ),
        (
            "target-user-path",
            re.compile(rf"/home/(?:{user_alternation})(?=/|\b)"),
            "$OBC_HOME",
        ),
        (
            "ssh-user",
            re.compile(rf"(?<![A-Za-z0-9_.-])(?:{user_alternation})@"),
            "operator@",
        ),
        (
            "local-user-name",
            re.compile(
                rf"(?<![A-Za-z0-9_.-])(?:{user_alternation})(?![A-Za-z0-9_.-])"
            ),
            "operator",
        ),
        (
            "private-ipv4-address",
            re.compile(
                r"(?<![0-9])(?:10\.(?:[0-9]{1,3}\.){2}[0-9]{1,3}|"
                r"192\.168\.(?:[0-9]{1,3}\.)[0-9]{1,3}|"
                r"172\.(?:1[6-9]|2[0-9]|3[01])\.(?:[0-9]{1,3}\.)[0-9]{1,3})"
            ),
            "<private-lab-host>",
        ),
        (
            "lab-serial-identifier",
            re.compile(serial_identifier),
            "$COMM_SERIAL_DEVICE",
        ),
    ]


def sanitize(
    source: Path, path: str, data: bytes
) -> tuple[bytes, dict[str, int], bool]:
    if Path(path).suffix.lower() not in TEXT_EXTENSIONS:
        return data, {}, False
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return data, {}, False

    counts: dict[str, int] = {}
    for label, pattern, replacement in replacement_rules(source):
        text, count = pattern.subn(replacement, text)
        if count:
            counts[label] = count
    return text.encode("utf-8"), counts, True


def sanitize_summary(
    source: Path, record_id: str, data: bytes
) -> tuple[bytes, dict[str, int]]:
    text = data.decode("utf-8")

    absolute_artifact_link = re.compile(
        r"\]\((?:/Users/[^/)]+/[^)]*?/)?docs/test-records/"
        r"(?P<record>[^/]+)/artifacts/[^)]*\)"
    )

    def descriptor_link(match: re.Match[str]) -> str:
        linked_record = match.group("record")
        target = (
            "ARTIFACTS.json"
            if linked_record == record_id
            else f"../{linked_record}/ARTIFACTS.json"
        )
        return f"]({target})"

    text, artifact_link_count = absolute_artifact_link.subn(descriptor_link, text)
    text, relative_link_count = re.subn(
        r"\]\(artifacts/[^)]*\)", "](ARTIFACTS.json)", text
    )
    public, redactions, _ = sanitize(source, f"{record_id}.md", text.encode("utf-8"))
    if artifact_link_count or relative_link_count:
        redactions["externalized-artifact-link"] = (
            artifact_link_count + relative_link_count
        )
    return public, redactions


def make_tar_member(name: str, data: bytes) -> tuple[tarfile.TarInfo, io.BytesIO]:
    info = tarfile.TarInfo(name=name)
    info.size = len(data)
    info.mode = 0o644
    info.mtime = 0
    info.uid = 0
    info.gid = 0
    info.uname = ""
    info.gname = ""
    return info, io.BytesIO(data)


def write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(payload, indent=2, sort_keys=True, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def summary_title(data: bytes, fallback: str) -> str:
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return fallback
    for line in text.splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return fallback


def evidence_context(record_id: str, summary: bytes, files: list[dict]) -> str:
    searchable = (
        record_id
        + "\n"
        + summary.decode("utf-8", errors="ignore")
        + "\n"
        + "\n".join(item["sourcePath"] for item in files)
    ).lower()
    hardware_terms = (
        "raspberry",
        "target",
        "socketcan",
        "watchdog",
        "uart",
        "serial",
        "canfd",
        "can-fd",
        "hardware",
    )
    if any(term in searchable for term in hardware_terms):
        return "previously-demonstrated-target-or-lab"
    if "hosted" in searchable or "simulat" in searchable:
        return "previously-demonstrated-hosted"
    return "documentary-or-static-record"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "release-assets",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    repo_root = args.repo_root.resolve()
    output_dir = args.output_dir.resolve()
    resolved_commit = str(git(source, "rev-parse", SOURCE_COMMIT, text=True)).strip()
    if resolved_commit != SOURCE_COMMIT:
        raise SystemExit(
            f"source commit mismatch: expected {SOURCE_COMMIT}, got {resolved_commit}"
        )

    paths = tracked_paths(source)
    summary_paths = [
        path
        for path in paths
        if re.fullmatch(r"docs/test-records/[^/]+/README\.md", path)
    ]
    artifact_paths = [path for path in paths if "/artifacts/" in path]
    record_ids = sorted(Path(path).parts[2] for path in summary_paths)
    if len(record_ids) != len(set(record_ids)):
        raise SystemExit("duplicate test-record identifiers")

    files_by_record: dict[str, list[dict]] = defaultdict(list)
    summaries: dict[str, tuple[bytes, bytes]] = {}
    summary_redaction_totals: Counter[str] = Counter()
    archive_payloads: list[tuple[str, bytes]] = []
    redaction_totals: Counter[str] = Counter()
    source_bytes_total = 0
    public_bytes_total = 0

    for source_summary_path in summary_paths:
        record_id = Path(source_summary_path).parts[2]
        original_summary = object_bytes(source, source_summary_path)
        public_summary, redactions = sanitize_summary(
            source, record_id, original_summary
        )
        summary_redaction_totals.update(redactions)
        summaries[record_id] = (original_summary, public_summary)
        public_summary_path = repo_root / source_summary_path
        public_summary_path.parent.mkdir(parents=True, exist_ok=True)
        public_summary_path.write_bytes(public_summary)

    for source_path in artifact_paths:
        original = object_bytes(source, source_path)
        public, redactions, treated_as_text = sanitize(source, source_path, original)
        record_id = Path(source_path).parts[2]
        relative_path = source_path.split("/artifacts/", 1)[1]
        source_bytes_total += len(original)
        public_bytes_total += len(public)
        redaction_totals.update(redactions)
        entry = {
            "relativePath": relative_path,
            "sourcePath": source_path,
            "mediaType": "text" if treated_as_text else "binary",
            "sourceBytes": len(original),
            "publicBytes": len(public),
            "sourceSha256": sha256(original),
            "publicSha256": sha256(public),
            "redactions": redactions,
        }
        files_by_record[record_id].append(entry)
        archive_payloads.append(
            (
                f"{ASSET_ROOT}/records/{record_id}/artifacts/{relative_path}",
                public,
            )
        )

    evidence_manifest = {
        "schemaVersion": 1,
        "releaseTag": RELEASE_TAG,
        "sourceCommit": SOURCE_COMMIT,
        "assetRoot": ASSET_ROOT,
        "policy": {
            "textFiles": "UTF-8 text with deterministic environment redaction",
            "binaryFiles": "byte-for-byte preserved",
            "timestamps": "archive members normalized to Unix epoch",
            "ownership": "archive uid/gid normalized to zero",
        },
        "redactionTotals": dict(sorted(redaction_totals.items())),
        "sourceArtifactCount": len(artifact_paths),
        "sourceBytes": source_bytes_total,
        "publicBytes": public_bytes_total,
        "records": [
            {
                "recordId": record_id,
                "fileCount": len(files_by_record[record_id]),
                "files": sorted(
                    files_by_record[record_id], key=lambda item: item["relativePath"]
                ),
            }
            for record_id in sorted(files_by_record)
        ],
    }
    manifest_bytes = (
        json.dumps(
            evidence_manifest, indent=2, sort_keys=True, ensure_ascii=False
        ).encode("utf-8")
        + b"\n"
    )
    archive_payloads.append((f"{ASSET_ROOT}/evidence-manifest.json", manifest_bytes))

    output_dir.mkdir(parents=True, exist_ok=True)
    asset_path = output_dir / ASSET_NAME
    with asset_path.open("wb") as raw_output:
        with gzip.GzipFile(
            filename="", mode="wb", fileobj=raw_output, mtime=0
        ) as gzip_output:
            with tarfile.open(
                fileobj=gzip_output, mode="w", format=tarfile.PAX_FORMAT
            ) as archive:
                for member_name, data in sorted(archive_payloads):
                    info, stream = make_tar_member(member_name, data)
                    archive.addfile(info, stream)

    asset_sha256 = sha256(asset_path.read_bytes())
    catalog_records = []
    for record_id in record_ids:
        source_summary_path = f"docs/test-records/{record_id}/README.md"
        summary, public_summary = summaries[record_id]
        files = sorted(
            files_by_record.get(record_id, []),
            key=lambda item: item["relativePath"],
        )
        context = evidence_context(record_id, summary, files)
        descriptor_path = None
        if files:
            descriptor_path = f"docs/test-records/{record_id}/ARTIFACTS.json"
            descriptor = {
                "schemaVersion": 1,
                "recordId": record_id,
                "releaseTag": RELEASE_TAG,
                "sourceCommit": SOURCE_COMMIT,
                "asset": {
                    "name": ASSET_NAME,
                    "downloadUrl": DOWNLOAD_URL,
                    "sha256": asset_sha256,
                    "archivePrefix": f"{ASSET_ROOT}/records/{record_id}/artifacts/",
                },
                "fileCount": len(files),
                "sourceBytes": sum(item["sourceBytes"] for item in files),
                "publicBytes": sum(item["publicBytes"] for item in files),
                "files": files,
            }
            write_json(repo_root / descriptor_path, descriptor)

        catalog_records.append(
            {
                "recordId": record_id,
                "title": summary_title(summary, record_id),
                "summaryPath": source_summary_path,
                "summarySourceSha256": sha256(summary),
                "summaryPublicSha256": sha256(public_summary),
                "artifactDescriptorPath": descriptor_path,
                "artifactFileCount": len(files),
                "evidenceContext": context,
                "releaseDelta": (
                    "Historical evidence preserved from the frozen source commit; "
                    "not a fresh final-commit target or lab rerun."
                    if context == "previously-demonstrated-target-or-lab"
                    else "Evidence preserved from the frozen source commit."
                ),
            }
        )

    catalog = {
        "schemaVersion": 1,
        "releaseTag": RELEASE_TAG,
        "sourceCommit": SOURCE_COMMIT,
        "asset": {
            "name": ASSET_NAME,
            "downloadUrl": DOWNLOAD_URL,
            "sha256": asset_sha256,
            "bytes": asset_path.stat().st_size,
            "sourceArtifactCount": len(artifact_paths),
            "sourceBytes": source_bytes_total,
            "publicBytes": public_bytes_total,
            "embeddedManifestSha256": sha256(manifest_bytes),
        },
        "nonClaims": [
            "Target and lab records are prior evidence from the frozen source commit.",
            "No final-publication-commit Raspberry Pi, serial-radio, or SocketCAN rerun is claimed.",
            "Text redaction changes environment identifiers only; both digests are indexed.",
        ],
        "recordCount": len(catalog_records),
        "summaryRedactionTotals": dict(sorted(summary_redaction_totals.items())),
        "records": catalog_records,
    }
    write_json(repo_root / "docs/evidence/catalog.json", catalog)

    checksum_targets = [
        repo_root / "SBOM.spdx.json",
        repo_root / "release/publication-manifest.json",
        repo_root / "scripts/verification-manifest.json",
        repo_root / "docs/evidence/catalog.json",
    ]
    checksum_lines = [f"{asset_sha256}  {ASSET_NAME}"]
    for path in checksum_targets:
        checksum_lines.append(
            f"{sha256(path.read_bytes())}  {path.relative_to(repo_root)}"
        )
    (repo_root / "release/SHA256SUMS").write_text(
        "\n".join(checksum_lines) + "\n", encoding="utf-8"
    )

    print(
        json.dumps(
            {
                "asset": str(asset_path),
                "assetSha256": asset_sha256,
                "assetBytes": asset_path.stat().st_size,
                "recordCount": len(record_ids),
                "artifactRecordCount": len(files_by_record),
                "artifactFileCount": len(artifact_paths),
                "redactions": dict(sorted(redaction_totals.items())),
                "summaryRedactions": dict(sorted(summary_redaction_totals.items())),
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
