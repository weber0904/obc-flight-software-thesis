#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

for executable in csp_zmqproxy pty_pair_bridge comm_csp_node sband_comm_csp_node uhf_comm_csp_node comm_service_probe OBC; do
  if [[ ! -x "${BIN_DIR}/${executable}" ]]; then
    echo "${executable} not found at ${BIN_DIR}/${executable}. Run fprime-util build first." >&2
    exit 1
  fi
done

port_is_available() {
  local port="${1:?port is required}"
  ! lsof -nP -iTCP:"${port}" >/dev/null 2>&1
}

find_free_port_pair() {
  local first_port="${1:?first port is required}"
  local second_offset="${2:?offset is required}"
  local candidate="${first_port}"
  while ! port_is_available "${candidate}" || ! port_is_available "$((candidate + second_offset))"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

find_free_port() {
  local candidate="${1:?starting port is required}"
  while ! port_is_available "${candidate}"; do
    candidate=$((candidate + 1))
  done
  printf '%s\n' "${candidate}"
}

CSP_HUB_HOST="${CSP_HUB_HOST:-127.0.0.1}"
CSP_PROXY_BIND_HOST="${CSP_PROXY_BIND_HOST:-0.0.0.0}"
if [[ -z "${CSP_HUB_SUB_PORT:-}" ]]; then
  CSP_HUB_SUB_PORT="$(find_free_port_pair 56580 1000)"
fi
if [[ -z "${CSP_HUB_PUB_PORT:-}" ]]; then
  CSP_HUB_PUB_PORT="$((CSP_HUB_SUB_PORT + 1000))"
fi
if [[ -z "${RADIO_PORT:-}" ]]; then
  RADIO_PORT="$(find_free_port 17180)"
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-dual-link-sim.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${PROBE_TMP_DIR}/runtime}"
mkdir -p "${PROBE_TMP_DIR}" "${RUNTIME_ROOT}"

python3 - \
  "${ROOT_DIR}" \
  "${BIN_DIR}" \
  "${PROBE_TMP_DIR}" \
  "${RUNTIME_ROOT}" \
  "${CSP_HUB_HOST}" \
  "${CSP_PROXY_BIND_HOST}" \
  "${CSP_HUB_SUB_PORT}" \
  "${CSP_HUB_PUB_PORT}" \
  "${RADIO_PORT}" <<'PY'
import os
import subprocess
import sys
import time

(
    root_dir,
    bin_dir,
    probe_tmp_dir,
    runtime_root,
    csp_hub_host,
    csp_proxy_bind_host,
    csp_hub_sub_port,
    csp_hub_pub_port,
    radio_port,
) = sys.argv[1:10]


def log_path(name):
    return os.path.join(probe_tmp_dir, name)


def start_process(args, *, env=None, handle=None, stdin=None, stdout=None):
    return subprocess.Popen(
        args,
        env=env,
        stdin=stdin if stdin is not None else subprocess.DEVNULL,
        stdout=stdout if stdout is not None else handle,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )


def require_running(process, name):
    if process.poll() is not None:
        raise RuntimeError(f"{name} exited early with code {process.returncode}")


def wait_for_log_fragment(path, fragment, timeout_sec):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        if os.path.exists(path):
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                if fragment in handle.read():
                    return
        time.sleep(0.2)
    raise RuntimeError(f"Timed out waiting for {fragment!r} in {path}")


def wait_for_log_count(path, fragment, expected_count, timeout_sec):
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        count = 0
        if os.path.exists(path):
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                count = handle.read().count(fragment)
        if count >= expected_count:
            return
        time.sleep(0.2)
    raise RuntimeError(f"Timed out waiting for {expected_count} occurrences of {fragment!r} in {path}")


def send_obc_command(obc_process, command_text, delay_sec=0.5):
    assert obc_process.stdin is not None
    obc_process.stdin.write(command_text + "\n")
    obc_process.stdin.flush()
    time.sleep(delay_sec)


def start_pty_bridge(label, processes):
    path = log_path(f"{label}-pty-bridge.log")
    handle = open(path, "w", encoding="utf-8", buffering=1)
    process = start_process(
        [os.path.join(bin_dir, "pty_pair_bridge")],
        handle=handle,
        stdout=subprocess.PIPE,
    )
    processes.append((f"{label}_pty_pair_bridge", process, handle))

    serial_paths = {}
    for _ in range(2):
        line = process.stdout.readline()
        if not line:
            raise RuntimeError(f"{label} pty_pair_bridge did not report PTY paths")
        handle.write(line)
        key, value = line.strip().split("=", 1)
        serial_paths[key] = value

    return serial_paths["PTY_A"], serial_paths["PTY_B"]


common_env = os.environ.copy()
common_env["CSP_TRANSPORT"] = "zmqhub"
common_env["CSP_HUB_HOST"] = csp_hub_host
common_env["CSP_HUB_SUB_PORT"] = csp_hub_sub_port
common_env["CSP_HUB_PUB_PORT"] = csp_hub_pub_port

links = [
    {
        "label": "generic",
        "executable": "comm_csp_node",
        "target_node": "4",
        "probe_node": "7",
        "startup": "executable=comm_csp_node link=generic node=4",
    },
    {
        "label": "sband",
        "executable": "sband_comm_csp_node",
        "target_node": "5",
        "probe_node": "8",
        "startup": "executable=sband_comm_csp_node link=sband node=5",
    },
    {
        "label": "uhf",
        "executable": "uhf_comm_csp_node",
        "target_node": "6",
        "probe_node": "9",
        "startup": "executable=uhf_comm_csp_node link=uhf node=6",
    },
]

processes = []
try:
    hub_handle = open(log_path("csp-zmqproxy.log"), "w", encoding="utf-8", buffering=1)
    hub = start_process(
        [
            os.path.join(bin_dir, "csp_zmqproxy"),
            "-s",
            f"tcp://{csp_proxy_bind_host}:{csp_hub_sub_port}",
            "-p",
            f"tcp://{csp_proxy_bind_host}:{csp_hub_pub_port}",
        ],
        handle=hub_handle,
    )
    processes.append(("csp_zmqproxy", hub, hub_handle))

    for link in links:
        peer_serial, node_serial = start_pty_bridge(link["label"], processes)
        link["peer_serial"] = peer_serial
        link["node_serial"] = node_serial

        node_handle = open(log_path(f"{link['label']}-comm-node.log"), "w", encoding="utf-8", buffering=1)
        node_process = start_process(
            [
                os.path.join(bin_dir, link["executable"]),
                "--serial-device",
                node_serial,
            ],
            env=common_env,
            handle=node_handle,
        )
        processes.append((f"{link['label']}_comm_node", node_process, node_handle))

    obc_log = log_path("obc.log")
    obc_handle = open(obc_log, "w", encoding="utf-8", buffering=1)
    obc_env = common_env.copy()
    obc_env["CSP_MANAGE_PROXY"] = "0"
    obc_env["RUNTIME_ROOT"] = runtime_root
    obc_env["GDS_PORT"] = "0"
    obc_env["GROUND_LINK_MODE"] = "disabled"
    obc_env["RADIO_PORT"] = radio_port
    obc = start_process(
        ["bash", os.path.join(root_dir, "scripts/run_dev_stack.sh")],
        env=obc_env,
        handle=obc_handle,
        stdin=subprocess.PIPE,
    )
    processes.append(("run_dev_stack", obc, obc_handle))

    time.sleep(4.0)
    for name, process, _ in processes:
        require_running(process, name)

    wait_for_log_fragment(obc_log, "OBC runtime started", 12.0)
    for link in links:
        wait_for_log_fragment(log_path(f"{link['label']}-comm-node.log"), link["startup"], 5.0)

    for link in links:
        send_obc_command(obc, f"csp ping {link['target_node']}", 0.7)
    wait_for_log_count(obc_log, "csp ping response=0 success=yes", 3, 8.0)

    helper_log = log_path("comm-service-probe.log")
    with open(helper_log, "w", encoding="utf-8", buffering=1) as helper_handle:
        for link in links:
            command = [
                os.path.join(bin_dir, "comm_service_probe"),
                "--link",
                link["label"],
                "--target-node",
                link["target_node"],
                "--local-node",
                link["probe_node"],
                "--serial-peer",
                link["peer_serial"],
                "--timeout-ms",
                "5000",
                "--interface-name",
                f"{link['label'].upper()}PRB",
            ]
            result = subprocess.run(
                command,
                env=common_env,
                stdout=helper_handle,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                raise RuntimeError(f"comm_service_probe failed for {link['label']} node {link['target_node']}")

    wait_for_log_count(helper_log, "comm_service_probe: PASS", 3, 1.0)
    send_obc_command(obc, "quit", 0.0)
    obc.wait(timeout=20.0)
finally:
    for _, process, _ in reversed(processes):
        if process.poll() is None:
            process.terminate()
    for _, process, _ in reversed(processes):
        if process.poll() is None:
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5.0)
    for _, _, handle in processes:
        try:
            handle.close()
        except Exception:
            pass
PY

echo "comm-dual-link-sim-foundation-probe: PASS"
echo "logs: ${PROBE_TMP_DIR}"
grep -hE 'COMM node startup:' "${PROBE_TMP_DIR}/generic-comm-node.log" "${PROBE_TMP_DIR}/sband-comm-node.log" "${PROBE_TMP_DIR}/uhf-comm-node.log"
grep -E 'csp ping response=0 success=yes' "${PROBE_TMP_DIR}/obc.log"
grep -E 'comm_service_probe: PASS' "${PROBE_TMP_DIR}/comm-service-probe.log"
