from __future__ import annotations

import pathlib
import socket


def set_soc(socket_path: str | pathlib.Path, value: float, transition_sec: float = 0.0, timeout_sec: float = 5.0) -> str:
    path = pathlib.Path(socket_path)
    if not path.exists():
        raise RuntimeError(f"EPS control socket does not exist: {path}")
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(timeout_sec)
        client.connect(str(path))
        command = f"set-soc {value:.2f} {transition_sec:.2f}\n".encode("utf-8")
        client.sendall(command)
        response = client.recv(4096).decode("utf-8", errors="replace").strip()
    if not response.startswith("OK "):
        raise RuntimeError(f"EPS control command failed: {response}")
    return response


def set_load_mode(
    socket_path: str | pathlib.Path,
    mode: str,
    timeout_sec: float = 5.0,
) -> str:
    if mode not in {"normal", "high-draw"}:
        raise RuntimeError(f"unsupported EPS load mode: {mode}")
    path = pathlib.Path(socket_path)
    if not path.exists():
        raise RuntimeError(f"EPS control socket does not exist: {path}")
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(timeout_sec)
        client.connect(str(path))
        command = f"set-load-mode {mode}\n".encode("utf-8")
        client.sendall(command)
        response = client.recv(4096).decode("utf-8", errors="replace").strip()
    if not response.startswith("OK "):
        raise RuntimeError(f"EPS control command failed: {response}")
    return response
