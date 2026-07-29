## ADDED Requirements

### Requirement: Project-Local Shared Core Types
The project SHALL define the shared `SatMode`, `AdcsMode`, `CommBand`, `BootSlot`, and `HealthItem` types in a project-local FPP module, and it SHALL define the shared base ID range constants plus CSP node constants for OBC, EPS, and ADCS in the same capability-owned area.

#### Scenario: Later components reuse a shared project-owned type
- **WHEN** a later project component needs a shared mode, band, slot, or health-item type
- **THEN** it SHALL import the project-local shared type instead of redefining the symbol

### Requirement: Concrete Core Component Interfaces
The project SHALL provide concrete F' component interfaces for `ModeManager`, `HealthMonitor`, and `CspBridge`, and those components SHALL own the `MODE_*`, `HEALTH_*`, `SYS_*`, and `CSP_*` public symbol families assigned to this capability.

#### Scenario: Core component ownership is visible in code
- **WHEN** the project builds the core contract capability
- **THEN** the source tree SHALL contain project-local component definitions for `ModeManager`, `HealthMonitor`, and `CspBridge` and SHALL expose the required public symbol families from those components

### Requirement: Minimum Verified Core Behaviors
The first implementation slice SHALL verify mode changes, health-threshold handling, and CSP initialization/ping/send behaviors with automated tests before the change is archived.

#### Scenario: Core contract change is archived
- **WHEN** `core-system-contracts-v1` is ready to archive
- **THEN** the change SHALL include automated verification evidence covering the core command and telemetry paths for the three new components
