## MODIFIED Requirements

### Requirement: Generic Comms Component Families

The subsystem SHALL implement `CommController`, `UartDriver`, and `RadioController` as reusable component families or single controller roles, SHALL use instance names rather than renamed public symbols to distinguish S-band and UHF behavior, and SHALL keep the transport choice below the controller layer.

#### Scenario: First implementation keeps transport-specific details below the controller layer
- **WHEN** `RadioController` is validated through TCP mock and PTY-backed byte-stream paths
- **THEN** the controller logic SHALL operate through a shared transport abstraction instead of branching on the concrete transport type

### Requirement: Comms Public Contract

The subsystem SHALL own the `COMM_*`, `RADIO_*`, and `UART_*` command, telemetry, and event families required for band selection, pass control, UART health, radio status, and error reporting.

#### Scenario: Pass window state is emitted by `CommController`
- **WHEN** a pass window starts, ticks down, or stops
- **THEN** `CommController` SHALL publish active-band, pass-active, remaining-seconds, and total-pass state through its owned telemetry and events

#### Scenario: Radio status is available without hardware-specific framing
- **WHEN** `RADIO_GET_STATUS` runs against the first hosted mock-radio backend
- **THEN** the subsystem SHALL return enabled, power, frequency, temperature, and RSSI status without requiring KISS or a vendor-specific protocol

### Requirement: Comms Verification Modes

The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, and SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware is unavailable.

#### Scenario: One integration test exercises both host transport modes
- **WHEN** the first comm subsystem slice is validated on the host
- **THEN** the test evidence SHALL include both a TCP mock path and a PTY-backed UART-replacement path
