## MODIFIED Requirements

### Requirement: Official HK Trend Data Product Boundary
The system SHALL generate a 30-tick housekeeping trend data product using
official F' product ports and official F' data product services.

#### Scenario: HK trend captures samples and flushes finalized chunks
- **WHEN** the HK trend producer cadence expires and a state snapshot is
  available
- **THEN** it SHALL capture exactly one HK trend sample with the current schema
  version
- **AND** it SHALL accumulate that sample into an in-memory pending chunk until
  a finalized official data product file is flushed

#### Scenario: Finalized official HK data product uses stock services
- **WHEN** the HK trend producer flushes a non-empty pending chunk
- **THEN** it SHALL request a data product container from `DpManager`
- **AND** it SHALL serialize `HkTrendChunkMeta` plus an array
  `HkTrendRecord` payload with the current schema version
- **AND** it SHALL send the filled container so `DpWriter` writes one official
  data product file

#### Scenario: Serialization failure returns allocated data product buffer
- **WHEN** the HK trend producer receives a data product container but cannot
  serialize the current HK chunk into it
- **THEN** it SHALL return the allocated buffer to the data product buffer
  manager
- **AND** it SHALL NOT leak the buffer or send a malformed data product
  downstream

#### Scenario: Current HK trend version is V6
- **WHEN** chunked official HK products replace the prior single-sample V5
  emission behavior
- **THEN** the product record SHALL remain `HkTrendRecord` id `0`
- **AND** the per-sample payload SHALL identify itself with `version = 6`
- **AND** `HkTrendChunkMeta` SHALL identify the sample range and flush reason
- **AND** `BeaconV1` wire format SHALL remain unchanged

### Requirement: Mission History Path Roles
The onboard state data system SHALL treat official F' HK `.fdp` products as the
only active HK/state mission-history path, SHALL treat command responses plus
bounded telemetry, events, and live beacon as nominal live operational
visibility rather than stored history, and SHALL keep logs, journal output,
captures, `.fdp` staging or file-store artifacts, and hosted status dumps as
diagnostic or operator-review surfaces unless a later governed requirement
promotes them.

#### Scenario: Data path roles remain separate
- **WHEN** onboard state data is produced during hosted runtime
- **THEN** command responses, telemetry, and events SHALL provide immediate
  operator visibility
- **AND** live beacon SHALL remain a no-ACK current-health broadcast
- **AND** official HK `.fdp` files SHALL be the only active stored HK/state
  mission-history target
- **AND** the runtime SHALL NOT require `runtime/hk`, `hk/index.csv`, or
  `HK_*` command surfaces

#### Scenario: Diagnostic captures do not become nominal downlinked truth
- **WHEN** logs, journal excerpts, beacon capture files, hosted status dumps,
  or ad hoc local inspection artifacts are used during verification
- **THEN** those artifacts SHALL remain diagnostic or review-support surfaces
- **AND** the repository SHALL NOT describe them as the nominal stored-history
  or flight-like live-downlink truth of the current baseline

#### Scenario: Manual flush supports freshness-sensitive official HK proofs
- **WHEN** an operator or hosted probe needs a fresh official HK `.fdp` before
  the current chunk reaches its size threshold
- **THEN** it SHALL be able to use `HK_TREND_FLUSH` to finalize the current
  non-empty chunk without changing the stock `DpCatalog -> FileDownlink` flow

#### Scenario: Live beacon wire format is unchanged
- **WHEN** HK trend moves to chunked V6 official `.fdp` files
- **THEN** the live beacon wire format SHALL remain unchanged
- **AND** any HK chunking metadata SHALL remain confined to the official data
  product path

### Requirement: HK Product Success Event Remains The Operator-Facing Product Completion Surface

The onboard data-products and live-beacon capability SHALL keep
`HK_TREND_PRODUCT_WRITTEN` as the current operator-facing HK product success
event and SHALL NOT require low-level writer success events to remain on the
default packetized ground event surface.

#### Scenario: HK product success remains visible even when writer success is packet-quiet
- **WHEN** the HK trend producer successfully emits an official HK data
  product
- **THEN** `HK_TREND_PRODUCT_WRITTEN` SHALL remain part of the current
  packetized operator event surface
- **AND** it SHALL identify chunk sequence, sample range, sample count, file
  size, and flush reason
- **AND** lower-level `DpWriter.FileWritten` success visibility MAY remain
  local/debug-only.
