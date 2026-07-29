## Why

The new verification matrix made the project's current testing debt explicit: several later-phase slices are verified primarily by integration tests and hosted probes, but still lack smaller automated tests around their public contracts and logic seams. This change backfills the highest-value automated tests now, while hardware-dependent work is paused, so future development inherits a more balanced baseline.

## What Changes

- Add targeted automated tests for the highest-priority weak spots called out by the verification matrix.
- Backfill contract-level tests for `GpsBridge` and `StorageHealthBridge` without forcing them into the older classic F' tester shape.
- Add small automated tests for `HousekeepingSnapshotProvider` aggregation behavior and `HousekeepingArchiveStore` logic seams.
- Add a dedicated unit test for `TransparentLinkFraming`, since that logic already has a clean standalone API.
- Update the verification matrix and inventory tooling so the repository reports the new coverage accurately.

## Capabilities

### New Capabilities

### Modified Capabilities
- `verification-evidence`: record the targeted UT/L2 backfill policy for weak slices, and allow logic/contract tests as valid backfill shapes when a classic component harness would be disproportionate

## Impact

- Affected code:
  - `OBC/Components/GpsBridge/`
  - `OBC/Components/StorageHealthBridge/`
  - `OBC/Components/HousekeepingArchive/`
  - `OBC/Top/`
  - `simulators/comm/`
  - `scripts/report_verification_inventory.py`
  - `docs/verification-matrix.md`
- Affected systems:
  - native UT/IT build and baseline verification gate
  - verification documentation and test evidence
