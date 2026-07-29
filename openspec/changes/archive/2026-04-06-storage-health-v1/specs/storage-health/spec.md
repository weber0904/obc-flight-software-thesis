## ADDED Requirements

### Requirement: Governed Runtime-Root Storage Health
The project SHALL provide a dedicated `storage-health` capability that observes the governed runtime storage roots and SHALL keep that capability distinct from boot/update control and from housekeeping archive capture ownership.

#### Scenario: Storage health ownership stays distinct from archive ownership
- **WHEN** the first storage health slice is implemented
- **THEN** runtime-root scanning, warning-state evaluation, and `STORAGE_*` contracts SHALL be owned by the storage-health subsystem instead of by `HousekeepingArchive` or `BootManager`

### Requirement: First-Version Storage Public Contract
The first storage health implementation slice SHALL own `STORAGE_*` command, telemetry, and event families sufficient to expose the latest bounded root statistics, scan success/failure state, and warning-threshold state.

#### Scenario: Operator can observe storage health
- **WHEN** the storage health bridge has completed one or more scans
- **THEN** the owned `STORAGE_*` telemetry and event families SHALL make the current bounded storage state reviewable

### Requirement: Bounded First-Version Root Scope
The first storage health slice SHALL observe only the governed runtime roots for housekeeping archive files, persistent data, staging data, and logs, and it SHALL report those roots separately rather than as one aggregated filesystem total.

#### Scenario: Scan reports per-root statistics
- **WHEN** a storage scan completes successfully
- **THEN** the cached storage health state SHALL distinguish `hk`, `persistent-data`, `staging`, and `logs` root statistics individually

### Requirement: Bounded Root Statistics
The first storage health slice SHALL record, for each governed root, whether it exists, how many regular files it contains, and the total bytes represented by those files, and it SHALL keep scan errors explicit rather than silently reporting zeros.

#### Scenario: Missing root does not masquerade as an empty root
- **WHEN** a governed root is missing or unreadable during a scan
- **THEN** the storage health result SHALL preserve that degraded state explicitly instead of treating it as a normal zero-byte empty directory

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
