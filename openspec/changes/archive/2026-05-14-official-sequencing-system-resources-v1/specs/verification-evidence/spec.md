## ADDED Requirements

### Requirement: Official Sequencing Evidence Proves Admission, Upload, and Execution Together

Verification evidence for the active official sequencing baseline SHALL prove the governed upload, admission, and execution path together.

#### Scenario: Hosted sequence proof covers the bounded active path

- **WHEN** evidence cites `official-sequencing-system-resources-v1`
- **THEN** it SHALL include the governed hosted CCSDS file upload path into the sequence staging directory
- **AND** it SHALL include wrapper-driven validate/run/manual control results
- **AND** it SHALL include dispatcher or sequencer observations proving the admitted sequence executed on the official sequencing surface
- **AND** it SHALL include the configured tick interval and declared timing-latency bound

### Requirement: Evidence Covers Backup Bounds And Stock-Control Denial

The first official sequencing evidence SHALL prove the bounded backup and direct-stock-command policy.

#### Scenario: Evidence records bounded sequence control policy

- **WHEN** the hosted probe is recorded
- **THEN** it SHALL include direct external denial of stock `SeqDispatcher.RUN`, `RUN_ARGS`, and raw `CmdSequencer` controls
- **AND** it SHALL include a backup-allowed read-only sequence acceptance case
- **AND** it SHALL include a backup rejection case where one inner command requires higher authority
- **AND** it SHALL include a rejected ownership-violating manual or cancel request

### Requirement: Evidence States Timing And Non-Claims Truthfully

Official sequencing evidence SHALL not over-claim scheduler or mission-time closure.

#### Scenario: Evidence records bounded truth

- **WHEN** official sequencing evidence is written
- **THEN** it SHALL state that absolute timing truth depends on correct hosted POSIX wallclock
- **AND** it SHALL NOT claim mission scheduler behavior, persistent onboard schedule, GPS mission-time scheduling, payload planner behavior, or conflict-free multi-sequence arbitration
