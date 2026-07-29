## ADDED Requirements

### Requirement: Active COMM SocketCAN Channel Role
The platform baseline SHALL allow the subsystem-side `can1` channel to move from reserved-channel evidence into active COMM node `4` participation on the shared CAN FD-capable bus through a governed COMM SocketCAN TT&C change.

#### Scenario: COMM can1 role change is explicit
- **WHEN** the repository proves COMM SocketCAN TT&C participation
- **THEN** it SHALL record that `subsystem.local:can1` is active for COMM node `4`
- **AND** it SHALL preserve the earlier reserved-channel isolation evidence as historical to the EPS/ADCS-only SocketCAN slice

#### Scenario: CAN probes prepare interfaces before verdict
- **WHEN** a repository-owned hardware CAN smoke or probe judges physical SocketCAN connectivity
- **THEN** it SHALL first bring the required CAN interfaces `UP` with explicit bit timing
- **AND** a missing interface or failed privileged bring-up SHALL be reported as setup failure rather than physical bus failure

#### Scenario: Shared bus is not dual-bus redundancy
- **WHEN** COMM joins the shared SocketCAN carrier
- **THEN** the evidence SHALL describe one shared bus with `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL NOT claim dual-bus redundancy
