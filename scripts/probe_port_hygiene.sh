#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: probe_port_hygiene.sh [--reap-known] <tcp-port> [<tcp-port> ...]

Checks whether bounded probe listener ports are already occupied.

Modes:
  default       fail if any listener is present
  --reap-known  terminate known repository-owned probe listeners on those ports,
                then fail only if an unknown listener remains
EOF
}

REAP_KNOWN=0
if [[ "${1:-}" == "--reap-known" ]]; then
  REAP_KNOWN=1
  shift
fi

if [[ $# -lt 1 ]]; then
  usage >&2
  exit 1
fi

if ! command -v lsof >/dev/null 2>&1; then
  echo "lsof is required for probe_port_hygiene.sh" >&2
  exit 1
fi

known_probe_command() {
  local command_line="${1:-}"
  [[ "${command_line}" =~ fprime-gds([[:space:]]|$)|fprime_gds\.executables\.comm|fprime_gds\.executables\.tcpserver|CustomDataHandlers|ground_ttc_gateway|fprime-cli\ events|fprime-cli\ channels|run_rpi_target_hardware_watchdog_reset_probe|run_rpi_target_recovery_restart_probe ]]
}

ps_command_for_pid() {
  local pid="${1:?pid is required}"
  ps -p "${pid}" -o command= 2>/dev/null || true
}

declare -a known_pids=()
declare -a unknown_records=()
SEEN_PIDS=""

for port in "$@"; do
  if ! [[ "${port}" =~ ^[1-9][0-9]*$ ]]; then
    echo "invalid TCP port: ${port}" >&2
    exit 1
  fi
  while IFS= read -r pid; do
    [[ -z "${pid}" ]] && continue
    if [[ " ${SEEN_PIDS} " == *" ${pid} "* ]]; then
      continue
    fi
    SEEN_PIDS="${SEEN_PIDS} ${pid}"
    cmd="$(ps_command_for_pid "${pid}")"
    if [[ -z "${cmd}" && "${REAP_KNOWN}" == "1" ]]; then
      known_pids+=("${pid}")
    elif known_probe_command "${cmd}"; then
      known_pids+=("${pid}")
    else
      unknown_records+=("port=${port} pid=${pid} cmd=${cmd}")
    fi
  done < <(lsof -nP -tiTCP:"${port}" -sTCP:LISTEN 2>/dev/null || true)
done

known_pid_count=${#known_pids[@]}
if (( REAP_KNOWN )) && (( known_pid_count > 0 )); then
  kill "${known_pids[@]}" >/dev/null 2>&1 || true
  sleep 1
  kill -9 "${known_pids[@]}" >/dev/null 2>&1 || true
fi

declare -a residual=()
for port in "$@"; do
  while IFS= read -r pid; do
    [[ -z "${pid}" ]] && continue
    cmd="$(ps_command_for_pid "${pid}")"
    residual+=("port=${port} pid=${pid} cmd=${cmd}")
  done < <(lsof -nP -tiTCP:"${port}" -sTCP:LISTEN 2>/dev/null || true)
done

unknown_record_count=${#unknown_records[@]}
residual_count=${#residual[@]}
if (( unknown_record_count > 0 )) || (( residual_count > 0 )); then
  echo "probe port hygiene failed" >&2
  if (( unknown_record_count > 0 )); then
    for row in "${unknown_records[@]}"; do
      echo "pre-existing unknown listener: ${row}" >&2
    done
  fi
  if (( residual_count > 0 )); then
    for row in "${residual[@]}"; do
      echo "residual listener: ${row}" >&2
    done
  fi
  exit 1
fi

echo "probe port hygiene PASS: $*"
