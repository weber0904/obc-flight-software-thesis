## ADDED Requirements

### Requirement: File-Based A/B Update Path
The boot/update subsystem SHALL perform updates by receiving an image into a staging area, validating that staged file, selecting the inactive slot as pending, and rebooting into that slot. The subsystem SHALL NOT use command-payload chunk uploads for full images.

#### Scenario: Chunk upload remains disallowed
- **WHEN** a first-version update interface is reviewed
- **THEN** the accepted interface SHALL use staging files and SHALL reject command-chunk upload as the formal update path

### Requirement: Boot Manager Public Contract
The subsystem SHALL own the `BOOT_*` command, telemetry, and event families, including status, prepare, verify, activate, confirm, rollback, and the active/pending/confirmed progress signals required to monitor the update state machine.

#### Scenario: Boot state is observable during update
- **WHEN** an operator performs prepare, verify, and activate actions
- **THEN** the subsystem SHALL surface the resulting status through the owned telemetry and events

### Requirement: Confirm And Rollback Defaults
The subsystem SHALL use a default `BOOT_CONFIRM` timeout of 60 seconds, SHALL allow configuration within the range of 30-180 seconds, and SHALL roll back when confirmation is missing, health checks fail repeatedly, metadata is invalid, or a rollback command is issued.

#### Scenario: Missing confirmation triggers rollback path
- **WHEN** a pending slot boots and no valid confirmation arrives before the timeout expires
- **THEN** the subsystem SHALL enter the rollback path toward the last known good slot

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL be persisted as structured files in persistent storage rather than in a database and SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, and error fields.

#### Scenario: Metadata store is reviewed
- **WHEN** the implementation selects a persistence mechanism for boot metadata v1
- **THEN** it SHALL use file-backed persistent storage and SHALL include the required minimum fields
