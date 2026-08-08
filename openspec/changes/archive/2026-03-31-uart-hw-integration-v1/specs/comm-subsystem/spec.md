## ADDED Requirements

### Requirement: Raspberry Pi Host UART Hardware Validation Path
The comm subsystem SHALL provide a governed validation path where the Raspberry Pi target runs the existing `CommController`, `RadioController`, and `UartDriver` stack against a development-host mock-radio peer over an explicit hardware serial device path, and this path SHALL preserve the same controller-layer behavior already validated through TCP mock and PTY-backed serial modes.

#### Scenario: Host mock-radio backend attaches to a real serial device
- **WHEN** an operator launches the hosted mock-radio backend for hardware-UART validation
- **THEN** the backend SHALL be able to open an explicit macOS serial device path and serve the existing mock-radio protocol over that hardware link instead of TCP or PTY

#### Scenario: Raspberry Pi target uses an explicit UART device path
- **WHEN** the Raspberry Pi target stack is launched for hardware-UART validation
- **THEN** the runtime SHALL accept an explicit target serial device path and run the external comm link through that device while keeping the controller-layer command and telemetry behavior unchanged

#### Scenario: Real serial hardware mode does not redefine the transport contract
- **WHEN** the project switches from PTY-backed validation to the Raspberry Pi to host hardware-UART path
- **THEN** the shared byte-stream transport contract SHALL remain the controller-facing interface and SHALL NOT require a protocol redesign
