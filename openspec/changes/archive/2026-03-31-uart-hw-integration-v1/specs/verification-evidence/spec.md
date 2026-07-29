## ADDED Requirements

### Requirement: Raspberry Pi Host UART Hardware Evidence
The verification evidence tree SHALL record the commands, device-path selections, observed link behavior, and final verdict for the governed Raspberry Pi to development-host UART/RS485 hardware validation flow that exercises the existing comm stack against the hosted mock-radio peer.

#### Scenario: Hardware UART flow is reviewable
- **WHEN** the Raspberry Pi to host hardware-UART validation change completes
- **THEN** reviewers SHALL be able to inspect the host peer launch path, the Raspberry Pi launch path, the chosen serial device identifiers, and the observed comm outcome from the repository evidence tree

#### Scenario: Remaining hardware gaps stay explicit
- **WHEN** the Raspberry Pi to host hardware-UART path passes but real radio or other external hardware validation is still not available
- **THEN** the evidence SHALL mark only the unresolved sub-cases as `Blocked-HW` instead of treating the full hardware-UART path as blocked
