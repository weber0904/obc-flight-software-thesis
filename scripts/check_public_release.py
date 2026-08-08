#!/usr/bin/env python3
"""Validate the curated public-release, evidence, and credential boundary."""

from __future__ import annotations

import hashlib
import json
import pathlib
import re
import subprocess
import sys
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parent.parent
PUBLICATION_MANIFEST = ROOT / "release/publication-manifest.json"
VERIFICATION_MANIFEST = ROOT / "scripts/verification-manifest.json"
SCRIPT_ALLOWLIST = ROOT / "scripts/public-allowlist.txt"
SCRIPT_CATALOG = ROOT / "scripts/CATALOG.md"
EVIDENCE_CATALOG = ROOT / "evidence/catalog.json"
SHA256SUMS = ROOT / "release/SHA256SUMS"

SOURCE_COMMIT = "142683f20ba46f59f894f594f2caf71dfeddf16f"
RELEASE_TAG = "thesis-submission-v1"
SOURCE_USER = "chen" + "youjun"
LEGACY_TARGET_USER = "you" + "jun"
LAB_SERIAL_IDENTIFIER = "BG03" + "OJK3"

REQUIRED = {
    ".github/workflows/verification-ci.yml",
    "CITATION.cff",
    "CONTRIBUTING.md",
    "LICENSE",
    "LICENSES/Apache-2.0.txt",
    "LICENSES/MIT.txt",
    "LICENSES/Zlib.txt",
    "NOTICE",
    "README.md",
    "README.zh-TW.md",
    "SBOM.spdx.json",
    "SECURITY.md",
    "THIRD_PARTY_NOTICES.md",
    "config/security/command-auth.example.ini",
    "evidence/README.md",
    "evidence/catalog.json",
    "release/RELEASE_PROVENANCE.md",
    "release/SHA256SUMS",
    "release/publication-manifest.json",
    "scripts/check_public_release.py",
    "scripts/CATALOG.md",
    "scripts/public-allowlist.txt",
    "scripts/verification-manifest.json",
}

FORBIDDEN_TEXT = {
    f"/Users/{SOURCE_USER}": "$REPO_ROOT",
    f"/home/{LEGACY_TARGET_USER}": "$OBC_HOME",
    f"{LEGACY_TARGET_USER}@": "operator role",
    LAB_SERIAL_IDENTIFIER: "$COMM_SERIAL_DEVICE",
}
PRIVATE_IPV4_RE = re.compile(
    r"(?<![0-9])(?:10\.(?:[0-9]{1,3}\.){2}[0-9]{1,3}|"
    r"192\.168\.(?:[0-9]{1,3}\.)[0-9]{1,3}|"
    r"172\.(?:1[6-9]|2[0-9]|3[01])\.(?:[0-9]{1,3}\.)[0-9]{1,3})"
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def git_files() -> set[str]:
    output = subprocess.check_output(
        ["git", "-C", str(ROOT), "ls-files", "-co", "--exclude-standard"],
        text=True,
    )
    return {
        line
        for line in output.splitlines()
        if line and (ROOT / line).exists()
    }


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def load_json(path: pathlib.Path, errors: list[str]) -> dict:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError) as exc:
        fail(errors, f"{path.relative_to(ROOT)}: invalid or missing JSON: {exc}")
        return {}
    if not isinstance(payload, dict):
        fail(errors, f"{path.relative_to(ROOT)}: root must be a JSON object")
        return {}
    return payload


def check_publication(
    errors: list[str], public_paths: set[str]
) -> tuple[dict, list[dict]]:
    payload = load_json(PUBLICATION_MANIFEST, errors)
    entries = payload.get("files", [])
    if payload.get("source", {}).get("commit") != SOURCE_COMMIT:
        fail(errors, "publication manifest source commit is not frozen")
    if payload.get("release") != RELEASE_TAG:
        fail(errors, "publication manifest release tag is inconsistent")

    source_paths = [entry.get("path") for entry in entries]
    if len(source_paths) != len(set(source_paths)):
        fail(errors, "publication manifest contains duplicate source paths")
    if len(source_paths) != payload.get("source", {}).get("trackedPaths"):
        fail(errors, "publication manifest tracked-path count is inconsistent")
    valid_dispositions = {"include", "transform", "externalize", "exclude"}
    if any(entry.get("disposition") not in valid_dispositions for entry in entries):
        fail(errors, "publication manifest contains an invalid disposition")
    actual_counts = Counter(entry.get("disposition") for entry in entries)
    if dict(actual_counts) != payload.get("counts"):
        fail(errors, "publication manifest disposition counts are inconsistent")

    source_backed_public: set[str] = set()
    for entry in entries:
        source = entry["path"]
        target = entry.get("publicPath")
        disposition = entry["disposition"]
        if disposition in {"include", "transform"}:
            source_backed_public.add(target)
            if target not in public_paths:
                fail(errors, f"{disposition} target is missing: {source} -> {target}")
        if disposition in {"externalize", "exclude"} and source in public_paths:
            fail(errors, f"{disposition} source leaked into public tree: {source}")

    declared_public_only = set(payload.get("publicOnlyPaths", []))
    observed_public_only = public_paths - source_backed_public
    if observed_public_only != declared_public_only:
        missing = sorted(observed_public_only - declared_public_only)
        stale = sorted(declared_public_only - observed_public_only)
        if missing:
            fail(errors, f"undeclared public-only paths: {missing[:10]}")
        if stale:
            fail(errors, f"stale public-only paths: {stale[:10]}")
    return payload, entries


def check_verification(
    errors: list[str], public_paths: set[str]
) -> dict:
    verification = load_json(VERIFICATION_MANIFEST, errors)
    executable_records = verification.get("executables", [])
    executable_paths = [record.get("path") for record in executable_records]
    if len(executable_paths) != len(set(executable_paths)):
        fail(errors, "verification manifest contains duplicate executable paths")
    source_executables = [
        record for record in executable_records if record.get("sourceObject")
    ]
    if len(source_executables) != verification.get("sourceExecutableCount"):
        fail(errors, "verification manifest source executable count is inconsistent")
    if Counter(record["status"] for record in executable_records) != Counter(
        verification.get("counts", {})
    ):
        fail(errors, "verification manifest status counts are inconsistent")

    for record in executable_records:
        path = record["path"]
        shipped = bool(record["shipped"])
        status = record["status"]
        if shipped and status not in {"maintained", "support/internal"}:
            fail(errors, f"shipped executable has forbidden status: {path}: {status}")
        if not shipped and status not in {
            "historical-evidence-only",
            "removed-alias",
        }:
            fail(errors, f"excluded executable has invalid status: {path}: {status}")
        if shipped and path not in public_paths:
            fail(errors, f"shipped executable is missing: {path}")
        if not shipped and path in public_paths:
            fail(errors, f"historical executable leaked into public tree: {path}")

    actual_executables = {
        path
        for path in public_paths
        if path.startswith("scripts/")
        and (ROOT / path).is_file()
        and (ROOT / path).stat().st_mode & 0o111
    }
    if set(executable_paths) != actual_executables:
        missing = sorted(actual_executables - set(executable_paths))
        stale = sorted(set(executable_paths) - actual_executables)
        fail(
            errors,
            f"verification manifest does not match tracked executables: "
            f"missing={missing[:10]}, stale={stale[:10]}",
        )
    return verification


def check_script_surface(errors: list[str], public_paths: set[str]) -> None:
    try:
        entries = {
            line.strip()
            for line in SCRIPT_ALLOWLIST.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        }
    except FileNotFoundError:
        fail(errors, "scripts/public-allowlist.txt is missing")
        return

    exact = {entry for entry in entries if not entry.endswith("/**")}
    prefixes = tuple(entry.removesuffix("**") for entry in entries if entry.endswith("/**"))
    if prefixes:
        fail(errors, "script allowlist must use exact file paths, not directory prefixes")
    script_paths = {path for path in public_paths if path.startswith("scripts/")}
    missing = sorted(path for path in exact if path not in script_paths)
    unlisted = sorted(
        path
        for path in script_paths
        if path not in exact and not any(path.startswith(prefix) for prefix in prefixes)
    )
    if missing:
        fail(errors, f"script allowlist paths are missing: {missing[:10]}")
    if unlisted:
        fail(errors, f"unlisted scripts are tracked: {unlisted[:10]}")

    try:
        catalog_text = SCRIPT_CATALOG.read_text(encoding="utf-8")
    except FileNotFoundError:
        fail(errors, "scripts/CATALOG.md is missing")
    else:
        catalog_paths = set(
            re.findall(r"^\| `(scripts/[^`]+)` \|", catalog_text, flags=re.MULTILINE)
        )
        if catalog_paths != script_paths:
            uncataloged = sorted(script_paths - catalog_paths)
            stale = sorted(catalog_paths - script_paths)
            if uncataloged:
                fail(errors, f"uncataloged script files: {uncataloged[:10]}")
            if stale:
                fail(errors, f"stale script catalog entries: {stale[:10]}")

    placeholders = sorted(path for path in script_paths if path.endswith("/.gitkeep"))
    if placeholders:
        fail(errors, f"redundant script placeholders are tracked: {placeholders}")

    tracked_artifacts = sorted(
        path
        for path in script_paths
        if pathlib.PurePosixPath(path).name == ".DS_Store"
        or "__pycache__" in pathlib.PurePosixPath(path).parts
    )
    if tracked_artifacts:
        fail(errors, f"local script artifacts are tracked: {tracked_artifacts[:10]}")


def check_evidence(
    errors: list[str], entries: list[dict], public_paths: set[str]
) -> dict:
    catalog = load_json(EVIDENCE_CATALOG, errors)
    records = catalog.get("records", [])
    record_ids = [record.get("recordId") for record in records]
    if catalog.get("sourceCommit") != SOURCE_COMMIT:
        fail(errors, "evidence catalog source commit is inconsistent")
    if catalog.get("releaseTag") != RELEASE_TAG:
        fail(errors, "evidence catalog release tag is inconsistent")
    if len(record_ids) != len(set(record_ids)):
        fail(errors, "evidence catalog contains duplicate record identifiers")
    if len(records) != catalog.get("recordCount"):
        fail(errors, "evidence catalog record count is inconsistent")

    summary_entries = {
        entry["publicPath"]
        for entry in entries
        if re.fullmatch(r"docs/test-records/[^/]+/README\.md", entry["path"])
    }
    catalog_summaries = {record.get("summaryPath") for record in records}
    if summary_entries != catalog_summaries:
        fail(errors, "evidence catalog does not cover every test-record summary")

    externalized = {
        entry["path"]
        for entry in entries
        if entry.get("disposition") == "externalize"
        and entry["path"].startswith("docs/test-records/")
    }
    descriptor_source_paths: set[str] = set()
    descriptor_count = 0
    asset_sha = catalog.get("asset", {}).get("sha256")
    for record in records:
        summary_path = record.get("summaryPath")
        if summary_path in public_paths:
            actual_public_sha = sha256((ROOT / summary_path).read_bytes())
            if actual_public_sha != record.get("summaryPublicSha256"):
                fail(errors, f"{summary_path}: public summary digest mismatch")
        descriptor_path = record.get("artifactDescriptorPath")
        if not descriptor_path:
            if record.get("artifactFileCount") != 0:
                fail(errors, f"{record.get('recordId')}: missing artifact descriptor")
            continue
        descriptor_count += 1
        descriptor = load_json(ROOT / descriptor_path, errors)
        files = descriptor.get("files", [])
        if descriptor.get("fileCount") != len(files):
            fail(errors, f"{descriptor_path}: file count is inconsistent")
        if descriptor.get("asset", {}).get("sha256") != asset_sha:
            fail(errors, f"{descriptor_path}: asset digest is inconsistent")
        for item in files:
            source_path = item.get("sourcePath")
            if source_path in descriptor_source_paths:
                fail(errors, f"artifact source is indexed more than once: {source_path}")
            descriptor_source_paths.add(source_path)
            if item.get("mediaType") == "binary":
                if item.get("sourceSha256") != item.get("publicSha256"):
                    fail(errors, f"{source_path}: binary evidence was modified")
                if item.get("redactions"):
                    fail(errors, f"{source_path}: binary evidence has redactions")

    if descriptor_source_paths != externalized:
        fail(errors, "artifact descriptors do not cover every externalized source path")
    if catalog.get("asset", {}).get("sourceArtifactCount") != len(externalized):
        fail(errors, "evidence asset source artifact count is inconsistent")
    if descriptor_count != sum(
        1 for record in records if record.get("artifactFileCount", 0) > 0
    ):
        fail(errors, "evidence descriptor count is inconsistent")

    artifact_directories = [
        path
        for path in public_paths
        if path.startswith("evidence/records/") and "/artifacts/" in path
    ]
    if artifact_directories:
        fail(errors, f"raw artifact paths leaked into Git: {artifact_directories[:5]}")
    return catalog


def check_checksums(errors: list[str], catalog: dict) -> None:
    try:
        lines = SHA256SUMS.read_text(encoding="utf-8").splitlines()
    except FileNotFoundError:
        fail(errors, "release/SHA256SUMS is missing")
        return
    seen: set[str] = set()
    for line in lines:
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        if not match:
            fail(errors, f"release/SHA256SUMS: malformed line: {line}")
            continue
        expected, relative = match.groups()
        if relative in seen:
            fail(errors, f"release/SHA256SUMS: duplicate path: {relative}")
        seen.add(relative)
        path = ROOT / relative
        if path.is_file() and sha256(path.read_bytes()) != expected:
            fail(errors, f"release/SHA256SUMS: digest mismatch: {relative}")
        if relative == catalog.get("asset", {}).get("name"):
            if expected != catalog.get("asset", {}).get("sha256"):
                fail(errors, "release asset checksum does not match evidence catalog")
            local_asset = ROOT / "release-assets" / relative
            if local_asset.is_file() and sha256(local_asset.read_bytes()) != expected:
                fail(errors, "local release evidence asset checksum mismatch")

    required_checksums = {
        catalog.get("asset", {}).get("name"),
        "SBOM.spdx.json",
        "release/publication-manifest.json",
        "scripts/verification-manifest.json",
        "evidence/catalog.json",
    }
    if not required_checksums.issubset(seen):
        fail(errors, "release/SHA256SUMS is missing required release objects")


def check_credentials_and_licenses(
    errors: list[str], public_paths: set[str]
) -> None:
    if "config/security/command-auth.ini" in public_paths:
        fail(errors, "real/local command-auth keystore is tracked")
    gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
    if "/config/security/command-auth.ini" not in gitignore:
        fail(errors, "runtime command-auth keystore is not ignored")
    package_script = (ROOT / "scripts/package_rpi_bundle.sh").read_text(
        encoding="utf-8"
    )
    for required_text in (
        "OBC_PACKAGE_KEYSTORE_PATH",
        "command-auth.example.ini",
        "cmp -s",
        "install -m 0600",
    ):
        if required_text not in package_script:
            fail(errors, f"package_rpi_bundle.sh lacks credential guard: {required_text}")

    sbom = load_json(ROOT / "SBOM.spdx.json", errors)
    package_names = {
        package.get("name") for package in sbom.get("packages", []) if isinstance(package, dict)
    }
    dependency_names = {
        "F Prime": {"F Prime", "fprime"},
        "libcsp": {"libcsp", "libcsp-csp-es"},
        "TooJPEG": {"TooJPEG"},
    }
    for dependency, accepted_names in dependency_names.items():
        if package_names.isdisjoint(accepted_names):
            fail(errors, f"SBOM is missing dependency: {dependency}")
    notices = (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8")
    for dependency in ("F Prime", "libcsp", "TooJPEG"):
        if dependency not in notices:
            fail(errors, f"third-party notices are missing: {dependency}")
    if not (ROOT / "lib/fprime/LICENSE.txt").is_file():
        fail(errors, "F Prime submodule license is missing")
    if not (ROOT / "lib/libcsp/LICENSE").is_file():
        fail(errors, "libcsp submodule license is missing")


def check_public_text(errors: list[str], public_paths: set[str]) -> None:
    for rel in sorted(public_paths):
        path = ROOT / rel
        if not path.is_file() or path.is_symlink():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for forbidden, replacement in FORBIDDEN_TEXT.items():
            if forbidden in text:
                fail(errors, f"{rel}: contains private identifier; use {replacement}")
        if PRIVATE_IPV4_RE.search(text):
            fail(errors, f"{rel}: contains a private lab IP instead of a host role")


def main() -> None:
    errors: list[str] = []
    public_paths = git_files()
    for required in sorted(REQUIRED):
        if required not in public_paths:
            fail(errors, f"required public file is missing: {required}")

    publication, entries = check_publication(errors, public_paths)
    verification = check_verification(errors, public_paths)
    check_script_surface(errors, public_paths)
    catalog = check_evidence(errors, entries, public_paths)
    check_checksums(errors, catalog)
    check_credentials_and_licenses(errors, public_paths)
    check_public_text(errors, public_paths)

    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

    print("PASS: public release boundary is consistent")
    print(f"- source paths classified: {len(entries)}")
    print(f"- dispositions: {publication['counts']}")
    print(f"- public files observed: {len(public_paths)}")
    print(f"- verification executables: {len(verification['executables'])}")
    print(f"- evidence summaries: {catalog['recordCount']}")
    print(f"- externalized artifacts: {catalog['asset']['sourceArtifactCount']}")


if __name__ == "__main__":
    main()
