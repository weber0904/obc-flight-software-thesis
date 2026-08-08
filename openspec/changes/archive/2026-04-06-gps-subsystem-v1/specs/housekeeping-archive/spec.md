## ADDED Requirements

### Requirement: Housekeeping Archive Captures GPS Cached State
The housekeeping archive SHALL include GPS cached state in each snapshot once the GPS subsystem is configured, and it SHALL store both fix-validity metadata and the bounded first-version GPS telemetry fields owned by the GPS subsystem.

#### Scenario: GPS state appears in a housekeeping capture
- **WHEN** the housekeeping archive captures a snapshot after the GPS subsystem has produced cached state
- **THEN** the snapshot SHALL include the current GPS cached state together with explicit metadata indicating whether the cached state contains a valid fix

### Requirement: GPS Archive Capture Reuses Existing Cached State
Housekeeping archive capture SHALL read GPS cached state from the GPS subsystem runtime accessor and SHALL NOT introduce a separate archive-time GPS transport read or parser invocation.

#### Scenario: GPS archive capture does not perform another source read
- **WHEN** a housekeeping archive capture occurs
- **THEN** the archive path SHALL reuse the latest cached GPS state instead of reading a new sentence directly from the GPS source
