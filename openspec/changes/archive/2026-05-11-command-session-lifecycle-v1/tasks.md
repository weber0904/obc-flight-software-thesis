## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `command-session-lifecycle-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate command-session-lifecycle-v1`.

## 2. Runtime Session Lifecycle

- [x] 2.1 Add dictionary-visible `SESSION_OPEN()` command support on `CommandIngressAuthority` and reject legacy non-envelope lifecycle use before `CmdDispatcher`.
- [x] 2.2 Add active session lifecycle state owned by `CommandIngressAuthority` for one active session per source epoch.
- [x] 2.3 Require `SESSION_OPEN` envelopes to use `sequence_number = 0`, accept fresh open on unopened source epochs, reject same-session reopen, and replace prior sessions on fresh `session_id`.
- [x] 2.4 Require non-lifecycle enveloped commands to match an active session before sequence evaluation or dispatch.
- [x] 2.5 Reuse the existing sequence helper for post-open strict-monotonic checks and clear prior session sequence state on replace/reset.
- [x] 2.6 Add dedicated session lifecycle events, rejection reasons, and bounded telemetry without changing the envelope wire shape.
- [x] 2.7 Keep `uhf-backup` able to open a session and continue enveloped read/status traffic without broadening its authority allowlist.

## 3. Tests And Hosted Probe

- [x] 3.1 Extend classic component UTs for unopened-command rejection, accepted open, increasing sequence after open, replace/resync, same-session reopen rejection, legacy direct lifecycle rejection, and reboot-memory-clear behavior.
- [x] 3.2 Keep helper tests green and extend them only where lifecycle-owned source/session reset behavior changes the helper contract.
- [x] 3.3 Add `scripts/run_command_session_lifecycle_probe.sh`.
- [x] 3.4 Cover hosted `sband-primary` open-then-accept, desync recovery by fresh open, and reboot/runtime-reset recovery.
- [x] 3.5 Cover hosted `uhf-backup` open plus read/status acceptance while mode-change remains authority denied and does not consume lifecycle state.
- [x] 3.6 Rerun and keep green the existing command authority, envelope metadata, and session sequence probes as regressions.

## 4. Evidence And Canonical Docs

- [x] 4.1 Add `evidence/records/command-session-lifecycle-v1/README.md`.
- [x] 4.2 Update `evidence/verification-path-registry.md` for the hosted command ingress authority profile path lifecycle boundary.
- [x] 4.3 Update `docs/roadmap/README.md` and `docs/roadmap/06-openspec-change-breakdown.md` to move the next recommended command/security change to `command-auth-envelope-v1`.
- [x] 4.4 Update `docs/architecture/current-development-architecture.md` for the new active session lifecycle baseline truth.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification build/gate with `bash scripts/run_verification_ci.sh <output-dir>`.
- [x] 5.2 Run focused affected tests and the repository-owned hosted probes after the fresh build.
- [x] 5.3 Run `openspec validate command-session-lifecycle-v1` and `openspec validate --specs`.
- [x] 5.4 Archive the change with `openspec archive command-session-lifecycle-v1 --yes`.
- [x] 5.5 Finish post-archive reconciliation updates and keep the worktree review-ready without pushing before approval.
