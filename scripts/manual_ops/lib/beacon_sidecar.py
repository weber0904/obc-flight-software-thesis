#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import pathlib
import sys
import termios
import time


def configure_raw_reader(fd: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[3] = attrs[3] & ~(termios.ICANON | termios.ECHO)
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    os.set_blocking(fd, False)


def capture(device: pathlib.Path, output: pathlib.Path) -> int:
    output.parent.mkdir(parents=True, exist_ok=True)
    fd = os.open(str(device), os.O_RDONLY | os.O_NOCTTY)
    try:
        configure_raw_reader(fd)
        with output.open("wb") as handle:
            while True:
                try:
                    chunk = os.read(fd, 4096)
                except BlockingIOError:
                    time.sleep(0.05)
                    continue
                if chunk == b"":
                    break
                handle.write(chunk)
                handle.flush()
    finally:
        os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture beacon bytes from a PTY/serial side-channel into a local file.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    capture_parser = subparsers.add_parser("capture")
    capture_parser.add_argument("--device", required=True, type=pathlib.Path)
    capture_parser.add_argument("--output", required=True, type=pathlib.Path)

    args = parser.parse_args()
    if args.command == "capture":
        return capture(args.device, args.output)
    raise RuntimeError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
