## ADDED Requirements

### Requirement: Housekeeping Archive Captures Storage Health Cached State
The housekeeping archive SHALL include storage health cached state in each snapshot once the storage-health subsystem is configured, and it SHALL store bounded root statistics together with overall warning-state metadata.

#### Scenario: Storage health appears in a housekeeping capture
- **WHEN** the housekeeping archive captures a snapshot after the storage-health subsystem has produced cached state
- **THEN** the snapshot SHALL include the latest bounded storage health state together with explicit warning/degraded metadata

### Requirement: Archive Capture Reuses Cached Storage State
Housekeeping archive capture SHALL read storage health cached state from the storage-health subsystem runtime accessor and SHALL NOT introduce a separate archive-time filesystem scan.

#### Scenario: Archive capture does not rescan runtime roots
- **WHEN** a housekeeping archive capture occurs
- **THEN** the archive path SHALL reuse the latest cached storage scan instead of performing a new filesystem traversal directly
