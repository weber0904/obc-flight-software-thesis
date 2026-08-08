from __future__ import annotations

import hashlib
import json
import os
import pathlib
import shlex
import shutil
import signal
import socket
import subprocess
import tempfile
import time
from typing import Any, Callable


ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
SCRIPTS_DIR = ROOT_DIR / "scripts"
FPRIME_VENV_BIN = ROOT_DIR / "fprime-venv" / "bin"
DEFAULT_SECURE_AUTH_TIMEOUT_SEC = 180


def utc_timestamp() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def ensure_dir(path: pathlib.Path) -> pathlib.Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def atomic_write_json(path: pathlib.Path, payload: dict[str, Any]) -> None:
    ensure_dir(path.parent)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=str(path.parent),
        prefix=f".{path.name}.",
        suffix=".tmp",
        delete=False,
    ) as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")
        temp_path = pathlib.Path(handle.name)
    os.replace(temp_path, path)


def read_json(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def pid_alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def find_local_tool(tool_name: str) -> str:
    candidate = FPRIME_VENV_BIN / tool_name
    if candidate.exists():
        return str(candidate)
    resolved = shutil_which(tool_name)
    if resolved:
        return resolved
    raise RuntimeError(f"{tool_name} not found in fprime-venv or PATH")


def shutil_which(tool_name: str) -> str | None:
    return shutil.which(tool_name)


def wait_port(host: str, port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {host}:{port}")


def find_free_port(host: str = "127.0.0.1") -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind((host, 0))
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        return int(sock.getsockname()[1])


def reserve_free_port(host: str = "127.0.0.1") -> tuple[int, socket.socket]:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((host, 0))
    return int(sock.getsockname()[1]), sock


def safe_remove(path: pathlib.Path) -> None:
    try:
        path.unlink()
    except FileNotFoundError:
        return


def shell_join(args: list[str]) -> str:
    return shlex.join(args)


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def ssh_command(target: str, script: str) -> list[str]:
    return ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", target, script]


def ssh_capture(target: str, script: str, *, check: bool = True) -> str:
    result = subprocess.run(
        ssh_command(target, script),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if check and result.returncode != 0:
        raise RuntimeError(
            f"ssh command failed target={target} rc={result.returncode}\n"
            f"cmd={script}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    return result.stdout


def ssh_capture_bytes(target: str, script: str, *, check: bool = True) -> bytes:
    result = subprocess.run(
        ssh_command(target, script),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and result.returncode != 0:
        raise RuntimeError(
            f"ssh command failed target={target} rc={result.returncode}\n"
            f"cmd={script}\nstdout-bytes={len(result.stdout)}\nstderr={result.stderr.decode('utf-8', errors='replace')}"
        )
    return result.stdout


def systemctl_show(target: str, unit: str, fields: tuple[str, ...]) -> dict[str, str]:
    props = " ".join(f"-p {shlex.quote(field)}" for field in fields)
    text = ssh_capture(target, f"systemctl show {props} {shlex.quote(unit)}")
    values: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def service_environment(target: str, service: str) -> dict[str, str]:
    text = ssh_capture(
        target,
        f"systemctl show {shlex.quote(service)} --property=Environment --value",
        check=False,
    ).strip()
    env: dict[str, str] = {}
    for token in shlex.split(text):
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        env[key] = value
    return env


def wait_remote_service_active(target: str, unit: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = ssh_capture(target, f"systemctl is-active {shlex.quote(unit)} || true", check=False).strip()
        if state == "active":
            return
        time.sleep(1.0)
    raise RuntimeError(f"{unit} on {target} did not become active")


def service_invocation_id(target: str, unit: str) -> str:
    return systemctl_show(target, unit, ("InvocationID",)).get("InvocationID", "").strip()


def remote_journal_since_now(target: str) -> str:
    marker = ssh_capture(
        target,
        "python3 -c 'import time; print(f\"@{time.time():.6f}\")'",
    ).strip()
    if marker.startswith("@"):
        return marker
    return ssh_capture(target, "date '+%Y-%m-%d %H:%M:%S.%N'").strip()


def wait_remote_journal_fragments(
    target: str,
    unit: str,
    fragments: tuple[str, ...],
    timeout: float,
    *,
    since: str | None = None,
    invocation_id: str | None = None,
) -> None:
    journal_since = since or remote_journal_since_now(target)
    deadline = time.time() + timeout
    last_journal = ""
    while time.time() < deadline:
        if invocation_id:
            last_journal = ssh_capture(
                target,
                f"journalctl _SYSTEMD_INVOCATION_ID={shlex.quote(invocation_id)} --no-pager || true",
                check=False,
            )
        else:
            last_journal = ssh_capture(
                target,
                f"journalctl -u {shlex.quote(unit)} --since {shlex.quote(journal_since)} --no-pager || true",
                check=False,
            )
        if all(fragment in last_journal for fragment in fragments):
            return
        time.sleep(0.5)
    raise RuntimeError(f"timed out waiting for journal fragments on {target}:{unit}; last={last_journal[-4000:]}")


def service_override_path(service: str, dropin_name: str) -> str:
    return f"/etc/systemd/system/{service}.d/{dropin_name}"


def render_service_override(env: dict[str, str]) -> str:
    lines = ["[Service]"]
    for key, value in env.items():
        lines.append(f"Environment={key}={value}")
    return "\n".join(lines) + "\n"


def apply_service_override(target: str, service: str, dropin_name: str, env: dict[str, str]) -> None:
    override_path = service_override_path(service, dropin_name)
    payload = render_service_override(env)
    remote_tmp = f"/tmp/{dropin_name}.tmp"
    result = subprocess.run(
        ssh_command(target, f"cat > {shlex.quote(remote_tmp)}"),
        input=payload,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"failed to stage override {dropin_name} for {service} on {target}: "
            f"rc={result.returncode}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    ssh_capture(
        target,
        (
            f"sudo mkdir -p {shlex.quote(f'/etc/systemd/system/{service}.d')} && "
            f"sudo install -m 0644 {shlex.quote(remote_tmp)} {shlex.quote(override_path)} && "
            f"rm -f {shlex.quote(remote_tmp)} && "
            "sudo systemctl daemon-reload && "
            f"sudo systemctl restart {shlex.quote(service)}"
        ),
    )


def remove_service_override(target: str, service: str, dropin_name: str) -> None:
    ssh_capture(
        target,
        (
            f"sudo rm -f {shlex.quote(service_override_path(service, dropin_name))} && "
            "sudo systemctl daemon-reload && "
            f"sudo systemctl restart {shlex.quote(service)}"
        ),
        check=False,
    )


def install_termination_handler(cleanup_fn: Callable[[], None]) -> None:
    state = {"done": False}

    def handler(signum: int, _frame: object) -> None:
        if not state["done"]:
            state["done"] = True
            cleanup_fn()
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)
