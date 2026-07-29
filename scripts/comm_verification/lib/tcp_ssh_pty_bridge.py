#!/usr/bin/env python3
from __future__ import annotations

import argparse
import errno
import os
import pty
import selectors
import shlex
import signal
import subprocess
import sys
import time
import tty


REMOTE_BRIDGE_CODE = r"""
import os
import selectors
import sys
import termios
import tty

path = sys.argv[1]
fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
tty.setraw(fd)
selector = selectors.DefaultSelector()
selector.register(sys.stdin.buffer, selectors.EVENT_READ, "stdin")
selector.register(fd, selectors.EVENT_READ, "pty")
while True:
    for key, _ in selector.select():
        if key.data == "stdin":
            chunk = os.read(sys.stdin.fileno(), 4096)
            if not chunk:
                os.close(fd)
                raise SystemExit(0)
            os.write(fd, chunk)
        else:
            chunk = os.read(fd, 4096)
            if not chunk:
                os.close(fd)
                raise SystemExit(0)
            os.write(sys.stdout.fileno(), chunk)
            sys.stdout.flush()
"""


def spawn_remote_bridge(ssh_target: str, remote_pty: str) -> subprocess.Popen[bytes]:
    remote_cmd = "python3 -u -c {code} {pty}".format(
        code=shlex.quote(REMOTE_BRIDGE_CODE),
        pty=shlex.quote(remote_pty),
    )
    return subprocess.Popen(
        ["ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=5", ssh_target, remote_cmd],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )


def write_all_nonblocking(fd: int, data: bytes, timeout: float = 30.0) -> None:
    if not data:
        return
    deadline = time.time() + timeout
    view = memoryview(data)
    selector = selectors.DefaultSelector()
    selector.register(fd, selectors.EVENT_WRITE)
    try:
        while view:
            try:
                written = os.write(fd, view)
            except BlockingIOError:
                written = 0
            except InterruptedError:
                continue
            except OSError as exc:
                if exc.errno in (errno.EAGAIN, errno.EWOULDBLOCK, errno.EIO):
                    written = 0
                else:
                    raise
            if written > 0:
                view = view[written:]
                continue
            remaining = deadline - time.time()
            if remaining <= 0:
                raise TimeoutError("timed out writing bridged PTY data")
            selector.select(timeout=min(0.5, remaining))
    finally:
        selector.close()


def bridge(local_master_fd: int, remote: subprocess.Popen[bytes]) -> None:
    if remote.stdin is None or remote.stdout is None:
        raise RuntimeError("remote bridge did not expose stdio pipes")
    os.set_blocking(local_master_fd, False)
    os.set_blocking(remote.stdout.fileno(), False)
    remote_stdin_fd = remote.stdin.fileno()
    os.set_blocking(remote_stdin_fd, False)
    selector = selectors.DefaultSelector()
    selector.register(local_master_fd, selectors.EVENT_READ, "local")
    selector.register(remote.stdout, selectors.EVENT_READ, "remote")
    while True:
        events = selector.select(timeout=0.5)
        if remote.poll() is not None:
            raise RuntimeError(f"remote bridge exited rc={remote.returncode}")
        for key, _ in events:
            if key.data == "local":
                chunk = os.read(local_master_fd, 4096)
                if not chunk:
                    return
                write_all_nonblocking(remote_stdin_fd, chunk)
            else:
                chunk = os.read(remote.stdout.fileno(), 4096)
                if not chunk:
                    return
                write_all_nonblocking(local_master_fd, chunk)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ssh-target", required=True)
    parser.add_argument("--remote-pty", required=True)
    args = parser.parse_args()

    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
    signal.signal(signal.SIGINT, lambda *_: sys.exit(0))

    local_master_fd, local_slave_fd = pty.openpty()
    try:
        tty.setraw(local_slave_fd)
        local_pty = os.ttyname(local_slave_fd)
        print(f"LOCAL_PTY={local_pty}", flush=True)
        remote = spawn_remote_bridge(args.ssh_target, args.remote_pty)
        try:
            bridge(local_master_fd, remote)
        finally:
            if remote.poll() is None:
                remote.terminate()
                try:
                    remote.wait(timeout=2.0)
                except subprocess.TimeoutExpired:
                    remote.kill()
                    remote.wait(timeout=2.0)
            if remote.returncode not in (0, None):
                stderr = b""
                if remote.stderr is not None:
                    stderr = remote.stderr.read() or b""
                if stderr:
                    sys.stderr.write(stderr.decode("utf-8", errors="replace"))
                    sys.stderr.flush()
                    print(f"REMOTE_STDERR {args.remote_pty}", flush=True)
    finally:
        os.close(local_master_fd)
        os.close(local_slave_fd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
