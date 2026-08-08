## ADDED Requirements

### Requirement: Legacy EnduroSat Transparent UART Validation Path
The comm subsystem SHALL provide a governed validation path for legacy EnduroSat-style UART transparent mode that uses the existing serial byte-stream transport and the OBC-side raw UART exchange path to validate payload delivery without requiring the current `mock-text` request/response grammar.

#### Scenario: Legacy transparent peer is selected for hardware-adjacent validation
- **WHEN** the project launches the legacy EnduroSat transparent-UART validation flow
- **THEN** the host-side peer SHALL behave as a transparent serial endpoint for payload exchange rather than as the current command/status mock-radio peer

#### Scenario: Transparent validation reuses the existing byte-stream transport
- **WHEN** the Raspberry Pi target validates the legacy EnduroSat transparent-UART path
- **THEN** the flow SHALL reuse the existing serial byte-stream transport contract and SHALL NOT require a controller-layer redesign

### Requirement: Transparent UART Path Stays Distinct From Controller-Oriented Mock Radio
The comm subsystem SHALL keep the controller-oriented `mock-text` radio baseline and the legacy EnduroSat transparent-UART validation path as separate governed modes so that controller regression and transparent data-path validation remain distinguishable.

#### Scenario: Default comm regression still uses the existing mock radio
- **WHEN** the standard hosted or Raspberry Pi comm regression suite runs without selecting the legacy transparent path
- **THEN** the subsystem SHALL continue to use the current `mock-text`-based radio validation behavior

#### Scenario: Transparent mode does not over-claim unsupported control semantics
- **WHEN** the legacy transparent-UART path is exercised
- **THEN** the subsystem SHALL treat that validation as a data-plane-first path and SHALL NOT claim that legacy EnduroSat control/configuration behavior is fully implemented unless a later change adds that control plane explicitly
