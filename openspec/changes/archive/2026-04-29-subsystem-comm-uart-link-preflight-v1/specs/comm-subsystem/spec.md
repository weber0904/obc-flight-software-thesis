## ADDED Requirements

### Requirement: Subsystem COMM UART Link Preflight
The comm subsystem SHALL provide a governed preflight validation path for the macOS-initiated physical UART request/reply link between the macOS development host and `subsystem.local`, and that path SHALL use explicit serial device paths on both hosts.

#### Scenario: Subsystem serial link exchanges bounded mock-text traffic
- **WHEN** `subsystem.local` runs the existing mock-text serial peer on an explicit subsystem serial device
- **AND** the macOS host runs the serial probe on an explicit host serial device
- **THEN** the macOS-initiated probe SHALL exchange bounded `STATUS`, `ENABLE 1`, and final `STATUS` requests over the physical serial link
- **AND** the final observed status SHALL show the link carried the enabled state update

#### Scenario: Subsystem cold-first traffic is not implied
- **WHEN** the subsystem UART preflight evidence is reused by later COMM work
- **THEN** the repository SHALL treat clean `subsystem.local` cold-first traffic into a passive macOS receiver as unproven unless later evidence proves that acquisition behavior separately

#### Scenario: Subsystem UART preflight stays distinct from TT&C
- **WHEN** the subsystem UART preflight evidence is recorded
- **THEN** the repository SHALL describe it as physical serial byte-exchange evidence only
- **AND** it SHALL NOT describe that result as clean subsystem-origin cold-first downlink, gateway-backed TT&C, RF, real radio, file/downlink, target OBC, or COMM shared CAN FD validation
