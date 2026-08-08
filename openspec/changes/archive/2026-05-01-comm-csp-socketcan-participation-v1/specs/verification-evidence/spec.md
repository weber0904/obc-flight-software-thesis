## ADDED Requirements

### Requirement: COMM SocketCAN TT&C Evidence
The repository SHALL maintain an evidence record for COMM SocketCAN TT&C that captures corrected CAN bring-up, physical interface health, gateway-backed TT&C observations, and excluded adjacent paths.

#### Scenario: Evidence records corrected Stage 0
- **WHEN** the change records Stage 0 CAN health evidence
- **THEN** it SHALL state that the probe brought up `obc.local:can0` and `subsystem.local:can0` before checking EPS/ADCS reachability
- **AND** it SHALL preserve the distinction between setup failures and physical bus failures

#### Scenario: Evidence records active CAN interfaces
- **WHEN** the formal COMM SocketCAN TT&C probe passes
- **THEN** the evidence SHALL record `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1` as active CAN interfaces
- **AND** it SHALL include pre/post health or statistics showing `ERROR-ACTIVE` and no `bus-off`

#### Scenario: Evidence records TT&C observations
- **WHEN** bounded TT&C is claimed
- **THEN** the evidence SHALL include OBC command readback, fprime-cli command events, `GROUND_LINK_TX_BYTES`, and non-empty CAN capture evidence

#### Scenario: Evidence excludes adjacent paths
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** it SHALL keep file/downlink, RF, no-preamble serial behavior, dual-bus redundancy, and independent COMM hardware outside the proven verdict
