## Overview

This change backfills automated tests only; it does not change flight behavior or subsystem requirements. The implementation strategy is to add the smallest durable automated tests that cover public contracts or pure logic seams for the slices already identified as weak by the verification matrix.

## Test Shape Strategy

### Prefer the smallest stable seam

This repository already uses mixed verification styles:

- classic F' component testers for early controller-style components
- small standalone logic tests
- integration tests
- hosted probes and hardware evidence

For the targeted weak slices in this change, the smallest stable seams are:

- `GpsBridge`: public runtime/test hooks (`setSentenceSourceForTest`, `pollStateForTest`, `getCachedStateForRuntime`, `setSourceModeForRuntime`)
- `StorageHealthBridge`: public runtime/test hooks (`configureRuntime`, `scanNowForTest`, `getCachedStateForRuntime`)
- `HousekeepingSnapshotProvider`: aggregation behavior against existing runtime getter surfaces
- `HousekeepingArchiveStore`: already-exposed pure/store logic
- `TransparentLinkFraming`: standalone encode/decode API

### Do not force classic harness adoption

This change intentionally does **not** require every later slice to adopt `register_fprime_ut()` just to look consistent. If a smaller contract or logic test is clearer and cheaper to maintain, that is the preferred backfill shape.

## Planned Backfill Set

### `GpsBridge`

Add a contract test that verifies:

- valid sentence updates cached state
- no-fix sentence clears fix state but preserves sample visibility
- malformed/rejected sentence increments the reject counter
- source mode switching rejects unavailable replay mode and keeps the runtime state coherent

### `StorageHealthBridge`

Add a contract test that verifies:

- scan fails cleanly before configuration
- configured scans increment scan counters and expose warning/degraded state
- repeated scans accumulate scan count and scan error count as designed

### `HousekeepingSnapshotProvider`

Add a small aggregation test that verifies:

- capture fails when required providers are absent
- capture succeeds with the required runtime providers configured
- optional GPS and storage state are included when available
- representative mode/comm/boot fields are propagated into the snapshot

### `HousekeepingArchiveStore`

Add a small logic test that verifies:

- initialization fails cleanly when not configured
- invalid index content is rejected with `INDEX_INVALID`
- missing slot or stale generation requests remain rejected as documented

### `TransparentLinkFraming`

Add a standalone unit test that verifies:

- encode/decode round-trip
- escaping of delimiter and escape bytes
- length mismatch / CRC mismatch / bad escape are surfaced by the standalone API

## Reporting And Matrix Updates

The verification inventory tool will be updated to classify `*_contract_test` as L2 and `*_unit_test` as L1 so the checked-in matrix can reflect the new backfill accurately. The matrix will then be updated to show the new coverage and to shrink the weak-spot list to only the remaining constrained areas.
