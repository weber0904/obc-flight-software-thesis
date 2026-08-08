## ADDED Requirements

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

#### Scenario: Not configured is distinct from unavailable state
- **WHEN** the beacon publisher lacks a source or sink
- **THEN** it SHALL report a not-configured status
- **AND** it SHALL NOT report the failure as a reduced-state source outage

### Requirement: Official HK Trend Data Product Boundary
The system SHALL generate a 30-tick housekeeping trend data product using official F' product ports and official F' data product services.

#### Scenario: HK trend writes official data product
- **WHEN** the HK trend producer cadence expires and a state snapshot is available
- **THEN** it SHALL request a data product container from `DpManager`
- **AND** it SHALL serialize one `HkTrendRecordV1`
- **AND** it SHALL send the filled container so `DpWriter` writes an official data product file

#### Scenario: Missing values are explicit
- **WHEN** one subsystem snapshot is missing during HK trend capture
- **THEN** the record SHALL set that subsystem validity flag to false
- **AND** it SHALL NOT silently present default numeric values as known real measurements

### Requirement: Repo-Local Catalog Shim Is Superseded
The custom `StateDataArchive`, `ProductCatalog`, OPD1 product file format, `catalog.csv`, and mission `BEACON_HISTORY` data product SHALL NOT be part of the replacement mission baseline.

#### Scenario: Old archive baseline is absent
- **WHEN** the hosted onboard data-products and live-beacon probe runs
- **THEN** it SHALL validate official F' data product files and beacon debug capture
- **AND** it SHALL reject unexpected old `data-products/catalog.csv` or mission `BEACON_HISTORY` files

### Requirement: Autonomy Control Remains Out Of Scope
This capability SHALL NOT command `MissionExecutive`, `ModeManager`, ADCS, EPS, LOW_POWER, DETUMBLE, or load-shedding actions.

#### Scenario: Health condition does not command autonomy
- **WHEN** reduced state classifies low battery, high ADCS rate, or degraded storage
- **THEN** this data-system capability SHALL only publish data/status
- **AND** it SHALL NOT issue flight control or mode-management commands
