## MODIFIED Requirements

### Requirement: Retired Housekeeping Archive Runtime Roots Stay Inactive
The runtime storage model SHALL NOT treat legacy housekeeping archive files or
the legacy housekeeping archive index as active runtime-root storage after the
HK fallback retirement. Historical evidence may still cite the old `hk/` root,
but current hosted or target-facing runtime storage SHALL use official
`data-products`, `persistent-data`, `staging`, and `logs` roots instead.

#### Scenario: Release switching does not recreate retired HK history
- **WHEN** the governed runtime launches from a hosted or Raspberry Pi runtime root
- **THEN** it SHALL NOT create `runtime/hk`, `hk/index.csv`, or HK slot files as active mission-history artifacts
- **AND** release switching SHALL preserve current mutable roots without relying on the retired HK archive root

### Requirement: Governed Runtime-Root Storage Monitoring
The resource-storage baseline SHALL provide observability for the governed
`persistent-data`, `staging`, `logs`, and `data-products` runtime roots, and
that path SHALL surface root-specific warning or degraded states instead of
requiring direct filesystem inspection as the only review method.

#### Scenario: Runtime-root storage state becomes reviewable
- **WHEN** the first storage-health slice completes a scan on the hosted runtime
- **THEN** the project SHALL be able to review bounded root statistics and warning status for the governed runtime roots through repository-owned runtime contracts and evidence

### Requirement: Storage Roles Stay Distinct
Official HK trend data products SHALL remain distinct from persistent
boot/update data, staging data, logs, and the retired housekeeping archive
ring/index history.

#### Scenario: Retired housekeeping archive remains separate
- **WHEN** the HK trend data-product path writes files
- **THEN** it SHALL use the official data-products root
- **AND** it SHALL NOT write into, replace, or recreate the retired `hk/` ring archive root
