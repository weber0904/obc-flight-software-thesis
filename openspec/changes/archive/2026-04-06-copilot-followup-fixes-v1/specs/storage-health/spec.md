## MODIFIED Requirements

### Requirement: First-Version Storage Public Contract
The first storage health implementation slice SHALL own `STORAGE_*` command, telemetry, and event families sufficient to expose the latest bounded root statistics, scan success/failure state, warning-threshold state, and a reviewable diagnostic failure code for scan failures.

#### Scenario: Operator can observe storage health
- **WHEN** the storage health bridge has completed one or more scans
- **THEN** the owned `STORAGE_*` telemetry and event families SHALL make the current bounded storage state reviewable

#### Scenario: Scan failure event includes diagnostic code
- **WHEN** a governed root scan fails after the storage-health capability has identified the affected root
- **THEN** the emitted `STORAGE_SCAN_FAILED` path SHALL include a reviewable failure code instead of a single fixed placeholder value

### Requirement: Bounded Root Statistics
The first storage health slice SHALL record, for each governed root, whether it exists, how many regular files it contains, and the total bytes represented by those files, it SHALL keep scan errors explicit rather than silently reporting zeros, and it SHALL distinguish a missing root from a root that exists but cannot be scanned.

#### Scenario: Missing root does not masquerade as an empty root
- **WHEN** a governed root is missing during a scan
- **THEN** the storage health result SHALL preserve that degraded state explicitly instead of treating it as a normal zero-byte empty directory

#### Scenario: Unreadable root is not reported as missing
- **WHEN** a governed root exists but cannot be scanned because of permission or other filesystem errors
- **THEN** the storage health result SHALL keep that root distinct from a missing-root condition and SHALL retain the failure as an explicit scan error
