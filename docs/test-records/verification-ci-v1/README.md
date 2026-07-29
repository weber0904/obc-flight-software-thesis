# Verification CI v1 Evidence

## Scope

This record captures implementation and verification evidence for the `verification-ci-v1` OpenSpec change.

## Environment

- Date: `2026-03-21`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Validation mode: repo-local CI script plus GitHub Actions workflow definition

## Implemented Artifacts

- GitHub Actions workflow at `.github/workflows/verification-ci.yml`
- Shared verification entrypoint at `scripts/run_verification_ci.sh`
- Reusable evidence template at `docs/test-records/templates/change-evidence-template.md`
- Updated narrative workflow and verification documents

## Commands Run

1. `bash scripts/run_verification_ci.sh $REPO_ROOT/build-artifacts/verification-ci-local`
2. `openspec validate verification-ci-v1`
3. `openspec validate --specs`

## Results

- The shared verification script completed successfully and produced per-step logs plus a markdown summary under `build-artifacts/verification-ci-local/`.
- The baseline gate covered `generate`, `build`, `generate --ut`, `build --ut`, `check --all`, and `openspec validate --specs`.
- The GitHub Actions workflow now delegates to the shared script instead of duplicating the gate inline.

## Verification Summary

- `run_verification_ci.sh`: passed
- Aggregate project UT gate during the script run: `100% tests passed, 0 tests failed out of 12`
- `openspec validate verification-ci-v1`: passed
- `openspec validate --specs`: passed `9/9`

## Notes

- The shared script is intentionally small and shell-based so developers can run exactly the same gate locally and in CI.
- The workflow provisions `zeromq`, Python, Node, the submodule tree, and the `openspec` CLI before delegating to the shared script.
- In the Codex desktop app, the local script needed one escalated rerun because socket- and PTY-based integration tests are restricted by the default sandbox; the same script then passed unchanged.
- OpenSpec validation still emits the known PostHog DNS flush warning in this local environment because `edge.openspec.dev` is unreachable; validation itself still completed successfully.

## Remaining Gaps

- `Deferred-RPi`: target-specific Raspberry Pi validation and any hardware-in-the-loop jobs remain outside the baseline CI workflow.
- `Blocked-HW`: CI cannot replace real hardware, cable, or power-interruption testing; those remain capability-specific evidence items.
