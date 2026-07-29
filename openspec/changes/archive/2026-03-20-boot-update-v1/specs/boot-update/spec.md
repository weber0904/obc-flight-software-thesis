## MODIFIED Requirements

### Requirement: Boot Manager Public Contract

The subsystem SHALL own the `BOOT_*` command, telemetry, and event families, including status, prepare, verify, activate, confirm, rollback, and the active/pending/confirmed progress signals required to monitor the update state machine.

#### Scenario: First hosted slice owns the full staged-image lifecycle
- **WHEN** the first `BootManager` implementation is reviewed
- **THEN** it SHALL provide the `BOOT_STATUS`, `BOOT_PREPARE_UPDATE`, `BOOT_VERIFY_STAGED_IMAGE`, `BOOT_ACTIVATE_STAGED_IMAGE`, `BOOT_CONFIRM`, and `BOOT_ROLLBACK` commands plus the owned active/pending/confirmed/error telemetry and update-lifecycle events

### Requirement: Confirm And Rollback Defaults

The subsystem SHALL use a default `BOOT_CONFIRM` timeout of 60 seconds, SHALL allow configuration within the range of 30-180 seconds, and SHALL roll back when confirmation is missing, health checks fail repeatedly, metadata is invalid, or a rollback command is issued.

#### Scenario: Hosted activation starts the confirm window immediately
- **WHEN** the first hosted implementation activates a verified staged image
- **THEN** it SHALL switch to the inactive slot in the modeled state, mark that slot pending, and begin the default 60-second confirm window that can end in either `BOOT_CONFIRM` or rollback

### Requirement: File-Backed Metadata

Boot metadata v1 SHALL be persisted as structured files in persistent storage rather than in a database and SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, and error fields.

#### Scenario: First metadata file stores the hosted state machine fields
- **WHEN** the first implementation persists metadata under persistent storage
- **THEN** it SHALL use a structured key/value file that includes the minimum required fields plus the staged-path, verified-state, and expected-size fields needed by the hosted update flow

#### Scenario: Staged-file verification uses the active project hash backend
- **WHEN** the first hosted implementation compares a staged image against the operator-supplied digest
- **THEN** it SHALL compute that digest through the active F' `Utils::Hash` backend and compare the lowercase hex result against the persisted expected digest
