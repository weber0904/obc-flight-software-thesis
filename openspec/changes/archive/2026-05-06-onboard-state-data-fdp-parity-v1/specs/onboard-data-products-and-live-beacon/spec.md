## ADDED Requirements

### Requirement: Mission History Path Roles
The onboard state data system SHALL treat official F' HK `.fdp` products as the primary HK/state mission-history path while preserving live beacon and telemetry/event paths for their existing roles.

#### Scenario: Data path roles remain separate
- **WHEN** onboard state data is produced during hosted runtime
- **THEN** telemetry and events SHALL provide immediate operator visibility
- **AND** live beacon SHALL remain a no-ACK current-health broadcast
- **AND** official HK `.fdp` files SHALL be the primary stored HK/state mission-history target
- **AND** the existing HK ring SHALL remain only a bounded transitional forensic fallback

#### Scenario: Live beacon wire format is unchanged
- **WHEN** storage health adds `data-products` root visibility
- **THEN** the live beacon wire format SHALL remain unchanged
- **AND** any `data-products` warning or degraded condition SHALL be reflected only through the existing storage masks
