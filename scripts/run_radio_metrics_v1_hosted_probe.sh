#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="${DICT_PATH:-$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)}"

find_local_tool() {
  local tool_name="${1:?tool_name is required}"
  if [[ -x "${ROOT_DIR}/fprime-venv/bin/${tool_name}" ]]; then
    printf '%s\n' "${ROOT_DIR}/fprime-venv/bin/${tool_name}"
    return 0
  fi
  command -v "${tool_name}"
}

FPRIME_GDS_BIN="$(find_local_tool fprime-gds || true)"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || -z "${FPRIME_GDS_BIN}" ]]; then
  echo "Required build outputs or F Prime tools are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/radio-metrics-v1-hosted.XXXXXX")}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_GDS_BIN="${FPRIME_GDS_BIN}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import re
import shlex
import shutil
import socket
import subprocess
import sys
import threading
import time


READY_TIMEOUT = 12.0


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
FPRIME_GDS_BIN = require_env("FPRIME_GDS_BIN")
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from probe_process_utils import (  # noqa: E402
    ManagedProcess,
    build_gds_stale_match_groups,
    cleanup_managed_processes,
    install_signal_cleanup,
    reap_matching_processes,
)


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


def build_fragment_group(*fragments: object) -> tuple[str, ...]:
    return tuple(str(fragment) for fragment in fragments if str(fragment))


def extract_logs_dir(summary: str) -> pathlib.Path:
    match = re.search(r"^logs=(.+)$", summary, re.MULTILINE)
    if not match:
        raise RuntimeError(f"probe summary did not contain logs path:\n{summary}")
    return pathlib.Path(match.group(1).strip())


def assert_fragments(path: pathlib.Path, fragments: list[str], label: str) -> None:
    text = path.read_text(encoding="utf-8", errors="replace")
    missing = [fragment for fragment in fragments if fragment not in text]
    if missing:
        raise RuntimeError(f"{label} missing required fragments {missing} in {path}")


def run_existing_probe(script_name: str, label: str, expected_fragments: list[str]) -> pathlib.Path:
    probe_dir = PROBE_TMP_DIR / label
    shutil.rmtree(probe_dir, ignore_errors=True)
    probe_dir.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(
        ["bash", str(ROOT_DIR / "scripts" / script_name)],
        env={**os.environ, "PROBE_TMP_DIR": str(probe_dir)},
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{script_name} failed rc={result.returncode}\nstdout={result.stdout}\nstderr={result.stderr}"
        )
    logs_dir = extract_logs_dir(result.stdout)
    assert_fragments(logs_dir / "obc.log", expected_fragments, label)
    return logs_dir


def run_direct_tcp_probe() -> pathlib.Path:
    probe_dir = PROBE_TMP_DIR / "direct-tcp"
    shutil.rmtree(probe_dir, ignore_errors=True)
    probe_dir.mkdir(parents=True, exist_ok=True)

    runtime_root = probe_dir / "runtime"
    gds_log_dir = probe_dir / "gds-logs"
    gds_wrapper_log = probe_dir / "gds-wrapper.log"
    radio_log = probe_dir / "radio-mock-server.log"
    obc_log = probe_dir / "obc.log"
    summary_log = probe_dir / "summary.log"
    processes: list[ManagedProcess] = []
    obc = None
    obc_thread = None
    obc_lines: list[str] = []

    def start_process(
        name: str,
        args: list[str],
        log_path: pathlib.Path,
        *,
        env: dict[str, str] | None = None,
        stdin=None,
        handle=None,
        stdout_pipe: bool = False,
        stale_match_groups: tuple[tuple[str, ...], ...] = (),
        stale_match_markers: tuple[str, ...] = (),
    ) -> subprocess.Popen[str]:
        if stale_match_groups:
            reap_matching_processes(stale_match_groups, markers=stale_match_markers)
        if handle is None:
            handle = log_path.open("w", encoding="utf-8", buffering=1)
        handle.write("$ " + shlex.join(str(arg) for arg in args) + "\n")
        handle.flush()
        process = subprocess.Popen(
            [str(arg) for arg in args],
            env=env,
            stdin=stdin if stdin is not None else subprocess.DEVNULL,
            stdout=subprocess.PIPE if stdout_pipe else handle,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        processes.append(
            ManagedProcess(
                name=name,
                process=process,
                handle=handle,
                stale_match_groups=stale_match_groups,
                stale_match_markers=stale_match_markers,
            )
        )
        return process

    def collect_output(process: subprocess.Popen[str], lines: list[str], handle) -> threading.Thread:
        def reader() -> None:
            assert process.stdout is not None
            for line in process.stdout:
                lines.append(line)
                try:
                    handle.write(line)
                    handle.flush()
                except ValueError:
                    break

        thread = threading.Thread(target=reader, daemon=True)
        thread.start()
        return thread

    def send_command(process: subprocess.Popen[str], command: str) -> None:
        if process.poll() is not None:
            raise RuntimeError(f"OBC exited before command '{command}'")
        assert process.stdin is not None
        process.stdin.write(command + "\n")
        process.stdin.flush()

    def request_status(process: subprocess.Popen[str], lines: list[str], timeout: float = 6.0) -> str:
        start = len(lines)
        send_command(process, "status")
        deadline = time.time() + timeout
        while time.time() < deadline:
            text = "".join(lines[start:])
            if "boot resetCause=" in text and "groundLink mode=" in text:
                return text
            time.sleep(0.05)
        raise RuntimeError("timed out waiting for hosted status output")

    def wait_for_status(process: subprocess.Popen[str], lines: list[str], required: list[str], timeout: float) -> str:
        deadline = time.time() + timeout
        last_status = ""
        while time.time() < deadline:
            last_status = request_status(process, lines)
            if all(item in last_status for item in required):
                return last_status
            time.sleep(0.2)
        raise RuntimeError(f"timed out waiting for status markers {required}; last status was:\n{last_status}")

    def cleanup_processes() -> None:
        cleanup_managed_processes(processes, timeout_sec=5.0)

    install_signal_cleanup(cleanup_processes)

    gds_port = free_port()
    gds_tts_port = free_port()
    radio_port = free_port()
    shutil.rmtree(runtime_root, ignore_errors=True)
    shutil.rmtree(gds_log_dir, ignore_errors=True)
    (runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
    (runtime_root / "staging").mkdir(parents=True, exist_ok=True)
    gds_log_dir.mkdir(parents=True, exist_ok=True)

    try:
        with obc_log.open("w", encoding="utf-8", buffering=1) as obc_handle:
            start_process(
                "fprime-gds",
                [
                    FPRIME_GDS_BIN,
                    "-n",
                    "-g",
                    "none",
                    "--dictionary",
                    str(DICT_PATH),
                    "--no-zmq",
                    "--ip-address",
                    "127.0.0.1",
                    "--ip-port",
                    str(gds_port),
                    "--tts-port",
                    str(gds_tts_port),
                    "--tts-addr",
                    "127.0.0.1",
                    "--log-directly",
                    "--logs",
                    str(gds_log_dir),
                ],
                gds_wrapper_log,
                stale_match_groups=build_gds_stale_match_groups(ip_port=gds_port, tts_port=gds_tts_port),
                stale_match_markers=("fprime-gds", "fprime_gds.executables.comm", "fprime_gds.executables.tcpserver"),
            )
            wait_port(gds_port, READY_TIMEOUT)
            wait_port(gds_tts_port, READY_TIMEOUT)

            start_process(
                "radio-mock-server",
                [str(BIN_DIR / "radio_mock_server"), "--port", str(radio_port)],
                radio_log,
                stale_match_groups=(build_fragment_group(str(BIN_DIR / "radio_mock_server"), f"--port {radio_port}"),),
            )

            obc = start_process(
                "obc",
                [
                    str(BIN_DIR / "OBC"),
                    "--comm",
                    "tcp",
                    "--comm-host",
                    "127.0.0.1",
                    "--comm-port",
                    str(radio_port),
                    "--radio-protocol",
                    "mock-text",
                    "--ground-link",
                    "direct-tcp",
                    "--gds-host",
                    "127.0.0.1",
                    "--gds-port",
                    str(gds_port),
                    "--runtime-root",
                    str(runtime_root),
                    "--persistent-root",
                    str(runtime_root / "persistent-data"),
                    "--staging-root",
                    str(runtime_root / "staging"),
                ],
                obc_log,
                stdin=subprocess.PIPE,
                handle=obc_handle,
                stdout_pipe=True,
                stale_match_groups=(build_fragment_group(str(BIN_DIR / "OBC"), f"--runtime-root {runtime_root}"),),
            )
            obc_thread = collect_output(obc, obc_lines, obc_handle)
            wait_for_status(
                obc,
                obc_lines,
                [
                    "groundLink mode=direct-tcp",
                    "sbandAvailable=yes",
                    "sbandReason=CONNECTED_ONLY_FALLBACK",
                    "radioObservation sample=yes ageTicks=0 result=OK",
                    "groundLinkRaw band=SBAND mode=direct-tcp semantics=CONNECTED_ONLY_FALLBACK connected=yes",
                    "txBytes=",
                    "rxBytes=",
                    "statusObs=",
                ],
                READY_TIMEOUT,
            )
            summary_log.write_text(
                "\n".join(
                    [
                        "radio-metrics-v1-direct-tcp: PASS",
                        "formal-verdict=direct-tcp-connected-only-fallback",
                        "sband-reason=CONNECTED_ONLY_FALLBACK",
                        "evidence=hosted-status-readback",
                        f"logs={probe_dir}",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
        return probe_dir
    finally:
        try:
            if obc is not None and obc.stdin is not None and obc.poll() is None:
                send_command(obc, "quit")
        except Exception:
            pass
        if obc_thread is not None:
            obc_thread.join(timeout=2.0)
        cleanup_processes()


def main() -> None:
    sband_logs = run_existing_probe(
        "run_sband_ccsds_primary_probe.sh",
        "sband-node5",
        [
            "radioObservation sample=yes ageTicks=0 result=OK",
            "groundLinkRaw band=SBAND mode=comm-csp semantics=ACTIVE_COMM_CSP connected=yes",
            "txBytes=",
            "rxBytes=",
            "statusObs=",
        ],
    )
    uhf_logs = run_existing_probe(
        "run_uhf_node6_backup_probe.sh",
        "uhf-node6",
        [
            "radioObservation sample=yes ageTicks=0 result=OK",
            "groundLinkRaw band=UHF mode=comm-csp semantics=ACTIVE_COMM_CSP connected=yes",
            "txBytes=",
            "rxBytes=",
            "statusObs=",
        ],
    )
    direct_logs = run_direct_tcp_probe()
    summary = "\n".join(
        [
            "radio-metrics-v1-hosted-probe: PASS",
            "hosted-node5=sband-comm-csp",
            "hosted-node6=uhf-comm-csp",
            "hosted-direct-tcp=connected-only-fallback",
            "contract=raw-ground-link-plus-cached-radio-observation",
            "evidence=hosted-status-readback-and-existing-command-path-probes",
            f"sband-logs={sband_logs}",
            f"uhf-logs={uhf_logs}",
            f"direct-logs={direct_logs}",
        ]
    )
    print(summary)


if __name__ == "__main__":
    main()
PY
