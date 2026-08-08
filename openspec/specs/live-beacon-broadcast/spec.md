# live-beacon-broadcast Specification

## Purpose
Define the bounded live-beacon broadcast behavior, payload scope, and validation boundary for COMM-facing no-acknowledgement health summaries.
## Requirements
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

#### Scenario: Beacon handoff avoids blocking state-data scheduling
- **WHEN** the live beacon publisher emits a frame from the data rate group
- **THEN** the COMM-facing sink SHALL perform only bounded in-memory handoff work on that caller path
- **AND** the UART byte-stream write SHALL run from the UART runtime path rather than from the beacon publisher scheduler call

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

### Requirement: Beacon Suppress Policy Stays Separate From Beacon Broadcast Proof

The live-beacon broadcast capability SHALL keep no-ACK beacon emission
separate from command-session suppress policy ownership even after suppress
runtime closure is implemented.

#### Scenario: Broadcast proof alone is not suppress proof
- **WHEN** the repository cites current live-beacon evidence without the
  dedicated suppress/runtime record
- **THEN** it SHALL describe that evidence as proof of bounded broadcast
  cadence, payload, capture, and decode only
- **AND** it SHALL NOT treat ordinary broadcast proof alone as evidence for
  suppress start, refresh, invalidation clear, or timeout resume behavior

### Requirement: Beacon Suppress And Resume Policy Is COMM-Owned

The current baseline SHALL treat UHF beacon suppress and resume as COMM runtime
owned by `CommController`, with suppress beginning at accepted authenticated
UHF secure-auth completion that synthesizes the owning session, refresh
extending only on accepted authenticated UHF same-session activity, and resume
happening after immediate session invalidation or fixed bounded inactivity
timeout.

#### Scenario: Beacon policy owner stays outside BeaconPublisher
- **WHEN** reviewers inspect current beacon/session runtime wording
- **THEN** `BeaconPublisher` SHALL remain a fixed-cadence broadcast producer
- **AND** it SHALL NOT be described as the owner of UHF command-session
  suppress or resume decisions

#### Scenario: Backup role is not rewritten as beacon-only
- **WHEN** current baseline wording describes UHF backup behavior
- **THEN** it SHALL keep beacon duty separate from the bounded allowlisted
  backup-ingress command surface
- **AND** it SHALL NOT describe `uhf-backup` as beacon-only

#### Scenario: Active UHF primary session suppresses beacon chatter only in that window
- **WHEN** the current baseline describes UHF primary clean-session behavior
- **THEN** it SHALL state that active accepted UHF command-session windows
  suppress beacon chatter
- **AND** it SHALL keep that statement scoped to the active UHF
  command-session window rather than to UHF primary packet quiet in general
  command-session window rather than as a blanket permanent UHF beacon disable
