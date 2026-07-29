## MODIFIED Requirements

### Requirement: Official HK Trend Data Product Boundary
The system SHALL generate a 30-tick housekeeping trend data product using official F' product ports and official F' data product services.

#### Scenario: HK trend writes official data product
- **WHEN** the HK trend producer cadence expires and a state snapshot is available
- **THEN** it SHALL request a data product container from `DpManager`
- **AND** it SHALL serialize one `HkTrendRecord` payload with the current schema version
- **AND** it SHALL send the filled container so `DpWriter` writes an official data product file

#### Scenario: Missing values are explicit
- **WHEN** one subsystem snapshot is missing during HK trend capture
- **THEN** the record SHALL set that subsystem validity flag to false
- **AND** it SHALL NOT silently present default numeric values as known real measurements

#### Scenario: Serialization failure returns allocated data product buffer
- **WHEN** the HK trend producer receives a data product container but cannot serialize the trend record into it
- **THEN** it SHALL return the allocated buffer to the data product buffer manager
- **AND** it SHALL NOT leak the buffer or send a malformed data product downstream

#### Scenario: Current HK trend version is V4
- **WHEN** final-design health/state cached fields are aligned into the official HK trend data product
- **THEN** the product record SHALL remain `HkTrendRecord` id `0`
- **AND** the payload SHALL identify itself with `version = 4`
- **AND** BeaconV1 wire format SHALL remain unchanged
