## ADDED Requirements

### Requirement: Core Recovery Status Surface Is Bounded
The core-system-contracts capability SHALL expose a bounded shared-recovery status surface on the active baseline.

#### Scenario: Shared recovery status is reviewable
- **WHEN** the operator issues `GET_RECOVERY_STATUS`
- **THEN** the active runtime SHALL expose the active incident source, highest recovery level reached, last recovery action, relatch truth, and pending reboot truth through reviewable telemetry and/or dedicated status events

#### Scenario: Bounded reboot and reset status is reviewable
- **WHEN** the operator issues `GET_RESET_CAUSE` or `GET_BOOT_COUNT`
- **THEN** the active runtime SHALL expose the current persisted reset-cause and boot-count truth through reviewable telemetry and/or dedicated status events

#### Scenario: Shared recovery status does not imply generic forced actions
- **WHEN** `recovery-executors-v1` adds shared recovery status surfaces
- **THEN** the active baseline SHALL NOT claim generic `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET`
- **AND** it MAY continue to expose bounded subsystem-specific reset commands that already exist outside that generic operator surface

## MODIFIED Requirements

### Requirement: Core Watchdog Status And Config Surface Is Bounded
The core-system-contracts capability SHALL expose a bounded watchdog public surface on the active baseline while keeping shared-recovery ownership separate from detector-local watchdog truth.

#### Scenario: Watchdog status is reviewable
- **WHEN** the operator issues `GET_WATCHDOG_STATUS`
- **THEN** the active runtime SHALL expose aggregate watchdog state together with per-source freshness, suppression, and detector-local escalation truth through reviewable telemetry and/or dedicated status events
- **AND** that status SHALL remain bounded to watchdog-local truth rather than replacing the separate shared-recovery status surface

#### Scenario: Watchdog config is mutable at runtime
- **WHEN** the operator issues `SET_WATCHDOG_CONFIG`
- **THEN** the active runtime SHALL validate the requested watchdog source and threshold values
- **AND** it SHALL update the in-memory watchdog config for that source only if the request is valid
- **AND** it SHALL emit reviewable config-update evidence

#### Scenario: V1 still does not claim unsupported generic restart executors
- **WHEN** `recovery-executors-v1` updates the core watchdog and recovery public surfaces
- **THEN** the active baseline SHALL NOT claim generic `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET` unless a later governed change adds truthful runtime executors for those operator-driven actions
