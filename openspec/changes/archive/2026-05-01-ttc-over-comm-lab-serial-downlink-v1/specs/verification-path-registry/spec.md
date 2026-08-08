## ADDED Requirements

### Requirement: Physical Lab Serial Bounded TT&C Path Is Registered Separately
The verification-path registry SHALL register bounded physical lab serial TT&C only after evidence proves both bounded physical serial command ingress and bounded event/telemetry downlink over the COMM path.

#### Scenario: Registry names the bounded physical TT&C boundary
- **WHEN** the physical lab serial downlink probe passes
- **THEN** the registry SHALL identify the newly proven path as `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local comm_csp_node -> CSP -> hosted OBC -> COMM downlink -> ground_ttc_gateway -> GDS -> fprime-cli`
- **AND** it SHALL state that the proven scope is bounded command readback plus command-event and telemetry visibility

#### Scenario: Registry keeps uplink ingress and full TT&C distinct
- **WHEN** reviewers inspect the physical lab serial TT&C registry entry
- **THEN** the registry SHALL keep the prior physical lab serial uplink ingress entry as a narrower adjacent path
- **AND** it SHALL not treat hosted PTY TT&C, UART preflight, or subsystem-origin acquisition as the same proof boundary

#### Scenario: Registry excludes future COMM work
- **WHEN** the bounded physical lab serial TT&C path is registered
- **THEN** the registry SHALL explicitly state that file/downlink, RF, target OBC migration, no-preamble first-byte-clean behavior, and COMM shared CAN FD participation remain unproven by that evidence
