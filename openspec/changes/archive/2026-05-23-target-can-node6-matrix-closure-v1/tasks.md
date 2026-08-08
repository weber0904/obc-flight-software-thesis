## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `target-can-node6-matrix-closure-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-can-node6-matrix-closure-v1`.

## 2. Shared Target CAN Harness

- [x] 2.1 Extract the target helper logic needed for matrix-owned target CAN
  sequence, file, and failover cases from the current governed target probe.
- [x] 2.2 Preserve bounded physical-UART preflight and quiet-UHF acceptance in
  the extracted helper contract.

## 3. Target CAN Matrix Cells

- [x] 3.1 Implement target CAN `sband-sequence-subsystem`.
- [x] 3.2 Implement target CAN `uhf-primary-file`.
- [x] 3.3 Implement target CAN `uhf-primary-sequence-subsystem`.
- [x] 3.4 Implement target CAN `failover-command`.

## 4. Verification

- [x] 4.1 Run touched script syntax and target helper checks.
- [x] 4.2 Run the target CAN matrix cells to passing governed evidence.
- [x] 4.3 Update focused evidence and the umbrella matrix dashboard for the
  passing cells only.
- [x] 4.4 Run `openspec validate target-can-node6-matrix-closure-v1` and
  `openspec validate --specs`.

## Notes

- The branch-local diagnostics added in this continuation step are retained to
  keep later physical-UHF regressions reviewable without widening the quiet-UHF
  claim boundary.
