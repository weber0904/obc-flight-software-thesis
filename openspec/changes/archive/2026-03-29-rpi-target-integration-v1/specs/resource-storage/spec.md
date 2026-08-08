## MODIFIED Requirements

### Requirement: Storage Layout And Metadata
The system SHALL define storage roles for Slot A, Slot B, staging, persistent data, and logs/evidence, boot metadata v1 SHALL be stored as structured files in persistent storage rather than in a database, and the runtime SHALL allow hosted and Raspberry Pi profiles to override the staging, persistent-data, and evidence/log roots without changing `BootManager` logic.

#### Scenario: Boot metadata storage choice
- **WHEN** the boot/update subsystem persists `active_slot`, `pending_slot`, or related metadata
- **THEN** that metadata SHALL be written to file-backed persistent storage and SHALL NOT require an embedded database

#### Scenario: Raspberry Pi runtime uses target-specific roots
- **WHEN** the `integ-rpi` profile launches on a Raspberry Pi target
- **THEN** the runtime SHALL be able to place staging data, persistent metadata, and evidence/log outputs under target-specific filesystem roots while preserving the same logical storage roles

### Requirement: Monitoring And Constrained Validation
The resource baseline SHALL define monitoring thresholds for memory, CPU, CSP free buffers, staging capacity, and log capacity, SHALL record the actual target-side storage and evidence paths used during Raspberry Pi validation, and SHALL allow hardware-limited checks to be marked as `Blocked-HW` or `Deferred-RPi` with replacement evidence.

#### Scenario: Hardware-limited storage validation
- **WHEN** a storage or target-platform validation cannot run because Raspberry Pi hardware is unavailable
- **THEN** the validation record SHALL include the correct constrained status and replacement evidence instead of silently dropping the test

#### Scenario: Target evidence names the real storage roots
- **WHEN** a Raspberry Pi target integration run is recorded
- **THEN** the evidence SHALL identify which target filesystem roots were used for staging, persistent metadata, and logs/evidence collection
