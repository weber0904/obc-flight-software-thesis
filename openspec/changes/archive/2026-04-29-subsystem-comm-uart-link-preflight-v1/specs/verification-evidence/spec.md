## ADDED Requirements

### Requirement: Subsystem COMM UART Preflight Evidence Is Reviewable
The verification evidence tree SHALL record the host serial device, subsystem serial device, baudrate, launch commands, host peer log, subsystem probe log, observed bounded exchange, final verdict, and any known reverse-direction diagnostic limitations for the subsystem-side COMM UART preflight.

#### Scenario: Subsystem UART preflight evidence separates adjacent paths
- **WHEN** the subsystem COMM UART preflight change completes
- **THEN** reviewers SHALL be able to inspect which macOS and `subsystem.local` endpoints were used
- **AND** the evidence SHALL identify the newly proven path as macOS-initiated physical serial request/reply byte exchange with `subsystem.local`
- **AND** the evidence SHALL state whether clean subsystem-origin cold-first traffic into a passive macOS receiver was proven or remains unproven
- **AND** the evidence SHALL keep historical OBC-side UART comm, hosted PTY gateway, gateway-backed TT&C, RF, file/downlink, target OBC, and COMM shared CAN FD behavior out of the verdict boundary
