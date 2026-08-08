#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
from typing import Pattern

from run_target_can_matrix_probe import reap_local_process_pattern


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Ensure the local dual-GDS ground baseline is clean.")
    parser.add_argument("--json-out", type=pathlib.Path, default=None, help="Optional JSON summary output path")
    return parser.parse_args()


class GroundBaselineManager:
    def __init__(self) -> None:
        self.exempt_pids = self.current_ancestry() | self.parse_extra_exempt_pids()
        self.process_snapshot_path = os.getenv("GROUND_BASELINE_PROCESS_SNAPSHOT", "")
        self.listener_snapshot_path = os.getenv("GROUND_BASELINE_LISTENER_SNAPSHOT", "")
        self.patterns: dict[str, Pattern[str]] = {
            "fprime-gds": re.compile(r"(^|/)fprime-gds(?:\s|$)"),
            "fprime_gds.executables.comm": re.compile(r"fprime_gds\.executables\.comm"),
            "fprime_gds.executables.tcpserver": re.compile(r"fprime_gds\.executables\.tcpserver"),
            "CustomDataHandlers": re.compile(r"CustomDataHandlers"),
            "ground_ttc_gateway": re.compile(r"(^|/)\S*ground_ttc_gateway(?:\s|$)"),
            "fprime-cli-events": re.compile(r"(^|/)\S*fprime-cli(?:\s+events|\s+--.*\bevents\b)"),
            "fprime-cli-channels": re.compile(r"(^|/)\S*fprime-cli(?:\s+channels|\s+--.*\bchannels\b)"),
            "secure-auth-helper": re.compile(
                r"run_target_secure_auth_command_path_probe\.py|target-secure-auth-command-path\.|target-secure-auth-proof-v1\."
            ),
        }

    def current_ancestry(self) -> set[int]:
        # A small exemption set is enough to avoid reaping the active wrapper.
        # Avoid shelling out to `ps -p` here because some managed environments
        # permit the top-level script but block nested point queries.
        return {os.getpid(), os.getppid()}

    def parse_extra_exempt_pids(self) -> set[int]:
        raw = os.getenv("GROUND_BASELINE_EXEMPT_PIDS", "")
        result: set[int] = set()
        for token in raw.split(","):
            token = token.strip()
            if not token:
                continue
            try:
                result.add(int(token))
            except ValueError:
                continue
        return result

    def local_processes(self, *, use_snapshot: bool = True) -> list[dict[str, object]]:
        if use_snapshot and self.process_snapshot_path:
            result_stdout = pathlib.Path(self.process_snapshot_path).read_text(encoding="utf-8")
        else:
            result_stdout = subprocess.run(
                ["ps", "-ax", "-o", "pid=", "-o", "command="],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
        rows: list[dict[str, object]] = []
        for line in result_stdout.splitlines():
            stripped = line.strip()
            if not stripped:
                continue
            parts = stripped.split(None, 1)
            if len(parts) != 2:
                continue
            pid_text, command = parts
            try:
                pid = int(pid_text)
            except ValueError:
                continue
            if pid in self.exempt_pids:
                continue
            rows.append({"pid": pid, "command": command})
        return rows

    def classify(self, rows: list[dict[str, object]]) -> dict[str, list[dict[str, object]]]:
        matches: dict[str, list[dict[str, object]]] = {name: [] for name in self.patterns}
        for row in rows:
            command = str(row["command"])
            for name, pattern in self.patterns.items():
                if pattern.search(command):
                    matches[name].append(row)
        return matches

    def known_listener_records(self, *, use_snapshot: bool = True) -> list[dict[str, object]]:
        if use_snapshot and self.listener_snapshot_path:
            result_stdout = pathlib.Path(self.listener_snapshot_path).read_text(encoding="utf-8")
        else:
            result = subprocess.run(
                ["lsof", "-nP", "-iTCP", "-sTCP:LISTEN"],
                check=False,
                capture_output=True,
                text=True,
            )
            if result.returncode not in (0, 1):
                raise RuntimeError(f"lsof failed: rc={result.returncode} stderr={result.stderr}")
            result_stdout = result.stdout
        rows = self.local_processes(use_snapshot=use_snapshot)
        by_pid = {int(row["pid"]): row for row in rows}
        listeners: list[dict[str, object]] = []
        for line in result_stdout.splitlines()[1:]:
            parts = line.split()
            if len(parts) < 9:
                continue
            try:
                pid = int(parts[1])
            except ValueError:
                continue
            command = by_pid.get(pid, {"command": parts[0]}).get("command", parts[0])
            listener_row = {"pid": pid, "command": command, "endpoint": parts[8]}
            for name, pattern in self.patterns.items():
                if pattern.search(str(command)):
                    listener_row["family"] = name
                    listeners.append(listener_row)
                    break
        return listeners

    def run(self) -> dict[str, object]:
        processes_before = self.local_processes()
        matches_before = self.classify(processes_before)
        residuals_found = [
            {"family": family, "pid": int(row["pid"]), "command": str(row["command"])}
            for family, rows in matches_before.items()
            for row in rows
        ]

        reaped: dict[str, list[int]] = {}
        for family, pattern in self.patterns.items():
            pids = reap_local_process_pattern(pattern.pattern)
            if pids:
                reaped[family] = sorted({int(pid) for pid in pids})

        processes_after = self.local_processes(use_snapshot=False)
        matches_after = self.classify(processes_after)
        listeners_after = self.known_listener_records(use_snapshot=False)
        residuals_after = [
            {"family": family, "pid": int(row["pid"]), "command": str(row["command"])}
            for family, rows in matches_after.items()
            for row in rows
        ]

        verdict = "ready"
        if residuals_found and not residuals_after and not listeners_after:
            verdict = "repaired"
        elif residuals_after or listeners_after:
            verdict = "blocked"

        return {
            "verdict": verdict,
            "residualsFound": residuals_found,
            "reaped": reaped,
            "residualsAfter": residuals_after,
            "listenerResiduals": listeners_after,
        }


def emit_human_summary(summary: dict[str, object]) -> None:
    verdict = str(summary["verdict"]).upper()
    print(f"ground-dual-gds-baseline: {verdict}")
    residuals_found = list(summary["residualsFound"])
    reaped = dict(summary["reaped"])
    residuals_after = list(summary["residualsAfter"])
    listeners_after = list(summary["listenerResiduals"])
    if not residuals_found:
        print("no-action-needed=yes")
    else:
        print("residuals-found:")
        for row in residuals_found:
            print(f"  - {row['family']} pid={row['pid']} cmd={row['command']}")
    if reaped:
        print("reaped-pids:")
        for family, pids in sorted(reaped.items()):
            print(f"  - {family}: {','.join(str(pid) for pid in pids)}")
    if residuals_after:
        print("residuals-after:")
        for row in residuals_after:
            print(f"  - {row['family']} pid={row['pid']} cmd={row['command']}")
    if listeners_after:
        print("listener-residuals:")
        for row in listeners_after:
            print(f"  - {row.get('family', 'unknown')} pid={row['pid']} endpoint={row['endpoint']} cmd={row['command']}")


def main() -> int:
    args = parse_args()
    try:
        manager = GroundBaselineManager()
        summary = manager.run()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        payload = {"verdict": "blocked", "error": str(exc)}
        if args.json_out is not None:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"ground-dual-gds-baseline: BLOCKED\nerror={exc}", file=sys.stderr)
        return 1

    if args.json_out is not None:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    emit_human_summary(summary)
    return 0 if summary["verdict"] != "blocked" else 1


if __name__ == "__main__":
    raise SystemExit(main())
