#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
if [[ -x "${ROOT_DIR}/fprime-venv/bin/fprime-cli" ]]; then
  FPRIME_CLI_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-cli"
else
  FPRIME_CLI_BIN="$(command -v fprime-cli || true)"
fi
if [[ -x "${ROOT_DIR}/fprime-venv/bin/fprime-gds" ]]; then
  FPRIME_GDS_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-gds"
else
  FPRIME_GDS_BIN="$(command -v fprime-gds || true)"
fi

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || -z "${FPRIME_CLI_BIN}" || -z "${FPRIME_GDS_BIN}" ]]; then
  echo "Required build outputs or F Prime tools are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/payload-ttc-mode-entry-ccsds.XXXXXX")}"
mkdir -p "${PROBE_TMP_DIR}"

BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
FPRIME_CLI_BIN="${FPRIME_CLI_BIN}" \
FPRIME_GDS_BIN="${FPRIME_GDS_BIN}" \
ROOT_DIR="${ROOT_DIR}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
python3 - <<'PY'
from __future__ import annotations

import os
import pathlib
import shutil
import socket
import subprocess
import sys
import time


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


sys.path.insert(0, str(pathlib.Path(require_env("ROOT_DIR")) / "scripts"))
from probe_process_utils import ManagedProcess, build_gds_stale_match_groups, cleanup_managed_processes, install_signal_cleanup, start_managed_process, stop_managed_process


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
            time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for TCP port {port}")


def start(
    name: str,
    args: list[str],
    log: pathlib.Path,
    env: dict[str, str] | None = None,
    stale_match_groups: tuple[tuple[str, ...], ...] = (),
) -> subprocess.Popen[str]:
    managed = start_managed_process(name, args, log, env=env, stale_match_groups=stale_match_groups)
    processes.append(managed)
    return managed.process


def run_cli(command: str, *args: str) -> None:
    full = [
        fprime_cli,
        "command-send",
        "--dictionary",
        dict_path,
        "--no-zmq",
        "--tts-port",
        str(gds_tts_port),
        command,
        *args,
    ]
    with command_log.open("a", encoding="utf-8") as handle:
        handle.write("$ " + " ".join(full) + "\n")
        handle.flush()
        result = subprocess.run(full, stdout=handle, stderr=subprocess.STDOUT, text=True, check=False)
        handle.write(f"returncode={result.returncode}\n")
    if result.returncode != 0:
        raise RuntimeError(f"fprime-cli command failed: {command}")


def extract_sys_mode_values(text: str) -> list[str]:
    modes: list[str] = []
    for line in text.splitlines():
        if "OBCApp.modeManager.SYS_MODE" not in line:
            continue
        for mode in ("SAFE", "IDLE", "HELL", "PAYLOAD", "TTC"):
            if mode in line:
                modes.append(mode)
                break
    return modes


def capture_sys_mode_snapshot(label: str) -> list[str]:
    safe_label = "".join(ch if ch.isalnum() else "-" for ch in label).strip("-").lower()
    capture_dir = probe_tmp / "channel-captures" / safe_label
    capture_dir.mkdir(parents=True, exist_ok=True)
    full = [
        fprime_cli,
        "channels",
        "--dictionary",
        dict_path,
        "--no-zmq",
        "--tts-port",
        str(gds_tts_port),
        "--search",
        "SYS_MODE",
        "--logs",
        str(capture_dir),
        "--log-directly",
        "--timeout",
        "30",
    ]
    try:
        result = subprocess.run(full, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False, timeout=40.0)
        output = result.stdout
        returncode: int | str = result.returncode
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="replace")
        returncode = "timeout"
    capture_text = read_text(capture_dir / "channel.log")
    with channels_log.open("a", encoding="utf-8") as handle:
        handle.write(f"$ {' '.join(full)} # {label}\n")
        handle.write(output)
        if output and not output.endswith("\n"):
            handle.write("\n")
        if capture_text:
            handle.write(capture_text)
            if not capture_text.endswith("\n"):
                handle.write("\n")
        handle.write(f"returncode={returncode}\n")
    if returncode != 0 and "OBCApp.modeManager.SYS_MODE" not in capture_text and "OBCApp.modeManager.SYS_MODE" not in output:
        raise RuntimeError(f"fprime-cli channels failed while checking {label}")
    observed_modes = extract_sys_mode_values(output + "\n" + capture_text)
    if not observed_modes:
        raise RuntimeError(f"expected SYS_MODE telemetry while checking {label}")
    return observed_modes


def wait_for_sys_mode_snapshot(label: str, expected_mode: str, attempts: int) -> None:
    observed: list[str] = []
    for attempt in range(1, attempts + 1):
        modes = capture_sys_mode_snapshot(f"{label} attempt {attempt}")
        observed.extend(modes)
        if expected_mode in modes:
            return
        time.sleep(0.5)
    raise RuntimeError(f"expected SYS_MODE telemetry {expected_mode} while checking {label}; observed {observed}")


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def wait_text(path: pathlib.Path, fragment: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if fragment in read_text(path):
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path}")


bin_dir = require_env("BIN_DIR")
dict_path = require_env("DICT_PATH")
fprime_cli = require_env("FPRIME_CLI_BIN")
fprime_gds = require_env("FPRIME_GDS_BIN")
probe_tmp = pathlib.Path(require_env("PROBE_TMP_DIR"))
runtime_root = probe_tmp / "runtime"
if runtime_root.exists():
    shutil.rmtree(runtime_root)
(runtime_root / "persistent-data").mkdir(parents=True, exist_ok=True)
(runtime_root / "staging").mkdir(parents=True, exist_ok=True)

gds_port = free_port()
gds_tts_port = free_port()
csp_sub_port = free_port()
csp_pub_port = free_port()
radio_port = free_port()
sband_port = free_port()

processes: list[ManagedProcess] = []
command_log = probe_tmp / "command-send.log"
events_log = probe_tmp / "events.log"
channels_log = probe_tmp / "channels.log"


def cleanup_processes() -> None:
    cleanup_managed_processes(processes, timeout_sec=5.0)


install_signal_cleanup(cleanup_processes)

env = os.environ.copy()
env.update(
    {
        "CSP_TRANSPORT": "zmqhub",
        "CSP_HUB_HOST": "127.0.0.1",
        "CSP_HUB_SUB_PORT": str(csp_sub_port),
        "CSP_HUB_PUB_PORT": str(csp_pub_port),
        "EPS_CSP_NODE_ID": "2",
        "ADCS_CSP_NODE_ID": "3",
    }
)

try:
    start(
        "fprime-gds",
        [
            fprime_gds,
            "-n",
            "-g",
            "none",
            "--framing-selection",
            "space-packet-space-data-link",
            "--scid",
            "68",
            "--vcid",
            "1",
            "--frame-size",
            "1024",
            "--dictionary",
            dict_path,
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
            str(probe_tmp / "gds-logs"),
        ],
        probe_tmp / "gds.log",
        stale_match_groups=build_gds_stale_match_groups(ip_port=gds_port, tts_port=gds_tts_port),
    )
    wait_port(gds_port, 20.0)
    wait_port(gds_tts_port, 20.0)

    start("csp_zmqproxy", [f"{bin_dir}/csp_zmqproxy", "-s", f"tcp://0.0.0.0:{csp_sub_port}", "-p", f"tcp://0.0.0.0:{csp_pub_port}"], probe_tmp / "csp_zmqproxy.log", env)
    start("eps_simulator", [f"{bin_dir}/eps_simulator", "--node-id", "2"], probe_tmp / "eps_simulator.log", env)
    start("adcs_simulator", [f"{bin_dir}/adcs_simulator", "--node-id", "3"], probe_tmp / "adcs_simulator.log", env)
    start("radio_mock_server", [f"{bin_dir}/radio_mock_server", "--port", str(radio_port)], probe_tmp / "radio_mock_server.log", env)
    start(
        "sband_comm_csp_node",
        [f"{bin_dir}/sband_comm_csp_node", "--tcp-listen-host", "127.0.0.1", "--tcp-listen-port", str(sband_port), "--node-id", "5"],
        probe_tmp / "sband_comm_csp_node.log",
        env,
    )
    wait_port(sband_port, 20.0)
    start(
        "ground_ttc_gateway",
        [
            f"{bin_dir}/ground_ttc_gateway",
            "--rf-tcp-host",
            "127.0.0.1",
            "--rf-tcp-port",
            str(sband_port),
            "--link-identity",
            "sband",
            "--gds-host",
            "127.0.0.1",
            "--gds-port",
            str(gds_port),
        ],
        probe_tmp / "ground_ttc_gateway.log",
        env,
    )
    time.sleep(1.0)
    obc_args = [
        f"{bin_dir}/OBC",
        "--comm",
        "tcp",
        "--comm-host",
        "127.0.0.1",
        "--comm-port",
        str(radio_port),
        "--radio-protocol",
        "mock-text",
        "--ground-link",
        "comm-csp",
        "--comm-csp-node",
        "5",
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
        "--tick-ms",
        "250",
        "--headless",
    ]
    start("OBC", obc_args, probe_tmp / "obc.log", env)
    wait_text(probe_tmp / "obc.log", "Runtime mode: headless", 20.0)
    wait_for_sys_mode_snapshot("initial boot", "SAFE", attempts=3)

    command = "OBCApp.modeManager.MODE_SET"
    events = start(
        "fprime-cli-events",
        [fprime_cli, "events", "--dictionary", dict_path, "--no-zmq", "--tts-port", str(gds_tts_port)],
        events_log,
    )
    time.sleep(1.0)

    event_sequence = [
        ("PAYLOAD", None),
        ("TTC", None),
        ("IDLE", "IDLE"),
        ("PAYLOAD", "PAYLOAD"),
        ("TTC", None),
        ("SAFE", "SAFE"),
    ]
    for mode, _expected_sys_mode in event_sequence:
        run_cli(command, "--arguments", mode)
        time.sleep(1.0)

    wait_for_sys_mode_snapshot("final after MODE_SET sequence", "SAFE", attempts=6)

    wait_text(events_log, "SYS_MODE_CHANGE", 20.0)
    wait_text(events_log, "SYS_MODE_TRANSITION_REJECTED", 20.0)
    wait_text(channels_log, "SYS_MODE", 20.0)

    event_text = read_text(events_log)
    channel_text = read_text(channels_log)
    command_text = read_text(command_log)
    if event_text.count("SYS_MODE_TRANSITION_REJECTED") < 3:
        raise RuntimeError("expected at least three rejected mode transition events")
    if "System mode changed to IDLE" not in event_text or "System mode changed to PAYLOAD" not in event_text or "System mode changed to SAFE" not in event_text:
        raise RuntimeError("expected IDLE, PAYLOAD, and SAFE mode-change events")
    if "System mode transition rejected from PAYLOAD to TTC reason 1" not in event_text:
        raise RuntimeError("expected rejected PAYLOAD -> TTC command after PAYLOAD entry")
    if "returncode=0" not in command_text:
        raise RuntimeError("expected successful fprime-cli command-send invocations")
    if "SAFE" not in channel_text:
        raise RuntimeError("expected final SYS_MODE telemetry evidence containing SAFE")

    summary = probe_tmp / "summary.log"
    summary.write_text(
        "\n".join(
            [
                "payload_ttc_mode_entry_ccsds_probe: PASS",
                f"gds-port={gds_port}",
                f"gds-tts-port={gds_tts_port}",
                f"sband-tcp-port={sband_port}",
                f"events-log={events_log}",
                f"channels-log={channels_log}",
                f"command-log={command_log}",
            ]
        )
        + "\n",
        encoding="utf-8",
    )
    print(summary.read_text(encoding="utf-8"), end="")
finally:
    cleanup_processes()
PY
