## Why

The repository now has an explicit governance rule that every real F' component must have a classic L2 harness. Three later true components still fall short of that baseline: `GpsBridge`, `StorageHealthBridge`, and `HousekeepingArchive`.

## What Changes

- Add classic F' tester harnesses for `GpsBridge`, `StorageHealthBridge`, and `HousekeepingArchive`.
- Preserve the existing helper tests and integration tests for those slices.
- Use the harnesses to cover the minimum command, telemetry, event, scheduler, and output-port behaviors that were previously only exercised indirectly.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `verification-evidence`: record the classic component-harness backfill for the three remaining later-phase real components while preserving the helper-test layer.

## Impact

- Affected code: `OBC/Components/GpsBridge/`, `OBC/Components/StorageHealthBridge/`, and `OBC/Components/HousekeepingArchive/`.
- Affected systems: native UT build, verification matrix reporting, and baseline gate expectations.
- No change to flight runtime semantics, simulator protocols, or target integration behavior.
