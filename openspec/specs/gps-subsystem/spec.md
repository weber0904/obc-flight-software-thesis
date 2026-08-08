# gps-subsystem Specification

## Purpose
Define the bounded GPS subsystem model for fake/replay-driven hosted validation, GPS-owned command and telemetry contracts, bounded NMEA parsing, cached runtime state, and the first governed direct OBC-attached live UART hardware path.
## Requirements
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
The GPS subsystem SHALL support fake, replay-driven, and live UART-driven GPS input modes, and it SHALL keep fake as the default mode so hosted and software-only validation can run without live hardware.

#### Scenario: Hosted validation still runs without live GPS hardware
- **WHEN** the project validates the existing hosted GPS path
- **THEN** the GPS bridge SHALL still be able to receive data from fake or replay sources without requiring a real UART device

### Requirement: Bounded First-Version NMEA Parsing
The first GPS slice SHALL parse a bounded first-version NMEA subset sufficient to derive fix validity, position, time, and basic reception metadata, it SHALL reject malformed or checksum-invalid sentences instead of treating them as valid fixes, and it SHALL enforce explicit first-version sentence-length and field-count bounds before continuing per-field parsing work.

#### Scenario: Valid sentence updates cached GPS state
- **WHEN** the GPS bridge receives a valid supported NMEA sentence from the current source
- **THEN** it SHALL update the cached GPS state with the fields owned by that sentence

#### Scenario: Invalid sentence preserves last valid fix
- **WHEN** the GPS bridge receives a malformed or checksum-invalid supported sentence
- **THEN** it SHALL preserve the last valid cached fix and SHALL raise the owned degraded or parse-fault path

#### Scenario: Oversized sentence is rejected as malformed
- **WHEN** the GPS bridge receives a supported sentence whose total length or field count exceeds the first-version parser bounds
- **THEN** it SHALL reject that sentence as `MALFORMED` instead of attempting unbounded parse work

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

### Requirement: Fake-First GPS Source Strategy
The GPS subsystem SHALL support fake, replay-driven, and live UART-driven GPS input modes, and it SHALL keep fake as the default mode so hosted and software-only validation can run without live hardware.

#### Scenario: Hosted validation still runs without live GPS hardware
- **WHEN** the project validates the existing hosted GPS path
- **THEN** the GPS bridge SHALL still be able to receive data from fake or replay sources without requiring a real UART device

### Requirement: Live UART GPS Source Is Governed
The GPS subsystem SHALL provide a governed live UART source mode that reads NMEA sentences from an explicit serial device path without routing GPS through the external radio/comm subsystem, and it SHALL ignore ignorable blank serial lines instead of surfacing them as source starvation.

#### Scenario: Live UART source activates on a configured serial device
- **WHEN** `OBC_GPS_SOURCE_MODE=live-uart` and the configured serial device can be opened and configured
- **THEN** the GPS bridge SHALL be able to read bounded NMEA sentence lines from that UART-backed source

#### Scenario: Live UART activation fails cleanly when the device is unavailable
- **WHEN** the operator selects `live-uart` but the configured serial device cannot be opened or configured
- **THEN** the GPS bridge SHALL reject that source-mode switch with a validation-style failure instead of silently claiming live operation

#### Scenario: Blank serial line does not count as source starvation
- **WHEN** the live UART source receives a newline-delimited line that normalizes to an empty sentence
- **THEN** it SHALL discard that blank line and continue waiting for bounded sentence data instead of surfacing that event as `NO_SOURCE_DATA`

### Requirement: Direct OBC UART Is The Active GPS Hardware Path
The near-term active target GPS hardware path SHALL be a direct OBC-attached UART path on `obc.local`, using governed GPS-owned serial configuration rather than the historical OBC-side external comm UART baseline.

#### Scenario: OBC-side GPS UART path is explicit
- **WHEN** the repository documents or validates the first target-side live GPS path
- **THEN** it SHALL identify the governed GPS serial device and baudrate used on `obc.local`

### Requirement: GPS Direct OBC UART Baseline Remains The Primary Hardware Direction
After the first governed live GPS UART slice, the GPS subsystem SHALL continue to treat direct OBC-attached UART as the primary hardware baseline unless a later governed architecture change explicitly redefines GPS as something else.

#### Scenario: Future GPS work extends the direct OBC baseline by default
- **WHEN** a later change extends live GPS behavior beyond the first UART slice
- **THEN** that change SHALL build on the existing direct OBC UART path instead of implicitly moving GPS into the CSP subsystem baseline

### Requirement: GPS Remains Outside The Planned Shared Subsystem CAN FD Bus
The GPS subsystem SHALL remain outside the planned shared `CAN FD` bus direction for `EPS`, `ADCS`, and `COMM` unless a later governed change explicitly redefines GPS architecture.

#### Scenario: Shared CAN FD migration does not implicitly absorb GPS
- **WHEN** the repository migrates subsystem traffic toward shared `CAN FD`
- **THEN** GPS SHALL remain a separate direct sensor path unless a later governed architecture change explicitly states otherwise

### Requirement: Scheduled GPS Live Visibility Is Summary-Oriented

The GPS subsystem SHALL keep scheduled current live observability summary-only
while preserving fresh detailed bounded readback on explicit GPS state
requests.

#### Scenario: Scheduled GPS refresh keeps fix summary and critical transitions
- **WHEN** `GpsBridge` performs a scheduled sentence poll
- **THEN** it SHALL keep only source-mode, sample-validity, fix-validity, and
  satellite-count telemetry as baseline live summary
- **AND** it SHALL keep fix-acquired, fix-lost, parse-error, and source-error
  events reviewable.

#### Scenario: Explicit GPS state readback remains fresh and detailed
- **WHEN** `GPS_GET_STATE` performs a fresh GPS poll
- **THEN** the subsystem SHALL apply the update through the same state logic
  used for scheduled refresh
- **AND** it SHALL make the detailed GPS telemetry reviewable as bounded
  readback for that explicit command.

