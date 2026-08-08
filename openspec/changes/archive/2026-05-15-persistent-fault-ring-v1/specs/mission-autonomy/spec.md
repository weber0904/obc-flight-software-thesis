## ADDED Requirements

### Requirement: RecoveryExecutor Persists Shared Recovery Lifecycle Breadcrumbs
The mission-autonomy capability SHALL make `RecoveryExecutor` append persistent
fault ring breadcrumbs for shared recovery incident open, action request,
action execution, reboot pending, reboot issued, and incident clear lifecycle
transitions.

#### Scenario: First shared incident opening records lifecycle start
- **WHEN** `RecoveryExecutor` accepts a new shared recovery incident for an
  active source in this change
- **THEN** it SHALL append an incident-open breadcrumb with the bounded source,
  level, and incident epoch context
- **AND** later action selection SHALL append action-requested and
  action-executed breadcrumbs as that incident progresses

#### Scenario: Reboot escalation records pending and issued transitions
- **WHEN** shared recovery reaches reboot intent and later issues the reboot-
  equivalent action
- **THEN** `RecoveryExecutor` SHALL append distinct reboot-pending and reboot-
  issued breadcrumbs
- **AND** those breadcrumbs SHALL remain separate from `BootManager`'s later
  boot-observed truth after relaunch

#### Scenario: Explicit clear records incident closure
- **WHEN** a detector later clears an active incident through the shared
  recovery path
- **THEN** `RecoveryExecutor` SHALL append an incident-cleared breadcrumb for
  that incident epoch
- **AND** it SHALL NOT infer closure only from mode changes or reboot outcomes

### Requirement: Detector-Local Owners Do Not Duplicate Persistent Writes
The mission-autonomy capability SHALL keep detector-local fault ownership in
`WatchdogSupervisor`, `EpsFdirController`, `AdcsFdirController`, and
`CommController` while routing shared persistent breadcrumb writes through
`RecoveryExecutor` only.

#### Scenario: Detector surfaces stay detector-local
- **WHEN** watchdog, EPS, ADCS, or COMM detector logic raises or clears a shared
  recovery incident
- **THEN** the detector SHALL continue to use its existing detector-local public
  contract
- **AND** it SHALL NOT append a parallel detector-owned persistent fault record
  directly in v1

