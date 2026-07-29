#!/usr/bin/env python3
"""Write the Route 1 formal campaign manifest with provenance-aware authority."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import pathlib
import sys
from typing import Any

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from check_formal_attempt_artifacts import validate_attempt_record


def build_campaign_manifest(
    records: list[dict[str, Any]],
    campaign_date: str,
    campaign_identity: dict[str, Any],
    provenance: dict[str, Any] | None,
    provenance_sha256: str | None,
    generated_at_utc: str,
) -> dict[str, Any]:
    identity_fields = ("branch", "head", "projectVersion")
    if (
        campaign_identity.get("schemaVersion") != 1
        or any(not campaign_identity.get(field) for field in identity_fields)
    ):
        raise ValueError("campaign source identity is incomplete")
    provenance_verdict = (
        provenance.get("verdict") if isinstance(provenance, dict) else "MISSING"
    )
    normalized_records: list[dict[str, Any]] = []
    for source_record in records:
        record = dict(source_record)
        attempt_evidence = record.get("attemptEvidence")
        attempt_evidence_valid = (
            isinstance(attempt_evidence, dict)
            and attempt_evidence.get("verdict") == "PASS"
            and attempt_evidence.get("failures") == []
        )
        target_provenance_sha256 = record.get(
            "targetRevisionProvenanceSha256"
        )
        target_provenance_bound = (
            isinstance(target_provenance_sha256, str)
            and len(target_provenance_sha256) == 64
            and all(
                char in "0123456789abcdef"
                for char in target_provenance_sha256.lower()
            )
        )
        record["authoritative"] = (
            record.get("verdict") == "PASS"
            and attempt_evidence_valid
            and (
                record.get("surface") != "target"
                or (
                    provenance_verdict == "PASS"
                    and target_provenance_bound
                    and target_provenance_sha256 == provenance_sha256
                )
            )
        )
        normalized_records.append(record)

    latest_by_surface: dict[str, dict[str, Any]] = {}
    for record in normalized_records:
        latest_by_surface[str(record["surface"])] = record
    required_surfaces = {"hosted", "target"}
    complete = set(latest_by_surface) == required_surfaces
    all_attempt_evidence_valid = all(
        isinstance(record.get("attemptEvidence"), dict)
        and record["attemptEvidence"].get("verdict") == "PASS"
        and record["attemptEvidence"].get("failures") == []
        for record in normalized_records
    )
    authoritative_pass = (
        complete
        and all_attempt_evidence_valid
        and all(
            latest_by_surface[surface]["authoritative"]
            for surface in required_surfaces
        )
    )
    if provenance_verdict == "FAIL":
        campaign_verdict = "FAIL"
    elif authoritative_pass:
        campaign_verdict = "PASS"
    elif complete:
        campaign_verdict = "FAIL"
    else:
        campaign_verdict = "INCOMPLETE"

    return {
        "campaign": "route1-sequence-formal-rerun",
        "campaignDate": campaign_date,
        "generatedAtUtc": generated_at_utc,
        "sourceProvenance": "deployment/",
        "campaignSourceIdentity": {
            "path": "deployment/campaign-source-identity.json",
            **{
                field: campaign_identity[field]
                for field in identity_fields
            },
        },
        "targetRevisionProvenance": {
            "path": "deployment/target-revision-provenance.json",
            "verdict": provenance_verdict,
            "failures": (
                provenance.get("failures", [])
                if isinstance(provenance, dict)
                else []
            ),
        },
        "authoritativeVerdict": campaign_verdict,
        "attempts": normalized_records,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--campaign-date", required=True)
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--campaign-identity", required=True)
    parser.add_argument("--target-provenance", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    records = [
        json.loads(line)
        for line in os.environ.get("ATTEMPT_RECORDS", "").splitlines()
        if line
    ]
    evidence_root = pathlib.Path(args.evidence_root).resolve()
    records = [
        {
            **record,
            "attemptEvidence": validate_attempt_record(evidence_root, record),
        }
        for record in records
    ]
    provenance_path = pathlib.Path(args.target_provenance)
    campaign_identity_path = pathlib.Path(args.campaign_identity)
    campaign_identity = json.loads(
        campaign_identity_path.read_text(encoding="utf-8")
    )
    provenance_bytes = (
        provenance_path.read_bytes() if provenance_path.is_file() else None
    )
    provenance = (
        json.loads(provenance_bytes.decode("utf-8"))
        if provenance_bytes is not None
        else None
    )
    provenance_sha256 = (
        hashlib.sha256(provenance_bytes).hexdigest()
        if provenance_bytes is not None
        else None
    )
    generated_at = (
        dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat()
    )
    payload = build_campaign_manifest(
        records,
        args.campaign_date,
        campaign_identity,
        provenance,
        provenance_sha256,
        generated_at,
    )
    output = pathlib.Path(args.output)
    output.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
