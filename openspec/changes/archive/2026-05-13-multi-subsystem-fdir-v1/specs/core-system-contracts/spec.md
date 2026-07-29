## MODIFIED Requirements

### Requirement: Core Recovery Status Surface Is Bounded
The core-system-contracts capability SHALL expose a bounded shared-recovery status surface on the active baseline.

#### Scenario: Shared recovery status covers EPS, ADCS, and COMM incidents
- **WHEN** the operator issues `GET_RECOVERY_STATUS` after `multi-subsystem-fdir-v1`
- **THEN** the active runtime SHALL be able to report bounded shared recovery source, current and highest level, last action, relatch truth, and pending reboot truth for `EPS`, `ADCS`, and `COMM` incidents in addition to watchdog incidents

#### Scenario: COMM recovery action truth is reviewable without implying reset cause
- **WHEN** the shared recovery owner performs bounded COMM failover actuation
- **THEN** the active runtime SHALL expose that COMM recovery action through recovery status, events, and/or telemetry
- **AND** it SHALL NOT imply that a COMM failover action alone rewrote persisted boot reset-cause truth

#### Scenario: Only reboot intent changes persisted reset-cause truth
- **WHEN** the operator later inspects `GET_RESET_CAUSE`
- **THEN** the persisted reset-cause value SHALL change only after a bounded shared recovery reboot intent
- **AND** non-reboot actions such as restart intent, subsystem interface reset, `SAFE` fallback, or COMM failover SHALL remain outside persisted reset-cause truth
