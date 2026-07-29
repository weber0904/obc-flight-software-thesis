## 1. Formalize and Validate Change Artifacts

- [x] 1.1 Create proposal, design, platform-baseline delta spec, verification-evidence delta spec, and tasks for `hosted-obc-runtime-maintainability-v1`.
- [x] 1.2 Run `openspec validate hosted-obc-runtime-maintainability-v1` before implementation.

## 2. Refactor Hosted Runtime Shape

- [x] 2.1 Add `OBC/Runtime` as shared `OBC_Runtime` helper module with common runtime config/state, parser helpers, status/help formatting, command dispatch, and runtime loop support.
- [x] 2.2 Replace duplicated default `OBC` command dispatch with the shared runtime helper through an `OBCApp` adapter while preserving startup/status output markers.
- [x] 2.3 Replace duplicated `OBC_CcsdsGroundLinkSpike` command dispatch with the shared runtime helper through an `OBCAppCcsds` adapter while preserving CCSDS spike topology output markers.
- [x] 2.4 Update build registration so both deployments depend on `OBC_Runtime` plus their existing topology modules.

## 3. Add Focused Helper Tests

- [x] 3.1 Add `hosted_runtime_unit_test` coverage for command routing, `quit`/`exit`, unknown commands, parser failure messages, help text, and valid launch argument parsing.
- [x] 3.2 Add launch parser coverage for malformed numeric arguments and invalid runtime configuration values.

## 4. Verify Behavior Preservation

- [x] 4.1 Run native generate/build and UT generate/build through the project `fprime-venv`.
- [x] 4.2 Run `hosted_runtime_unit_test` and any affected focused helper tests.
- [x] 4.3 Run the full local verification gate with `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-hosted-obc-runtime-maintainability-v1`.
- [x] 4.4 Rerun focused hosted probes after the fresh build: mode-model v2, S-band TCP ground link, UHF UART backup link, and CCSDS ground link spike.
- [x] 4.5 Run `openspec validate hosted-obc-runtime-maintainability-v1` and `openspec validate --specs`.

## 5. Close Out

- [x] 5.1 Add `docs/test-records/hosted-obc-runtime-maintainability-v1/README.md` with behavior-preservation evidence and reused path boundaries.
- [x] 5.2 Archive the OpenSpec change through the governed archive flow after evidence and validation pass.
- [x] 5.3 Update pending management notes for completed status without adding `pending/` to the PR.
- [x] 5.4 Prepare a reviewable local git boundary with Conventional Commit title `refactor(runtime): share hosted OBC command dispatch` and do not push.
