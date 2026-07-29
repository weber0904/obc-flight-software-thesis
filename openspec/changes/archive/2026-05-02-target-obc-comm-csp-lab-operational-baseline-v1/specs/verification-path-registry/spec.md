## ADDED Requirements

### Requirement: Lab Target COMM CSP Operational Path Is Registered Separately
The verification-path registry SHALL register the lab target COMM CSP operational path separately from the earlier probe-owned physical COMM SocketCAN command/event/channel and file/downlink paths.

#### Scenario: Registry names the service-managed lab path
- **WHEN** the lab target operational evidence passes
- **THEN** the registry SHALL identify the newly reusable path as service-managed `obc.local` installed OBC plus service-managed `subsystem.local` EPS/ADCS/COMM, macOS `fprime-gds` plus `ground_ttc_gateway`, physical lab serial ingress, and shared SocketCAN COMM path

#### Scenario: Registry distinguishes probe evidence from operational baseline
- **WHEN** reviewers inspect the registry entry
- **THEN** it SHALL cite the earlier SocketCAN TT&C and SocketCAN file/downlink entries as supporting adjacent evidence
- **AND** it SHALL state that the new entry proves the service-managed lab operational path rather than introducing a new COMM wire contract

#### Scenario: Registry excludes flight and future communications claims
- **WHEN** the lab target operational path is registered
- **THEN** it SHALL explicitly state that final flight deployment, RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning remain unproven
