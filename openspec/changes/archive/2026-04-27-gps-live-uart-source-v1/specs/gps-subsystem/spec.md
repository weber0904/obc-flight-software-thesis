## MODIFIED Requirements

### Requirement: Fake-First GPS Source Strategy
The GPS subsystem SHALL support fake, replay-driven, and live UART-driven GPS input modes, and it SHALL keep fake as the default mode so hosted and software-only validation can run without live hardware.

#### Scenario: Hosted validation still runs without live GPS hardware
- **WHEN** the project validates the existing hosted GPS path
- **THEN** the GPS bridge SHALL still be able to receive data from fake or replay sources without requiring a real UART device

### Requirement: Live UART GPS Source Is Governed
The GPS subsystem SHALL provide a governed live UART source mode that reads NMEA sentences from an explicit serial device path without routing GPS through the external radio/comm subsystem.

#### Scenario: Live UART source activates on a configured serial device
- **WHEN** `OBC_GPS_SOURCE_MODE=live-uart` and the configured serial device can be opened and configured
- **THEN** the GPS bridge SHALL be able to read bounded NMEA sentence lines from that UART-backed source

#### Scenario: Live UART activation fails cleanly when the device is unavailable
- **WHEN** the operator selects `live-uart` but the configured serial device cannot be opened or configured
- **THEN** the GPS bridge SHALL reject that source-mode switch with a validation-style failure instead of silently claiming live operation

### Requirement: Direct OBC UART Is The Active GPS Hardware Path
The near-term active target GPS hardware path SHALL be a direct OBC-attached UART path on `obc.local`, using governed GPS-owned serial configuration rather than the historical OBC-side external comm UART baseline.

#### Scenario: OBC-side GPS UART path is explicit
- **WHEN** the repository documents or validates the first target-side live GPS path
- **THEN** it SHALL identify the governed GPS serial device and baudrate used on `obc.local`
