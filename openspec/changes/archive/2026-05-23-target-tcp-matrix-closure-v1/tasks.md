## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `target-tcp-matrix-closure-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-tcp-matrix-closure-v1`.

## 2. Target TCP High-Level Cases

- [x] 2.1 Implement target TCP `sband-sequence-subsystem` by reusing the shared
  sequencing helper on the parity launcher.
- [x] 2.2 Implement target TCP `uhf-primary-sequence-subsystem` on the same
  foundation.
- [x] 2.3 Implement target TCP `failover-command` with explicit S-band loss,
  UHF primary switch, session reopen, and command/readback closure.

## 3. Evidence And Dashboard

- [x] 3.1 Record focused target TCP evidence for the passing cells.
- [x] 3.2 Update the umbrella matrix dashboard and registry only for the cells
  that now have passing governed evidence.

## 4. Verification

- [x] 4.1 Run touched script syntax and helper checks.
- [x] 4.2 Run the target TCP sequence and failover cells to passing evidence.
- [x] 4.3 Run `openspec validate target-tcp-matrix-closure-v1` and
  `openspec validate --specs`.
