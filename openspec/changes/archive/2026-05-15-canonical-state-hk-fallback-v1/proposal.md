## Why

The current runtime still carries two overlapping state-projection paths:
`OnboardStateSnapshotSource` for official `.fdp` / reduced-state consumers and
`HousekeepingSnapshotProvider` for the transitional HK ring. That duplication
keeps `HousekeepingArchive`, `hk/index.csv`, HK downlink commands, storage-health
HK-root observability, and shared COMM downlink arbitration alive as active
baseline behavior even though official `.fdp` products are already the primary
history path.

This change removes that split. `OnboardStateSnapshotSource` and
`StateSnapshot` remain the single canonical runtime state surface, and the
transitional HK fallback is fully retired instead of being preserved as a
second active history plane or repurposed into the future persistent fault
ring.

## What Changes

- Keep `OnboardStateSnapshotSource` + `StateSnapshot` as the only active
  runtime state projection boundary for `.fdp`, reduced-state, and beacon
  consumers.
- Remove `HousekeepingArchive`, `HousekeepingSnapshotProvider`, the `HK_*`
  command surface, `hk/index.csv`, HK slot files, and HK-owned shared-downlink
  arbitration from the active baseline.
- Migrate `HkTrendRecord` to a V5 payload while preserving product record name
  and id, and remove HK-root fields from the official `.fdp` surface.
- Retire HK-specific public storage-health contract elements, including the
  `HK` root kind, `hkIndexExists`, HK root telemetry/state fields, and the old
  root-mask numbering.
- Update hosted runtime status, probes, specs, roadmap/current-truth docs, and
  current evidence to treat official `.fdp` products plus `DpCatalog` as the
  only current mission-history path.

## Capabilities

### Modified Capabilities

- `housekeeping-archive`: retire the active HK ring capability and operator
  surface from the current baseline.
- `onboard-data-products-and-live-beacon`: keep the canonical snapshot source,
  advance `HkTrendRecord` to V5, and remove the statement that HK fallback
  remains active.
- `storage-health`: reduce governed root scope to `persistent-data`, `staging`,
  `logs`, and `data-products`, and redefine root-kind/mask semantics
  accordingly.
- `verification-evidence`: update current evidence expectations so official
  `.fdp` / `DpCatalog` is the only current history path.

## Impact

- Affected code: topology wiring, state/data-product paths, storage-health,
  COMM downlink arbitration, hosted runtime status output, probes, and tests.
- Affected public contracts: `StorageRootKind`, storage warning/degraded mask
  semantics, `HkTrendRecord` payload version, and operator-facing history-path
  commands.
- Explicitly out of scope: persistent fault/event ring, recovery/fault summary
  fields, manual `.fdp` capture trigger, replay/boot hardening, payload ops,
  and target-recovery closure.
