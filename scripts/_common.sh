#!/usr/bin/env bash

obc_root_dir() {
  local source_path="${BASH_SOURCE[0]}"
  local script_dir
  script_dir="$(cd "$(dirname "${source_path}")" && pwd)"
  cd "${script_dir}/.." && pwd
}

obc_default_obc_ssh_target() {
  printf '%s\n' "operator@obc.local"
}

obc_default_subsystem_sim_ssh_target() {
  printf '%s\n' "operator@subsystem.local"
}

obc_default_remote_workspace_dir() {
  printf '%s\n' "/home/operator/lab/fprime/v0"
}

obc_resolve_obc_ssh_target() {
  if [[ -n "${OBC_SSH_TARGET:-}" ]]; then
    printf '%s\n' "${OBC_SSH_TARGET}"
    return 0
  fi

  if [[ -n "${RPI_SSH_TARGET:-}" ]]; then
    printf '%s\n' "${RPI_SSH_TARGET}"
    return 0
  fi

  obc_default_obc_ssh_target
}

obc_resolve_subsystem_sim_ssh_target() {
  if [[ -n "${SUBSYSTEM_SIM_SSH_TARGET:-}" ]]; then
    printf '%s\n' "${SUBSYSTEM_SIM_SSH_TARGET}"
    return 0
  fi

  obc_default_subsystem_sim_ssh_target
}

obc_resolve_subsystem_sim_host() {
  local ssh_target
  ssh_target="$(obc_resolve_subsystem_sim_ssh_target)"
  printf '%s\n' "${ssh_target#*@}"
}

obc_resolve_subsystem_sim_remote_dir() {
  if [[ -n "${SUBSYSTEM_SIM_REMOTE_DIR:-}" ]]; then
    printf '%s\n' "${SUBSYSTEM_SIM_REMOTE_DIR}"
    return 0
  fi

  obc_default_remote_workspace_dir
}

obc_require_distinct_remote_host_roles() {
  local obc_target="${1:?obc_target is required}"
  local subsystem_target="${2:?subsystem_target is required}"

  if [[ "${obc_target}" == "${subsystem_target}" ]]; then
    echo "OBC target and subsystem target must not be the same SSH endpoint." >&2
    return 1
  fi

  local obc_host=""
  local subsystem_host=""
  obc_host="$(obc_ssh "${obc_target}" hostname -s 2>/dev/null || true)"
  subsystem_host="$(obc_ssh "${subsystem_target}" hostname -s 2>/dev/null || true)"

  if [[ -n "${obc_host}" && -n "${subsystem_host}" && "${obc_host}" == "${subsystem_host}" ]]; then
    echo "OBC target (${obc_target}) and subsystem target (${subsystem_target}) resolve to the same remote host (${obc_host})." >&2
    return 1
  fi
}

obc_require_systemd_service_name() {
  local service_name="${1:?service_name is required}"

  if [[ ! "${service_name}" =~ ^[A-Za-z0-9_.@:-]+\.service$ ]]; then
    echo "SERVICE_NAME must be a safe systemd unit name ending in .service." >&2
    return 1
  fi
}

obc_require_can_device_name() {
  local device_name="${1:?device_name is required}"

  if [[ ! "${device_name}" =~ ^[A-Za-z0-9._-]+$ ]]; then
    echo "CAN device name must be a safe Linux interface identifier." >&2
    return 1
  fi
}

obc_require_target_comm_profile() {
  local profile="${1:?profile is required}"

  case "${profile}" in
    sband|uhf-primary|uhf-backup)
      ;;
    *)
      echo "TARGET_COMM_PROFILE must be one of: sband, uhf-primary, uhf-backup." >&2
      return 1
      ;;
  esac
}

obc_target_comm_profile_node() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf '5\n'
      ;;
    uhf-primary|uhf-backup)
      printf '6\n'
      ;;
  esac
}

obc_target_comm_profile_authority() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf 'sband-primary\n'
      ;;
    uhf-primary)
      printf 'uhf-primary\n'
      ;;
    uhf-backup)
      printf 'uhf-backup\n'
      ;;
  esac
}

obc_target_comm_profile_requires_uhf_primary_switch() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    uhf-primary)
      printf '1\n'
      ;;
    sband|uhf-backup)
      printf '0\n'
      ;;
  esac
}

obc_target_comm_profile_service_authority() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband|uhf-primary|uhf-backup)
      printf 'sband-primary\n'
      ;;
  esac
}

obc_target_comm_profile_initial_band() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband|uhf-backup)
      printf 'sband\n'
      ;;
    uhf-primary)
      printf 'uhf\n'
      ;;
  esac
}

obc_target_comm_profile_enable_primary_ground_link_driver() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband|uhf-backup)
      printf '1\n'
      ;;
    uhf-primary)
      printf '0\n'
      ;;
  esac
}

obc_target_comm_profile_subsystem_stack_target() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf 'subsystem-sband-csp-stack.target\n'
      ;;
    uhf-primary|uhf-backup)
      printf 'subsystem-uhf-csp-stack.target\n'
      ;;
  esac
}

obc_target_comm_profile_subsystem_comm_service() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf 'subsystem-sband-csp.service\n'
      ;;
    uhf-primary|uhf-backup)
      printf 'subsystem-uhf-csp.service\n'
      ;;
  esac
}

obc_target_comm_profile_ground_path_kind() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf 'tcp\n'
      ;;
    uhf-primary|uhf-backup)
      printf 'serial\n'
      ;;
  esac
}

obc_target_comm_profile_ccsds_vcid() {
  local profile="${1:?profile is required}"
  obc_require_target_comm_profile "${profile}" >/dev/null
  case "${profile}" in
    sband)
      printf '1\n'
      ;;
    uhf-primary|uhf-backup)
      printf '2\n'
      ;;
  esac
}

obc_can_parentdev() {
  local device_name="${1:?device_name is required}"

  ip -details link show dev "${device_name}" 2>/dev/null | sed -n 's/.*parentdev \([^ ]*\).*/\1/p' | head -n 1
}

obc_can_bring_up() {
  local device_name="${1:?device_name is required}"
  local bitrate="${2:-500000}"
  local dbitrate="${3:-2000000}"
  local restart_ms="${4:-${CAN_RESTART_MS:-100}}"

  obc_require_can_device_name "${device_name}"
  sudo -n ip link set "${device_name}" down >/dev/null 2>&1 || true
  sudo -n ip link set "${device_name}" up type can bitrate "${bitrate}" dbitrate "${dbitrate}" restart-ms "${restart_ms}" fd on
}

obc_ssh() {
  local -a ssh_args=(-o BatchMode=yes -o ConnectTimeout=5)

  if [[ -n "${OBC_SSH_KNOWN_HOSTS_FILE:-}" ]]; then
    ssh_args+=(-o StrictHostKeyChecking=yes -o UserKnownHostsFile="${OBC_SSH_KNOWN_HOSTS_FILE}")
  fi

  ssh "${ssh_args[@]}" "$@"
}

obc_find_native_bin_dir() {
  local root_dir="${1:?root_dir is required}"
  local bin_root="${root_dir}/build-fprime-automatic-native/bin"
  local candidate

  if [[ ! -d "${bin_root}" ]]; then
    return 1
  fi

  while IFS= read -r candidate; do
    if [[ -x "${candidate}/OBC" && -x "${candidate}/csp_zmqproxy" && -x "${candidate}/eps_simulator" && -x "${candidate}/adcs_simulator" && -x "${candidate}/radio_mock_server" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done < <(find "${bin_root}" -mindepth 1 -maxdepth 1 -type d | sort)

  return 1
}

obc_force_cleanup_process_pattern() {
  local pattern="${1:?pattern is required}"
  local pids
  local remaining

  pids="$(pgrep -f -- "${pattern}" || true)"
  if [[ -z "${pids}" ]]; then
    return 0
  fi

  # Try graceful shutdown first, then escalate because past hosted probe
  # failures have left high-CPU orphaned fprime-gds child processes behind.
  kill ${pids} >/dev/null 2>&1 || true
  sleep 1

  remaining="$(pgrep -f -- "${pattern}" || true)"
  if [[ -n "${remaining}" ]]; then
    kill -9 ${remaining} >/dev/null 2>&1 || true
  fi
}

obc_find_dictionary_path() {
  local root_dir="${1:?root_dir is required}"
  local deployment_name="${2:-OBC}"
  local candidate

  if [[ ! -d "${root_dir}/build-artifacts" ]]; then
    return 1
  fi

  while IFS= read -r candidate; do
    if [[ -f "${candidate}" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done < <(find "${root_dir}/build-artifacts" -path "*/${deployment_name}/dict/AppTopologyDictionary.json" | sort)

  while IFS= read -r candidate; do
    if [[ -f "${candidate}" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done < <(find "${root_dir}/build-artifacts" -path "*/${deployment_name}/dict/*.json" | sort)

  return 1
}

obc_default_gds_host() {
  if command -v scutil >/dev/null 2>&1; then
    local local_name
    local_name="$(scutil --get LocalHostName 2>/dev/null || true)"
    if [[ -n "${local_name}" ]]; then
      printf '%s.local\n' "${local_name}"
      return 0
    fi
  fi

  hostname
}

obc_default_remote_carrier_host() {
  local candidate

  for candidate in en0 en1; do
    if command -v ifconfig >/dev/null 2>&1; then
      local ipv4
      ipv4="$(ifconfig "${candidate}" 2>/dev/null | awk '/inet / { print $2; exit }')"
      if [[ -n "${ipv4}" && "${ipv4}" != 127.* ]]; then
        printf '%s\n' "${ipv4}"
        return 0
      fi
    fi
  done

  obc_default_gds_host
}

obc_sha256_file() {
  local file_path="${1:?file_path is required}"

  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${file_path}" | awk '{print $1}'
    return 0
  fi

  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "${file_path}" | awk '{print $1}'
    return 0
  fi

  python3 - "${file_path}" <<'PY'
import hashlib
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
digest = hashlib.sha256()
with path.open("rb") as handle:
    for chunk in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(chunk)
print(digest.hexdigest())
PY
}

obc_sanitize_name() {
  local raw_name="${1:?raw_name is required}"
  printf '%s' "${raw_name}" | tr -cs 'A-Za-z0-9._-' '_'
}

obc_lowercase() {
  local raw_value="${1:?raw_value is required}"
  printf '%s' "${raw_value}" | tr '[:upper:]' '[:lower:]'
}

obc_latest_rpi_bundle() {
  local root_dir="${1:?root_dir is required}"
  local candidate

  while IFS= read -r candidate; do
    if [[ -f "${candidate}" ]]; then
      printf '%s\n' "${candidate}"
    fi
  done < <(find "${root_dir}/build-artifacts/packages/rpi" -type f -name 'obc-rpi-*.tar.gz' | sort) | tail -n 1
}

obc_iter_process_rows() {
  ps -ax -o pid= -o command= | awk '{
    pid=$1
    $1=""
    sub(/^ /, "", $0)
    printf "%s\t%s\n", pid, $0
  }'
}

obc_command_matches_all_fragments() {
  local command="${1:?command is required}"
  shift
  local fragment
  for fragment in "$@"; do
    if [[ "${command}" != *"${fragment}"* ]]; then
      return 1
    fi
  done
  return 0
}

obc_signal_matching_processes() {
  local signal_name="${1:?signal name is required}"
  shift
  local line
  local pid
  local command

  while IFS=$'\t' read -r pid command; do
    [[ -n "${pid}" && -n "${command}" ]] || continue
    if obc_command_matches_all_fragments "${command}" "$@"; then
      kill "-${signal_name}" "${pid}" >/dev/null 2>&1 || true
    fi
  done < <(obc_iter_process_rows)
}

obc_reap_matching_processes() {
  if [[ "$#" -lt 1 ]]; then
    return 0
  fi
  obc_signal_matching_processes TERM "$@"
  sleep 1
  obc_signal_matching_processes KILL "$@"
}

obc_cleanup_job_processes() {
  local timeout_sec="${1:-5}"
  shift || true
  local -a pids=()
  local pid
  local survivors=()
  local deadline

  while IFS= read -r pid; do
    [[ -n "${pid}" ]] || continue
    pids+=("${pid}")
  done < <(jobs -p)

  if [[ "${#pids[@]}" -eq 0 ]]; then
    return 0
  fi

  kill "${pids[@]}" >/dev/null 2>&1 || true
  deadline=$((SECONDS + timeout_sec))

  while :; do
    survivors=()
    for pid in "${pids[@]}"; do
      if kill -0 "${pid}" >/dev/null 2>&1; then
        survivors+=("${pid}")
      fi
    done

    if [[ "${#survivors[@]}" -eq 0 ]]; then
      break
    fi

    if [[ "${SECONDS}" -ge "${deadline}" ]]; then
      kill -9 "${survivors[@]}" >/dev/null 2>&1 || true
      break
    fi

    sleep 1
  done

  wait >/dev/null 2>&1 || true
}
