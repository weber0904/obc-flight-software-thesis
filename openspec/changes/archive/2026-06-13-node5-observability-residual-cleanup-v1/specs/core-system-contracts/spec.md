## ADDED Requirements

### Requirement: Node-5 Resource Keep-Live Truth Uses WatchdogSupervisor SYS Surfaces

The core-system-contracts capability SHALL define `WatchdogSupervisor` `SYS_*`
as the formal node-`5` resource keep-live truth while keeping
`SystemResources` as a supplemental integrated diagnostics surface.

#### Scenario: Formal node-5 resource truth uses SYS CPU and RSS surfaces
- **WHEN** current docs, runbooks, or observability proofs describe node-`5`
  pass-time resource truth
- **THEN** they SHALL use `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY`
- **AND** they SHALL NOT describe `SystemResources.*` as the preferred or
  formal node-`5` pass-time resource truth

#### Scenario: SystemResources remains available but supplemental
- **WHEN** the active baseline still instantiates `Svc::SystemResources`
- **THEN** current docs MAY keep it as hosted or targeted diagnostics/review
  telemetry
- **AND** they SHALL state that the surface is supplemental rather than
  baseline node-`5` operator truth

### Requirement: SystemResources Enable Governance Survives The Resource-Truth Split

The core-system-contracts capability SHALL preserve the governed
runtime-configuration status of `SystemResources.ENABLE` after
`SystemResources.*` is removed from formal node-`5` pass-time truth.

#### Scenario: Resource-truth reclassification does not weaken enable governance
- **WHEN** current docs or authority policy describe `SystemResources.ENABLE`
- **THEN** they SHALL keep it as a controlled runtime configuration surface
- **AND** they SHALL NOT reinterpret the command as a baseline status or
  backup-writable observability surface
