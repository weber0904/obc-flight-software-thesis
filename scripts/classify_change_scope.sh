#!/usr/bin/env bash

set -euo pipefail

has_files=0
mode="lightweight"

is_lightweight_path() {
  local path="$1"

  case "$path" in
    README.md|AGENTS.md)
      return 0
      ;;
    obc-dev-spec/*)
      return 0
      ;;
    openspec/specs/*)
      return 0
      ;;
    openspec/changes/archive/*)
      return 0
      ;;
    docs/*.md|docs/operator/*|evidence/*.md|evidence/records/*)
      return 0
      ;;
    openspec/reconciliation/baseline-reconciliation-matrix.json|openspec/reconciliation/baseline-reconciliation-matrix.md)
      return 0
      ;;
    .codex/skills/*/SKILL.md)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

while IFS= read -r path || [[ -n "$path" ]]; do
  [[ -z "$path" ]] && continue
  has_files=1
  if ! is_lightweight_path "$path"; then
    # Keep draining stdin after identifying a full-scope change. The GitHub
    # workflow writes a complete changed-file list through a pipe; exiting
    # here would close that pipe early and make its writer fail under pipefail.
    mode="full"
  fi
done

if [[ "$has_files" -eq 0 ]]; then
  mode="full"
fi

echo "$mode"
