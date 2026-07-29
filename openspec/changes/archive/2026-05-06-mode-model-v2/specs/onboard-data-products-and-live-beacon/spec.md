## MODIFIED Requirements

### Requirement: Autonomy Control Remains Out Of Scope
This capability SHALL NOT command `MissionExecutive`, `ModeManager`, ADCS, EPS, HELL, SAFE, IDLE, PAYLOAD, TTC, DETUMBLE, or load-shedding actions.

#### Scenario: Health condition does not command autonomy
- **WHEN** reduced state classifies low battery, high ADCS rate, or degraded storage
- **THEN** this data-system capability SHALL only publish data/status
- **AND** it SHALL NOT issue flight control or mode-management commands

## ADDED Requirements

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
