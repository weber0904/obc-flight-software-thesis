# storage-health Specification

## Purpose
Define the first bounded storage-health capability for governed runtime-root observability, warning-state review, and cached storage statistics reuse.
## Requirements
### Requirement: Governed Runtime-Root Storage Health
The project SHALL provide a dedicated `storage-health` capability that observes the governed runtime storage roots and SHALL keep that capability distinct from boot/update control and from any retired housekeeping-archive path.

#### Scenario: Storage health ownership stays distinct from retired archive history
- **WHEN** the first storage health slice is implemented
- **THEN** runtime-root scanning, warning-state evaluation, and `STORAGE_*` contracts SHALL be owned by the storage-health subsystem instead of by `BootManager`

### Requirement: First-Version Storage Public Contract
The first storage health implementation slice SHALL own `STORAGE_*` command, telemetry, and event families sufficient to expose the latest bounded root statistics, scan success/failure state, warning-threshold state, and a reviewable diagnostic failure code for scan failures.

#### Scenario: Operator can observe storage health
- **WHEN** the storage health bridge has completed one or more scans
- **THEN** the owned `STORAGE_*` telemetry and event families SHALL make the current bounded storage state reviewable

#### Scenario: Scan failure event includes diagnostic code
- **WHEN** a governed root scan fails after the storage-health capability has identified the affected root
- **THEN** the emitted `STORAGE_SCAN_FAILED` path SHALL include a reviewable failure code instead of a single fixed placeholder value

### Requirement: Bounded Runtime-Root Scope
The active storage health slice SHALL observe only the governed runtime roots for persistent data, staging data, logs, and official data products, and it SHALL report those roots separately rather than as one aggregated filesystem total.

#### Scenario: Scan reports per-root statistics
- **WHEN** a storage scan completes successfully
- **THEN** the cached storage health state SHALL distinguish `persistent-data`, `staging`, `logs`, and `data-products` root statistics individually

### Requirement: Bounded Root Statistics
The first storage health slice SHALL record, for each governed root, whether it exists, how many regular files it contains, and the total bytes represented by those files, it SHALL keep scan errors explicit rather than silently reporting zeros, and it SHALL distinguish a missing root from a root that exists but cannot be scanned.

#### Scenario: Missing root does not masquerade as an empty root
- **WHEN** a governed root is missing during a scan
- **THEN** the storage health result SHALL preserve that degraded state explicitly instead of treating it as a normal zero-byte empty directory

#### Scenario: Unreadable root is not reported as missing
- **WHEN** a governed root exists but cannot be scanned because of permission or other filesystem errors
- **THEN** the storage health result SHALL keep that root distinct from a missing-root condition and SHALL retain the failure as an explicit scan error

### Requirement: Warning Threshold Evaluation
The first storage health slice SHALL evaluate bounded warning thresholds against the governed root statistics and SHALL publish whether any root currently exceeds the configured warning condition.

#### Scenario: Threshold warning becomes reviewable
- **WHEN** a governed root exceeds the configured first-version warning threshold
- **THEN** the cached storage health state and owned event path SHALL make that warning visible

### Requirement: Cached Runtime Access
The first storage health slice SHALL expose cached storage health state for runtime consumers and SHALL allow those consumers to reuse the latest scan result without triggering another filesystem scan during the same observation path.

#### Scenario: Runtime consumer reads cached storage state
- **WHEN** another OBC feature requests storage health runtime state
- **THEN** it SHALL receive the latest cached storage scan maintained by the storage health bridge

### Requirement: Data Products Root Observability
Storage health SHALL scan the governed `<runtime-root>/data-products/` root alongside persistent, staging, and logs roots.

#### Scenario: Data products root contributes cached state
- **WHEN** a storage scan completes
- **THEN** the cached storage state SHALL include `data-products` existence, scan status, file count, byte count, and error code
- **AND** warning and degraded masks SHALL use bit 3 for the `data-products` root without changing the mask width from `U8`

#### Scenario: Data products root events use stable root kind
- **WHEN** the `data-products` root is missing, cannot be scanned, or exceeds the configured warning threshold
- **THEN** storage-health root events SHALL report root kind `DATA_PRODUCTS`

### Requirement: Observe Only Data Products Policy Fields
Storage health SHALL expose first-slice policy visibility for `data-products` without deleting or cleaning files.

#### Scenario: Quota and retention policy are visible
- **WHEN** the `data-products` root is scanned
- **THEN** its root stats SHALL include quota bytes, watermark bytes, quota status, and retention status
- **AND** quota status SHALL distinguish not configured, ok, over quota, and unavailable when the root scan fails
- **AND** retention status SHALL identify observe-only behavior

#### Scenario: No destructive retention
- **WHEN** the `data-products` root exceeds any configured quota or watermark
- **THEN** the scanner SHALL report status through cached state, telemetry, and events
- **AND** it SHALL NOT delete or rewrite any data-product file

### Requirement: Storage Status Readback Is Fresh-By-Default

The storage-health capability SHALL treat `STORAGE_GET_STATUS` as the fresh
operator readback command and SHALL NOT keep a second operator-visible scan
command that duplicates the same fresh readback semantics.

#### Scenario: Storage status command performs a fresh scan
- **WHEN** an operator runs `STORAGE_GET_STATUS`
- **THEN** `StorageHealthBridge` SHALL perform a fresh storage scan before
  replying
- **AND** it SHALL make the detailed storage-root telemetry reviewable as
  bounded readback for that command.

#### Scenario: Redundant scan-now command is retired
- **WHEN** the current baseline defines operator-facing storage status commands
- **THEN** it SHALL keep `STORAGE_GET_STATUS` as the fresh readback surface
- **AND** it SHALL retire `STORAGE_SCAN_NOW` from the current command
  contract, authority catalog, and operator documentation.

### Requirement: Scheduled Storage Visibility Is Summary-Oriented

The storage-health capability SHALL keep scheduled storage refresh focused on
cache maintenance, selected summary telemetry, and threshold/failure events
rather than broad repeated detailed root telemetry as live baseline truth.

#### Scenario: Scheduled storage scan keeps summary and critical events
- **WHEN** `StorageHealthBridge` performs its scheduled refresh
- **THEN** it SHALL keep only selected warning/degraded/quota summary telemetry
  as baseline live visibility
- **AND** it SHALL keep root-missing, scan-failed, and warning-threshold
  events reviewable.

