# hk-data-products Specification

## Purpose
Define the official F' housekeeping trend data-product path for storing queryable onboard state history without mixing it with the legacy housekeeping ring archive.
## Requirements
### Requirement: Official HK Trend Data Product Generation
The system SHALL generate housekeeping trend records using official F' data
product producer ports, `DpManager`, and `DpWriter`.

#### Scenario: HK trend cadence appends one sample to the pending chunk
- **WHEN** the HK trend producer scheduler reaches the 30-tick cadence boundary
  and a state snapshot is available
- **THEN** the producer SHALL capture exactly one HK trend sample with the
  current per-sample schema version
- **AND** it SHALL append that sample to an in-memory pending chunk unless the
  append would exceed the configured final `.fdp` size target

#### Scenario: HK trend flush emits one finalized official data product file
- **WHEN** the HK trend producer flushes a non-empty pending chunk because of a
  size threshold, manual flush, or target-size shrink
- **THEN** the producer SHALL request one official F' data product container
  from `DpManager`
- **AND** it SHALL serialize one `HkTrendChunkMeta` record and one array
  `HkTrendRecord` record into that container
- **AND** it SHALL send the filled container so `DpWriter` writes one official
  data product file

#### Scenario: HK trend buffer unavailable
- **WHEN** `DpManager` cannot provide a valid data product buffer for a pending
  HK chunk flush
- **THEN** the HK trend producer SHALL report the failure through events and
  telemetry
- **AND** it SHALL NOT report a successful product write

#### Scenario: HK trend retained backlog remains bounded during repeated flush failure
- **WHEN** repeated HK chunk flush attempts keep failing while scheduled
  captures continue
- **THEN** the producer SHALL bound retained pending samples to at most two
  max-sized official HK chunks worth of samples
- **AND** once that bound is reached it SHALL reject newly captured samples
  instead of growing memory without bound
- **AND** it SHALL report the rejection through existing HK trend
  event/telemetry surfaces

### Requirement: HK Trend Records Actual Subsystem Values
The HK trend product record SHALL store actual or near-actual cached subsystem measurements with explicit validity flags, not only reduced health masks or threshold-derived state.

#### Scenario: HK trend includes subsystem measurements
- **WHEN** the HK trend producer serializes a nominal record
- **THEN** the record SHALL include at minimum EPS battery voltage, current, state-of-charge, and temperature; ADCS mode and angular-rate norm; GPS validity and sentence counters; storage warning/degraded state; COMM pass and packet counters; boot slot; mode; timestamp; and uptime

#### Scenario: Missing subsystem values remain explicit
- **WHEN** one subsystem snapshot is missing during HK trend capture
- **THEN** the record SHALL set that subsystem validity flag to false
- **AND** it SHALL NOT silently present default numeric values as known real measurements

### Requirement: Official HK Product Catalog And Downlink
The system SHALL use official `DpCatalog` to build the catalog of HK trend data product files and to start downlink through the existing `FileHandling.fileDownlink` path.

#### Scenario: Catalog build finds HK trend products
- **WHEN** one or more HK trend official data product files exist under the governed runtime data-products directory
- **THEN** `DpCatalog.BUILD_CATALOG` SHALL include those products in its pending catalog state

#### Scenario: Catalog transmission queues file downlink
- **WHEN** `DpCatalog.START_XMIT_CATALOG` runs after a successful catalog build
- **THEN** `DpCatalog` SHALL queue pending HK trend product files through `FileHandling.fileDownlink`

### Requirement: HK Trend Data-Products Storage Root
The HK trend official F' data product SHALL include the data-products storage
root while preserving the `HkTrendRecord` product record name and id across
payload version bumps.

#### Scenario: Data-products storage root is generated from cached state
- **WHEN** `HkTrendProductProducer` captures a state snapshot
- **THEN** each captured HK sample SHALL serialize as `HkTrendRecord` id `0`
  using the current HK trend payload version
- **AND** the sample payload SHALL include the existing HK trend fields plus
  data-products root existence, scan status, file count, bytes, error code,
  quota bytes, watermark bytes, quota status, and retention status
- **AND** it SHALL use the existing cached snapshot source rather than polling
  subsystem transports directly

#### Scenario: Finalized HK chunk carries self-describing metadata
- **WHEN** the HK trend producer emits one finalized official `.fdp`
- **THEN** that file SHALL include `HkTrendChunkMeta` record id `1`
- **AND** the metadata SHALL identify the chunk sequence, first and last sample
  sequence, sample count, configured target bytes, and flush reason

#### Scenario: Historical backward decode is not claimed
- **WHEN** reviewers evaluate this change
- **THEN** the evidence SHALL treat `.fdp` files generated by the current
  dictionary as the decode target
- **AND** it SHALL NOT claim that old V1, V2, or V3 `.fdp` files decode with
  the current dictionary unless separate compatibility evidence exists

### Requirement: Hosted FDP Received File Parity
The hosted data-product path SHALL prove that a generated HK `.fdp` can be
downlinked to GDS with byte-for-byte fidelity and decoded.

#### Scenario: Hosted GDS receives byte-matched FDP
- **WHEN** the hosted onboard-state-data probe runs after a fresh build
- **THEN** it SHALL wait until at least one HK sample is pending, issue
  `HK_TREND_FLUSH`, and generate one or more HK trend `.fdp` files under
  `<runtime-root>/data-products/`
- **AND** it SHALL build the `DpCatalog` catalog and start catalog transmit
- **AND** it SHALL find a GDS-received `.fdp` file that byte-matches the
  matching source runtime file

#### Scenario: Hosted received FDP decodes
- **WHEN** a byte-matched GDS-received `.fdp` file is available
- **THEN** the probe SHALL decode the received file with the current dictionary
- **AND** the decoded output SHALL identify `HkTrendRecord` record id `0` as an
  array record, `HkTrendChunkMeta` record id `1`, and the current payload
  version

### Requirement: HK Trend Mode V2 Compatibility
The HK trend official F' data product SHALL store the current primary spacecraft mode using the `SatMode` v2 enum while preserving the `HkTrendRecord` product record name and id and advancing the payload schema version to distinguish the new mode semantics.

#### Scenario: HK trend stores v2 mode
- **WHEN** `HkTrendProductProducer` serializes a record from a state snapshot
- **THEN** the `mode` field SHALL use one of `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, or `TTC`
- **AND** it SHALL NOT serialize retired primary mode names as current mission modes

#### Scenario: HK trend product identity remains stable
- **WHEN** the mode-model-v2 change updates the mode enum used by `HkTrendRecord`
- **THEN** the product record SHALL remain `HkTrendRecord` id `0`
- **AND** the payload version SHALL distinguish mode-model-v2 products from pre-mode-model-v2 products
- **AND** later payload version bumps SHALL preserve the mode field semantics unless explicitly changed by a later spec

### Requirement: HK Trend V6 Chunked Official Payload
The HK trend official F' data product SHALL use a V6 payload that preserves the
`HkTrendRecord` product record name and id while retiring the old
HK-fallback-specific storage fields from the active schema.

#### Scenario: V6 payload is generated from cached state
- **WHEN** `HkTrendProductProducer` captures a state snapshot
- **THEN** each captured sample SHALL serialize with `version = 6`
- **AND** the finalized official `.fdp` SHALL preserve `HkTrendRecord` id `0`
  for the sample array
- **AND** the payload SHALL include the active cached state fields except for
  the retired HK-root-specific storage fields
- **AND** it SHALL use the existing cached snapshot source rather than polling
  subsystem transports directly

#### Scenario: Missing subsystem values remain explicit in V6
- **WHEN** one subsystem snapshot is missing during HK trend capture
- **THEN** the V6 sample SHALL set that subsystem validity flag to false
- **AND** it SHALL NOT silently present neutral numeric values as known real
  measurements

#### Scenario: Historical backward decode is not claimed for V6
- **WHEN** reviewers evaluate this change
- **THEN** the evidence SHALL treat V6 `.fdp` files generated by the current
  dictionary as the decode target
- **AND** it SHALL NOT claim that old V1, V2, V3, V4, or V5 `.fdp` files
  decode with the V6 dictionary

### Requirement: Default Hosted CCSDS S-band FDP Parity
The hosted default CCSDS S-band data-product path SHALL prove that a generated
HK `.fdp` can be cataloged, transmitted to GDS, byte-matched, and decoded.

#### Scenario: Default hosted CCSDS S-band receives byte-matched FDP
- **WHEN** the hosted CCSDS S-band FDP parity probe runs after a fresh build
- **THEN** it SHALL use the default `OBC` binary and `OBCApp.*` command
  namespace
- **AND** it SHALL route GDS traffic through CCSDS framing,
  `ground_ttc_gateway` raw relay, S-band TCP, and `sband_comm_csp_node` node
  `5`
- **AND** it SHALL wait until at least one HK sample is pending, issue
  `HK_TREND_FLUSH`, and generate one or more HK trend `.fdp` files under
  `<runtime-root>/data-products/`
- **AND** it SHALL build the `DpCatalog` catalog and start catalog transmit
- **AND** it SHALL find a GDS-received `.fdp` file that byte-matches the
  matching source runtime file

#### Scenario: Default hosted CCSDS S-band received FDP decodes
- **WHEN** a byte-matched GDS-received `.fdp` file is available from the
  default hosted CCSDS S-band path
- **THEN** the probe SHALL decode the received file with the current dictionary
- **AND** the decoded output SHALL identify `HkTrendRecord` record id `0` as an
  array record, `HkTrendChunkMeta` record id `1`, and `version = 6`

#### Scenario: Retired HK ring is not part of the active catalog path
- **WHEN** the V6 `.fdp` evidence is recorded
- **THEN** it SHALL describe `DpCatalog` as the official `.fdp` catalog/index
  surface
- **AND** it SHALL state that `HousekeepingArchive`, `runtime/hk`, and
  `hk/index.csv` are retired from the active baseline

### Requirement: Official HK `.fdp` transfers may use the bounded reliable path

The HK data-product baseline SHALL allow current official HK `.fdp` whole-file
transfers to traverse the bounded reliable sidecar on either current exact
allowed path:

- default S-band node-`5`
- explicit-switched `uhf-primary-after-failover` node-`6`

#### Scenario: Official HK `.fdp` generation remains unchanged

- **WHEN** `uhf-reliable-transfer-v1` is enabled for a selected official HK
  `.fdp` request
- **THEN** `DpWriter`, `DpCatalog`, and the current official HK `.fdp`
  generation flow SHALL remain unchanged before transmit admission
- **AND** `DpCatalog` SHALL still own catalog build and selected transmit
  initiation

#### Scenario: Completion truth remains whole-file at the catalog boundary

- **WHEN** a selected official HK `.fdp` request completes through either
  bounded reliable path
- **THEN** `DpCatalog` SHALL still receive one final whole-file completion
  result
- **AND** that result SHALL be derived from reliable-transfer success or final
  failure instead of stock ground-side file-store completion alone

#### Scenario: Local ceiling remains bounded on both current paths

- **WHEN** a selected official HK `.fdp` request exceeds the bounded helper
  ceiling of `64` segments / `10,240` bytes
- **THEN** `CommController` SHALL reject the reliable-transfer start before
  in-transfer delivery begins
- **AND** node-`5` or node-`6` reliable-transfer reception SHALL reject a
  `BEGIN` that advertises a larger file or segment count
