#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import signal
import socketserver
from typing import Any

from secure_link_auth_lib import (
    SERVICE_ID_SBAND,
    SERVICE_ID_UHF,
    default_command_auth_keystore_path,
    derive_session_key,
    keystore_entry_for_service_id,
    load_command_auth_keystore,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Repo-local secure server simulator for secure auth probes.")
    parser.add_argument("--socket-path", required=True, help="Unix-domain socket path to bind")
    return parser.parse_args()


class SecureServerState:
    def __init__(self, socket_path: pathlib.Path, root_keys: dict[int, bytes]) -> None:
        self.socket_path = socket_path
        self.root_keys = root_keys

    def handle_request(self, payload: dict[str, Any]) -> dict[str, Any]:
        action = payload.get("action")
        if action != "GetSessionKey":
            return {"ok": False, "error": f"unsupported action: {action!r}"}

        service_id = payload.get("serviceId")
        challenge_hex = payload.get("challengeHex")
        if not isinstance(service_id, int):
            return {"ok": False, "error": "serviceId must be an integer"}
        if not isinstance(challenge_hex, str):
            return {"ok": False, "error": "challengeHex must be a string"}

        root_key = self.root_keys.get(service_id)
        if root_key is None:
            return {"ok": False, "error": f"unknown serviceId: {service_id}"}

        try:
            challenge = bytes.fromhex(challenge_hex)
            session_key = derive_session_key(root_key, service_id, challenge)
        except ValueError as exc:
            return {"ok": False, "error": str(exc)}

        return {"ok": True, "sessionKeyHex": session_key.hex()}


class RequestHandler(socketserver.StreamRequestHandler):
    def handle(self) -> None:
        line = self.rfile.readline()
        if not line:
            return
        try:
            payload = json.loads(line.decode("utf-8"))
        except json.JSONDecodeError as exc:
            response = {"ok": False, "error": f"invalid json: {exc}"}
        else:
            response = self.server.state.handle_request(payload)  # type: ignore[attr-defined]
        self.wfile.write(json.dumps(response).encode("utf-8") + b"\n")
        self.wfile.flush()


class UnixStreamServer(socketserver.ThreadingUnixStreamServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, state: SecureServerState) -> None:
        self.state = state
        super().__init__(str(state.socket_path), RequestHandler)


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket_path)
    socket_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        socket_path.unlink()
    except FileNotFoundError:
        pass

    keystore = load_command_auth_keystore(default_command_auth_keystore_path())
    root_keys = {
        SERVICE_ID_SBAND: keystore_entry_for_service_id(keystore, SERVICE_ID_SBAND).key_bytes,
        SERVICE_ID_UHF: keystore_entry_for_service_id(keystore, SERVICE_ID_UHF).key_bytes,
    }
    state = SecureServerState(socket_path, root_keys)
    server = UnixStreamServer(state)

    def shutdown_handler(signum: int, frame: object) -> None:
        del signum, frame
        server.shutdown()

    signal.signal(signal.SIGTERM, shutdown_handler)
    signal.signal(signal.SIGINT, shutdown_handler)

    print(f"security-server-sim listening on {socket_path}", flush=True)
    try:
        server.serve_forever(poll_interval=0.2)
    finally:
        server.server_close()
        try:
            socket_path.unlink()
        except FileNotFoundError:
            pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
