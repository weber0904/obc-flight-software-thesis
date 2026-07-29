## ADDED Requirements

### Requirement: Transparent Link Framing Evidence
The verification evidence tree SHALL record the frame format, representative payload bytes, framing mode selected, commands used, and observed framed payload exchange result for the governed transparent link framing path.

#### Scenario: Framed transparent validation is reviewable
- **WHEN** the transparent link framing change completes
- **THEN** reviewers SHALL be able to inspect the selected host-peer mode, the target launch path, representative binary-safe payloads, and the observed framing/deframing outcome from the repository evidence tree

### Requirement: Framed Transparent Scope Stays Distinct From Ground-Station Integration
The evidence for the framed transparent path SHALL explicitly distinguish transparent link framing from the direct TCP/GDS baseline and from any still-unimplemented ground gateway, RF behavior, or vendor-specific control/configuration work.

#### Scenario: Framed transparent pass does not imply full ground-station integration
- **WHEN** the framed transparent path passes
- **THEN** the evidence SHALL mark only the transparent framed UART/RS485 link behavior as passed and SHALL keep ground-gateway, RF, and vendor-specific control work explicit as future scope or `Blocked-HW`, whichever applies
