## ADDED Requirements

### Requirement: Official HK Trend Data Product Generation
The system SHALL generate housekeeping trend records using official F' data product producer ports, `DpManager`, and `DpWriter`.

#### Scenario: HK trend cadence writes official data product
- **WHEN** the HK trend producer scheduler reaches the 30-tick cadence boundary and a state snapshot is available
- **THEN** the producer SHALL request an official F' data product container from `DpManager`
- **AND** it SHALL serialize one HK trend record into the container
- **AND** it SHALL send the filled container so `DpWriter` writes an official data product file

#### Scenario: HK trend buffer unavailable
- **WHEN** `DpManager` cannot provide a valid data product buffer
- **THEN** the HK trend producer SHALL report the failure through events and telemetry
- **AND** it SHALL NOT report a successful product write

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
