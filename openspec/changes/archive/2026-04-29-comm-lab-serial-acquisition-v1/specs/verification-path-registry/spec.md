## ADDED Requirements

### Requirement: Lab Serial Acquisition Path Is Registered Separately
The verification-path registry SHALL register subsystem-origin lab serial acquisition separately from both the macOS-initiated subsystem UART preflight and any gateway-backed TT&C path.

#### Scenario: Registry separates acquisition from TT&C
- **WHEN** the lab serial acquisition probe passes
- **THEN** the registry SHALL identify the path as passive macOS acquisition of bounded frames transmitted by `subsystem.local`
- **AND** it SHALL state that the path does not prove full TT&C, stock F' event/telemetry downlink, RF, file/downlink, target OBC, or COMM shared CAN FD

#### Scenario: Registry remains unchanged if acquisition fails
- **WHEN** the lab serial acquisition probe fails
- **THEN** the registry SHALL NOT add the acquisition path as a proven validation path
