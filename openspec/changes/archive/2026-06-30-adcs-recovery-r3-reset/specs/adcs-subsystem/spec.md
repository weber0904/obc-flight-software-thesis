## MODIFIED Requirements

### Requirement: ADCS CSP Host Protocol
The ADCS subsystem SHALL define ADCS-owned CSP service request and response
payloads for the hosted simulator and the OBC-side bridge, and those payloads
SHALL preserve the formal node `3` contract while avoiding libcsp reserved
service ports `0` through `3`. ADCS runtime DTO and result semantics SHALL
remain ADCS-owned support types and SHALL NOT be treated as a shared
cross-subsystem on-wire protocol.

#### Scenario: ADCS request includes a sequence and returns one state payload
- **WHEN** the hosted bridge requests ADCS state
- **THEN** the protocol SHALL carry a sequence field and SHALL return the
  required ADCS state in a single response payload

#### Scenario: ADCS reset uses an owned non-reserved service port
- **WHEN** the active hosted ADCS CSP protocol exposes a recovery reset service
- **THEN** that service SHALL use an ADCS-owned application port outside libcsp
  reserved ports `0` through `3`
- **AND** it SHALL remain distinct from the existing ADCS state, mode, target,
  and calibration services

### Requirement: Hosted ADCS Simulator
The hosted ADCS implementation SHALL provide a simulator executable that serves
state, mode, target, calibration, and recovery-reset services as libcsp node
`3` over the hosted ZMQHUB-backed internal CSP substrate while maintaining
deterministic quaternion, angular-rate, pointing-error, and sensor-validity
state.

#### Scenario: Reset command restores the deterministic default ADCS state
- **WHEN** a client issues the ADCS reset service over the hosted internal CSP
  substrate after earlier ADCS mode or target changes
- **THEN** the simulator SHALL restore the deterministic default ADCS state,
  including its quaternion, target quaternion, angular rates, sensor-validity
  state, and derived pointing/magnetometer fields
- **AND** the reply SHALL return that restored ADCS state through the normal
  ADCS response payload

### Requirement: ADCS Shared Recovery Incidents Are Bounded
The ADCS subsystem SHALL support one bounded ADCS FDIR detector that normalizes
scheduled poll-health failures into shared recovery incidents without making
every local sensor fault a shared FDIR fault.

#### Scenario: Scheduled thresholds drive shared recovery
- **WHEN** the ADCS FDIR detector evaluates scheduled poll health in this
  change
- **THEN** it SHALL latch `ADCS_POLL_TRANSPORT` after `3` consecutive scheduled
  transport failures
- **AND** it SHALL latch `ADCS_POLL_FRESHNESS` after `3` consecutive scheduled
  no-valid-refresh cycles
- **AND** it SHALL clear the active ADCS detector fault on the first scheduled
  healthy valid cycle

#### Scenario: ADCS shared recovery starts with subsystem-interface reset
- **WHEN** the active runtime opens an `ADCS_POLL_TRANSPORT` or
  `ADCS_POLL_FRESHNESS` shared recovery incident for the first time
- **THEN** the incident SHALL begin at
  `R3_RESET_SUBSYSTEM_INTERFACE`
- **AND** the first recovery action SHALL be the ADCS subsystem CSP reset
  service rather than process restart persistence
- **AND** higher-level relatch escalation MAY continue through the existing
  shared recovery logic after that first ADCS reset action
