#!/usr/bin/env python3
"""Validate the frozen thesis-backed Route 1 functional-observation bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
from typing import Any


ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_CAMPAIGN = (
    ROOT
    / "evidence/records/chapter5-integrated-route-closure-v1/artifacts"
    / "2026-07-20-route1-target-abc-rerun"
)
DEFAULT_MANIFEST_NAME = "dedup-manifest.json"
FROZEN_CAMPAIGN = "2026-07-20-route1-target-abc-rerun"
EXPECTED_CLASSIFICATION = "thesis-backed-functional-observation"
SUPERSEDED_CAMPAIGN = "2026-07-19-route1-sequence-formal-rerun"
REQUIRED_SEQUENCE_INPUTS = (
    "prepare-and-capture/scenario/sequence-src/route1-sequence-demo.seq",
    "prepare-and-capture/scenario/sequence-bin/route1-sequence-demo.bin",
)
REQUIRED_PIPELINE_RECEIVED_FDP = (
    "prepare-and-capture/scenario/sband-ground/pipeline-store/fprime-downlink/"
    "_home_youjun_obc-deploy_runtime_comm-csp-lab-obc_data-products_"
    "Dp_268673025_1784485107_00846579.fdp"
)
REQUIRED_CAMPAIGN_README = "README.md"
CAMPAIGN_README_NON_AUTHORITY = (
    "This record is not the authoritative governed target closure"
)
CAMPAIGN_README_REQUALIFICATION = (
    "Route 1 target requalification therefore remains pending"
)
HISTORICAL_NON_AUTHORITY_PATTERN = re.compile(
    r"\bnot current target authority\b", re.IGNORECASE
)
OBSERVATION_NON_AUTHORITY_PATTERN = re.compile(
    r"\bdoes not promote\b"
    r"|\bnot (?:governed )?(?:target )?a/b/c authority\b",
    re.IGNORECASE,
)
AUTHORITY_NEGATION_PATTERN = re.compile(
    r"\bnon[- ]?authoritative\b"
    r"|(?:\bnot\b|\bcannot\b|\bdoes not\b|\bmust not\b|\bneither\b)"
    r"[^.!?;:]{0,120}\bauthorit"
    r"|(?:不符合|不是|不可|不能|不具)[^。！？；：]{0,120}authorit",
    re.IGNORECASE,
)
DOUBLE_AUTHORITY_NEGATION_PATTERN = re.compile(
    r"\b(?:not|never)\s+non[- ]?authoritative\b"
    r"|\b(?:does|must|can|cannot)\s+not\s+not\b"
    r"|(?:不是|不可|不能|不具)\s*不(?:具|是|符合)?[^。！？；：]{0,40}authorit",
    re.IGNORECASE,
)
AUTHORITY_COORDINATOR_PATTERN = re.compile(
    r"[;；:：]"
    r"|(?:,\s*|，\s*)?"
    r"(?:\b(?:and|but|yet|however|nevertheless|whereas|while|although|though)\b"
    r"|(?:但(?:是)?|然而|卻|却|且|以及|和))"
    r"|[,，](?=\s*[^,.!?;:]{0,60}"
    r"\b(?:is|are|was|were|becomes?|became|constitutes?|constituted"
    r"|establishes?|established|proves?|proved)\b"
    r"[^,.!?;:]{0,60}\bauthorit)",
    re.IGNORECASE,
)
AUTHORITY_REJECTION_PATTERN = re.compile(
    r"\bauthorit[^.!?;:]{0,80}"
    r"\b(?:fail|fails|failed|reject|rejects|rejected|block|blocks|blocked"
    r"|prevent|prevents|prevented)\b",
    re.IGNORECASE,
)
POSITIVE_AUTHORITY_ASSERTION_PATTERN = re.compile(
    r"\b(?:is|are|was|were|becomes?|became|remains?|remained"
    r"|continues?|continued|constitutes?|constituted"
    r"|establishes?|established|proves?|proved)\b\s+"
    r"(?![^.!?;:]{0,80}\b(?:not|never|neither|no\s+longer"
    r"|non[- ]?authoritative)\b)"
    r"(?:[\w/-]+[\s-]+){0,5}\bauthorit",
    re.IGNORECASE,
)
ROUTE1_DATE_ALIASES = (
    (
        re.compile(
            r"\b(?:july|jul\.?)\s+12(?:th)?(?:,\s*|\s+)2026\b"
            r"|\b12(?:th)?\s+(?:july|jul\.?)\s+2026\b"
            r"|\b2026[/.]0?7[/.]12\b"
            r"|\b0?7[-/]12[-/]2026\b"
            r"|2026年0?7月12日",
            re.IGNORECASE,
        ),
        "2026-07-12",
    ),
    (
        re.compile(
            r"\b(?:july|jul\.?)\s+20(?:th)?(?:,\s*|\s+)2026\b"
            r"|\b20(?:th)?\s+(?:july|jul\.?)\s+2026\b"
            r"|\b2026[/.]0?7[/.]20\b"
            r"|\b0?7[-/]20[-/]2026\b"
            r"|2026年0?7月20日",
            re.IGNORECASE,
        ),
        "2026-07-20",
    ),
)


class EvidenceError(RuntimeError):
    """Raised when the frozen evidence contract is not satisfied."""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=pathlib.Path, default=ROOT)
    parser.add_argument("--campaign-root", type=pathlib.Path, default=DEFAULT_CAMPAIGN)
    parser.add_argument("--manifest", default=DEFAULT_MANIFEST_NAME)
    return parser.parse_args()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise EvidenceError(message)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_relative_path(root: pathlib.Path, value: Any, field: str) -> pathlib.Path:
    require(isinstance(value, str) and value, f"{field} must be a non-empty path")
    relative = pathlib.PurePosixPath(value)
    require(not relative.is_absolute(), f"{field} must be relative: {value}")
    require(".." not in relative.parts, f"{field} escapes its root: {value}")
    path = root.joinpath(*relative.parts)
    try:
        path.resolve().relative_to(root.resolve())
    except ValueError as exc:
        raise EvidenceError(f"{field} escapes its root: {value}") from exc
    return path


def load_json(path: pathlib.Path, label: str) -> dict[str, Any]:
    require(path.is_file(), f"missing {label}: {path}")
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise EvidenceError(f"invalid {label} {path}: {exc}") from exc
    require(isinstance(value, dict), f"{label} must contain a JSON object: {path}")
    return value


def check_hashed_file(
    root: pathlib.Path, record: dict[str, Any], label: str
) -> pathlib.Path:
    path = safe_relative_path(root, record.get("path"), f"{label}.path")
    expected = record.get("sha256")
    require(
        isinstance(expected, str) and len(expected) == 64,
        f"{label}.sha256 must be a SHA-256 hex digest",
    )
    require(path.is_file(), f"missing {label}: {path.relative_to(root)}")
    actual = sha256_file(path)
    require(
        actual == expected,
        f"{label} SHA-256 mismatch for {path.relative_to(root)}: "
        f"expected {expected}, got {actual}",
    )
    return path


def check_canonicalizations(
    campaign_root: pathlib.Path, records: Any
) -> None:
    require(isinstance(records, list) and len(records) == 2, "expected two canonicalizations")
    policies: set[str] = set()
    canonical_hashes: set[str] = set()
    removed_paths: set[pathlib.Path] = set()

    for index, raw_record in enumerate(records):
        label = f"canonicalizations[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        record = raw_record
        policy = record.get("policy")
        require(
            policy in {"CAPTURE_AUTO", "CAPTURE_DETERMINISTIC"},
            f"{label}.policy is invalid: {policy}",
        )
        require(policy not in policies, f"duplicate canonical policy: {policy}")
        policies.add(policy)

        canonical_path = safe_relative_path(
            campaign_root, record.get("canonicalPath"), f"{label}.canonicalPath"
        )
        canonical_hash = record.get("sha256")
        require(
            isinstance(canonical_hash, str) and len(canonical_hash) == 64,
            f"{label}.sha256 must be a SHA-256 hex digest",
        )
        require(canonical_path.is_file(), f"missing canonical artifact: {canonical_path}")
        require(
            sha256_file(canonical_path) == canonical_hash,
            f"canonical SHA-256 mismatch: {canonical_path.relative_to(campaign_root)}",
        )
        canonical_hashes.add(canonical_hash)

        removed = record.get("removedEquivalentPaths")
        require(isinstance(removed, list) and removed, f"{label} has no removed equivalents")
        for removed_index, removed_value in enumerate(removed):
            removed_path = safe_relative_path(
                campaign_root,
                removed_value,
                f"{label}.removedEquivalentPaths[{removed_index}]",
            )
            require(
                not removed_path.exists(),
                f"removed equivalent still exists: {removed_path.relative_to(campaign_root)}",
            )
            require(removed_path not in removed_paths, f"duplicate removed path: {removed_value}")
            removed_paths.add(removed_path)

        source_fdp = record.get("sourceFdp")
        require(isinstance(source_fdp, dict), f"{label}.sourceFdp must be an object")
        check_hashed_file(campaign_root, source_fdp, f"{label}.sourceFdp")

        received_fdp = record.get("receivedFdp")
        if policy == "CAPTURE_DETERMINISTIC":
            require(
                isinstance(received_fdp, dict),
                f"{label}.receivedFdp is required for the ground-received "
                "DETERMINISTIC product",
            )
        if received_fdp is not None:
            require(
                isinstance(received_fdp, dict),
                f"{label}.receivedFdp must be an object or null",
            )
            check_hashed_file(campaign_root, received_fdp, f"{label}.receivedFdp")

        command = record.get("reconstructionCommand")
        require(
            isinstance(command, str)
            and "scripts/payload_fdp_extract.py" in command
            and "fprime-venv/bin/fprime-dp-write" in command,
            f"{label} lacks a repository-owned reconstruction command",
        )

    require(
        policies == {"CAPTURE_AUTO", "CAPTURE_DETERMINISTIC"},
        f"canonical policy set mismatch: {sorted(policies)}",
    )

    matching_json: dict[str, list[pathlib.Path]] = {digest: [] for digest in canonical_hashes}
    for path in campaign_root.rglob("*.json"):
        digest = sha256_file(path)
        if digest in matching_json:
            matching_json[digest].append(path)
    for digest, paths in matching_json.items():
        require(
            len(paths) == 1,
            f"canonical hash {digest} occurs {len(paths)} times instead of once: "
            + ", ".join(str(path.relative_to(campaign_root)) for path in paths),
        )


def check_retained_artifacts(
    campaign_root: pathlib.Path, records: Any
) -> set[pathlib.Path]:
    require(isinstance(records, list) and records, "retainedArtifacts must be a non-empty list")
    seen: set[pathlib.Path] = set()
    for index, raw_record in enumerate(records):
        label = f"retainedArtifacts[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        path = check_hashed_file(campaign_root, raw_record, label)
        require(path not in seen, f"duplicate retained artifact: {path.relative_to(campaign_root)}")
        seen.add(path)
    return seen


def check_required_globs(
    campaign_root: pathlib.Path,
    records: Any,
    hashed_paths: set[pathlib.Path],
) -> None:
    require(isinstance(records, list) and records, "requiredArtifactGlobs must be a non-empty list")
    for index, raw_record in enumerate(records):
        label = f"requiredArtifactGlobs[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        pattern = raw_record.get("pattern")
        minimum = raw_record.get("minimumCount")
        require(isinstance(pattern, str) and pattern, f"{label}.pattern is invalid")
        require(isinstance(minimum, int) and minimum > 0, f"{label}.minimumCount is invalid")
        matches = [path for path in campaign_root.glob(pattern) if path.is_file()]
        require(
            len(matches) >= minimum,
            f"artifact pattern {pattern!r} expected at least {minimum}, found {len(matches)}",
        )
        unhashed = sorted(
            path.relative_to(campaign_root)
            for path in matches
            if path not in hashed_paths
        )
        require(
            not unhashed,
            f"artifact pattern {pattern!r} has unhashed matches: "
            + ", ".join(str(path) for path in unhashed),
        )


def check_text_assertions(campaign_root: pathlib.Path, records: Any) -> None:
    require(isinstance(records, list) and records, "criticalAssertions must be a non-empty list")
    for index, raw_record in enumerate(records):
        label = f"criticalAssertions[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        path = safe_relative_path(campaign_root, raw_record.get("path"), f"{label}.path")
        require(path.is_file(), f"missing critical assertion file: {path.relative_to(campaign_root)}")
        text = path.read_text(encoding="utf-8", errors="replace")
        snippets = raw_record.get("contains")
        require(isinstance(snippets, list) and snippets, f"{label}.contains must be a non-empty list")
        for snippet in snippets:
            require(isinstance(snippet, str) and snippet, f"{label} contains an invalid snippet")
            require(
                snippet in text,
                f"critical value {snippet!r} missing from {path.relative_to(campaign_root)}",
            )


def check_sizes(campaign_root: pathlib.Path, records: Any) -> None:
    require(isinstance(records, list) and records, "criticalSizes must be a non-empty list")
    for index, raw_record in enumerate(records):
        label = f"criticalSizes[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        path = safe_relative_path(campaign_root, raw_record.get("path"), f"{label}.path")
        expected = raw_record.get("bytes")
        require(isinstance(expected, int) and expected >= 0, f"{label}.bytes is invalid")
        require(path.is_file(), f"missing critical-size artifact: {path.relative_to(campaign_root)}")
        require(
            path.stat().st_size == expected,
            f"critical size mismatch for {path.relative_to(campaign_root)}: "
            f"expected {expected}, got {path.stat().st_size}",
        )


def normalized_prose(value: str) -> str:
    return " ".join(value.replace("**", "").split())


def normalize_route1_date_aliases(value: str) -> str:
    for pattern, canonical in ROUTE1_DATE_ALIASES:
        value = pattern.sub(canonical, value)
    return value


def require_no_authority_promotion(
    text: str,
    *,
    subjects: tuple[str, ...],
    context: str,
) -> None:
    for statement in re.split(
        r"(?<=[.!?])\s+|\n\s*\n"
        r"|\n(?=\s*(?:[-*+]\s|#{1,6}\s))",
        normalize_route1_date_aliases(text.replace("**", "")),
    ):
        subject_in_statement = False
        for clause in AUTHORITY_COORDINATOR_PATTERN.split(statement):
            clause = " ".join(clause.split())
            lowered = f" {clause.lower()} "
            clause_has_subject = any(
                subject in lowered for subject in subjects
            )
            subject_in_statement = subject_in_statement or clause_has_subject
            if "authorit" not in lowered or not subject_in_statement:
                continue
            if (
                AUTHORITY_REJECTION_PATTERN.search(lowered) is not None
                and POSITIVE_AUTHORITY_ASSERTION_PATTERN.search(lowered) is None
            ):
                continue
            require(
                DOUBLE_AUTHORITY_NEGATION_PATTERN.search(lowered) is None,
                f"{context} contains a double-negated authority promotion: "
                f"{clause}",
            )
            negation = AUTHORITY_NEGATION_PATTERN.search(lowered)
            require(
                POSITIVE_AUTHORITY_ASSERTION_PATTERN.search(lowered) is None
                or negation is not None,
                f"{context} contains a positive authority assertion: {clause}",
            )
            require(
                negation is not None,
                f"{context} contains an authority-promoting clause: {clause}",
            )


def check_campaign_readme_classification(
    campaign_root: pathlib.Path, hashed_paths: set[pathlib.Path]
) -> None:
    readme_path = campaign_root / REQUIRED_CAMPAIGN_README
    require(
        readme_path in hashed_paths,
        "campaign README is not independently hash-locked",
    )
    raw_text = readme_path.read_text(encoding="utf-8", errors="replace")
    text = normalized_prose(raw_text)
    require(
        CAMPAIGN_README_NON_AUTHORITY in text,
        "campaign README does not explicitly classify this record as "
        "non-authoritative",
    )
    require(
        CAMPAIGN_README_REQUALIFICATION in text,
        "campaign README does not keep Route 1 target requalification pending",
    )

    authority_subjects = (
        "this record",
        "2026-07-20",
        "7/20",
        "functional observation",
        "campaign",
    )
    require_no_authority_promotion(
        raw_text,
        subjects=authority_subjects,
        context="campaign README",
    )


def check_governing_documents(
    repo_root: pathlib.Path, records: Any, status_assertions: Any
) -> None:
    require(
        isinstance(records, list) and records,
        "governingDocuments must be a non-empty list",
    )
    document_paths: set[pathlib.Path] = set()
    for index, value in enumerate(records):
        path = safe_relative_path(repo_root, value, f"governingDocuments[{index}]")
        require(path.is_file(), f"missing governing document: {path.relative_to(repo_root)}")
        require(
            path not in document_paths,
            f"duplicate governing document: {path.relative_to(repo_root)}",
        )
        document_paths.add(path)
        text = path.read_text(encoding="utf-8", errors="replace")
        require(
            FROZEN_CAMPAIGN in text,
            f"frozen campaign link missing from {path.relative_to(repo_root)}",
        )
        require(
            "2026-07-12" in text,
            f"historical 2026-07-12 proof status missing from "
            f"{path.relative_to(repo_root)}",
        )
        require(
            SUPERSEDED_CAMPAIGN not in text,
            f"superseded campaign remains referenced by {path.relative_to(repo_root)}",
        )
        require_no_authority_promotion(
            text,
            subjects=(
                "2026-07-12",
                "2026-07-20",
                "7/12",
                "7/20",
            ),
            context=f"governing document {path.relative_to(repo_root)}",
        )

    require(
        isinstance(status_assertions, list) and status_assertions,
        "governanceStatusAssertions must be a non-empty list",
    )
    asserted_paths: set[pathlib.Path] = set()
    for index, raw_record in enumerate(status_assertions):
        label = f"governanceStatusAssertions[{index}]"
        require(isinstance(raw_record, dict), f"{label} must be an object")
        path = safe_relative_path(
            repo_root, raw_record.get("path"), f"{label}.path"
        )
        require(path in document_paths, f"{label}.path is not a governing document")
        require(path not in asserted_paths, f"duplicate authority assertion: {path}")
        asserted_paths.add(path)

        historical = raw_record.get("historicalProofContains")
        non_authoritative = raw_record.get("nonAuthoritativeObservationContains")
        requalification = raw_record.get("requalificationContains")
        require(
            isinstance(historical, str)
            and "2026-07-12" in historical
            and HISTORICAL_NON_AUTHORITY_PATTERN.search(historical) is not None,
            f"{label}.historicalProofContains must explicitly classify "
            "2026-07-12 as non-authoritative",
        )
        require(
            isinstance(non_authoritative, str)
            and "2026-07-20" in non_authoritative
            and OBSERVATION_NON_AUTHORITY_PATTERN.search(non_authoritative)
            is not None,
            f"{label}.nonAuthoritativeObservationContains must explicitly "
            "classify 2026-07-20 as non-authoritative",
        )
        require(
            isinstance(requalification, str)
            and "requalification" in requalification.lower()
            and "pending" in requalification.lower(),
            f"{label}.requalificationContains must explicitly classify "
            "Route 1 target requalification as pending",
        )
        text = path.read_text(encoding="utf-8", errors="replace")
        require(
            historical in text,
            f"historical 2026-07-12 status assertion missing from "
            f"{path.relative_to(repo_root)}",
        )
        require(
            non_authoritative in text,
            f"non-authoritative 2026-07-20 assertion missing from "
            f"{path.relative_to(repo_root)}",
        )
        require(
            requalification in text,
            f"pending Route 1 target requalification assertion missing from "
            f"{path.relative_to(repo_root)}",
        )

    require(
        asserted_paths == document_paths,
        "governance status assertions must cover every governing document",
    )


def check_bundle(
    repo_root: pathlib.Path, campaign_root: pathlib.Path, manifest_name: str
) -> None:
    repo_root = repo_root.resolve()
    campaign_root = campaign_root.resolve()
    require(campaign_root.is_dir(), f"campaign root does not exist: {campaign_root}")
    manifest_path = safe_relative_path(campaign_root, manifest_name, "manifest")
    manifest = load_json(manifest_path, "dedup manifest")
    require(manifest.get("schemaVersion") == 1, "unsupported dedup manifest schemaVersion")
    require(manifest.get("campaign") == FROZEN_CAMPAIGN, "dedup manifest campaign mismatch")
    require(
        manifest.get("evidenceClassification") == EXPECTED_CLASSIFICATION,
        "dedup manifest evidence classification mismatch",
    )
    limitations = manifest.get("governanceLimitations")
    require(isinstance(limitations, dict), "governanceLimitations must be an object")
    require(
        limitations.get("authoritativeTargetClosure") is False,
        "7/20 bundle must not be classified as authoritative target closure",
    )
    require(
        limitations.get("cRestartedSharedObcService") is True,
        "7/20 C-owned shared-service restart limitation must remain explicit",
    )
    require(
        limitations.get("completeContemporaneousRevisionProvenance") is False,
        "7/20 incomplete contemporaneous revision provenance must remain explicit",
    )

    check_canonicalizations(campaign_root, manifest.get("canonicalizations"))
    hashed_paths = check_retained_artifacts(
        campaign_root, manifest.get("retainedArtifacts")
    )
    check_campaign_readme_classification(campaign_root, hashed_paths)
    for relative_path in REQUIRED_SEQUENCE_INPUTS:
        required_path = campaign_root.joinpath(
            *pathlib.PurePosixPath(relative_path).parts
        )
        require(
            required_path in hashed_paths,
            f"required sequence input is not hash-locked: {relative_path}",
        )
    pipeline_fdp_path = campaign_root.joinpath(
        *pathlib.PurePosixPath(REQUIRED_PIPELINE_RECEIVED_FDP).parts
    )
    require(
        pipeline_fdp_path in hashed_paths,
        "pipeline-stored received FDP is not independently hash-locked",
    )
    check_required_globs(
        campaign_root, manifest.get("requiredArtifactGlobs"), hashed_paths
    )
    check_text_assertions(campaign_root, manifest.get("criticalAssertions"))
    check_sizes(campaign_root, manifest.get("criticalSizes"))
    check_governing_documents(
        repo_root,
        manifest.get("governingDocuments"),
        manifest.get("governanceStatusAssertions"),
    )


def main() -> int:
    args = parse_args()
    try:
        check_bundle(args.repo_root, args.campaign_root, args.manifest)
    except EvidenceError as exc:
        print(f"Route 1 evidence bundle: FAIL: {exc}", file=sys.stderr)
        return 1
    print("Route 1 evidence bundle: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
