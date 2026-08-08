## ADDED Requirements

### Requirement: Multi-Subsystem FDIR Evidence Records Shared Recovery Closure
The verification evidence SHALL record reviewable local and hosted proof for `multi-subsystem-fdir-v1`, including focused component/helper coverage, classic F' component coverage, hosted shared-recovery probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover bounded detector and executor behavior
- **WHEN** `multi-subsystem-fdir-v1` completes
- **THEN** evidence SHALL list focused coverage for:
  - shared recovery source and action mapping
  - lock-outside-action execution for `RecoveryExecutor`
  - unchanged EPS timeout behavior
  - scheduled ADCS poll-health thresholds and clear behavior
  - command-triggered ADCS transport failure staying outside shared ADCS FDIR
  - COMM detector fault truth without detector-side primary-link switching
  - `WATCHDOG_ADCS_FDIR` supervision and normalization
  - reboot-only boot reset-cause truth for ADCS and COMM escalation

#### Scenario: Hosted probe proves bounded three-subsystem closure
- **WHEN** the repository-owned hosted `multi-subsystem-fdir-v1` probe runs
- **THEN** the evidence SHALL include the probe command, isolated runtime roots or ports, the bounded EPS, ADCS, and COMM fault-injection method, the shared recovery status observations, the reboot-equivalent relaunch observations, and the final verdict
- **AND** it SHALL separately state what the hosted path does not prove

#### Scenario: Scope boundaries remain explicit
- **WHEN** `multi-subsystem-fdir-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove GPS, payload, TTC pass scheduling, storage-health recovery, generic all-subsystem FDIR, COMM RF recovery, target-hardware reset proof, persistent recovery configuration, or a broader spacecraft fault-protection platform
