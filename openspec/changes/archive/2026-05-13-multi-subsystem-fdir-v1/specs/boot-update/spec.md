## MODIFIED Requirements

### Requirement: Boot Manager Public Contract
The subsystem SHALL own the `BOOT_*` command, telemetry, and event families, the reviewable `GET_RESET_CAUSE` and `GET_BOOT_COUNT` status commands, the staged-image lifecycle status/prepare/verify/activate/confirm/rollback behavior, and the active/pending/confirmed/error/recovery progress signals required to monitor the boot and recovery state machine.

#### Scenario: Reboot truth covers bounded subsystem recovery sources
- **WHEN** the active runtime restarts after a shared recovery reboot intent from `EPS`, `ADCS`, or `COMM`
- **THEN** `BootManager` SHALL reload the persisted reset-cause, boot-count, consecutive-reset-count, last-recovery-source, and last-recovery-level fields
- **AND** it SHALL keep those fields observable through the owned boot contract

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL remain persisted as structured files in persistent storage rather than in a database, SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, error, reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields, and SHALL rewrite the persisted file cleanly when field values shrink so stale trailing bytes are not left behind. Communication and framing paths MAY continue to use CRC-32, but staged-image verification SHALL use lowercase SHA-256 hex without requiring the project to switch the global F' `Utils::Hash` backend.

#### Scenario: Non-reboot recovery actions do not pollute boot truth
- **WHEN** shared recovery executes restart intent, EPS reset, `SAFE` fallback, or COMM failover without later issuing reboot intent
- **THEN** the persisted boot metadata SHALL preserve the prior reset-cause truth
- **AND** it SHALL NOT record those non-reboot actions as though a reset had already occurred
