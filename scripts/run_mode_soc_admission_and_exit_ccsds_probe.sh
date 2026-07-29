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

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/mode-soc-admission-ccsds.XXXXXX")}"
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
import re
import shutil
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from typing import Optional


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


sys.path.insert(0, str(pathlib.Path(require_env("ROOT_DIR")) / "scripts"))
from probe_process_utils import ManagedProcess, build_gds_stale_match_groups, cleanup_managed_processes, install_signal_cleanup, start_managed_process, stop_managed_process


@dataclass(frozen=True)
class ProbeCase:
    name: str
    initial_soc: float
    command_modes: tuple[str, ...]
    expected_mode_set_results: tuple[str, ...]
    expected_final_mode: str
    expected_event_fragments: tuple[str, ...]
    forbidden_event_fragments: tuple[str, ...] = ()
    stop_eps_before_commands: bool = False
    require_final_mode_telemetry: bool = True


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


def stop_process(process: subprocess.Popen[str]) -> None:
    for managed in processes:
        if managed.process is process:
            stop_managed_process(managed, timeout_sec=5.0)
            return


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
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or ""
        if isinstance(output, bytes):
            output = output.decode("utf-8", errors="replace")
    capture_text = read_text(capture_dir / "channel.log")
    with channels_log.open("a", encoding="utf-8") as handle:
        handle.write(f"$ {' '.join(full)} # {label}\n")
        handle.write(output)
        if output and not output.endswith("\n"):
            handle.write("\n")
        handle.write(capture_text)
        if capture_text and not capture_text.endswith("\n"):
            handle.write("\n")
    observed_modes = extract_sys_mode_values(output + "\n" + capture_text)
    return observed_modes


def wait_for_sys_mode_snapshot(label: str, expected_mode: str, attempts: int = 6) -> None:
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


def count_matching_lines(text: str, pattern: re.Pattern[str]) -> int:
    return sum(1 for line in text.splitlines() if pattern.search(line))


def wait_line_count(path: pathlib.Path, pattern: re.Pattern[str], expected_count: int, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if count_matching_lines(read_text(path), pattern) >= expected_count:
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {expected_count} matches of {pattern.pattern!r} in {path}")


def run_cli_until_count_increases(
    command: str,
    args: tuple[str, ...],
    response_path: pathlib.Path,
    response_pattern: re.Pattern[str],
    baseline_count: int,
    attempts: int = 3,
    timeout: float = 20.0,
) -> int:
    target_count = baseline_count + 1
    for _attempt in range(1, attempts + 1):
        run_cli(command, *args)
        time.sleep(1.0)
        try:
            wait_line_count(response_path, response_pattern, target_count, timeout)
            return count_matching_lines(read_text(response_path), response_pattern)
        except RuntimeError:
            continue
    raise RuntimeError(
        f"expected {command} to increase {response_pattern.pattern!r} count beyond {baseline_count} "
        f"after {attempts} attempts"
    )


bin_dir = require_env("BIN_DIR")
dict_path = require_env("DICT_PATH")
fprime_cli = require_env("FPRIME_CLI_BIN")
fprime_gds = require_env("FPRIME_GDS_BIN")
probe_tmp = pathlib.Path(require_env("PROBE_TMP_DIR"))

cases = (
    ProbeCase(
        "safe_to_idle_accept",
        60.0,
        ("IDLE",),
        ("OK",),
        "IDLE",
        ("System mode changed to IDLE",),
    ),
    ProbeCase(
        "safe_to_idle_reject_boundary",
        50.0,
        ("IDLE",),
        ("VALIDATION_ERROR",),
        "SAFE",
        ("System mode transition rejected from SAFE to IDLE reason 4",),
        ("System mode changed to IDLE",),
        require_final_mode_telemetry=False,
    ),
    ProbeCase(
        "safe_to_idle_reject_unavailable",
        60.0,
        ("IDLE",),
        ("VALIDATION_ERROR",),
        "SAFE",
        ("System mode transition rejected from SAFE to IDLE reason 3",),
        ("System mode changed to IDLE",),
        stop_eps_before_commands=True,
        require_final_mode_telemetry=False,
    ),
    ProbeCase(
        "idle_to_payload_accept",
        71.0,
        ("IDLE", "PAYLOAD"),
        ("OK", "OK"),
        "PAYLOAD",
        ("System mode changed to PAYLOAD",),
    ),
    ProbeCase(
        "idle_to_payload_reject_boundary",
        70.0,
        ("IDLE", "PAYLOAD"),
        ("OK", "VALIDATION_ERROR"),
        "IDLE",
        ("System mode transition rejected from IDLE to PAYLOAD reason 4",),
        ("System mode changed to PAYLOAD",),
        require_final_mode_telemetry=False,
    ),
    ProbeCase(
        "idle_to_ttc_unchanged",
        60.0,
        ("IDLE", "TTC"),
        ("OK", "OK"),
        "TTC",
        ("System mode changed to TTC",),
    ),
)

summary_lines: list[str] = []
mode_set_success_pattern = re.compile(r"CdhCore\.cmdDisp\.OpCodeCompleted .* Opcode 0x10030000 completed")
mode_set_validation_error_pattern = re.compile(
    r"CdhCore\.cmdDisp\.OpCodeError .* Opcode 0x10030000 completed with error VALIDATION_ERROR"
)
mode_get_success_pattern = re.compile(r"CdhCore\.cmdDisp\.OpCodeCompleted .* Opcode 0x10030001 completed")

for case in cases:
    runtime_root = probe_tmp / case.name / "runtime"
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

    case_dir = probe_tmp / case.name
    case_dir.mkdir(parents=True, exist_ok=True)
    command_log = case_dir / "command-send.log"
    events_log = case_dir / "events.log"
    channels_log = case_dir / "channels.log"
    processes: list[ManagedProcess] = []

    def cleanup_case_processes() -> None:
        cleanup_managed_processes(processes, timeout_sec=5.0)

    install_signal_cleanup(cleanup_case_processes)

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
                str(case_dir / "gds-logs"),
            ],
            case_dir / "gds.log",
            stale_match_groups=build_gds_stale_match_groups(ip_port=gds_port, tts_port=gds_tts_port),
        )
        wait_port(gds_port, 20.0)
        wait_port(gds_tts_port, 20.0)

        start("csp_zmqproxy", [f"{bin_dir}/csp_zmqproxy", "-s", f"tcp://0.0.0.0:{csp_sub_port}", "-p", f"tcp://0.0.0.0:{csp_pub_port}"], case_dir / "csp_zmqproxy.log", env)
        eps_process = start("eps_simulator", [f"{bin_dir}/eps_simulator", "--node-id", "2", "--initial-soc", f"{case.initial_soc:.2f}"], case_dir / "eps_simulator.log", env)
        start("adcs_simulator", [f"{bin_dir}/adcs_simulator", "--node-id", "3"], case_dir / "adcs_simulator.log", env)
        start("radio_mock_server", [f"{bin_dir}/radio_mock_server", "--port", str(radio_port)], case_dir / "radio_mock_server.log", env)
        start(
            "sband_comm_csp_node",
            [f"{bin_dir}/sband_comm_csp_node", "--tcp-listen-host", "127.0.0.1", "--tcp-listen-port", str(sband_port), "--node-id", "5"],
            case_dir / "sband_comm_csp_node.log",
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
            case_dir / "ground_ttc_gateway.log",
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
        start("OBC", obc_args, case_dir / "obc.log", env)
        wait_text(case_dir / "obc.log", "Runtime mode: headless", 20.0)

        wait_for_sys_mode_snapshot("initial boot", "SAFE", attempts=3)

        command = "OBCApp.modeManager.MODE_SET"
        mode_get_command = "OBCApp.modeManager.MODE_GET"
        start(
            "fprime-cli-events",
            [fprime_cli, "events", "--dictionary", dict_path, "--no-zmq", "--tts-port", str(gds_tts_port)],
            events_log,
        )
        time.sleep(1.0)

        warmup_mode_get_successes = run_cli_until_count_increases(
            mode_get_command,
            (),
            events_log,
            mode_get_success_pattern,
            0,
        )

        if case.stop_eps_before_commands:
            stop_process(eps_process)
            time.sleep(3.0)

        observed_mode_set_successes = 0
        observed_mode_set_validation_errors = 0
        for mode, expected_result in zip(case.command_modes, case.expected_mode_set_results):
            if expected_result == "OK":
                observed_mode_set_successes = run_cli_until_count_increases(
                    command,
                    ("--arguments", mode),
                    events_log,
                    mode_set_success_pattern,
                    observed_mode_set_successes,
                )
            elif expected_result == "VALIDATION_ERROR":
                observed_mode_set_validation_errors = run_cli_until_count_increases(
                    command,
                    ("--arguments", mode),
                    events_log,
                    mode_set_validation_error_pattern,
                    observed_mode_set_validation_errors,
                )
            else:
                raise RuntimeError(f"{case.name}: unsupported expected MODE_SET result {expected_result!r}")

        observed_mode_get_successes = run_cli_until_count_increases(
            mode_get_command,
            (),
            events_log,
            mode_get_success_pattern,
            warmup_mode_get_successes,
        )

        if case.require_final_mode_telemetry:
            wait_for_sys_mode_snapshot(f"final mode for {case.name}", case.expected_final_mode, attempts=6)
        expected_mode_set_successes = sum(1 for result in case.expected_mode_set_results if result == "OK")
        expected_mode_set_validation_errors = sum(
            1 for result in case.expected_mode_set_results if result == "VALIDATION_ERROR"
        )
        wait_line_count(events_log, mode_get_success_pattern, observed_mode_get_successes, 20.0)

        event_text = read_text(events_log)
        command_text = read_text(command_log)
        if "returncode=0" not in command_text:
            raise RuntimeError(f"{case.name}: expected successful fprime-cli command-send invocations")
        observed_mode_set_successes = count_matching_lines(event_text, mode_set_success_pattern)
        observed_mode_set_validation_errors = count_matching_lines(event_text, mode_set_validation_error_pattern)
        if observed_mode_set_successes != expected_mode_set_successes:
            raise RuntimeError(
                f"{case.name}: expected {expected_mode_set_successes} MODE_SET success responses, "
                f"observed {observed_mode_set_successes}"
            )
        if observed_mode_set_validation_errors != expected_mode_set_validation_errors:
            raise RuntimeError(
                f"{case.name}: expected {expected_mode_set_validation_errors} MODE_SET VALIDATION_ERROR responses, "
                f"observed {observed_mode_set_validation_errors}"
            )
        observed_mode_get_successes = count_matching_lines(event_text, mode_get_success_pattern)
        expected_mode_get_successes = warmup_mode_get_successes + 1
        if observed_mode_get_successes != expected_mode_get_successes:
            raise RuntimeError(
                f"{case.name}: expected {expected_mode_get_successes} MODE_GET success responses, "
                f"observed {observed_mode_get_successes}"
            )
        for fragment in case.expected_event_fragments:
            if fragment not in event_text:
                raise RuntimeError(f"{case.name}: missing expected event fragment {fragment!r}")
        for fragment in case.forbidden_event_fragments:
            if fragment in event_text:
                raise RuntimeError(f"{case.name}: observed forbidden event fragment {fragment!r}")

        summary_lines.append(
            f"{case.name}: initial_soc={case.initial_soc:.2f} final={case.expected_final_mode} "
            f"mode_set_ok={observed_mode_set_successes} mode_set_validation_error={observed_mode_set_validation_errors} "
            f"events={events_log} channels={channels_log}"
        )
    finally:
        cleanup_case_processes()

summary = probe_tmp / "summary.log"
summary.write_text("\n".join(["mode_soc_admission_and_exit_ccsds_probe: PASS", *summary_lines]) + "\n", encoding="utf-8")
print(summary.read_text(encoding="utf-8"), end="")
PY
