## Why

Once the target TCP parity launcher exists, the remaining blocked cells are no
longer topology problems. They become capability problems: official sequencing
over the same path and command continuity after failover from S-band to UHF
primary.

This change closes those remaining high-level target TCP cells.

## What Changes

- Reuse the target TCP parity foundation to close
  `sband-sequence-subsystem`, `uhf-primary-sequence-subsystem`, and
  `failover-command`.
- Reuse the shared official sequencing helper instead of inventing a TCP-only
  sequence path.
- Record focused target TCP evidence and update the umbrella dashboard.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require reviewable target TCP same-path sequencing
  and failover continuity evidence on the governed parity topology.

## Impact

- Affected code:
  - target TCP sequence and failover case wrappers
  - reuse of the parity launcher and shared sequencing helper
  - focused evidence and dashboard updates
- Public/operator impact:
  - target TCP matrix moves from partial parity to full satcom capability proof
- Non-goals:
  - no target direct-control case yet
