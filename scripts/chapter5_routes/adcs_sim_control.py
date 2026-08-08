#!/usr/bin/env python3
from __future__ import annotations

import argparse
import socket
import sys
from pathlib import Path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Send a runtime control request to the ADCS simulator control socket."
    )
    parser.add_argument(
        "--socket",
        required=True,
        help="Path to the ADCS simulator Unix-domain control socket.",
    )
    parser.add_argument(
        "--timeout-sec",
        type=float,
        default=3.0,
        help="Socket connect/read timeout in seconds.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    drop_state = subparsers.add_parser("drop-state", help="Drop the next N ADCS STATE replies.")
    drop_state.add_argument("--count", required=True, type=int, help="Number of STATE replies to drop.")
    subparsers.add_parser(
        "restart-pointing-pass",
        help="Restart the synthetic ADCS pointing-pass phase.",
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
        print(f"ADCS control socket not found: {socket_path}", file=sys.stderr)
        return 2

    if args.command == "drop-state":
        if args.count < 0:
            print("--count must be non-negative.", file=sys.stderr)
            return 2
        payload = f"drop-state {args.count}\n"
    elif args.command == "restart-pointing-pass":
        payload = "restart-pointing-pass\n"
    else:
        print(f"Unsupported command: {args.command}", file=sys.stderr)
        return 2
    try:
        response = send_request(socket_path, args.timeout_sec, payload)
    except OSError as exc:
        print(f"Failed to contact ADCS control socket {socket_path}: {exc}", file=sys.stderr)
        return 1

    print(response)
    if not response.startswith("OK "):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
