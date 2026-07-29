## ADDED Requirements

### Requirement: Radio Protocol Adapter Strategy
The comm subsystem SHALL route radio command/status framing through a protocol adapter layer that sits below `RadioController` and above the shared byte-stream transport, so future framing changes do not require controller or transport redesign.

#### Scenario: Controller logic stays protocol-agnostic
- **WHEN** `RadioController` requests status, enable, power, or frequency operations
- **THEN** it SHALL continue to use the same controller-visible command, telemetry, and event behavior regardless of which supported radio protocol adapter is selected underneath

#### Scenario: Default adapter preserves hosted mock behavior
- **WHEN** the runtime uses the default first-version radio protocol adapter
- **THEN** the subsystem SHALL preserve the existing hosted text mock-radio request/response behavior already validated through TCP, PTY, and Raspberry Pi UART paths

#### Scenario: Future protocol selection is configuration-driven
- **WHEN** the project later adds KISS or a vendor-specific adapter
- **THEN** the selected adapter SHALL be chosen through runtime or profile configuration rather than by rewriting `RadioController` or the byte-stream transport implementation
