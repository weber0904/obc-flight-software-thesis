## ADDED Requirements

### Requirement: External Link Transport Strategy
The comm subsystem SHALL keep the external comms link separate from the internal CSP network, SHALL use TCP mock as the first-version development transport, and SHALL allow target integration to switch to UART through profile and instance selection.

#### Scenario: External link transport changes by profile
- **WHEN** the project moves from software-only development to Raspberry Pi integration
- **THEN** the external link transport MAY change from TCP mock to UART without redefining the internal CSP network baseline

### Requirement: Generic Comms Component Families
The subsystem SHALL implement `CommController`, `UartDriver`, and `RadioController` as reusable component families or single controller roles, and SHALL use instance names rather than renamed public symbols to distinguish S-band and UHF behavior.

#### Scenario: Reused radio commands across instances
- **WHEN** both `sbandCtrl` and `uhfCtrl` are present
- **THEN** each instance SHALL expose the same `RADIO_*` public contract family and rely on the instance name for meaning

### Requirement: Comms Public Contract
The subsystem SHALL own the `COMM_*`, `RADIO_*`, and `UART_*` command, telemetry, and event families required for band selection, pass control, UART health, radio status, and error reporting.

#### Scenario: Pass window state is observable
- **WHEN** an operator starts or stops a pass window
- **THEN** the subsystem SHALL surface the pass state, active band, and related events through its owned contracts

### Requirement: Comms Verification Modes
The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, and SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware is unavailable.

#### Scenario: Virtual UART validation
- **WHEN** a PTY pair is used to simulate a UART path
- **THEN** the subsystem SHALL treat that path as a valid first-version verification method for driver behavior and recovery handling
