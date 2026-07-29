#!/usr/bin/env python3
"""Focused contract test for attempt-aware formal artifact imports."""

from __future__ import annotations

import hashlib
import json
import os
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
IMPORTER = ROOT / "scripts/chapter5_routes/import_formal_artifacts.py"
ATTEMPT_CHECKER = (
    ROOT / "scripts/chapter5_routes/check_formal_attempt_artifacts.py"
)
CAMPAIGN_WRITER = (
    ROOT / "scripts/chapter5_routes/write_route1_campaign_manifest.py"
)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="formal-artifact-import-test-") as temp_dir:
        temp_root = pathlib.Path(temp_dir)
        wrapper_root = temp_root / "wrapper"
        external_root = temp_root / "external-stage"
        destination_root = temp_root / "evidence"
        wrapper_root.mkdir()
        external_root.mkdir()
        (external_root / "summary.json").write_text('{"verdict":"PASS"}\n', encoding="utf-8")
        (wrapper_root / "summary.log").write_text(
            f"nested evidence: {external_root}\n", encoding="utf-8"
        )

        subprocess.run(
            [
                "python3",
                str(IMPORTER),
                "--destination-root",
                str(destination_root),
                "--route",
                "route1",
                "--surface",
                "target",
                "--attempt-label",
                "attempt-02",
                "--wrapper-root",
                str(wrapper_root),
                "--command",
                "bash scripts/chapter5_routes/target/run_route1_target.sh",
                "--verdict",
                "PASS",
                "--retry-count",
                "1",
                "--retry-of",
                "attempt-01",
                "--failure-class",
                "ground-observability",
                "--timestamp",
                "2026-07-19T00:00:00Z",
            ],
            check=True,
        )

        attempt_root = destination_root / "route1/target/attempt-02"
        manifest = json.loads((attempt_root / "manifest.json").read_text(encoding="utf-8"))
        assert manifest["schemaVersion"] == 2
        assert manifest["attemptLabel"] == "attempt-02"
        assert manifest["retryOf"] == "attempt-01"
        assert manifest["failureClass"] == "ground-observability"
        assert manifest["retryCount"] == 1
        assert (attempt_root / "wrapper-root/wrapper/summary.log").is_file()
        assert (attempt_root / "external-roots/external-stage/summary.json").is_file()
        retained_paths = [record["path"] for record in manifest["retainedFiles"]]
        assert retained_paths == sorted(retained_paths)
        assert retained_paths == [
            "external-roots/external-stage/summary.json",
            "wrapper-root/wrapper/summary.log",
        ]

        manifest_path = attempt_root / "manifest.json"
        manifest_sha256 = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        attempt_provenance_path = (
            destination_root
            / "deployment/target-revision-provenance-attempt-02.json"
        )
        attempt_provenance_path.parent.mkdir(parents=True)
        attempt_provenance_bytes = (
            json.dumps(
                {
                    "schemaVersion": 1,
                    "verdict": "PASS",
                    "failures": [],
                },
                sort_keys=True,
            )
            + "\n"
        ).encode("utf-8")
        attempt_provenance_path.write_bytes(attempt_provenance_bytes)
        attempt_provenance_sha256 = hashlib.sha256(
            attempt_provenance_bytes
        ).hexdigest()
        campaign_path = destination_root / "campaign-manifest.json"
        campaign_path.write_text(
            json.dumps(
                {
                    "attempts": [
                        {
                            "route": "route1",
                            "surface": "target",
                            "attempt": "attempt-02",
                            "command": "bash scripts/chapter5_routes/target/run_route1_target.sh",
                            "verdict": "PASS",
                            "retryCount": 1,
                            "retryOf": "attempt-01",
                            "failureClass": "ground-observability",
                            "targetRevisionProvenancePath": (
                                "deployment/"
                                "target-revision-provenance-attempt-02.json"
                            ),
                            "targetRevisionProvenanceSha256": (
                                attempt_provenance_sha256
                            ),
                            "attemptManifestSha256": manifest_sha256,
                        }
                    ]
                }
            )
            + "\n",
            encoding="utf-8",
        )

        def validate_attempt() -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [
                    "python3",
                    str(ATTEMPT_CHECKER),
                    "--evidence-root",
                    str(destination_root),
                    "--campaign-manifest",
                    str(campaign_path),
                    "--surface",
                    "target",
                    "--attempt",
                    "attempt-02",
                ],
                check=False,
                capture_output=True,
                text=True,
            )

        assert validate_attempt().returncode == 0

        attempt_provenance_path.write_text(
            '{"schemaVersion":1,"verdict":"CORRUPT"}\n',
            encoding="utf-8",
        )
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-target-provenance-hash-mismatch" in result.stderr
        attempt_provenance_path.write_bytes(attempt_provenance_bytes)
        assert validate_attempt().returncode == 0
        attempt_provenance_path.unlink()
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-target-provenance-missing" in result.stderr
        attempt_provenance_path.write_bytes(attempt_provenance_bytes)
        assert validate_attempt().returncode == 0

        campaign_identity_path = destination_root / "campaign-source-identity.json"
        campaign_identity_path.write_text(
            json.dumps(
                {
                    "schemaVersion": 1,
                    "branch": "feature/test",
                    "head": "1" * 40,
                    "projectVersion": "test-version",
                }
            )
            + "\n",
            encoding="utf-8",
        )
        generated_campaign_path = destination_root / "generated-campaign.json"
        writer_env = os.environ.copy()
        writer_env["ATTEMPT_RECORDS"] = json.dumps(
            json.loads(campaign_path.read_text(encoding="utf-8"))["attempts"][0]
        )
        subprocess.run(
            [
                "python3",
                str(CAMPAIGN_WRITER),
                "--output",
                str(generated_campaign_path),
                "--campaign-date",
                "2026-07-19",
                "--evidence-root",
                str(destination_root),
                "--campaign-identity",
                str(campaign_identity_path),
                "--target-provenance",
                str(destination_root / "missing-target-provenance.json"),
            ],
            env=writer_env,
            check=True,
        )
        generated_campaign = json.loads(
            generated_campaign_path.read_text(encoding="utf-8")
        )
        assert generated_campaign["attempts"][0]["attemptEvidence"]["verdict"] == "PASS"
        assert generated_campaign["attempts"][0]["authoritative"] is False

        retained_summary = (
            attempt_root / "external-roots/external-stage/summary.json"
        )
        retained_summary.write_text('{"verdict":"CORRUPT"}\n', encoding="utf-8")
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-artifact-hash-mismatch" in result.stderr

        retained_summary.write_text('{"verdict":"PASS"}\n', encoding="utf-8")
        assert validate_attempt().returncode == 0
        retained_summary.unlink()
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-artifact-file-set-mismatch" in result.stderr

        retained_summary.write_text('{"verdict":"PASS"}\n', encoding="utf-8")
        (attempt_root / "unlisted.txt").write_text("unbound\n", encoding="utf-8")
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-artifact-file-set-mismatch" in result.stderr
        (attempt_root / "unlisted.txt").unlink()
        assert validate_attempt().returncode == 0

        original_manifest_bytes = manifest_path.read_bytes()
        manifest["verdict"] = "FAIL"
        manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        result = validate_attempt()
        assert result.returncode != 0
        assert "attempt-manifest-hash-mismatch" in result.stderr
        assert "attempt-manifest-record-mismatch" in result.stderr
        manifest_path.write_bytes(original_manifest_bytes)
        assert validate_attempt().returncode == 0

        before_invalid = {
            path.relative_to(destination_root)
            for path in destination_root.rglob("*")
        }
        for invalid_label in (
            str(temp_root / "absolute-attempt"),
            "../traversal-attempt",
            "nested/attempt",
        ):
            result = subprocess.run(
                [
                    "python3",
                    str(IMPORTER),
                    "--destination-root",
                    str(destination_root),
                    "--route",
                    "route1",
                    "--surface",
                    "target",
                    "--attempt-label",
                    invalid_label,
                    "--wrapper-root",
                    str(wrapper_root),
                    "--command",
                    "invalid-label-test",
                    "--verdict",
                    "FAIL",
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            assert result.returncode != 0
            assert "--attempt-label must be one safe path component" in result.stderr
            after_invalid = {
                path.relative_to(destination_root)
                for path in destination_root.rglob("*")
            }
            assert after_invalid == before_invalid
        assert not (temp_root / "absolute-attempt").exists()
        assert not (destination_root / "route1/traversal-attempt").exists()

    print("import_formal_artifacts attempt layout: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
