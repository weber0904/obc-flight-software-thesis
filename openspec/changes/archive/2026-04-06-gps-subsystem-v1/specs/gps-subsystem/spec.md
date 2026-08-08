## ADDED Requirements

### Requirement: Dedicated GPS Subsystem Boundary
The project SHALL provide a dedicated `gps-subsystem` capability for GPS ingestion, parsing, cached state, and GPS-owned command, telemetry, and event contracts, and it SHALL NOT treat GPS as part of the external radio/comm subsystem solely because the hardware transport may use UART.

#### Scenario: GPS ownership stays distinct from comm ownership
- **WHEN** the first GPS slice is implemented
- **THEN** GPS fix, navigation, and sentence-validity behavior SHALL be owned by the GPS subsystem instead of the radio/comm subsystem

### Requirement: First-Version GPS Public Contract
The first GPS implementation slice SHALL own `GPS_*` command, telemetry, and event families sufficient to expose the latest valid GPS state, fix validity, source-mode behavior, and parser or transport fault conditions.

#### Scenario: Operator can observe GPS state
- **WHEN** the GPS bridge has processed one or more source updates
- **THEN** the owned `GPS_*` telemetry and event families SHALL make the current fix and fault state reviewable

### Requirement: Fake-First GPS Source Strategy
The first GPS slice SHALL support a fake or replay-driven GPS input path that allows hosted validation without requiring live Raspberry Pi UART hardware or a real satellite fix.

#### Scenario: Hosted validation runs without live GPS hardware
- **WHEN** the project validates the first GPS slice on the hosted runtime
- **THEN** the GPS bridge SHALL be able to receive data from a fake or replay source instead of a real UART device

### Requirement: Bounded First-Version NMEA Parsing
The first GPS slice SHALL parse a bounded first-version NMEA subset sufficient to derive fix validity, position, time, and basic reception metadata, and it SHALL reject malformed or checksum-invalid sentences instead of treating them as valid fixes.

#### Scenario: Valid sentence updates cached GPS state
- **WHEN** the GPS bridge receives a valid supported NMEA sentence from the current source
- **THEN** it SHALL update the cached GPS state with the fields owned by that sentence

#### Scenario: Invalid sentence preserves last valid fix
- **WHEN** the GPS bridge receives a malformed or checksum-invalid supported sentence
- **THEN** it SHALL preserve the last valid cached fix and SHALL raise the owned degraded or parse-fault path

### Requirement: Explicit No-Fix Handling
The first GPS slice SHALL distinguish between "no valid fix" and "valid fix" states, and it SHALL preserve that distinction in cached runtime state rather than fabricating a position.

#### Scenario: No-fix sentence does not claim a position fix
- **WHEN** the GPS bridge receives a supported sentence indicating no valid fix
- **THEN** it SHALL update the cached GPS validity state to "no fix" and SHALL NOT report a fabricated valid position

### Requirement: Cached GPS Runtime Access
The first GPS slice SHALL expose cached GPS runtime state for other OBC consumers and SHALL allow those consumers to read the latest known GPS state without performing another transport read during the same observation path.

#### Scenario: Runtime consumer reads cached GPS state
- **WHEN** another OBC feature requests GPS runtime state
- **THEN** it SHALL receive the latest cached GPS state maintained by the GPS bridge
