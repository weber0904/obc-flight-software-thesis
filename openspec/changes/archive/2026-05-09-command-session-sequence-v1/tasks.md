## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, verification-path-registry delta spec, and tasks for `command-session-sequence-v1`.
- [x] 1.2 Validate with `openspec validate command-session-sequence-v1`.

## 2. Runtime Sequence Enforcement

- [x] 2.1 Add sequence rejection reason/response mapping near `CommandIngressAuthority`.
- [x] 2.2 Add owned `CommandSequenceWindow` state to `CommandIngressAuthority`.
- [x] 2.3 Evaluate sequence only after envelope parse, metadata observation, and authority allow.
- [x] 2.4 Reject duplicate/lower/wraparound/table-full sequence results before `CmdDispatcher` with exactly one synthetic response.
- [x] 2.5 Add dedicated sequence rejection event and bounded telemetry.
- [x] 2.6 Preserve legacy non-envelope command behavior and existing authority-denied behavior.

## 3. Tests And Hosted Probe

- [x] 3.1 Extend classic component UTs for first/increasing sequence acceptance, duplicate/lower/wraparound rejection, per-key independence, and synthetic response behavior.
- [x] 3.2 Cover authority-denied envelopes not consuming sequence state.
- [x] 3.3 Cover sequence-window-full mapping to `EXECUTION_ERROR`.
- [x] 3.4 Add or update helper tests only if a new helper mapping is introduced.
- [x] 3.5 Add `scripts/run_command_session_sequence_probe.sh`.
- [x] 3.6 Keep existing command ingress authority and command envelope metadata probes passing.

## 4. Evidence And Verification

- [x] 4.1 Run affected helper/component tests and catalog check.
- [x] 4.2 Run focused hosted command authority, envelope metadata, and session sequence probes after a fresh build.
- [x] 4.3 Add `docs/test-records/command-session-sequence-v1/README.md`.
- [x] 4.4 Update `docs/verification-path-registry.md` for the active sequence enforcement evidence boundary.
- [x] 4.5 Run `openspec validate command-session-sequence-v1`, `openspec validate --specs`, and `python3 scripts/check_repo_consistency.py`.
