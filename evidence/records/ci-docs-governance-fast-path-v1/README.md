# ci-docs-governance-fast-path-v1 Evidence

## Scope

This record covers a CI/governance change that adds a conservative `lightweight` verification mode for documentation and governance-only pull requests while preserving the single required `baseline-gate` job.

This change does not alter runtime code, topology, component behavior, simulator behavior, command/event/channel contracts, or validation-path registry entries.

## Implemented Artifacts

- `scripts/classify_change_scope.sh`: repo-local whitelist classifier for PR changed files.
- `.github/workflows/verification-ci.yml`: workflow classification now delegates to the repo-local classifier.
- `scripts/run_verification_ci.sh`: `lightweight` mode runs static governance checks and skips F' build/test.
- `openspec/specs/delivery-workflow/spec.md`, `README.md`, and `obc-dev-spec/08_delivery_workflow.md`: governance wording for verification scope classification.

## Verification Scope

- Expected hosted CI for this change: `full`, because the PR edits `.github/workflows/**` and `scripts/**`.
- Expected future docs/governance-only PR behavior: `lightweight`, if and only if every changed file is in the explicit whitelist.
- Unknown paths, active OpenSpec change workspaces, scripts, workflows, build configuration, and source files default to `full`.

## Classifier Cases

The classifier returned `lightweight` for:

- `README.md`
- `docs/planning/comm-roadmap.md` at the time of this change; that path is now retired in favor of `docs/roadmap/comm-link-roadmap.md`
- `openspec/specs/delivery-workflow/spec.md`
- `evidence/records/x/README.md`
- `.codex/skills/change-closeout/SKILL.md`

The classifier returned `full` for:

- `OBC/Main.cpp`
- `scripts/run_verification_ci.sh`
- `.github/workflows/verification-ci.yml`
- `openspec/changes/active/.openspec.yaml`
- `.codex/skills/change-closeout/agents/openai.yaml`
- mixed `README.md` plus `OBC/Main.cpp`

## Validation

| Step | Command | Result |
|---|---|---|
| classifier syntax | `bash -n scripts/classify_change_scope.sh` | PASS |
| classifier shell checks | expected lightweight/full cases listed above | PASS |
| lightweight local gate | `VERIFICATION_CI_MODE=lightweight VERIFICATION_CI_CHANGED_FILES=<docs/governance files> bash scripts/run_verification_ci.sh build-artifacts/verification-ci-lightweight-local` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| change validation | `openspec validate ci-docs-governance-fast-path-v1` | PASS |
| specs validation | `openspec validate --specs` | PASS |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| diff whitespace | `git diff --check` | PASS |

## Notes

- OpenSpec CLI commands may print PostHog telemetry flush errors when network access to `edge.openspec.dev` is unavailable. Validation verdicts are based on local command output.
- Lightweight mode still installs and runs OpenSpec in CI; it only avoids the F' venv, submodules, system build packages, and build/test steps.
