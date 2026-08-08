## Why

The first onboard state data PR proved a useful separation between cached subsystem state, reduced state, and beacon formatting, but it also introduced a repo-local data-product archive and catalog shim. The later WIP corrected the storage direction toward official F' data products and live beacon broadcast, but its branch name and OpenSpec boundary no longer described the full scope.

This replacement change establishes one reviewable baseline for onboard state monitoring, live beacon generation, and official F' housekeeping trend data products. It keeps autonomy control, RF claims, CFDP/ARQ, and the older housekeeping ring archive out of scope.

## What Changes

- Add `OnboardStateMonitor` and shared `OnboardStateData` helpers that reduce existing subsystem cached state into `ReducedStateV1`.
- Emit a fixed `BeaconV1` frame every 17 scheduler ticks through a COMM-facing `IBeaconSink`, with CRC-protected payload fields and no ground ACK requirement.
- Add `HkTrendProductProducer` that creates official F' HK trend data products through product get/send ports, `DpManager`, `DpWriter`, and `DpCatalog`.
- Wire official data product components into the OBC topology under `<runtime-root>/data-products/` while preserving `ComFprime`, command, event, telemetry, `FileHandling.fileDownlink`, and `HousekeepingArchive`.
- Remove `StateDataArchive`, `ProductCatalog`, OPD1 product files, `catalog.csv`, and mission `BEACON_HISTORY` from the formal baseline.
- Fix applicable PR #41 review feedback around stale reduced-state invalidation, beacon sequence handling, not-configured reporting, ring buffer indexing, and centralized CRC32.

## Capabilities

### New Capabilities

- `onboard-data-products-and-live-beacon`: Defines the onboard reduced-state, live beacon, and official HK trend data-product baseline.
- `live-beacon-broadcast`: Defines the COMM-facing no-ACK beacon payload and debug decode evidence boundary.
- `hk-data-products`: Defines official F' HK trend product generation, storage, catalog, and downlink queueing.

### Modified Capabilities

- `resource-storage`: Adds governed runtime-root storage for official F' data product files and catalog state.
- `comm-subsystem`: Adds the COMM-facing live beacon broadcast sink as distinct from TT&C, RF, and reliable file transfer.
- `verification-evidence`: Adds evidence requirements for reduced-state behavior, live beacon decode, official data-product generation/catalog behavior, and explicit exclusions.

## Impact

- Affected flight components: `OnboardStateMonitor`, `BeaconPublisher`, `HkTrendProductProducer`, `UartDriver`, and OBC topology wiring.
- Affected support logic: onboard state helper types, beacon encode/decode, CRC helper, snapshot source, hosted probe, and debug decode tooling.
- Affected verification: L1 helper tests, classic F' L2 component harnesses, hosted probe evidence, OpenSpec validation, verification matrix, and path registry.
- Explicitly out of scope: `MissionExecutive` autonomy actions, LOW_POWER, DETUMBLE, load shedding, RF validation, CFDP, ARQ, CCSDS migration, target-side SD claims, and replacing `HousekeepingArchive`.
