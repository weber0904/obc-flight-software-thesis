## Why

The onboard data-products baseline writes official F' HK `.fdp` files and proves `DpCatalog` queueing, but repository evidence still stops short of proving GDS-received `.fdp` byte match and decode. Storage observability also scans the older HK, persistent, staging, and logs roots while leaving the new `data-products` mission-history root outside the storage-health baseline.

This change closes that parity gap before treating official `.fdp` products as the primary HK/state mission-history path. It keeps the existing `HousekeepingArchive` ring as a bounded transitional fallback and does not claim RF, CFDP, ARQ, or physical COMM `.fdp` parity.

## What Changes

- Extend storage-health observability to include `<runtime-root>/data-products/`, with root stats plus first-slice quota/watermark/observe-only retention status.
- Promote official HK trend `.fdp` products to the primary mission-history path by migrating `HkTrendRecord` to V2 under the same product record name and id.
- Keep `HousekeepingArchive` as a transitional forensic fallback and bump its binary file-format version for the extended storage-root record shape.
- Extend the hosted onboard-state-data probe to prove GDS-received `.fdp` byte match and decode against the current dictionary.
- Record evidence and path-registry updates that distinguish hosted `.fdp` parity from physical COMM, RF, CFDP, ARQ, or arbitrary file downlink claims.

## Capabilities

### New Capabilities

### Modified Capabilities

- `onboard-data-products-and-live-beacon`: Clarifies official HK `.fdp` as the primary mission-history path while leaving live beacon wire format unchanged.
- `hk-data-products`: Adds V2 HK trend product content and hosted received-file byte-match/decode parity criteria.
- `housekeeping-archive`: Keeps the HK ring as transitional fallback and updates its binary record version for the expanded storage schema.
- `resource-storage`: Extends governed runtime-root storage requirements for the `data-products` directory and observe-only policy fields.
- `storage-health`: Adds `data-products` root health, masks, telemetry, and root events.
- `verification-evidence`: Adds evidence requirements for hosted official `.fdp` byte-match/decode and explicit parity exclusions.

## Impact

- Affected components: `StorageHealthBridge`, `HkTrendProductProducer`, and the existing housekeeping archive implementation.
- Affected support logic: `StorageScanner`, storage-health cached state structs, HK trend FPP data types, hosted probe decode helpers, and archive serialization tests.
- Affected evidence: onboard data-products test record, verification-path registry, verification matrix, OpenSpec specs, and repository consistency surfaces.
- Explicitly out of scope: deleting `HousekeepingArchive`, changing `BeaconV1` wire format, physical COMM `.fdp` parity, RF validation, CFDP, ARQ/NACK, segment recovery, and arbitrary onboard file downlink.
