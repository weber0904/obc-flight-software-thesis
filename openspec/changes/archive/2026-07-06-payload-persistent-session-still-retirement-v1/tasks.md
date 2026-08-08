## 1. Change artifacts and spec deltas

- [x] 1.1 Add proposal, design, tasks, and delta specs for payload operations, verification evidence, and verification-path registry.
- [x] 1.2 Update the change artifacts so Phase 1 persistent session and Phase 2 `STILL` retirement are explicitly separated.

## 2. Phase 1 runtime and proof implementation

- [x] 2.1 Refactor payload runtime and driver lifecycle so `PAYLOAD_PREPARE` establishes a persistent warm non-RAW session and capture reuses it until shutdown/abort/mode-exit.
- [x] 2.2 Freeze session-level mismatch behavior to reject instead of silently re-preparing, while preserving the special `RAW_SENSOR` gate.
- [x] 2.3 Update hosted stub/helper-backed semantics and add focused controller/manager/backend tests for persistent-session behavior.
- [x] 2.4 Add or update the governed hosted and target payload proofs for `AUTO -> metadata -> DETERMINISTIC` on one shared persistent non-RAW session.

## 3. Phase 2 `STILL` retirement

- [x] 3.1 Remove `PAYLOAD_CAPTURE_STILL` from the current public payload surface and migrate maintained code/tests/scripts to the deterministic family.
- [x] 3.2 Update current docs, records, and registry entries so only `AUTO` and `DETERMINISTIC` remain as maintained normal still-capture policies.
- [x] 3.3 Preserve historical records with current-note retirement guidance instead of rewriting their original evidence text.

## 4. Verification and local-ready closeout

- [x] 4.1 Run fresh focused payload builds/tests plus fresh hosted semantic proof and governed target persistent-session proof.
- [x] 4.2 Update the new dedicated evidence record and any adjacent current-note references.
- [x] 4.3 Run `openspec validate payload-persistent-session-still-retirement-v1`, `openspec validate --specs`, repo consistency/documentation governance checks, and archive the change.
- [x] 4.4 Reconcile post-archive docs/specs/evidence, confirm a clean worktree, and reach `local-ready`.
