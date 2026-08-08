# persistent-fault-ring Specification

## Purpose
Define the active baseline for persistent recovery breadcrumb storage, including
the single public `PersistentFaultManager` owner, the dual-copy
whole-file-snapshot ring under `persistent-data/recovery/`, and the bounded
hosted or command readback surfaces that keep this history distinct from
official `.fdp` mission-history products and existing boot metadata truth.
## Requirements
### Requirement: PersistentFaultManager Owns Public Persistent History
The system SHALL provide a dedicated `PersistentFaultManager` owner for bounded
persistent fault history, and that owner SHALL remain distinct from
`BootManager`, `RecoveryExecutor`, and detector-local FDIR components.

#### Scenario: One public owner is instantiated on both topology lines
- **WHEN** the active `TopCcsds` or legacy `Top` topology is built with this
  change
- **THEN** each topology SHALL instantiate and wire one `PersistentFaultManager`
  public owner
- **AND** `BootManager`, `RecoveryExecutor`, `WatchdogSupervisor`,
  `EpsFdirController`, `AdcsFdirController`, and `CommController` SHALL NOT
  expose separate public persistent-history readback surfaces in v1

#### Scenario: V1 keeps write-side control internal
- **WHEN** an operator reviews the v1 persistent fault ring surface
- **THEN** the public owner SHALL expose bounded read-status history access
- **AND** it SHALL NOT expose a write-side operator command, manual capture
  trigger, or arbitrary record injection interface in this change

### Requirement: Persistent History Readback Is Bounded And Latest-First
`PersistentFaultManager` SHALL expose dictionary-visible bounded history readback
through `GET_PERSISTENT_FAULT_HISTORY(limit)` and a matching hosted
`fault history [count]` runtime shell surface.

#### Scenario: Command readback reports status before records
- **WHEN** `GET_PERSISTENT_FAULT_HISTORY(limit)` executes
- **THEN** `PersistentFaultManager` SHALL emit one status/header observation that
  states total stored records, returned record count, and active copy identity
- **AND** it SHALL then emit individual history observations in latest-first
  order

#### Scenario: Readback is bounded instead of dumping arbitrary history
- **WHEN** the requested `limit` exceeds the v1 bounded maximum
- **THEN** `PersistentFaultManager` SHALL cap the returned count to the governed
  maximum
- **AND** it SHALL report only that bounded latest-first subset

#### Scenario: Hosted shell mirrors the same history boundary
- **WHEN** the hosted runtime operator runs `fault history 8`
- **THEN** the hosted shell SHALL surface the same latest-first persistent fault
  history contract as the owned command path
- **AND** it SHALL NOT require `.fdp` generation or file downlink to review v1
  history

### Requirement: Persistent Fault Store Uses Dual Whole-File Snapshots
The persistent fault ring SHALL use governed dual whole-file snapshots under
`persistent-data/recovery/fault-ring-a.bin` and
`persistent-data/recovery/fault-ring-b.bin`, and runtime load SHALL select the
newest valid copy while falling back to the older valid copy when needed.

#### Scenario: Newest valid copy is used after a normal append
- **WHEN** the runtime appends a new persistent fault record successfully
- **THEN** it SHALL write the updated ring snapshot to the inactive copy
- **AND** later load SHALL treat the copy with the newest valid
  `magic/version/generation/capacity/count/next-index/crc` header as active

#### Scenario: Corrupt newer copy falls back to older valid copy
- **WHEN** the newer snapshot copy is torn or fails header or CRC validation
- **THEN** runtime load SHALL reject that copy
- **AND** it SHALL continue from the older valid copy instead of treating the
  store as empty

#### Scenario: Both invalid copies yield an empty store
- **WHEN** both snapshot copies fail validation during load
- **THEN** the persistent fault ring SHALL load as an empty store
- **AND** the runtime SHALL NOT invent synthetic history records from `.fdp`,
  events, or boot metadata alone

### Requirement: V1 Record Scope Is Recovery Lifecycle Only
The v1 persistent fault ring SHALL record only boot and shared-recovery
lifecycle breadcrumbs, and it SHALL keep official `.fdp`, `StateSnapshot`,
`HkTrendRecord`, and live beacon payloads unchanged in this change.

#### Scenario: Persistent ring stays distinct from official mission history
- **WHEN** the active baseline writes periodic health or state history
- **THEN** official `.fdp` data products SHALL remain the only active mission-
  history plane
- **AND** the persistent fault ring SHALL remain a separate bounded breadcrumb
  store for boot and recovery lifecycle truth

#### Scenario: V1 does not broaden readback into other public payloads
- **WHEN** this change is reviewed for public data outputs
- **THEN** it SHALL NOT add persistent fault history fields to
  `OnboardStateSnapshotSource`, `StateSnapshot`, `HkTrendRecord`, or live beacon
  payloads
- **AND** readback SHALL remain limited to the owned command and hosted shell
  surfaces
