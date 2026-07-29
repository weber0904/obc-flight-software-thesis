## 1. OpenSpec Contracts

- [x] 1.1 Add proposal, design, tasks, and delta specs for HK retirement, V5
  official `.fdp`, and four-root storage health.
- [x] 1.2 Validate the change artifacts before implementation proceeds.

## 2. Remove HK Runtime Surface

- [x] 2.1 Remove `HousekeepingArchive`, `HousekeepingSnapshotProvider`, and HK
  wiring from `TopCcsds`, legacy `Top`, and related build registrations.
- [x] 2.2 Remove `HK_*` commands, HK authority catalog/policy entries, and HK
  shared-downlink owner logic from COMM downlink scheduling.
- [x] 2.3 Delete HK-specific tests and probe expectations that only exist for
  the retired fallback path.

## 3. Keep Canonical State On `StateSnapshot`

- [x] 3.1 Keep `OnboardStateSnapshotSource` as the single runtime snapshot
  source for `OnboardStateMonitor` and `HkTrendProductProducer`.
- [x] 3.2 Update affected state/data-path tests so they no longer expect a
  second HK snapshot surface.

## 4. Rebase Storage Health And Official FDP

- [x] 4.1 Update storage-health types, root enum, masks, telemetry, scanner,
  and runtime status to the four-root contract.
- [x] 4.2 Migrate `HkTrendRecord` to V5 while preserving record name/id and
  removing HK-root fields from the official `.fdp` payload.
- [x] 4.3 Update UT coverage for storage-health, state snapshot, HK trend, and
  COMM downlink scheduling to match the new public contract.

## 5. Current Truth, Evidence, And Verification

- [x] 5.1 Rewrite or retire hosted probes and current evidence so official
  `.fdp` / `DpCatalog` is the only current history path.
- [x] 5.2 Update current specs, architecture/roadmap docs, verification
  summaries, and operator/runbook truth for HK retirement and V5 official `.fdp`.
- [x] 5.3 Run full verification, `openspec validate canonical-state-hk-fallback-v1`,
  and `openspec validate --specs`, then mark completed tasks.
