#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

SUBSYSTEM_SIM_SSH_TARGET="${SUBSYSTEM_SIM_SSH_TARGET:-$(obc_resolve_subsystem_sim_ssh_target)}"
HOST_SERIAL_DEVICE="${HOST_SERIAL_DEVICE:-${SERIAL_DEVICE:-}}"
SUBSYSTEM_SIM_COMM_DEVICE="${SUBSYSTEM_SIM_COMM_DEVICE:-${COMM_DEVICE:-}}"
COMM_BAUDRATE="${COMM_BAUDRATE:-115200}"
ACQ_FRAME_COUNT="${ACQ_FRAME_COUNT:-8}"
ACQ_PREAMBLE_LINES="${ACQ_PREAMBLE_LINES:-20}"
ACQ_FRAME_SPACING_MS="${ACQ_FRAME_SPACING_MS:-500}"
ACQ_HOST_STARTUP_DELAY="${ACQ_HOST_STARTUP_DELAY:-1}"
ACQ_HOST_TIMEOUT_SEC="${ACQ_HOST_TIMEOUT_SEC:-20}"
ACQ_NONCE="${ACQ_NONCE:-$(date +%s)-$$}"
RUN_SUBSYSTEM_SENDER_WITH_SUDO="${RUN_SUBSYSTEM_SENDER_WITH_SUDO:-0}"
PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/obc-comm-lab-acq.XXXXXX")}"
HOST_LOG="${HOST_LOG:-${PROBE_TMP_DIR}/host-receiver.log}"
SUBSYSTEM_LOG="${SUBSYSTEM_LOG:-${PROBE_TMP_DIR}/subsystem-sender.log}"

usage() {
  cat >&2 <<EOF
Usage:
  HOST_SERIAL_DEVICE=/dev/cu.<adapter> SUBSYSTEM_SIM_COMM_DEVICE=/dev/<tty> bash scripts/run_comm_lab_serial_acquisition_probe.sh

Environment:
  HOST_SERIAL_DEVICE             macOS serial endpoint connected to subsystem.local
  SUBSYSTEM_SIM_COMM_DEVICE      subsystem.local serial endpoint connected to macOS
  COMM_BAUDRATE                  serial baudrate, default 115200
  ACQ_FRAME_COUNT                acquisition frames to send, default 8
  ACQ_PREAMBLE_LINES             ASCII preamble lines before frames, default 20
  ACQ_FRAME_SPACING_MS           delay between frames, default 500
  ACQ_HOST_TIMEOUT_SEC           macOS receiver timeout, default 20
  RUN_SUBSYSTEM_SENDER_WITH_SUDO run subsystem sender through sudo -n when set to 1
EOF
}

require_bool() {
  local name="$1"
  local value="$2"
  case "${value}" in
    0|1) ;;
    *)
      echo "${name} must be 0 or 1." >&2
      exit 2
      ;;
  esac
}

require_positive_integer() {
  local name="$1"
  local value="$2"
  case "${value}" in
    ''|*[!0-9]*|0)
      echo "${name} must be a positive integer." >&2
      exit 2
      ;;
  esac
}

if [[ -z "${HOST_SERIAL_DEVICE}" || -z "${SUBSYSTEM_SIM_COMM_DEVICE}" ]]; then
  usage
  exit 2
fi
require_bool RUN_SUBSYSTEM_SENDER_WITH_SUDO "${RUN_SUBSYSTEM_SENDER_WITH_SUDO}"
require_positive_integer ACQ_FRAME_COUNT "${ACQ_FRAME_COUNT}"

if [[ ! -e "${HOST_SERIAL_DEVICE}" ]]; then
  echo "Host serial device not found: ${HOST_SERIAL_DEVICE}" >&2
  exit 1
fi

mkdir -p "${PROBE_TMP_DIR}"

HOST_RECEIVER_PID=""
cleanup() {
  local status=$?
  if [[ -n "${HOST_RECEIVER_PID}" ]]; then
    kill "${HOST_RECEIVER_PID}" >/dev/null 2>&1 || true
    wait "${HOST_RECEIVER_PID}" >/dev/null 2>&1 || true
  fi
  if [[ ${status} -ne 0 ]]; then
    echo "COMM lab serial acquisition probe failed; logs are in ${PROBE_TMP_DIR}" >&2
    echo "  host receiver    : ${HOST_LOG}" >&2
    echo "  subsystem sender : ${SUBSYSTEM_LOG}" >&2
  fi
  exit "${status}"
}
trap cleanup EXIT INT TERM

python3 - \
  "${HOST_SERIAL_DEVICE}" \
  "${COMM_BAUDRATE}" \
  "${ACQ_FRAME_COUNT}" \
  "${ACQ_HOST_TIMEOUT_SEC}" \
  "${ACQ_NONCE}" >"${HOST_LOG}" 2>&1 <<'PY' &
import os
import fcntl
import select
import struct
import sys
import termios
import time
import zlib

device, baudrate, frame_count, timeout_sec, nonce = sys.argv[1:6]
frame_count = int(frame_count)
timeout_sec = float(timeout_sec)
magic = "OBCACQ1"


def baud_constant(rate):
    name = f"B{rate}"
    if not hasattr(termios, name):
        raise RuntimeError(f"unsupported baudrate for termios: {rate}")
    return getattr(termios, name)


def configure_raw(fd, rate):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    if hasattr(termios, "CRTSCTS"):
        attrs[2] &= ~termios.CRTSCTS
    attrs[3] = 0
    attrs[4] = baud_constant(rate)
    attrs[5] = baud_constant(rate)
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def assert_modem_lines(fd):
    bits = 0
    if hasattr(termios, "TIOCM_DTR"):
        bits |= termios.TIOCM_DTR
    if hasattr(termios, "TIOCM_RTS"):
        bits |= termios.TIOCM_RTS
    if bits and hasattr(termios, "TIOCMBIS"):
        fcntl.ioctl(fd, termios.TIOCMBIS, struct.pack("I", bits))


def parse_line(raw_line, decoded):
    text = raw_line.decode("ascii", errors="ignore")
    start = text.find(magic)
    if start < 0:
        return
    fields = text[start:].strip().split()
    if len(fields) != 5 or fields[0] != magic:
        return
    try:
        seq = int(fields[1], 10)
        payload_len = int(fields[2], 10)
        payload = bytes.fromhex(fields[3])
        observed_crc = int(fields[4], 16)
    except ValueError:
        return
    if len(payload) != payload_len:
        return
    expected_crc = zlib.crc32(f"{seq}:".encode("ascii") + payload) & 0xFFFFFFFF
    if observed_crc == expected_crc:
        decoded[seq] = payload


fd = os.open(device, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
raw = bytearray()
decoded = {}
try:
    configure_raw(fd, baudrate)
    assert_modem_lines(fd)
    deadline = time.time() + timeout_sec
    buffer = bytearray()
    while time.time() < deadline and len(decoded) < frame_count:
        readable, _, _ = select.select([fd], [], [], 0.1)
        if not readable:
            continue
        chunk = os.read(fd, 4096)
        if not chunk:
            continue
        raw.extend(chunk)
        buffer.extend(chunk)
        while b"\n" in buffer:
            line, _, remainder = buffer.partition(b"\n")
            parse_line(bytes(line), decoded)
            buffer = bytearray(remainder)
finally:
    os.close(fd)

expected = {i: f"acq-{nonce}-{i}".encode("ascii") for i in range(frame_count)}
missing = [i for i, payload in expected.items() if decoded.get(i) != payload]
print(f"host-serial-device={device}")
print(f"baudrate={baudrate}")
print(f"nonce={nonce}")
print(f"raw-bytes={len(raw)}")
print(f"decoded-frames={len(decoded)}")
print(f"expected-frames={frame_count}")
print(f"raw-prefix-hex={bytes(raw[:96]).hex()}")
for seq in sorted(decoded):
    print(f"decoded-frame seq={seq} payload={decoded[seq].decode('ascii', errors='replace')}")
if missing:
    print(f"missing-or-mismatched={missing}", file=sys.stderr)
    raise SystemExit(1)
print("comm-lab-serial-acquisition-receiver: PASS")
PY
HOST_RECEIVER_PID=$!

sleep "${ACQ_HOST_STARTUP_DELAY}"
if ! kill -0 "${HOST_RECEIVER_PID}" >/dev/null 2>&1; then
  echo "Host acquisition receiver failed to start." >&2
  cat "${HOST_LOG}" >&2 || true
  exit 1
fi

if ! obc_ssh "${SUBSYSTEM_SIM_SSH_TARGET}" /bin/bash >"${SUBSYSTEM_LOG}" 2>&1 <<EOF
set -euo pipefail
SUBSYSTEM_SERIAL_DEVICE=$(printf '%q' "${SUBSYSTEM_SIM_COMM_DEVICE}")
RUN_WITH_SUDO=$(printf '%q' "${RUN_SUBSYSTEM_SENDER_WITH_SUDO}")
if [[ ! -e "\${SUBSYSTEM_SERIAL_DEVICE}" ]]; then
  echo "Subsystem serial device not found: \${SUBSYSTEM_SERIAL_DEVICE}" >&2
  exit 1
fi
SERIAL_REALPATH="\$(readlink -f "\${SUBSYSTEM_SERIAL_DEVICE}" || true)"
if [[ "\${RUN_WITH_SUDO}" != "1" ]] && { [[ ! -r "\${SUBSYSTEM_SERIAL_DEVICE}" ]] || [[ ! -w "\${SUBSYSTEM_SERIAL_DEVICE}" ]]; }; then
  echo "Subsystem serial device is not readable/writable by \$(id -un); set RUN_SUBSYSTEM_SENDER_WITH_SUDO=1 or fix device permissions." >&2
  ls -l "\${SUBSYSTEM_SERIAL_DEVICE}" "\${SERIAL_REALPATH:-\${SUBSYSTEM_SERIAL_DEVICE}}" 2>&1 || true
  exit 1
fi
RUNNER=()
if [[ "\${RUN_WITH_SUDO}" == "1" ]]; then
  RUNNER=(sudo -n)
fi
"\${RUNNER[@]}" python3 - \
  "\${SUBSYSTEM_SERIAL_DEVICE}" \
  $(printf '%q' "${COMM_BAUDRATE}") \
  $(printf '%q' "${ACQ_FRAME_COUNT}") \
  $(printf '%q' "${ACQ_PREAMBLE_LINES}") \
  $(printf '%q' "${ACQ_FRAME_SPACING_MS}") \
  $(printf '%q' "${ACQ_NONCE}") <<'PY'
import os
import fcntl
import struct
import sys
import termios
import time
import zlib

device, baudrate, frame_count, preamble_lines, spacing_ms, nonce = sys.argv[1:7]
frame_count = int(frame_count)
preamble_lines = int(preamble_lines)
spacing_sec = int(spacing_ms) / 1000.0
magic = "OBCACQ1"


def baud_constant(rate):
    name = f"B{rate}"
    if not hasattr(termios, name):
        raise RuntimeError(f"unsupported baudrate for termios: {rate}")
    return getattr(termios, name)


def configure_raw(fd, rate):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    if hasattr(termios, "CRTSCTS"):
        attrs[2] &= ~termios.CRTSCTS
    attrs[3] = 0
    attrs[4] = baud_constant(rate)
    attrs[5] = baud_constant(rate)
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def assert_modem_lines(fd):
    bits = 0
    if hasattr(termios, "TIOCM_DTR"):
        bits |= termios.TIOCM_DTR
    if hasattr(termios, "TIOCM_RTS"):
        bits |= termios.TIOCM_RTS
    if bits and hasattr(termios, "TIOCMBIS"):
        fcntl.ioctl(fd, termios.TIOCMBIS, struct.pack("I", bits))


def make_frame(seq, payload):
    crc = zlib.crc32(f"{seq}:".encode("ascii") + payload) & 0xFFFFFFFF
    return f"{magic} {seq} {len(payload)} {payload.hex()} {crc:08x}\n".encode("ascii")


fd = os.open(device, os.O_RDWR | os.O_NOCTTY)
try:
    configure_raw(fd, baudrate)
    assert_modem_lines(fd)
    for index in range(preamble_lines):
        os.write(fd, f"OBCACQ-PREAMBLE {nonce} {index}\n".encode("ascii"))
        termios.tcdrain(fd)
        time.sleep(spacing_sec)
    for seq in range(frame_count):
        payload = f"acq-{nonce}-{seq}".encode("ascii")
        frame = make_frame(seq, payload)
        os.write(fd, frame)
        termios.tcdrain(fd)
        time.sleep(spacing_sec)
finally:
    os.close(fd)

print(f"subsystem-serial-device={device}")
print(f"baudrate={baudrate}")
print(f"nonce={nonce}")
print(f"preamble-lines={preamble_lines}")
print(f"sent-frames={frame_count}")
print("comm-lab-serial-acquisition-sender: PASS")
PY
EOF
then
  echo "Subsystem acquisition sender failed." >&2
  cat "${SUBSYSTEM_LOG}" >&2 || true
  exit 1
fi

if ! wait "${HOST_RECEIVER_PID}"; then
  HOST_RECEIVER_PID=""
  echo "Host acquisition receiver did not decode the expected frame set." >&2
  cat "${HOST_LOG}" >&2 || true
  cat "${SUBSYSTEM_LOG}" >&2 || true
  exit 1
fi
HOST_RECEIVER_PID=""

echo "comm-lab-serial-acquisition-probe: PASS"
echo "host-serial-device=${HOST_SERIAL_DEVICE}"
echo "subsystem-serial-device=${SUBSYSTEM_SIM_COMM_DEVICE}"
echo "baudrate=${COMM_BAUDRATE}"
echo "frame-count=${ACQ_FRAME_COUNT}"
echo "preamble-lines=${ACQ_PREAMBLE_LINES}"
echo "log-dir=${PROBE_TMP_DIR}"
echo "=== Host Receiver Log ==="
cat "${HOST_LOG}"
echo "=== Subsystem Sender Log ==="
cat "${SUBSYSTEM_LOG}"
