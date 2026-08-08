## MODIFIED Requirements

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
