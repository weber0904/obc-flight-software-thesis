## 1. OpenSpec And Documentation Scaffolding

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `formal-comm-verification-matrix-v1`.
- [x] 1.2 Add a new operator runbook for the comm-verification suite and a
  scaffold evidence record under
  `docs/test-records/comm-link-revalidation-v1/`.
- [x] 1.3 Validate the change artifacts with
  `openspec validate formal-comm-verification-matrix-v1`.

## 2. Suite Subtree

- [x] 2.1 Add `scripts/comm_verification/lib/` helpers for artifact layout,
  case execution, and summary rendering.
- [x] 2.2 Add `scripts/comm_verification/env/` definitions for `hosted`,
  `rpi_tcp`, and `rpi_can`.
- [x] 2.3 Add `scripts/comm_verification/matrix/` runners for one environment,
  all environments, and cleanup smoke.
- [x] 2.4 Add thin root wrapper scripts in `scripts/` that delegate into the
  new subtree.

## 3. Capability Case Wrappers

- [x] 3.1 Add governed case wrappers for:
  - `direct-control`
  - `sband-command`
  - `sband-file`
  - `sband-sequence-subsystem`
  - `uhf-primary-command`
  - `uhf-primary-file`
  - `uhf-primary-sequence-subsystem`
  - `failover-command`
  - `csp-reachability`
- [x] 3.2 Reuse existing governed probe entrypoints where they already prove
  the required path and record explicit blockers where a dedicated matrix cell
  still lacks a trustworthy harness.
- [x] 3.3 Ensure each case writes isolated artifacts and machine-readable
  metadata, including carrier kind and blocker classification.

## 4. Verification

- [x] 4.1 Run shell syntax checks and Python compile checks for the new suite.
- [x] 4.2 Run the new cleanup smoke script or a bounded equivalent for the new
  matrix wrappers where the local environment allows it.
- [x] 4.3 Run `openspec validate formal-comm-verification-matrix-v1` and
  `openspec validate --specs`.

## 5. Follow-Up Runtime Matrix

- [x] 5.1 Record hosted, target TCP, and target CAN matrix execution results in
  `docs/test-records/comm-link-revalidation-v1/README.md`.
- [x] 5.2 Update `docs/verification-path-registry.md` only for matrix cells
  that finish with passing governed evidence.
- [x] 5.3 Keep the umbrella matrix dashboard aligned with the follow-on change
  train:
  - `comm-verification-matrix-foundation-v1`
  - `comm-verification-sequence-subsystem-harness-v1`
  - `target-can-node6-matrix-closure-v1`
  - `target-tcp-southbound-parity-foundation-v1`
  - `target-tcp-matrix-closure-v1`
  - `target-direct-control-matrix-cases-v1`
