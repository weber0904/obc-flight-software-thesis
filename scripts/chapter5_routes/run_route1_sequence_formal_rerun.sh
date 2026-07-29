#!/usr/bin/env bash
# Freeze a fresh Route 1 sequence campaign without taking ownership of C-level
# target baseline lifecycle. The target wrapper remains the A -> B -> C -> A -> B owner.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

CANONICAL_OBC_COMM_CSP_SERVICE_NAME="obc-comm-csp-stack.service"
if [[ -n "${OBC_COMM_CSP_SERVICE_NAME:-}" && \
      "${OBC_COMM_CSP_SERVICE_NAME}" != "${CANONICAL_OBC_COMM_CSP_SERVICE_NAME}" ]]; then
  echo "Route 1 formal evidence requires OBC_COMM_CSP_SERVICE_NAME=${CANONICAL_OBC_COMM_CSP_SERVICE_NAME}" >&2
  exit 2
fi
export OBC_COMM_CSP_SERVICE_NAME="${CANONICAL_OBC_COMM_CSP_SERVICE_NAME}"

EVIDENCE_DATE="${EVIDENCE_DATE:-$(date +%F)}"
EVIDENCE_ROOT="${EVIDENCE_ROOT:-${ROOT_DIR}/evidence/records/chapter5-integrated-route-closure-v1/artifacts/${EVIDENCE_DATE}-route1-sequence-formal-rerun}"
RUNTIME_ROOT="${RUNTIME_ROOT:-/private/tmp/route1-sequence-formal-rerun-${EVIDENCE_DATE}}"
# FileDownlink stores source paths in a bounded F' file-entry buffer. Keep the
# hosted runtime independently short even when the evidence campaign name is
# intentionally descriptive.
HOSTED_RUNTIME_ROOT="${HOSTED_RUNTIME_ROOT:-/private/tmp/r1h-${EVIDENCE_DATE//-/}}"
SKIP_DEPLOY="${SKIP_DEPLOY:-0}"
RESUME="${RESUME:-0}"
IMPORTER="${ROOT_DIR}/scripts/chapter5_routes/import_formal_artifacts.py"
ATTEMPT_CHECKER="${ROOT_DIR}/scripts/chapter5_routes/check_formal_attempt_artifacts.py"
PROVENANCE_CHECKER="${ROOT_DIR}/scripts/chapter5_routes/check_route1_target_revision_provenance.py"
CAMPAIGN_MANIFEST_WRITER="${ROOT_DIR}/scripts/chapter5_routes/write_route1_campaign_manifest.py"
HOSTED_WRAPPER="${ROOT_DIR}/scripts/chapter5_routes/hosted/run_route1_hosted.sh"
TARGET_WRAPPER="${ROOT_DIR}/scripts/chapter5_routes/target/run_route1_target.sh"
HOSTED_BUILD_TARGETS=(
  OBC
  payload_camera_backend_helper
  csp_zmqproxy
  eps_simulator
  adcs_simulator
  radio_mock_server
  pty_pair_bridge
  sband_comm_csp_node
  uhf_comm_csp_node
  ground_ttc_gateway
)
OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
RPI_INSTALL_ROOT="${RPI_INSTALL_ROOT:-/home/operator/obc-deploy}"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python3"
TARGET_PROVENANCE_PATH="${EVIDENCE_ROOT}/deployment/target-revision-provenance.json"
CAMPAIGN_IDENTITY_PATH="${EVIDENCE_ROOT}/deployment/campaign-source-identity.json"
TRUSTED_INSTALLED_MANIFEST_PATH="${TRUSTED_INSTALLED_MANIFEST_PATH:-}"
INSTALLED_MANIFEST_RECEIPT_PATH="${EVIDENCE_ROOT}/deployment/trusted-installed-manifest.json"
TRUSTED_SERVICE_UNIT_PATH="${TRUSTED_SERVICE_UNIT_PATH:-}"
SERVICE_UNIT_RECEIPT_PATH="${EVIDENCE_ROOT}/deployment/trusted-obc-comm-csp-stack.service"
EVIDENCE_SYNC_EXCLUDE_PATH="$("${PYTHON_BIN}" - "${ROOT_DIR}" "${EVIDENCE_ROOT}" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve()
evidence = pathlib.Path(sys.argv[2]).resolve()
try:
    print(evidence.relative_to(root).as_posix())
except ValueError:
    print("")
PY
)"

if [[ -e "${EVIDENCE_ROOT}" && "${RESUME}" != "1" ]]; then
  echo "Evidence destination already exists: ${EVIDENCE_ROOT}" >&2
  exit 2
fi

mkdir -p "${EVIDENCE_ROOT}/deployment/logs" "${RUNTIME_ROOT}"
ATTEMPT_RECORDS=""

if [[ "${RESUME}" == "1" ]]; then
  if [[ ! -f "${INSTALLED_MANIFEST_RECEIPT_PATH}" ]]; then
    echo "Cannot resume without ${INSTALLED_MANIFEST_RECEIPT_PATH}" >&2
    exit 2
  fi
  if [[ ! -f "${SERVICE_UNIT_RECEIPT_PATH}" ]]; then
    echo "Cannot resume without ${SERVICE_UNIT_RECEIPT_PATH}" >&2
    exit 2
  fi
elif [[ "${SKIP_DEPLOY}" == "1" ]]; then
  if [[ -z "${TRUSTED_INSTALLED_MANIFEST_PATH}" || ! -f "${TRUSTED_INSTALLED_MANIFEST_PATH}" ]]; then
    echo "SKIP_DEPLOY=1 requires TRUSTED_INSTALLED_MANIFEST_PATH from the accepted package/install receipt" >&2
    exit 2
  fi
  cp "${TRUSTED_INSTALLED_MANIFEST_PATH}" "${INSTALLED_MANIFEST_RECEIPT_PATH}"
  if [[ -z "${TRUSTED_SERVICE_UNIT_PATH}" || ! -f "${TRUSTED_SERVICE_UNIT_PATH}" ]]; then
    echo "SKIP_DEPLOY=1 requires TRUSTED_SERVICE_UNIT_PATH from the accepted systemd install receipt" >&2
    exit 2
  fi
  cp "${TRUSTED_SERVICE_UNIT_PATH}" "${SERVICE_UNIT_RECEIPT_PATH}"
fi

run_logged() {
  local label="$1"
  shift
  local log_path="${EVIDENCE_ROOT}/deployment/logs/${label}.log"
  printf '+ %q' "$@" | tee "${log_path}"
  printf '\n' | tee -a "${log_path}"
  set +e
  "$@" 2>&1 | tee -a "${log_path}"
  local status=${PIPESTATUS[0]}
  set -e
  return "${status}"
}

if [[ "${SKIP_DEPLOY}" != "1" && "${RESUME}" != "1" ]]; then
  # Campaign identity now verifies the exact recursive committed tree. Repair
  # a fresh or revision-drifted submodule checkout before that fail-closed
  # identity boundary, while preserving SKIP_DEPLOY and resume behavior.
  run_logged submodule-update \
    git -C "${ROOT_DIR}" submodule update --init --recursive
fi

if [[ "${RESUME}" == "1" ]]; then
  if [[ ! -f "${EVIDENCE_ROOT}/campaign-manifest.json" ]]; then
    echo "Cannot resume without ${EVIDENCE_ROOT}/campaign-manifest.json" >&2
    exit 2
  fi
  "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check-campaign-identity \
    --repo-root "${ROOT_DIR}" \
    --identity "${CAMPAIGN_IDENTITY_PATH}" \
    --campaign-manifest "${EVIDENCE_ROOT}/campaign-manifest.json" \
    --allow-untracked-root "${ROOT_DIR}/output" \
    --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
    --allow-untracked-root "${EVIDENCE_ROOT}"
  ATTEMPT_RECORDS="$("${ROOT_DIR}/fprime-venv/bin/python3" - "${EVIDENCE_ROOT}/campaign-manifest.json" <<'PY'
import json
import pathlib
import sys

for record in json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")).get("attempts", []):
    print(json.dumps(record, sort_keys=True))
PY
)"
  [[ -z "${ATTEMPT_RECORDS}" ]] || ATTEMPT_RECORDS+=$'\n'
else
  "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" write-campaign-identity \
    --repo-root "${ROOT_DIR}" \
    --identity "${CAMPAIGN_IDENTITY_PATH}" \
    --allow-untracked-root "${ROOT_DIR}/output" \
    --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
    --allow-untracked-root "${EVIDENCE_ROOT}"
fi

write_campaign_manifest() {
  ATTEMPT_RECORDS="${ATTEMPT_RECORDS}" \
    "${PYTHON_BIN}" "${CAMPAIGN_MANIFEST_WRITER}" \
      --output "${EVIDENCE_ROOT}/campaign-manifest.json" \
      --campaign-date "${EVIDENCE_DATE}" \
      --evidence-root "${EVIDENCE_ROOT}" \
      --campaign-identity "${CAMPAIGN_IDENTITY_PATH}" \
      --target-provenance "${TARGET_PROVENANCE_PATH}"
}

target_provenance_attempt_path() {
  local attempt_label="${1:?attempt label is required}"
  printf '%s/deployment/target-revision-provenance-%s.json\n' \
    "${EVIDENCE_ROOT}" "${attempt_label}"
}

snapshot_local_deployment() {
  local phase="${1:?snapshot phase is required}"
  local destination="${EVIDENCE_ROOT}/deployment/${phase}"
  mkdir -p "${destination}"
  git -C "${ROOT_DIR}" branch --show-current >"${destination}/local-branch.txt"
  git -C "${ROOT_DIR}" rev-parse HEAD >"${destination}/local-head.txt"
  git -C "${ROOT_DIR}" describe --tags --always --dirty --broken \
    >"${destination}/local-project-version.txt"
  git -C "${ROOT_DIR}" status --short >"${destination}/local-status.txt"
  git -C "${ROOT_DIR}" submodule status --recursive >"${destination}/submodules.txt"
  git -C "${ROOT_DIR}" diff --check >"${destination}/diff-check.txt"
}

classify_failure() {
  local log_path="$1"
  if rg -qi 'SEQ_RUN.*(FAIL|failed)|sequence.*(semantic|validation).*fail|luma.*(fail|missing)|hash.*mismatch|PAYLOAD_CAPTURE.*(fail|error)|SYS_MODE_CHANGE.*(missing|fail)' "${log_path}"; then
    printf 'product-or-semantic\n'
  elif rg -qi 'timed out waiting for CHALLENGE|southbound-read-failed|southbound-to-gds\.bin.*0 bytes' "${log_path}"; then
    # A missing challenge can originate below the ground observer (for
    # example, an incomplete node-5 V3 data delivery), so do not falsely
    # label it as a ground-only observability fault.
    printf 'sband-challenge-return-path\n'
  elif rg -qi 'FileDownlink\.cpp:129|sourceFilename\.length\(\).*FILE_ENTRY_FILENAME_LEN' "${log_path}"; then
    printf 'runtime-path-length\n'
  elif rg -qi 'ssh.*(timed out|banner)|ground.*(observability|readiness|timeout)|baseline.*(not ready|failed)|connection.*(refused|reset)' "${log_path}"; then
    printf 'transient-environment\n'
  else
    printf 'unknown\n'
  fi
}

is_retryable_failure() {
  case "$1" in
    sband-challenge-return-path|transient-environment|runtime-path-length) return 0 ;;
    *) return 1 ;;
  esac
}

import_attempt() {
  local surface="$1"
  local attempt_label="$2"
  local wrapper_root="$3"
  local command="$4"
  local verdict="$5"
  local retry_count="$6"
  local retry_of="$7"
  local failure_class="$8"

  local target_provenance_sha256=""
  local target_provenance_relative_path=""
  if [[ "${surface}" == "target" ]]; then
    local target_provenance_attempt_path_value
    target_provenance_attempt_path_value="$(
      target_provenance_attempt_path "${attempt_label}"
    )"
    if [[ ! -f "${target_provenance_attempt_path_value}" ]]; then
      echo "Cannot bind target attempt without ${target_provenance_attempt_path_value}" >&2
      return 1
    fi
    target_provenance_relative_path="$(
      "${PYTHON_BIN}" - "${EVIDENCE_ROOT}" "${target_provenance_attempt_path_value}" <<'PY'
import pathlib
import sys

print(pathlib.Path(sys.argv[2]).resolve().relative_to(
    pathlib.Path(sys.argv[1]).resolve()
).as_posix())
PY
    )"
    target_provenance_sha256="$(
      "${PYTHON_BIN}" - "${target_provenance_attempt_path_value}" <<'PY'
import hashlib
import pathlib
import sys

digest = hashlib.sha256()
with pathlib.Path(sys.argv[1]).open("rb") as handle:
    for chunk in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(chunk)
print(digest.hexdigest())
PY
    )"
  fi

  "${ROOT_DIR}/fprime-venv/bin/python3" "${IMPORTER}" \
    --destination-root "${EVIDENCE_ROOT}" \
    --route route1 \
    --surface "${surface}" \
    --attempt-label "${attempt_label}" \
    --wrapper-root "${wrapper_root}" \
    --command "${command}" \
    --verdict "${verdict}" \
    --retry-count "${retry_count}" \
    --retry-of "${retry_of}" \
    --failure-class "${failure_class}" \
    --timestamp "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    --note "Immediate repo import after wrapper completion; no artifact pruning."

  local attempt_manifest_sha256
  attempt_manifest_sha256="$(
    "${PYTHON_BIN}" - "${EVIDENCE_ROOT}/route1/${surface}/${attempt_label}/manifest.json" <<'PY'
import hashlib
import pathlib
import sys

digest = hashlib.sha256()
with pathlib.Path(sys.argv[1]).open("rb") as handle:
    for chunk in iter(lambda: handle.read(1024 * 1024), b""):
        digest.update(chunk)
print(digest.hexdigest())
PY
  )"

  local record
  record="$("${ROOT_DIR}/fprime-venv/bin/python3" - "${surface}" "${attempt_label}" "${command}" "${verdict}" "${retry_count}" "${retry_of}" "${failure_class}" "${target_provenance_sha256}" "${target_provenance_relative_path}" "${attempt_manifest_sha256}" <<'PY'
import json
import sys
print(json.dumps({
    "route": "route1",
    "surface": sys.argv[1],
    "attempt": sys.argv[2],
    "command": sys.argv[3],
    "verdict": sys.argv[4],
    "retryCount": int(sys.argv[5]),
    "retryOf": sys.argv[6] or None,
    "failureClass": sys.argv[7] or None,
    "targetRevisionProvenanceSha256": sys.argv[8] or None,
    "targetRevisionProvenancePath": sys.argv[9] or None,
    "attemptManifestSha256": sys.argv[10],
    "authoritative": sys.argv[4] == "PASS",
}))
PY
)"
  ATTEMPT_RECORDS+="${record}"$'\n'
  write_campaign_manifest
}

prepare_target_attempt() {
  local attempt_number="$1"
  local attempt_label
  attempt_label="$(printf 'attempt-%02d' "${attempt_number}")"
  local target_provenance_attempt_path_value
  target_provenance_attempt_path_value="$(
    target_provenance_attempt_path "${attempt_label}"
  )"

  # A target retry is a new functional observation. Rebind it to the campaign
  # identity and current installed revision instead of recursively reusing the
  # provenance that preceded the failed attempt.
  if ! run_logged "campaign-identity-target-${attempt_label}" \
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check-campaign-identity \
      --repo-root "${ROOT_DIR}" \
      --identity "${CAMPAIGN_IDENTITY_PATH}" \
      --allow-untracked-root "${ROOT_DIR}/output" \
      --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
      --allow-untracked-root "${EVIDENCE_ROOT}"; then
    return 1
  fi

  if ! run_logged "target-baseline-pre-${attempt_label}" \
    env TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1 \
    bash "${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"; then
    return 1
  fi

  if ! run_logged "target-revision-provenance-${attempt_label}" \
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check \
      --repo-root "${ROOT_DIR}" \
      --ssh-target "${OBC_SSH_TARGET}" \
      --remote-workspace "${RPI_REMOTE_DIR}" \
      --install-root "${RPI_INSTALL_ROOT}" \
      --trusted-installed-manifest "${INSTALLED_MANIFEST_RECEIPT_PATH}" \
      --trusted-service-unit "${SERVICE_UNIT_RECEIPT_PATH}" \
      --allow-untracked-root "${ROOT_DIR}/output" \
      --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
      --allow-untracked-root "${EVIDENCE_ROOT}" \
      --output "${target_provenance_attempt_path_value}"; then
    if [[ -f "${target_provenance_attempt_path_value}" ]]; then
      cp "${target_provenance_attempt_path_value}" "${TARGET_PROVENANCE_PATH}"
    fi
    write_campaign_manifest
    echo "Route 1 ${attempt_label} blocked by revision provenance failure: ${target_provenance_attempt_path_value}" >&2
    return 1
  fi
  cp "${target_provenance_attempt_path_value}" "${TARGET_PROVENANCE_PATH}"
  write_campaign_manifest
}

prepare_hosted_attempt() {
  local attempt_number="$1"
  local attempt_label
  attempt_label="$(printf 'attempt-%02d' "${attempt_number}")"

  # SKIP_DEPLOY controls target installation only. Every hosted observation
  # must rebuild from the campaign source identity immediately before C so an
  # old native tree cannot supply a PASS for the current checkout.
  if ! run_logged "campaign-identity-hosted-${attempt_label}" \
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check-campaign-identity \
      --repo-root "${ROOT_DIR}" \
      --identity "${CAMPAIGN_IDENTITY_PATH}" \
      --allow-untracked-root "${ROOT_DIR}/output" \
      --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
      --allow-untracked-root "${EVIDENCE_ROOT}"; then
    return 1
  fi

  if ! run_logged "native-build-hosted-${attempt_label}" \
    "${ROOT_DIR}/fprime-venv/bin/cmake" \
      --build "${ROOT_DIR}/build-fprime-automatic-native" \
      --target "${HOSTED_BUILD_TARGETS[@]}" -- -j4; then
    return 1
  fi
}

run_surface_attempt() {
  local surface="$1"
  local attempt_number="$2"
  local retry_of="$3"
  local wrapper="$4"
  local attempt_label
  attempt_label="$(printf 'attempt-%02d' "${attempt_number}")"

  if [[ "${surface}" == "hosted" ]]; then
    if ! prepare_hosted_attempt "${attempt_number}"; then
      return 1
    fi
  elif [[ "${surface}" == "target" ]]; then
    if ! prepare_target_attempt "${attempt_number}"; then
      return 1
    fi
  fi

  local wrapper_root
  if [[ "${surface}" == "hosted" ]]; then
    # Linux/macOS Unix-domain sockets have a short pathname limit. Keep the
    # live root short while the importer preserves it under the canonical tree.
    wrapper_root="$(mktemp -d "/tmp/r1sfh-${attempt_number}.XXXXXX")"
  else
    wrapper_root="$(mktemp -d "/private/tmp/r1sft-${attempt_number}.XXXXXX")"
  fi
  local command="bash ${wrapper}"
  local status=0

  mkdir -p "${wrapper_root}"
  if [[ "${surface}" == "hosted" ]]; then
    set +e
    PROBE_TMP_DIR="${wrapper_root}" RUNTIME_ROOT="${HOSTED_RUNTIME_ROOT}" bash "${wrapper}"
    status=$?
    set -e
  else
    set +e
    PROBE_ROOT="${wrapper_root}" bash "${wrapper}"
    status=$?
    set -e
  fi

  local verdict="PASS"
  local failure_class=""
  if [[ "${status}" -ne 0 ]]; then
    verdict="FAIL"
    failure_class="$(classify_failure "${wrapper_root}/summary.log")"
  fi
  import_attempt "${surface}" "${attempt_label}" "${wrapper_root}" "${command}" "${verdict}" \
    "$((attempt_number - 1))" "${retry_of}" "${failure_class}"

  if [[ "${status}" -eq 0 ]]; then
    return 0
  fi
  if [[ "${attempt_number}" -eq 1 ]] && is_retryable_failure "${failure_class}"; then
    printf 'Retrying %s after classified retryable failure: %s.\n' "${surface}" "${failure_class}" >&2
    run_surface_attempt "${surface}" 2 "attempt-01" "${wrapper}"
    return $?
  fi
  printf '%s attempt-%s failed (%s); no further retry is permitted.\n' \
    "${surface}" "${attempt_number}" "${failure_class}" >&2
  return "${status}"
}

run_pending_surface() {
  local surface="$1"
  local wrapper="$2"
  if [[ "${RESUME}" != "1" ]]; then
    run_surface_attempt "${surface}" 1 "" "${wrapper}"
    return
  fi

  local prior
  prior="$("${ROOT_DIR}/fprime-venv/bin/python3" - "${EVIDENCE_ROOT}/campaign-manifest.json" "${surface}" <<'PY'
import json
import pathlib
import sys

records = [
    record for record in json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")).get("attempts", [])
    if record.get("surface") == sys.argv[2]
]
if not records:
    print("0|||" )
else:
    record = records[-1]
    print("|".join((record["attempt"].removeprefix("attempt-"), record["attempt"], record["verdict"], record.get("failureClass") or "")))
PY
)"
  local prior_number prior_label prior_verdict prior_class
  IFS='|' read -r prior_number prior_label prior_verdict prior_class <<<"${prior}"
  if [[ "${prior_number}" == "0" ]]; then
    run_surface_attempt "${surface}" 1 "" "${wrapper}"
    return
  fi
  if [[ "${prior_verdict}" == "PASS" ]]; then
    if ! run_logged "retained-attempt-${surface}-${prior_label}" \
      "${PYTHON_BIN}" "${ATTEMPT_CHECKER}" \
        --evidence-root "${EVIDENCE_ROOT}" \
        --campaign-manifest "${EVIDENCE_ROOT}/campaign-manifest.json" \
        --surface "${surface}" \
        --attempt "${prior_label}"; then
      return 1
    fi
    printf 'Resume: %s already has authoritative %s; skipping.\n' "${surface}" "${prior_label}"
    return
  fi
  if [[ "${prior_number}" -lt 2 ]] && is_retryable_failure "${prior_class}"; then
    run_surface_attempt "${surface}" "$((10#${prior_number} + 1))" "${prior_label}" "${wrapper}"
    return
  fi
  echo "Resume cannot rerun ${surface}: prior ${prior_label} verdict=${prior_verdict} class=${prior_class}" >&2
  return 1
}

if [[ "${RESUME}" != "1" ]]; then
  snapshot_local_deployment pre-deploy
fi
if [[ "${SKIP_DEPLOY}" != "1" && "${RESUME}" != "1" ]]; then
  run_logged native-build-target-deploy \
    "${ROOT_DIR}/fprime-venv/bin/cmake" \
      --build "${ROOT_DIR}/build-fprime-automatic-native" \
      --target OBC -- -j4
  run_logged local-target-sync-provenance \
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check-local \
      --repo-root "${ROOT_DIR}" \
      --allow-untracked-root "${ROOT_DIR}/output" \
      --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
      --allow-untracked-root "${EVIDENCE_ROOT}"
  run_logged target-workspace-bootstrap \
    env RPI_SYNC_EXTRA_EXCLUDE_PATH="${EVIDENCE_SYNC_EXCLUDE_PATH}" \
    RPI_SYNC_TRACKED_ONLY=1 \
    RPI_SYNC_REPLACE_REMOTE=1 \
    bash "${ROOT_DIR}/scripts/bootstrap_rpi_workspace.sh"
  run_logged target-workspace-provenance \
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" mark-workspace \
      --repo-root "${ROOT_DIR}" \
      --ssh-target "${OBC_SSH_TARGET}" \
      --remote-workspace "${RPI_REMOTE_DIR}" \
      --allow-untracked-root "${ROOT_DIR}/output" \
      --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
      --allow-untracked-root "${EVIDENCE_ROOT}"
  run_logged package-rpi-bundle bash "${ROOT_DIR}/scripts/package_rpi_bundle.sh"
  bundle_path="$(sed -n 's/^  tarball: //p' "${EVIDENCE_ROOT}/deployment/logs/package-rpi-bundle.log" | tail -n 1)"
  if [[ -z "${bundle_path}" || ! -f "${bundle_path}" ]]; then
    echo "Could not determine packaged OBC bundle from package-rpi-bundle.log" >&2
    exit 1
  fi
  package_manifest_path="$(sed -n 's/^  manifest: //p' "${EVIDENCE_ROOT}/deployment/logs/package-rpi-bundle.log" | tail -n 1)"
  if [[ -z "${package_manifest_path}" || ! -f "${package_manifest_path}" ]]; then
    echo "Could not determine trusted package manifest from package-rpi-bundle.log" >&2
    exit 1
  fi
  cp "${package_manifest_path}" "${INSTALLED_MANIFEST_RECEIPT_PATH}"
  printf '%s\n' "${bundle_path}" >"${EVIDENCE_ROOT}/deployment/package-path.txt"
  run_logged install-rpi-bundle env FORCE_INSTALL=1 bash "${ROOT_DIR}/scripts/install_rpi_bundle.sh" "${bundle_path}"
  run_logged install-rpi-comm-autostart \
    env START_NOW=0 \
      RENDERED_UNIT_RECEIPT_PATH="${SERVICE_UNIT_RECEIPT_PATH}" \
      bash "${ROOT_DIR}/scripts/install_rpi_comm_csp_autostart.sh"
  run_logged subsystem-workspace-bootstrap bash "${ROOT_DIR}/scripts/bootstrap_subsystem_sim_workspace.sh"
fi

if [[ "${RESUME}" != "1" ]]; then
  # Keep the established A -> B readiness preflight before functional work.
  # The final A-owned reload occurs after hosted execution, immediately before
  # target provenance, so this early check does not create a live-revision
  # claim that can become stale while hosted runs.
  run_logged target-baseline-preflight \
    bash "${ROOT_DIR}/scripts/ensure_target_comm_lab_baseline.sh"
  run_logged ground-baseline-preflight bash "${ROOT_DIR}/scripts/ensure_ground_dual_gds_baseline.sh"
  snapshot_local_deployment pre-functional
fi

run_pending_surface hosted "${HOSTED_WRAPPER}"

# Hosted execution may be long enough for an external checkout change. Bind
# the upcoming target surface back to the identity recorded before the first
# campaign attempt instead of relying only on current-checkout target
# provenance.
run_logged campaign-identity-post-hosted \
  "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" check-campaign-identity \
    --repo-root "${ROOT_DIR}" \
    --identity "${CAMPAIGN_IDENTITY_PATH}" \
    --allow-untracked-root "${ROOT_DIR}/output" \
    --allow-untracked-root "${ROOT_DIR}/.codex_thesis_work" \
    --allow-untracked-root "${EVIDENCE_ROOT}"

target_resume_state="PENDING"
if [[ "${RESUME}" == "1" ]]; then
  target_resume_state="$(
    "${PYTHON_BIN}" "${PROVENANCE_CHECKER}" target-resume-state \
      --manifest "${EVIDENCE_ROOT}/campaign-manifest.json" \
      --provenance "${TARGET_PROVENANCE_PATH}"
  )"
fi

# A owns the shared target service lifecycle. Every target attempt that reaches
# run_surface_attempt performs its own A-owned reload and provenance gate.
# A retained authoritative PASS skips that function and keeps its original
# validated provenance instead of being rebound to the current install.
if [[ "${target_resume_state}" == "RETAINED_AUTHORITATIVE_PASS" ]]; then
  printf 'Resume: preserving retained target PASS provenance: %s\n' \
    "${TARGET_PROVENANCE_PATH}"
fi

run_pending_surface target "${TARGET_WRAPPER}"

write_campaign_manifest
"${PYTHON_BIN}" - "${EVIDENCE_ROOT}/campaign-manifest.json" <<'PY'
import json
import pathlib
import sys

manifest_path = pathlib.Path(sys.argv[1])
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
if manifest.get("authoritativeVerdict") != "PASS":
    raise SystemExit(
        "Route 1 campaign is not authoritative PASS: "
        f"{manifest.get('authoritativeVerdict')}"
    )
PY
printf 'Route 1 sequence formal rerun PASS: %s\n' "${EVIDENCE_ROOT}"
