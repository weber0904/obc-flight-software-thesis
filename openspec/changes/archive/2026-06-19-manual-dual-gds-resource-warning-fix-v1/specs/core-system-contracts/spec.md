## MODIFIED Requirements

### Requirement: Core Resource Monitoring Survives Owner Migration
The core-system-contracts capability SHALL preserve the existing
resource-monitoring behavior while moving that behavior under
`WatchdogSupervisor`.

#### Scenario: Resource monitoring remains operator-visible
- **WHEN** an operator uses the active baseline after watchdog-v1
- **THEN** the deployment SHALL still expose resource-monitoring enable and
  threshold commands
- **AND** it SHALL still expose CPU and RSS review surfaces through the owned
  `SYS_*` telemetry and warning events
- **AND** those surfaces SHALL now be owned by `WatchdogSupervisor` rather than
  a separate `HealthMonitor`

#### Scenario: Hosted RSS telemetry reflects current resident memory
- **WHEN** the hosted runtime publishes `SYS_MEM_RSS_MB`
- **THEN** it SHALL sample the current resident memory of the running `OBC`
  process rather than a historical high-water RSS value
- **AND** the published value SHALL remain suitable for direct comparison
  against the configured RSS warning threshold

#### Scenario: Resource warnings are emitted on threshold crossing
- **WHEN** `SYS_CPU_USAGE` or `SYS_MEM_RSS_MB` rises from at-or-below the
  configured threshold to above the configured threshold while monitoring is
  enabled
- **THEN** `WatchdogSupervisor` SHALL emit the corresponding warning event once
- **AND** it SHALL NOT re-emit the same warning on later samples that remain
  above threshold without first dropping back to at-or-below threshold

### Requirement: Node-5 Resource Keep-Live Truth Uses WatchdogSupervisor SYS Surfaces

The core-system-contracts capability SHALL define `WatchdogSupervisor` `SYS_*`
as the formal node-`5` resource keep-live truth while keeping
`SystemResources` as a supplemental integrated diagnostics surface.

#### Scenario: Formal node-5 resource truth uses SYS CPU and RSS surfaces
- **WHEN** current docs, runbooks, or observability proofs describe node-`5`
  pass-time resource truth
- **THEN** they SHALL use `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY`
- **AND** they SHALL describe `SYS_MEM_RSS_MB` as current resident memory and
  `SYS_RESOURCE_DEGRADED` / `SYS_LOW_MEMORY` as threshold-crossing warnings
- **AND** they SHALL NOT describe `SystemResources.*` as the preferred or
  formal node-`5` pass-time resource truth.

#### Scenario: SystemResources remains available but supplemental
- **WHEN** the active baseline still instantiates `Svc::SystemResources`
- **THEN** current docs MAY keep it as hosted or targeted diagnostics/review
  telemetry
- **AND** they SHALL state that the surface is supplemental rather than
  baseline node-`5` operator truth.
