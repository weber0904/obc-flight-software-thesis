## Why

Both `sband-sequence-subsystem` and `uhf-primary-sequence-subsystem` remain
blocked because the matrix does not yet own a dedicated official sequencing
harness with the right oracle. The repository already has governed sequencing
proofs, but they are broader than the matrix cell needs and are not packaged as
reusable same-path subsystem-round-trip helpers.

This change extracts that shared sequencing harness and uses it to close the
hosted sequence cells first.

## What Changes

- Extract a shared official sequencing helper from the existing governed
  sequencing probe.
- Fix the sequence shape and oracle for matrix-owned sequence-subsystem cases.
- Close hosted `sband-sequence-subsystem` and hosted
  `uhf-primary-sequence-subsystem`.
- Leave the helper reusable for target CAN and target TCP follow-on changes.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require a reviewable official-sequencing proof for
  matrix-owned same-path subsystem round-trip cases.

## Impact

- Affected code:
  - shared sequencing helper under `scripts/comm_verification/`
  - hosted sequence-subsystem case wrappers
  - focused evidence for hosted sequence matrix cells
- Public/operator impact:
  - hosted sequence-subsystem matrix cells become governed and reviewable
- Non-goals:
  - no target CAN or target TCP sequence closure yet
