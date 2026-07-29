# onboard-data-products-and-live-beacon Specification

## Purpose
Define the low-coupling onboard state reduction boundary that feeds live beacon frames and official housekeeping data products without adding autonomy control actions.
## Requirements
### Requirement: Onboard Reduced State Boundary
The system SHALL provide an `OnboardStateMonitor` capability that produces bounded `ReducedStateV1` from existing cached subsystem state and SHALL keep high-rate raw samples onboard-only by default.

#### Scenario: Reduced state reuses cached subsystem state
- **WHEN** the onboard state monitor runs in the OBC topology
- **THEN** it SHALL read existing cached state through `IStateSnapshotSource`
- **AND** it SHALL NOT directly poll EPS, ADCS, GPS, COMM, storage, boot, or radio transports

#### Scenario: Source failure invalidates cached reduced state
- **WHEN** the source cannot provide a state snapshot after a previous successful reduction
- **THEN** `getReducedStateForRuntime` SHALL return false
- **AND** downstream consumers SHALL NOT receive the previous reduced state as if it were current

#### Scenario: High-rate samples do not become public downlink traffic
- **WHEN** high-rate source samples are available
- **THEN** the system SHALL classify and reduce them for onboard use without emitting those raw samples as normal public telemetry or file-downlink products by default

### Requirement: Live Beacon Publisher Boundary
The system SHALL provide a `BeaconPublisher` capability that emits a fixed-format `BeaconV1` packet every 17 scheduler ticks from the latest reduced state through a COMM-facing beacon sink.

#### Scenario: Beacon uses reduced state only
- **WHEN** a beacon packet is produced
- **THEN** it SHALL use `IReducedStateSource`
- **AND** it SHALL NOT directly read EPS, ADCS, GPS, COMM, storage, boot, or radio component instances

#### Scenario: Beacon sequence advances only on successful sink send
- **WHEN** the beacon sink rejects a packet
- **THEN** the publisher SHALL report the sink failure
- **AND** it SHALL NOT advance the beacon sequence number

#### Scenario: Beacon sink does not block data product rate group on transport I/O
- **WHEN** the live beacon sink accepts a packet from the data rate group
- **THEN** it SHALL use a bounded handoff to the UART runtime path
- **AND** it SHALL NOT perform blocking byte-stream transport writes on the data rate group execution path

#### Scenario: Beacon decoder rejects non-canonical frame lengths
- **WHEN** the system decodes a `BeaconV1` packet
- **THEN** it SHALL accept only the canonical fixed wire-frame length
- **AND** it SHALL reject extended frames even when their CRC is internally consistent

#### Scenario: Beacon decoder updates output only on success
- **WHEN** the system rejects a malformed `BeaconV1` packet
- **THEN** it SHALL leave the caller-provided decoded output object unchanged
- **AND** it SHALL NOT expose partially decoded fields as valid output

#### Scenario: Not configured is distinct from unavailable state
- **WHEN** the beacon publisher lacks a source or sink
- **THEN** it SHALL report a not-configured status
- **AND** it SHALL NOT report the failure as a reduced-state source outage

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

### Requirement: Repo-Local Catalog Shim Is Superseded
The custom `StateDataArchive`, `ProductCatalog`, OPD1 product file format, `catalog.csv`, and mission `BEACON_HISTORY` data product SHALL NOT be part of the replacement mission baseline.

#### Scenario: Old archive baseline is absent
- **WHEN** the hosted onboard data-products and live-beacon probe runs
- **THEN** it SHALL validate official F' data product files and beacon debug capture
- **AND** it SHALL reject unexpected old `data-products/catalog.csv` or mission `BEACON_HISTORY` files

### Requirement: Autonomy Control Remains Out Of Scope
This capability SHALL NOT command `MissionExecutive`, `ModeManager`, ADCS, EPS, HELL, SAFE, IDLE, PAYLOAD, TTC, DETUMBLE, or load-shedding actions.

#### Scenario: Health condition does not command autonomy
- **WHEN** reduced state classifies low battery, high ADCS rate, or degraded storage
- **THEN** this data-system capability SHALL only publish data/status
- **AND** it SHALL NOT issue flight control or mode-management commands

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

### Requirement: Mode V2 Reduced State And Beacon Compatibility
The onboard state data system SHALL encode, decode, and validate spacecraft mode using the `SatMode` v2 enum while preserving the existing `BeaconV1` wire size and frame layout and advancing the beacon schema version to distinguish the new mode semantics.

#### Scenario: Beacon accepts mode v2 values
- **WHEN** a `BeaconV1` frame is encoded from reduced state containing `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, or `TTC`
- **THEN** the beacon decoder SHALL accept the frame and report the same mode value

#### Scenario: Beacon rejects unknown mode values
- **WHEN** a `BeaconV1` frame carries a mode byte outside the `SatMode` v2 valid numeric set
- **THEN** the decoder SHALL reject the frame as an invalid field
- **AND** it SHALL leave the caller-provided decoded output unchanged

#### Scenario: Beacon wire shape remains unchanged
- **WHEN** mode-model-v2 updates the valid mode set
- **THEN** `BeaconV1` SHALL retain the same canonical wire length, field order, and CRC behavior
- **AND** it SHALL emit schema version `2`

#### Scenario: Beacon rejects pre-mode-model-v2 schema
- **WHEN** a `BeaconV1` wire-layout frame carries schema version `1`
- **THEN** the decoder SHALL reject the frame as an unsupported version

### Requirement: Current Operational Visibility Distinguishes Curated Live Summary From Bounded Detailed Readback

The onboard data-products and live-beacon capability SHALL describe current
live operational visibility as curated post-auth live summary plus bounded
explicit detailed readback rather than broad family chatter by default.

#### Scenario: Live summary and detailed readback stay distinct
- **WHEN** the repository describes current operational visibility for onboard
  state families
- **THEN** it SHALL keep beacon as the no-ACK reduced-state broadcast
- **AND** it SHALL describe node-`5` auth-gated scheduled family summary as
  current live visibility
- **AND** it SHALL describe explicit detailed `GET_*` readback as bounded
  operator readback rather than broad ambient live telemetry.

#### Scenario: Residual runtime chatter is not rewritten as baseline operator truth
- **WHEN** reviewers encounter additional runtime live surfaces outside the
  curated summary families
- **THEN** the current documentation SHALL be able to mark them as
  `non-baseline live`
- **AND** it SHALL NOT collapse those residual surfaces back into the current
  operator baseline.

### Requirement: Node-5 Live Visibility Keeps Keep-Live Truth, Reviewable Proof, And Diagnostics Separate

The onboard data-products and live-visibility documentation SHALL keep
node-`5` pass-time keep-live truth, reviewable proof / transport observability,
bounded fresh readback, and diagnostics-only residuals as distinct current
surfaces.

#### Scenario: Reviewable proof does not collapse into keep-live summary
- **WHEN** current docs describe node-`5` live operational visibility
- **THEN** they SHALL keep curated pass-time summary and operator-facing
  `CommController` state separate from reviewable transport, queue, owner, and
  egress proof surfaces
- **AND** they SHALL NOT imply that `GROUND_LINK_TX_BYTES`, `QueueOverflow`, or
  similar reviewable proof surfaces are part of the small pass-time summary.

#### Scenario: Resource wording does not collapse supplemental SystemResources back into live truth
- **WHEN** the same docs describe current resource observability
- **THEN** they SHALL identify `SYS_*` as the formal node-`5` resource
  keep-live truth
- **AND** they SHALL keep `SystemResources.*` in the supplemental
  diagnostics-only bucket.

### Requirement: Live Operational Visibility Distinguishes Beacon, Auth-Gated Live Packets, And Bounded Summary Readback

The onboard state data system SHALL describe nominal live operational
visibility as three distinct current surfaces: always-on live beacon,
auth-gated packetized live observability, and bounded `GET_*` summary readback.

#### Scenario: Mission-history wording does not collapse current observability tiers
- **WHEN** the repository describes current live versus stored HK/state roles
- **THEN** it SHALL keep live beacon as a no-ACK current-health broadcast
- **AND** it SHALL describe packetized S-band `event/tlm` as auth-gated live
  operational visibility rather than as unconditional startup chatter
- **AND** it SHALL describe bounded `GET_*` summary readback as on-demand live
  visibility rather than mission-history storage.

#### Scenario: Bounded readback does not become a second storage path
- **WHEN** component-owned `GET_*` or read/status commands emit summary
  events/telemetry
- **THEN** those summaries SHALL remain operator-facing live readback surfaces
- **AND** they SHALL NOT be described as replacing official HK `.fdp`
  mission-history products or as opening a new generic stored-history path.

### Requirement: Reduced-State Update Event Is Mask-Change-Oriented

The onboard data-products and live-beacon capability SHALL treat
`STATE_MONITOR_UPDATED` as a reduced-state mask-transition event rather than a
periodic heartbeat.

#### Scenario: Successful reduction without mask change stays event-quiet
- **WHEN** `OnboardStateMonitor` produces a new successful reduced-state sample
  whose `healthMask`, `faultMask`, and `qualityMask` match the previous
  successful reduced-state sample
- **THEN** it SHALL continue updating its cached reduced state and telemetry
- **AND** it SHALL NOT emit `STATE_MONITOR_UPDATED`.

#### Scenario: Successful reduction with mask change emits one update event
- **WHEN** `OnboardStateMonitor` produces a new successful reduced-state sample
  whose `healthMask`, `faultMask`, or `qualityMask` differs from the previous
  successful reduced-state sample
- **THEN** it SHALL emit `STATE_MONITOR_UPDATED` with the new three mask
  values.

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
