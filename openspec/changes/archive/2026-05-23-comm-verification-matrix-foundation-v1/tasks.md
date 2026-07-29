## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `comm-verification-matrix-foundation-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate comm-verification-matrix-foundation-v1`.

## 2. Matrix Foundation Hardening

- [x] 2.1 Normalize `result.meta` writing so required case fields are always
  present and malformed metadata becomes a harness bug.
- [x] 2.2 Normalize summary rendering and carrier-kind naming across hosted,
  target TCP, and target CAN outputs.
- [x] 2.3 Make case-wrapper intent explicit for reused probes, narrowed probes,
  and bounded blockers.

## 3. Cleanup And Documentation

- [x] 3.1 Harden the matrix cleanup smoke and immediate-rerun contract for the
  suite wrappers touched by this change.
- [x] 3.2 Update the umbrella matrix dashboard or runbook references where the
  strengthened foundation contract needs to be explained.

## 4. Verification

- [x] 4.1 Run shell syntax and Python compile checks for touched suite files.
- [x] 4.2 Run the cleanup smoke or a bounded equivalent for the touched matrix
  wrappers.
- [x] 4.3 Run `openspec validate comm-verification-matrix-foundation-v1` and
  `openspec validate --specs`.
