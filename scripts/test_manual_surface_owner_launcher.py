#!/usr/bin/env python3
from __future__ import annotations

import os
import pathlib
import signal
import sys
import tempfile
import time
import unittest

from manual_ops.lib.detached_owner_launcher import launch_detached_owner


class DetachedOwnerLauncherTest(unittest.TestCase):
    def test_launches_owner_in_a_new_session_with_a_persistent_log(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            log_path = pathlib.Path(temporary_directory) / "owner.log"
            owner_process = launch_detached_owner(
                [
                    sys.executable,
                    "-c",
                    "import time; print('detached-owner-ready', flush=True); time.sleep(30)",
                ],
                log_path=log_path,
            )
            owner_pid = owner_process.pid
            try:
                self.assertEqual(os.getsid(owner_pid), owner_pid)
                deadline = time.monotonic() + 5.0
                while time.monotonic() < deadline:
                    if log_path.exists() and "detached-owner-ready" in log_path.read_text(encoding="utf-8"):
                        break
                    time.sleep(0.05)
                else:
                    self.fail("detached owner did not write its launcher log")
            finally:
                try:
                    os.killpg(owner_pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                owner_process.wait(timeout=5.0)


if __name__ == "__main__":
    unittest.main()
