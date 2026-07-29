#!/usr/bin/env bash
set -euo pipefail

commv_repo_root() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
  printf '%s\n' "${script_dir}"
}

COMMV_ROOT_DIR="${COMMV_ROOT_DIR:-$(commv_repo_root)}"
COMMV_SUITE_DIR="${COMMV_ROOT_DIR}/scripts/comm_verification"

commv_now_utc() {
  date -u +"%Y-%m-%dT%H:%M:%SZ"
}

commv_timestamp_token() {
  date -u +"%Y%m%dT%H%M%SZ"
}

commv_log() {
  printf '[commv] %s\n' "$*"
}

commv_require_env() {
  local name="${1:?name is required}"
  if [[ -z "${!name:-}" ]]; then
    echo "Missing required environment variable: ${name}" >&2
    exit 20
  fi
}

commv_matrix_run_root() {
  local env_name="${1:?env name is required}"
  if [[ -n "${COMM_VERIFICATION_RUN_ROOT:-}" ]]; then
    printf '%s\n' "${COMM_VERIFICATION_RUN_ROOT}"
    return 0
  fi
  printf '%s\n' "${COMMV_ROOT_DIR}/build-artifacts/comm-verification/${env_name}/$(commv_timestamp_token)"
}

commv_case_dir() {
  local case_id="${1:?case id is required}"
  commv_require_env COMM_VERIFICATION_CASE_ROOT
  printf '%s/%s\n' "${COMM_VERIFICATION_CASE_ROOT}" "${case_id}"
}

commv_metadata_path() {
  local case_dir="${1:?case dir is required}"
  printf '%s/result.meta\n' "${case_dir}"
}

commv_write_meta() {
  local meta_path="${1:?meta path is required}"
  local key="${2:?key is required}"
  local value="${3-}"
  printf '%s=%s\n' "${key}" "${value}" >>"${meta_path}"
}

commv_replace_meta() {
  local meta_path="${1:?meta path is required}"
  local key="${2:?key is required}"
  local value="${3-}"
  python3 - "${meta_path}" "${key}" "${value}" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
key = sys.argv[2]
value = sys.argv[3]
lines = []
if path.exists():
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if raw.startswith(key + "="):
            continue
        lines.append(raw)
lines.append(f"{key}={value}")
path.write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
}

commv_prepare_case_dir() {
  local case_id="${1:?case id is required}"
  local case_dir
  case_dir="$(commv_case_dir "${case_id}")"
  rm -rf "${case_dir}"
  mkdir -p "${case_dir}"
  printf '%s\n' "${case_dir}"
}

commv_record_static_meta() {
  local meta_path="${1:?meta path is required}"
  local case_id="${2:?case id is required}"
  : >"${meta_path}"
  commv_write_meta "${meta_path}" case_id "${case_id}"
  commv_write_meta "${meta_path}" environment "${COMM_VERIFICATION_ENV}"
  commv_write_meta "${meta_path}" started_at "$(commv_now_utc)"
  commv_write_meta "${meta_path}" carrier_kind ""
  commv_write_meta "${meta_path}" wrapper_intent ""
  commv_write_meta "${meta_path}" registered_path_reuse ""
  commv_write_meta "${meta_path}" new_claim_attempted ""
  commv_write_meta "${meta_path}" verdict ""
  commv_write_meta "${meta_path}" blocker_class ""
  commv_write_meta "${meta_path}" rerun_safe unknown
  commv_write_meta "${meta_path}" artifact_root ""
  commv_write_meta "${meta_path}" message ""
  commv_write_meta "${meta_path}" finished_at ""
}

commv_mark_wrapper_intent() {
  local meta_path="${1:?meta path is required}"
  local intent="${2:?intent is required}"
  commv_replace_meta "${meta_path}" wrapper_intent "${intent}"
}

commv_mark_rerun_safe() {
  local meta_path="${1:?meta path is required}"
  local value="${2:?value is required}"
  commv_replace_meta "${meta_path}" rerun_safe "${value}"
}

commv_set_claim_flags() {
  local meta_path="${1:?meta path is required}"
  local registered_path_reuse="${2:?reuse is required}"
  local new_claim_attempted="${3:?claim is required}"
  commv_replace_meta "${meta_path}" registered_path_reuse "${registered_path_reuse}"
  commv_replace_meta "${meta_path}" new_claim_attempted "${new_claim_attempted}"
}

commv_set_artifact_root() {
  local meta_path="${1:?meta path is required}"
  local artifact_root="${2:?artifact_root is required}"
  commv_replace_meta "${meta_path}" artifact_root "${artifact_root}"
}

commv_env_script() {
  local env_name="${1:?env name is required}"
  printf '%s/env/%s.sh\n' "${COMMV_SUITE_DIR}" "${env_name}"
}

commv_load_env() {
  commv_require_env COMM_VERIFICATION_ENV
  local env_script
  env_script="$(commv_env_script "${COMM_VERIFICATION_ENV}")"
  if [[ ! -f "${env_script}" ]]; then
    echo "Unknown comm verification environment: ${COMM_VERIFICATION_ENV}" >&2
    exit 20
  fi
  # shellcheck disable=SC1090
  source "${env_script}"
}

commv_join_quoted() {
  local out=()
  local item
  for item in "$@"; do
    out+=("$(printf '%q' "${item}")")
  done
  local joined
  joined="${out[*]}"
  printf '%s\n' "${joined}"
}

commv_blocker() {
  local meta_path="${1:?meta path is required}"
  local blocker_class="${2:?blocker class is required}"
  local message="${3:?message is required}"
  commv_replace_meta "${meta_path}" verdict blocked
  commv_replace_meta "${meta_path}" blocker_class "${blocker_class}"
  commv_replace_meta "${meta_path}" rerun_safe unknown
  commv_replace_meta "${meta_path}" message "${message}"
  commv_replace_meta "${meta_path}" finished_at "$(commv_now_utc)"
  echo "${message}" >&2
  exit 21
}

commv_complete_pass() {
  local meta_path="${1:?meta path is required}"
  commv_replace_meta "${meta_path}" verdict pass
  commv_replace_meta "${meta_path}" blocker_class ""
  commv_replace_meta "${meta_path}" rerun_safe not-checked
  commv_replace_meta "${meta_path}" finished_at "$(commv_now_utc)"
}

commv_complete_fail() {
  local meta_path="${1:?meta path is required}"
  local blocker_class="${2:?blocker class is required}"
  commv_replace_meta "${meta_path}" verdict fail
  commv_replace_meta "${meta_path}" blocker_class "${blocker_class}"
  commv_replace_meta "${meta_path}" rerun_safe unknown
  commv_replace_meta "${meta_path}" finished_at "$(commv_now_utc)"
}

commv_run_logged_command() {
  local meta_path="${1:?meta path is required}"
  shift
  local case_dir
  case_dir="$(dirname "${meta_path}")"
  local command_path="${case_dir}/command.txt"
  local stdout_path="${case_dir}/stdout.log"
  local status_path="${case_dir}/exit_code.txt"
  commv_join_quoted "$@" >"${command_path}"
  "$@" >"${stdout_path}" 2>&1
  local status=$?
  printf '%s\n' "${status}" >"${status_path}"
  return "${status}"
}

commv_read_meta_value() {
  local meta_path="${1:?meta path is required}"
  local key="${2:?key is required}"
  python3 - "${meta_path}" "${key}" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
key = sys.argv[2]
value = ""
if path.exists():
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.startswith(key + "="):
            continue
        value = line.split("=", 1)[1]
print(value)
PY
}

commv_validate_meta_contract() {
  local meta_path="${1:?meta path is required}"
  python3 - "${meta_path}" <<'PY'
import pathlib
import sys

required = [
    "case_id",
    "environment",
    "carrier_kind",
    "wrapper_intent",
    "registered_path_reuse",
    "new_claim_attempted",
    "verdict",
    "blocker_class",
    "rerun_safe",
    "artifact_root",
    "message",
    "started_at",
    "finished_at",
]
path = pathlib.Path(sys.argv[1])
data = {}
for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
    if "=" not in raw:
        continue
    key, value = raw.split("=", 1)
    data[key] = value

missing = [key for key in required if key not in data]
empty = [key for key in required if key in data and key not in {"blocker_class", "message"} and data[key] == ""]
verdict = data.get("verdict", "")
if verdict == "blocked" and not data.get("blocker_class"):
    empty.append("blocker_class")
if verdict == "":
    empty.append("verdict")
if missing or empty:
    problems = []
    if missing:
      problems.append("missing=" + ",".join(missing))
    if empty:
      problems.append("empty=" + ",".join(sorted(set(empty))))
    raise SystemExit("; ".join(problems))
PY
}
