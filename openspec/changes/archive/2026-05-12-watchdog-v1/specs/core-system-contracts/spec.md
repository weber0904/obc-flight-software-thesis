## ADDED Requirements

### Requirement: WatchdogSupervisor Owns Core Resource And Watchdog Surfaces

The core-system-contracts capability SHALL provide concrete F' component interfaces for `ModeManager`, `WatchdogSupervisor`, and `CspBridge`, and those components SHALL own the `MODE_*`, core resource-monitoring `HEALTH_*`, watchdog status/config, `SYS_*`, and `CSP_*` public symbol families assigned to this capability.

#### Scenario: Core component ownership is visible after watchdog-v1

- **WHEN** `watchdog-v1` is implemented
- **THEN** the source tree SHALL contain project-local component definitions for `ModeManager`, `WatchdogSupervisor`, and `CspBridge`
- **AND** the separate `HealthMonitor` owner SHALL no longer remain as the active core owner for resource-monitoring public symbols

### Requirement: Core Resource Monitoring Survives Owner Migration

The core-system-contracts capability SHALL preserve the existing resource-monitoring behavior while moving that behavior under `WatchdogSupervisor`.

#### Scenario: Resource monitoring remains operator-visible

- **WHEN** an operator uses the active baseline after `watchdog-v1`
- **THEN** the deployment SHALL still expose resource-monitoring enable and threshold commands
- **AND** it SHALL still expose CPU and RSS review surfaces through the owned `SYS_*` telemetry and warning events
- **AND** those surfaces SHALL now be owned by `WatchdogSupervisor` rather than a separate `HealthMonitor`

### Requirement: Core Watchdog Status And Config Surface Is Bounded

The core-system-contracts capability SHALL expose a bounded watchdog public surface on the active baseline.

#### Scenario: Watchdog status is reviewable

- **WHEN** the operator issues `GET_WATCHDOG_STATUS`
- **THEN** the active runtime SHALL expose aggregate watchdog state together with per-source freshness or escalation truth through reviewable telemetry and/or dedicated status events

#### Scenario: Watchdog config is mutable at runtime

- **WHEN** the operator issues `SET_WATCHDOG_CONFIG`
- **THEN** the active runtime SHALL validate the requested watchdog source and threshold values
- **AND** it SHALL update the in-memory watchdog config for that source only if the request is valid
- **AND** it SHALL emit reviewable config-update evidence

#### Scenario: V1 does not claim unsupported restart executors

- **WHEN** `watchdog-v1` updates the core watchdog public surface
- **THEN** the active baseline SHALL NOT claim `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET` unless a later governed change adds a truthful runtime executor for those actions
