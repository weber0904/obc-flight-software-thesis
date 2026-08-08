## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `comm-verification-sequence-subsystem-harness-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate comm-verification-sequence-subsystem-harness-v1`.

## 2. Shared Sequencing Helper

- [x] 2.1 Extract a shared official sequencing helper from the existing
  governed sequencing probe instead of duplicating sequence control logic.
- [x] 2.2 Fix the matrix sequence shape and oracle for same-path subsystem
  round-trip proof.

## 3. Hosted Matrix Closure

- [x] 3.1 Implement hosted `sband-sequence-subsystem` on top of the shared
  helper.
- [x] 3.2 Implement hosted `uhf-primary-sequence-subsystem` on top of the same
  helper.
- [x] 3.3 Record focused hosted evidence and update the umbrella dashboard for
  the hosted sequence cells.

## 4. Verification

- [x] 4.1 Run shell syntax, Python compile, and touched hosted probe checks.
- [x] 4.2 Run the hosted sequence-subsystem matrix cells to passing evidence.
- [x] 4.3 Run `openspec validate comm-verification-sequence-subsystem-harness-v1`
  and `openspec validate --specs`.
