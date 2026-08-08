## MODIFIED Requirements

### Requirement: Spacecraft-Side Internal Bus Direction Is Shared CAN FD
The near-term spacecraft-side physical carrier direction SHALL treat `EPS`, `ADCS`, and `COMM` as future CSP-facing subsystems whose traffic is intended to converge onto a shared `CAN FD` bus architecture, while direct sensor paths such as GPS remain separate where required.

#### Scenario: First governed physical internal carrier uses a CAN FD-capable SocketCAN bus
- **WHEN** the repository proves the first physical internal CSP carrier
- **THEN** that proof SHALL use a CAN FD-capable Linux SocketCAN bus for `EPS` and `ADCS`, SHALL keep GPS on direct OBC UART, and SHALL keep the direct `GDS -> OBC` path separate from the internal carrier change

### Requirement: Near-Term Hardware Role Allocation Is Reviewable
The platform baseline SHALL document the near-term host and bus role allocation used for future communication architecture work, including `obc.local` as the OBC host, `subsystem.local` as the subsystem-side host, direct OBC UART for GPS, and dual subsystem-side CAN channel groups where `EPS/ADCS` share one group and `COMM` uses the other.

#### Scenario: First physical internal CSP proof leaves the COMM channel reserved
- **WHEN** the first governed shared-bus proof is recorded
- **THEN** it SHALL identify one subsystem CAN channel as the active `EPS/ADCS` path and SHALL record the second subsystem channel as reserved and self-tested rather than as an active COMM path
