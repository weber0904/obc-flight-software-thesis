## ADDED Requirements

### Requirement: COMM-Facing Live Beacon Broadcast
The system SHALL emit a bounded live beacon frame every 17 scheduler ticks through a COMM-facing broadcast sink that does not require ground acknowledgement, command response, or RF validation.

#### Scenario: Beacon cadence emits broadcast frame
- **WHEN** the beacon publisher scheduler reaches the cadence boundary with a valid reduced-state source and connected sink
- **THEN** the system SHALL encode and send exactly one live beacon frame through the sink
- **AND** the frame SHALL include a sequence number and CRC

#### Scenario: Beacon broadcast remains distinct from TT&C
- **WHEN** a live beacon frame is sent through the COMM-facing sink
- **THEN** the path SHALL be treated as no-ACK broadcast traffic
- **AND** it SHALL NOT be described as command/event/channel TT&C, reliable file transfer, or RF behavior

### Requirement: Beacon Payload Uses Actual Critical Measurements
The live beacon payload SHALL include actual critical cached measurements with validity metadata and summary masks.

#### Scenario: Beacon carries subsystem values
- **WHEN** the beacon publisher encodes a nominal frame
- **THEN** the frame SHALL include at minimum EPS battery voltage, current, state-of-charge, and temperature; ADCS mode and angular-rate norm; GPS fix validity and sentence counters; storage warning/degraded state; COMM pass and packet counters; boot slot; mode; timestamp; sequence; and CRC

### Requirement: Beacon Debug Capture Is Not Mission History
The system SHALL keep beacon capture/decode support as debug or verification tooling only and SHALL NOT store beacon history as a formal mission data product unless a later governed requirement adds that behavior.

#### Scenario: Beacon history is not cataloged
- **WHEN** the official F' data product catalog is built
- **THEN** debug beacon captures SHALL NOT appear as mission `BEACON_HISTORY` data products
