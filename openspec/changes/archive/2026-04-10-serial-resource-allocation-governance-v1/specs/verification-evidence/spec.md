## ADDED Requirements

### Requirement: Serial Resource Conflicts Are Named In Constrained Evidence
When a hardware validation path is blocked because the required serial resource is already owned by another subsystem, the evidence SHALL name the conflicting allocation instead of using a generic hardware-blocked note.

#### Scenario: GPS live UART is blocked by comm serial ownership
- **WHEN** GPS live UART remains unvalidated because the Raspberry Pi `/dev/serial0` UART is reserved for external comm
- **THEN** the GPS constrained evidence SHALL cite that serial ownership conflict and name the condition required to clear it
