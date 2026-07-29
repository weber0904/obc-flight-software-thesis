## ADDED Requirements

### Requirement: Shared Identifiers And Types
The core system contracts capability SHALL define the shared base ID ranges, SHALL reserve CSP node IDs `1`, `2`, and `3` for OBC, EPS, and ADCS respectively, and SHALL publish the shared `SatMode`, `AdcsMode`, `CommBand`, and `BootSlot` types for reuse by other capabilities.

#### Scenario: Subsystem capability reuses shared types
- **WHEN** a subsystem capability needs to reference a mode, band, or boot slot
- **THEN** it SHALL reference the shared type owned by `core-system-contracts` instead of redefining that type

### Requirement: Core Control Interface
The system SHALL expose the `MODE_*`, `HEALTH_*`, and `CSP_*` core command families and SHALL provide the `SYS_*` and `CSP_*` telemetry required to observe system state, uptime, resource pressure, and CSP connectivity.

#### Scenario: Core CSP operation is diagnosable
- **WHEN** an operator issues `CSP_INIT` followed by `CSP_PING`
- **THEN** the deployment SHALL support initializing the local node, attempting the ping, and surfacing enough telemetry to diagnose connectivity results

### Requirement: Single Owner For Public Symbol Families
Each public command, telemetry, and event family SHALL have exactly one owning capability. `core-system-contracts` SHALL own only the shared types and core system symbol families, while subsystem-specific public symbols SHALL be owned by their respective subsystem capabilities.

#### Scenario: Subsystem contracts are not duplicated centrally
- **WHEN** EPS, ADCS, comms, or boot public symbols are defined
- **THEN** those symbols SHALL be owned by their subsystem capability and SHALL NOT be duplicated in the central core contract capability
