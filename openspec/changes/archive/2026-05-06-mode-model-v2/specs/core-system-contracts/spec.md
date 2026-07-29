## MODIFIED Requirements

### Requirement: Shared Identifiers And Types
The core system contracts capability SHALL define the shared base ID ranges, SHALL reserve CSP node IDs `1`, `2`, and `3` for OBC, EPS, and ADCS respectively, and SHALL publish the shared `SatMode`, `AdcsMode`, `CommBand`, and `BootSlot` types for reuse by other capabilities. The primary `SatMode` contract SHALL use the v2 values `SAFE = 0`, `IDLE = 1`, `HELL = 2`, `PAYLOAD = 3`, and `TTC = 4`.

#### Scenario: Subsystem capability reuses shared types
- **WHEN** a subsystem capability needs to reference a mode, band, or boot slot
- **THEN** it SHALL reference the shared type owned by `core-system-contracts` instead of redefining that type

#### Scenario: Primary mode enum uses v2 values
- **WHEN** the project generates the shared `SatMode` dictionary and component interfaces
- **THEN** the generated enum SHALL expose `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC` with the governed v2 numeric values
- **AND** it SHALL NOT expose `NOMINAL`, `LOW_POWER`, `DEBUG`, or `UPDATE` as primary mission modes

### Requirement: Project-Local Shared Core Types
The project SHALL define the shared `SatMode`, `AdcsMode`, `CommBand`, `BootSlot`, and `HealthItem` types in a project-local FPP module, and it SHALL define the shared base ID range constants plus CSP node constants for OBC, EPS, and ADCS in the same capability-owned area. The project-local `SatMode` type SHALL be the only primary spacecraft mode enum used by command, telemetry, event, HK trend, and live beacon mode fields.

#### Scenario: Later components reuse a shared project-owned type
- **WHEN** a later project component needs a shared mode, band, slot, or health-item type
- **THEN** it SHALL import the project-local shared type instead of redefining the symbol

#### Scenario: Public mode surfaces stay aligned
- **WHEN** `ModeManager.MODE_SET`, `SYS_MODE`, `SYS_MODE_CHANGE`, runtime status text, HK trend records, or live beacon encode/decode logic expose spacecraft mode
- **THEN** they SHALL use the project-local `SatMode` v2 values and names

## ADDED Requirements

### Requirement: Mode Shell Boundary
The first mode-model-v2 implementation SHALL make `HELL`, `PAYLOAD`, and `TTC` commandable primary modes without adding the later behaviors associated with those modes.

#### Scenario: Mode shells do not imply deferred subsystems
- **WHEN** an operator commands `HELL`, `PAYLOAD`, or `TTC` in the first mode-model-v2 slice
- **THEN** the system SHALL update the public mode state
- **AND** it SHALL NOT claim load shedding, payload execution, pass scheduling, COMM split-link behavior, CCSDS behavior, storage-policy behavior, or FDIR behavior from that mode transition alone
