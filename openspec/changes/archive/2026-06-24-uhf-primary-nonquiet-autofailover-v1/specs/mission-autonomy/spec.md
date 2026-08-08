## ADDED Requirements

### Requirement: Current Route Closure May Reuse COMM Primary-Unavailable Recovery As Autonomous UHF Promotion

The mission-autonomy baseline SHALL treat the existing
`COMM_PRIMARY_UNAVAILABLE` detector plus executor-owned COMM recovery action as
the maintained autonomous promotion path for current target route closure.

#### Scenario: Detector truth remains separate from action ownership
- **WHEN** the current primary COMM link becomes unavailable
- **THEN** `CommController` SHALL continue to own only the detector fault truth
- **AND** the current route closure SHALL cite `RecoveryExecutor` as the owner
  of the actual failover actuation.

#### Scenario: Current route closure does not replace autonomous failover with manual switch
- **WHEN** current target evidence is recorded for Route 2, Route 3, or the
  maintained failover proof family
- **THEN** it SHALL use the detector-triggered recovery path as the current
  autonomous UHF promotion truth
- **AND** it SHALL NOT restate a manual `COMM_SET_ACTIVE(UHF)` operator step as
  the maintained failover boundary.
