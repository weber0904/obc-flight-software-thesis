#!/usr/bin/env python3
"""Launch a manual-surface owner outside its caller's process session."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
from collections.abc import Sequence


def launch_detached_owner(command: Sequence[str], *, log_path: pathlib.Path) -> subprocess.Popen[bytes]:
    """Start ``command`` in its own session and return the owner process."""
    if not command:
        raise ValueError("detached owner command must not be empty")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("ab", buffering=0) as log_handle:
        process = subprocess.Popen(
            list(command),
            stdin=subprocess.DEVNULL,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            close_fds=True,
            start_new_session=True,
        )
    return process


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log-path", required=True, type=pathlib.Path)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    print(launch_detached_owner(command, log_path=args.log_path).pid)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
