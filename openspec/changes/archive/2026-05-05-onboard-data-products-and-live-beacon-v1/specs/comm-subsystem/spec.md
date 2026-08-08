## ADDED Requirements

### Requirement: COMM-Facing Beacon Sink
The comm subsystem SHALL provide or accept a spacecraft-side sink for live beacon frames that sends bounded payloads through the hosted COMM-facing path without requiring ground acknowledgement, command response, or RF validation.

#### Scenario: Beacon broadcast does not alter COMM service contract
- **WHEN** this change adds live beacon broadcast behavior
- **THEN** existing COMM node identity and TT&C service ports SHALL remain unchanged
- **AND** the change SHALL NOT require replacing `ComFprime` or adding CCSDS framing

#### Scenario: Beacon path claim remains bounded
- **WHEN** the hosted beacon probe captures a frame
- **THEN** the evidence SHALL identify it as hosted/COMM-facing behavior
- **AND** it SHALL NOT claim physical RF, real-radio, CFDP, ARQ, or pass-window behavior
