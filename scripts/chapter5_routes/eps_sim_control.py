#!/usr/bin/env python3
from __future__ import annotations

import argparse
import socket
import sys
from pathlib import Path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Send a runtime SoC control request to the EPS simulator control socket."
    )
    parser.add_argument(
        "--socket",
        required=True,
        help="Path to the EPS simulator Unix-domain control socket.",
    )
    parser.add_argument(
        "--timeout-sec",
        type=float,
        default=3.0,
        help="Socket connect/read timeout in seconds.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    set_soc = subparsers.add_parser("set-soc", help="Set a new EPS SoC target.")
    set_soc.add_argument("--value", required=True, type=float, help="Target SoC percentage.")
    set_soc.add_argument(
        "--transition-sec",
        default=0.0,
        type=float,
        help="Ramp duration in seconds. Use 0 for an immediate jump.",
    )
    set_load_mode = subparsers.add_parser("set-load-mode", help="Set the EPS runtime load mode.")
    set_load_mode.add_argument(
        "--mode",
        required=True,
        choices=("normal", "high-draw"),
        help="Named load mode to apply.",
    )
    drop_status = subparsers.add_parser(
        "drop-status",
        help="Drop the next N EPS STATUS replies.",
    )
    drop_status.add_argument(
        "--count",
        required=True,
        type=int,
        help="Number of STATUS replies to drop.",
    )
    return parser


def send_request(socket_path: Path, timeout_sec: float, payload: str) -> str:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(timeout_sec)
        client.connect(str(socket_path))
        client.sendall(payload.encode("utf-8"))
        data = b""
        while not data.endswith(b"\n"):
            chunk = client.recv(4096)
            if not chunk:
                break
            data += chunk
    return data.decode("utf-8", errors="replace").strip()


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    socket_path = Path(args.socket)
    if not socket_path.exists():
        print(f"EPS control socket not found: {socket_path}", file=sys.stderr)
        return 2

    if args.command == "set-soc":
        if args.transition_sec < 0.0:
            print("--transition-sec must be non-negative.", file=sys.stderr)
            return 2
        payload = f"set-soc {args.value:.6f} {args.transition_sec:.6f}\n"
    elif args.command == "set-load-mode":
        payload = f"set-load-mode {args.mode}\n"
    elif args.command == "drop-status":
        if args.count < 0:
            print("--count must be non-negative.", file=sys.stderr)
            return 2
        payload = f"drop-status {args.count}\n"
    else:
        print(f"Unsupported command: {args.command}", file=sys.stderr)
        return 2

    try:
        response = send_request(socket_path, args.timeout_sec, payload)
    except OSError as exc:
        print(f"Failed to contact EPS control socket {socket_path}: {exc}", file=sys.stderr)
        return 1

    print(response)
    if not response.startswith("OK "):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
