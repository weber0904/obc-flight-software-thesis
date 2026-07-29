## MODIFIED Requirements

### Requirement: Official HK Trend Data Product Boundary
The system SHALL generate a 30-tick housekeeping trend data product using
official F' product ports and official F' data product services.

#### Scenario: Current HK trend version is V5
- **WHEN** HK fallback fields are retired from the official history path
- **THEN** the product record SHALL remain `HkTrendRecord` id `0`
- **AND** the payload SHALL identify itself with `version = 5`
- **AND** the payload SHALL NOT include HK-root storage fields
- **AND** BeaconV1 wire format SHALL remain unchanged

### Requirement: Mission History Path Roles
The onboard state data system SHALL treat official F' HK `.fdp` products as the
primary HK/state mission-history path while preserving live beacon and
telemetry/event paths for their existing roles.

#### Scenario: Data path roles remain separate
- **WHEN** onboard state data is produced during hosted runtime
- **THEN** telemetry and events SHALL provide immediate operator visibility
- **AND** live beacon SHALL remain a no-ACK current-health broadcast
- **AND** official HK `.fdp` files plus `DpCatalog` SHALL be the only current
  stored HK/state mission-history path
- **AND** the active baseline SHALL NOT keep an HK ring fallback history plane
