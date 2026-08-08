## ADDED Requirements

### Requirement: GPS Live UART Requires Non-Conflicting Serial Allocation
The GPS subsystem SHALL keep live Raspberry Pi UART bring-up outside the validated baseline until a non-conflicting serial device allocation is approved, because the current `/dev/serial0` allocation belongs to the external comm/radio path.

#### Scenario: GPS live UART remains blocked while serial ownership is unresolved
- **WHEN** GPS live UART is discussed before a separate USB-UART, secondary UART, or other hardware allocation is approved
- **THEN** the GPS evidence and matrix SHALL mark that work as `Blocked-HW` with the reason that `/dev/serial0` is reserved for comm
