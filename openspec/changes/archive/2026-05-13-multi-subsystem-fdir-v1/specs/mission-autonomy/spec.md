## ADDED Requirements

### Requirement: RecoveryExecutor Owns Bounded Multi-Subsystem Recovery
The mission-autonomy capability SHALL use one shared `RecoveryExecutor` owner for bounded `EPS`, `ADCS`, and `COMM` detector incidents on the active baseline.

#### Scenario: Three bounded subsystem lines share one recovery owner
- **WHEN** `multi-subsystem-fdir-v1` is implemented
- **THEN** `RecoveryExecutor` SHALL remain distinct from `EpsFdirController`, `AdcsFdirController`, `CommController`, `WatchdogSupervisor`, `ModeSafetyController`, and `BootManager`
- **AND** the active baseline SHALL limit shared subsystem recovery consumers to `EPS`, `ADCS`, and `COMM`
- **AND** it SHALL NOT claim GPS, payload, TTC, scheduler, or storage-health recovery ownership in this change

### Requirement: RecoveryExecutor Uses Short-Lock Decision And Lock-Free Action Execution
The mission-autonomy capability SHALL keep `RecoveryExecutor` incident normalization and progression decisions under a bounded internal lock while executing external recovery actions after that lock is released.

#### Scenario: External recovery action does not run under the executor lock
- **WHEN** `RecoveryExecutor` issues `SAFE`, EPS reset, COMM failover, or reboot intent
- **THEN** it SHALL decide the next action while holding its internal recovery mutex
- **AND** it SHALL execute the external runtime action only after releasing that mutex
- **AND** it SHALL later record the action outcome without requiring external runtime code to reenter the same locked region

### Requirement: Recovery Progression Is Deterministic Per Incident
The mission-autonomy capability SHALL keep explicit incident epoch, relatch, level, and clear state for each active shared recovery source.

#### Scenario: Repeated failure escalates instead of looping forever
- **WHEN** a subsystem incident relatches after clear or remains uncleared for `3` executor ticks after its first bounded action
- **THEN** `RecoveryExecutor` SHALL advance that incident to `R6_OBC_REBOOT`
- **AND** it SHALL NOT reissue the same bounded action forever for the same incident epoch

### Requirement: ADCS FDIR Uses Scheduled Poll Health Only
The mission-autonomy capability SHALL treat ADCS shared recovery incidents as detector outputs derived only from scheduled ADCS poll health.

#### Scenario: ADCS command-path failures do not become FDIR incidents
- **WHEN** `ADCS_GET_ATTITUDE`, `ADCS_SET_MODE`, `ADCS_SET_TARGET`, or `ADCS_CALIBRATE` encounters transport failure outside the scheduled poll
- **THEN** the active runtime MAY emit local ADCS comm-error evidence
- **AND** it SHALL NOT advance the scheduled ADCS FDIR counters or latch a shared ADCS recovery incident from that command-path failure alone

#### Scenario: ADCS scheduled poll thresholds are fixed
- **WHEN** the active baseline evaluates scheduled ADCS poll health in this change
- **THEN** it SHALL latch `ADCS_POLL_TRANSPORT` after `3` consecutive scheduled transport failures
- **AND** it SHALL latch `ADCS_POLL_FRESHNESS` after `3` consecutive scheduled no-valid-refresh cycles
- **AND** it SHALL clear either ADCS incident on the first scheduled healthy valid cycle

### Requirement: COMM Detector Is Separate From COMM Recovery Actuation
The mission-autonomy capability SHALL keep COMM fault detection in `CommController` while making fault-driven COMM recovery actuation executor-owned.

#### Scenario: COMM detector does not switch primary links directly
- **WHEN** the current primary COMM link becomes faulted by repeated unavailability or repeated transport-error growth
- **THEN** `CommController` SHALL emit reviewable COMM detector fault truth
- **AND** it SHALL NOT directly switch primary links, revoke the current primary session, or clear downlink ownership as part of detector-side fault handling

#### Scenario: COMM scheduled thresholds are fixed
- **WHEN** the active baseline evaluates COMM detector health in this change
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE` after `3` consecutive scheduled cycles where the current primary link is unavailable
- **AND** it SHALL latch `COMM_PRIMARY_TRANSPORT` after `3` consecutive scheduled cycles with primary-link `tx/rx` error growth
- **AND** it SHALL clear the COMM fault on the first scheduled cycle where the current primary link is available and has no new `tx/rx` error growth

#### Scenario: COMM recovery action stays bounded
- **WHEN** `RecoveryExecutor` performs COMM recovery in this change
- **THEN** the real action SHALL be bounded to failover, session revoke, and downlink-owner clear as needed
- **AND** it SHALL NOT claim a new COMM hardware reset plane, RF recovery, or automatic nominal-link restore in this change

## MODIFIED Requirements

### Requirement: WatchdogSupervisor Owner Is Separate From Mode Safety And EPS Timeout FDIR
The mission-autonomy capability SHALL provide a focused `WatchdogSupervisor` owner for runtime liveness supervision, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController`, the subsystem FDIR detectors, and the shared `RecoveryExecutor`.

#### Scenario: ADCS FDIR joins the bounded watchdog supervised set
- **WHEN** `multi-subsystem-fdir-v1` is implemented
- **THEN** the active runtime SHALL instantiate, configure, and schedule `AdcsFdirController`
- **AND** the watchdog supervised source set SHALL include `ADCS_FDIR` in addition to the previously governed sources
- **AND** `RecoveryExecutor` SHALL normalize that stale source as `WATCHDOG_ADCS_FDIR`

### Requirement: EPS Timeout Escalation Uses Normal Mode Surface
The mission-autonomy capability SHALL keep EPS timeout faults on the shared recovery path while preserving the existing EPS detector-only contract.

#### Scenario: EPS timeout still uses the shared executor as the action owner
- **WHEN** the EPS timeout detector first latches fault at consecutive failure count `3`
- **THEN** `EpsFdirController` SHALL remain responsible only for EPS-local retry, latch, and first-success clear truth
- **AND** `RecoveryExecutor` SHALL remain the only runtime owner allowed to execute the later EPS reset, `SAFE`, or reboot-intent actions
