#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT_DIR="${1:-$ROOT_DIR/build-artifacts/verification-ci}"
VENV_BIN_DIR="$ROOT_DIR/fprime-venv/bin"
FPRIME_UTIL="$VENV_BIN_DIR/fprime-util"
MODE="${VERIFICATION_CI_MODE:-full}"
CHANGED_FILES="${VERIFICATION_CI_CHANGED_FILES:-}"
UT_BUILD_DIR="$ROOT_DIR/build-fprime-automatic-native-ut"
UT_LAST_TEST_LOG="$UT_BUILD_DIR/Testing/Temporary/LastTest.log"

mkdir -p "$ARTIFACT_DIR"

SUMMARY_FILE="$ARTIFACT_DIR/summary.md"
: > "$SUMMARY_FILE"

cat <<EOF_SUMMARY > "$SUMMARY_FILE"
# Verification CI Summary

- Date: $(date '+%Y-%m-%d %H:%M:%S %Z')
- Workspace: $ROOT_DIR
- Virtual environment: $VENV_BIN_DIR
- Mode: $MODE

## Steps
EOF_SUMMARY

emit_ctest_failure_debug() {
  local rerun_log="$ARTIFACT_DIR/ctest-rerun-failed.log"

  if [[ ! -d "$UT_BUILD_DIR" ]]; then
    return
  fi

  if ! command -v ctest >/dev/null 2>&1; then
    return
  fi

  if [[ -f "$UT_LAST_TEST_LOG" ]]; then
    echo "" >&2
    echo "--- LastTest.log ---" >&2
    cat "$UT_LAST_TEST_LOG" >&2
  fi

  echo "" >&2
  echo "--- ctest --rerun-failed --output-on-failure ---" >&2
  if ctest --test-dir "$UT_BUILD_DIR" --rerun-failed --output-on-failure >"$rerun_log" 2>&1; then
    cat "$rerun_log" >&2
  else
    cat "$rerun_log" >&2
  fi
}

run_step() {
  local label="$1"
  shift

  local log_file="$ARTIFACT_DIR/${label}.log"
  echo ">>> $label"
  printf -- "- \`%s\`: " "$label" >> "$SUMMARY_FILE"

  if "$@" >"$log_file" 2>&1; then
    echo "PASS" | tee -a "$SUMMARY_FILE"
  else
    echo "FAIL" | tee -a "$SUMMARY_FILE"
    echo "" >> "$SUMMARY_FILE"
    echo "See log: \`$(basename "$log_file")\`" >> "$SUMMARY_FILE"
    cat "$log_file" >&2
    emit_ctest_failure_debug
    exit 1
  fi
}

if [[ "$MODE" == "lightweight" ]]; then
  {
    echo ""
    echo "## Lightweight Path"
    echo ""
    echo "Only documentation and governance files changed. Skipping F' generate/build, UT build, and \`fprime-util check --all\`; running static governance checks only."
    if [[ -n "$CHANGED_FILES" ]]; then
      echo ""
      echo "## Changed Files"
      printf '%s\n' "$CHANGED_FILES" | sed 's/^/- /'
    fi
    echo ""
  } >> "$SUMMARY_FILE"

  run_step "01_check_repo_consistency" python3 "$ROOT_DIR/scripts/check_repo_consistency.py"
  run_step "02_check_public_release" python3 "$ROOT_DIR/scripts/check_public_release.py"
  run_step "03_check_documentation_governance" python3 "$ROOT_DIR/scripts/check_documentation_governance.py"
  run_step "04_check_transport_mtu_apid_contract" python3 "$ROOT_DIR/scripts/check_transport_mtu_apid_contract.py"
  run_step "05_check_component_test_baseline" python3 "$ROOT_DIR/scripts/check_component_test_baseline.py"
  run_step "06_check_legacy_zmq_retired" python3 "$ROOT_DIR/scripts/check_legacy_zmq_retired.py"
  run_step "07_openspec_validate_specs" openspec validate --specs

  echo "" >> "$SUMMARY_FILE"
  echo "All lightweight verification steps passed." >> "$SUMMARY_FILE"
  echo "lightweight: static governance checks passed"
  exit 0
fi

if [[ ! -x "$FPRIME_UTIL" ]]; then
  echo "error: expected $FPRIME_UTIL to exist and be executable" >&2
  exit 1
fi

if ! command -v openspec >/dev/null 2>&1; then
  echo "error: openspec command not found on PATH" >&2
  exit 1
fi

export PATH="$VENV_BIN_DIR:$PATH"

run_step "00_bootstrap_dev_config" bash "$ROOT_DIR/scripts/bootstrap_dev_config.sh"
run_step "01_generate" "$FPRIME_UTIL" generate -f
run_step "02_build" "$FPRIME_UTIL" build
run_step "03_generate_ut" "$FPRIME_UTIL" generate --ut -f
run_step "04_build_ut" "$FPRIME_UTIL" build --ut
run_step "05_check_all" "$FPRIME_UTIL" check --all
run_step "06_check_repo_consistency" python3 "$ROOT_DIR/scripts/check_repo_consistency.py"
run_step "07_check_public_release" python3 "$ROOT_DIR/scripts/check_public_release.py"
run_step "08_check_documentation_governance" python3 "$ROOT_DIR/scripts/check_documentation_governance.py"
run_step "09_check_transport_mtu_apid_contract" python3 "$ROOT_DIR/scripts/check_transport_mtu_apid_contract.py"
run_step "10_check_component_test_baseline" python3 "$ROOT_DIR/scripts/check_component_test_baseline.py"
run_step "11_check_legacy_zmq_retired" python3 "$ROOT_DIR/scripts/check_legacy_zmq_retired.py"
run_step "12_openspec_validate_specs" openspec validate --specs

echo "" >> "$SUMMARY_FILE"
echo "All baseline verification steps passed." >> "$SUMMARY_FILE"
