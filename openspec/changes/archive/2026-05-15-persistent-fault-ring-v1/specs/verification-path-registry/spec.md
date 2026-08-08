## ADDED Requirements

### Requirement: Hosted Persistent Fault Ring Relaunch Path Is Registered Separately
The verification-path registry SHALL register a distinct hosted persistent
fault ring relaunch path once repository-owned evidence proves same-runtime-root
relaunch history readback and newer-copy corruption fallback for the persistent
fault ring.

#### Scenario: Registry names the persistent ring relaunch boundary
- **WHEN** the persistent fault ring hosted probe passes
- **THEN** the registry SHALL identify the proven path as a hosted same-runtime-
  root relaunch boundary that records `RecoveryExecutor` lifecycle breadcrumbs,
  later `BootManager` boot breadcrumbs, and bounded history readback through
  `PersistentFaultManager`

#### Scenario: Registry keeps adjacent reboot and recovery paths distinct
- **WHEN** reviewers inspect the persistent fault ring relaunch entry
- **THEN** the registry SHALL keep boot metadata restart truth and the earlier
  shared recovery probe as adjacent prerequisite or neighboring paths rather
  than treating them as the same proof boundary
