#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import sys


def read_meta(path: pathlib.Path) -> dict[str, str]:
    data: dict[str, str] = {}
    if not path.exists():
        return data
    for raw_line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" not in raw_line:
            continue
        key, value = raw_line.split("=", 1)
        data[key] = value
    return data


REQUIRED_KEYS = [
    "case_id",
    "environment",
    "carrier_kind",
    "wrapper_intent",
    "registered_path_reuse",
    "new_claim_attempted",
    "verdict",
    "blocker_class",
    "rerun_safe",
    "artifact_root",
    "message",
    "started_at",
    "finished_at",
]


def validate_case(path: pathlib.Path, case: dict[str, str]) -> dict[str, str]:
    missing = [key for key in REQUIRED_KEYS if key not in case]
    empty = [
        key
        for key in REQUIRED_KEYS
        if key in case and key not in {"blocker_class", "message"} and case[key] == ""
    ]
    if case.get("verdict") == "blocked" and not case.get("blocker_class"):
        empty.append("blocker_class")
    if missing or empty:
        problem = {
            "case_id": case.get("case_id", path.parent.name),
            "environment": case.get("environment", ""),
            "carrier_kind": case.get("carrier_kind", "unknown"),
            "wrapper_intent": "harness-bug",
            "registered_path_reuse": "false",
            "new_claim_attempted": "false",
            "verdict": "fail",
            "blocker_class": "harness-bug",
            "rerun_safe": "unknown",
            "artifact_root": case.get("artifact_root", str(path.parent)),
            "message": "Metadata contract invalid: "
            + ", ".join(filter(None, [
                "missing=" + ",".join(missing) if missing else "",
                "empty=" + ",".join(sorted(set(empty))) if empty else "",
            ])),
            "started_at": case.get("started_at", ""),
            "finished_at": case.get("finished_at", ""),
        }
        return problem
    return case


def main() -> int:
    if len(sys.argv) < 5:
        raise SystemExit(
            "usage: render_summary.py <env-name> <env-label> <summary-json> <summary-md> <meta...>"
        )

    env_name = sys.argv[1]
    env_label = sys.argv[2]
    summary_json = pathlib.Path(sys.argv[3])
    summary_md = pathlib.Path(sys.argv[4])
    meta_paths = [pathlib.Path(arg) for arg in sys.argv[5:]]

    cases = [validate_case(path, read_meta(path)) for path in meta_paths]
    status = "pass"
    if any(case.get("verdict") != "pass" for case in cases):
        status = "fail"

    payload = {
        "environment": env_name,
        "label": env_label,
        "status": status,
        "case_count": len(cases),
        "cases": cases,
    }
    summary_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        f"# {env_label} Comm Verification Summary",
        "",
        f"- environment: `{env_name}`",
        f"- overall-status: `{status}`",
        f"- case-count: `{len(cases)}`",
        "",
        "| Case | Verdict | Carrier | Intent | Blocker | Rerun | Registered Reuse | New Claim |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for case in cases:
        lines.append(
            "| {case_id} | {verdict} | {carrier_kind} | {wrapper_intent} | {blocker_class} | {rerun_safe} | {registered_path_reuse} | {new_claim_attempted} |".format(
                case_id=case.get("case_id", ""),
                verdict=case.get("verdict", ""),
                carrier_kind=case.get("carrier_kind", ""),
                wrapper_intent=case.get("wrapper_intent", ""),
                blocker_class=case.get("blocker_class", ""),
                rerun_safe=case.get("rerun_safe", ""),
                registered_path_reuse=case.get("registered_path_reuse", ""),
                new_claim_attempted=case.get("new_claim_attempted", ""),
            )
        )
        if case.get("message"):
            lines.append("")
            lines.append(f"  notes `{case.get('case_id', '')}`: {case['message']}")
        if case.get("artifact_root"):
            lines.append(f"  artifacts `{case.get('case_id', '')}`: `{case['artifact_root']}`")
    summary_md.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
