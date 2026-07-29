## ADDED Requirements

### Requirement: EPS Timeout FDIR Owner Is Separate From Mode Safety
The mission-autonomy capability SHALL provide a focused `EpsFdirController` owner for EPS timeout FDIR, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController`.

#### Scenario: EPS timeout FDIR does not broaden ModeSafetyController
- **WHEN** `fdir-subsystem-timeout-v1` is implemented
- **THEN** the active runtime SHALL instantiate and schedule a focused EPS timeout FDIR owner
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy instead of taking subsystem timeout, retry, or recovery ownership

### Requirement: EPS Timeout FDIR Uses Consecutive Poll Failures Only
The mission-autonomy capability SHALL define EPS timeout FDIR v1 from the EPS runtime poll-health contract using consecutive poll failures only.

#### Scenario: First two failures are retry-only
- **WHEN** the EPS runtime health provider reports consecutive poll-failure count `1` or `2`
- **THEN** the EPS timeout FDIR owner SHALL treat the subsystem as retrying only
- **AND** it SHALL NOT request `SAFE`

#### Scenario: Third consecutive failure latches fault
- **WHEN** the EPS runtime health provider reports consecutive poll-failure count `3`
- **THEN** the EPS timeout FDIR owner SHALL latch an EPS fault
- **AND** it SHALL emit fault or escalation evidence for that first threshold crossing

#### Scenario: V1 does not add timestamp freshness
- **WHEN** the EPS timeout FDIR owner evaluates EPS poll-health state in this change
- **THEN** it SHALL use consecutive poll failures only
- **AND** it SHALL NOT add timestamp freshness, age-threshold logic, or mixed failure-plus-freshness policy in this change

### Requirement: EPS Timeout Escalation Uses Normal Mode Surface
The mission-autonomy capability SHALL make EPS timeout escalation request `SAFE` through the existing internal mode-control path instead of inventing a parallel mode store.

#### Scenario: Active modes escalate to SAFE on first threshold crossing
- **WHEN** the EPS timeout FDIR owner first latches fault at consecutive failure count `3`
- **AND** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **THEN** it SHALL request `SAFE` through the normal internal mode-control runtime path
- **AND** it SHALL tag that request with a distinct subsystem-fault internal apply source

#### Scenario: SAFE and HELL are fault-only cases
- **WHEN** the EPS timeout FDIR owner first latches fault at consecutive failure count `3`
- **AND** the current mode is `SAFE` or `HELL`
- **THEN** it SHALL record the fault condition
- **AND** it SHALL NOT request another mode change

#### Scenario: Repeated failures do not re-escalate while latched
- **WHEN** the EPS timeout FDIR owner is already fault-latched
- **AND** EPS poll failures continue
- **THEN** it SHALL NOT emit repeated escalation requests
- **AND** it SHALL NOT re-request `SAFE`

### Requirement: EPS Timeout Recovery Clears On First Success
The mission-autonomy capability SHALL clear the EPS timeout fault latch on the first successful EPS poll after a fault.

#### Scenario: First success clears latched fault
- **WHEN** the EPS timeout FDIR owner is fault-latched
- **AND** the EPS runtime health provider reports a later successful poll
- **THEN** it SHALL clear the fault latch
- **AND** it SHALL emit recovered or fault-cleared evidence

#### Scenario: Recovery does not auto-restore prior mode
- **WHEN** the EPS timeout FDIR owner clears a latched fault after a successful poll
- **THEN** it SHALL NOT automatically restore the pre-fault mode
- **AND** it SHALL leave later recovery-mode decisions to future governed changes
