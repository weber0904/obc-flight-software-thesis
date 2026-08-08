## ADDED Requirements

### Requirement: Raspberry Pi Serial0 Is Reserved For External Comm
The comm subsystem SHALL own the default Raspberry Pi `/dev/serial0` allocation for the external comm/radio UART path until a later governed hardware architecture change explicitly reallocates or replaces that resource.

#### Scenario: Other subsystems cannot silently reuse the comm UART
- **WHEN** a future subsystem needs a live UART device on the Raspberry Pi target
- **THEN** it SHALL use a separately governed device allocation and SHALL NOT assume `/dev/serial0` is available while the comm path owns it
