#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]

SCAN_FILES = [
    pathlib.Path("README.md"),
    pathlib.Path("simulators/README.md"),
    pathlib.Path("simulators/CMakeLists.txt"),
    pathlib.Path("docs/verification-matrix.md"),
    pathlib.Path("docs/verification-path-registry.md"),
    pathlib.Path("obc-dev-spec/03_eps_subsystem.md"),
    pathlib.Path("obc-dev-spec/04_adcs_subsystem.md"),
    pathlib.Path("obc-dev-spec/07_verification_evidence.md"),
]

SCAN_DIRS = [
    pathlib.Path("OBC"),
    pathlib.Path("simulators/eps"),
    pathlib.Path("simulators/adcs"),
    pathlib.Path("simulators/scenario"),
    pathlib.Path("scripts"),
]

SELF = pathlib.Path("scripts/check_legacy_zmq_retired.py")

FORBIDDEN_PATTERNS = [
    "simulators/common/protocol.h",
    "ZmqEpsTransport",
    "ZmqAdcsTransport",
    "eps_zmq_integration_test",
    "adcs_zmq_integration_test",
    "tcp://127.0.0.1:5555",
    "tcp://127.0.0.1:5556",
    "EPS_ENDPOINT",
    "ADCS_ENDPOINT",
    "DEFAULT_ENDPOINT",
    "ZMQ_REQ",
    "ZMQ_REP",
    "zmq_send",
    "zmq_recv",
]

TEXT_SUFFIXES = {
    ".c",
    ".cpp",
    ".hpp",
    ".h",
    ".fpp",
    ".md",
    ".py",
    ".sh",
    ".txt",
    ".cmake",
}


def iter_scan_paths() -> list[pathlib.Path]:
    paths: list[pathlib.Path] = []
    for relative in SCAN_FILES:
        path = ROOT / relative
        if path.exists():
            paths.append(path)

    for relative_dir in SCAN_DIRS:
        root = ROOT / relative_dir
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix in TEXT_SUFFIXES:
                paths.append(path)

    return sorted(path for path in set(paths) if path.relative_to(ROOT) != SELF)


def main() -> int:
    scan_paths = iter_scan_paths()
    failures: list[str] = []
    for path in scan_paths:
        relative = path.relative_to(ROOT)
        text = path.read_text(encoding="utf-8", errors="replace")
        for line_no, line in enumerate(text.splitlines(), start=1):
            for pattern in FORBIDDEN_PATTERNS:
                if pattern in line:
                    failures.append(f"{relative}:{line_no}: forbidden legacy EPS/ADCS direct-ZMQ pattern: {pattern}")

    if failures:
        print("FAIL: legacy EPS/ADCS direct-ZMQ remnants found")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("PASS: legacy EPS/ADCS direct-ZMQ active paths are retired")
    print(f"- scanned files: {len(scan_paths)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
