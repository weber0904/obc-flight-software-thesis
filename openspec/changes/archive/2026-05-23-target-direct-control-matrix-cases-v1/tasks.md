## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `target-direct-control-matrix-cases-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-direct-control-matrix-cases-v1`.

## 2. Target Direct-Control Wrappers

- [x] 2.1 Add a dedicated target TCP `direct-control` wrapper that reuses the
  registered target direct `OBC -> GDS` path.
- [x] 2.2 Add a dedicated target CAN `direct-control` wrapper on the same
  contract.
- [x] 2.3 Keep the wrappers isolated from southbound parity launchers and
  satcom-specific helpers.

## 3. Evidence And Dashboard

- [x] 3.1 Record focused target direct-control evidence.
- [x] 3.2 Update the umbrella matrix dashboard and registry for the passing
  target direct-control cells only.

## 4. Verification

- [x] 4.1 Run touched script syntax and helper checks.
- [x] 4.2 Run target TCP and target CAN `direct-control` to passing evidence.
- [x] 4.3 Confirm the wrappers do not leave repo-root residue.
- [x] 4.4 Run `openspec validate target-direct-control-matrix-cases-v1` and
  `openspec validate --specs`.
