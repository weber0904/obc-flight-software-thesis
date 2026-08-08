## ADDED Requirements

### Requirement: EPS Timeout Recovery Ownership Stays Shared
The EPS subsystem SHALL keep `EpsBridge` as the EPS poll-health owner and `EpsFdirController` as the EPS timeout detector while leaving later recovery action ownership inside the shared executor.

#### Scenario: EPS timeout detector does not regain direct recovery ownership
- **WHEN** `multi-subsystem-fdir-v1` broadens the shared recovery path to ADCS and COMM
- **THEN** `EpsBridge` SHALL continue to own EPS poll-health truth
- **AND** `EpsFdirController` SHALL continue to own EPS timeout retry, latch, and first-success clear truth
- **AND** `RecoveryExecutor` SHALL remain the only owner allowed to execute the bounded EPS reset, `SAFE`, or reboot-intent actions

#### Scenario: EPS timeout thresholds remain unchanged
- **WHEN** reviewers inspect the active EPS timeout detector after this change
- **THEN** the detector SHALL still treat failures `1-2` as retry-only
- **AND** it SHALL still latch EPS timeout fault at consecutive failure count `3`
- **AND** it SHALL still clear the latched EPS timeout on the first successful EPS poll
