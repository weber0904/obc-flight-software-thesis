## Why

The matrix already treats `direct-control` as a separate capability family, but
today only hosted has a dedicated matrix-owned direct-control wrapper. The
target environments still rely on adjacent registered evidence rather than a
purpose-built matrix entrypoint.

This change closes that last separation-of-concerns gap.

## What Changes

- Add dedicated target TCP and target CAN `direct-control` matrix wrappers.
- Reuse the existing registered Raspberry Pi direct `OBC -> GDS` path instead
  of routing through southbound `node 5` or `node 6`.
- Record focused evidence and close the final target direct-control cells.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require reviewable target-side direct-control matrix
  proof that stays separate from satcom and southbound parity claims.

## Impact

- Affected code:
  - target direct-control wrappers
  - focused target direct-control evidence
  - umbrella dashboard updates
- Public/operator impact:
  - all three environments gain matrix-owned direct-control cases
- Non-goals:
  - no new satcom, failover, or UHF provenance claim
