## ADDED Requirements

### Requirement: Resource Budgets
The project SHALL express first-version resource limits using Linux-oriented budgets for RSS, thread count, open file count, and buffer allocations rather than MCU-style raw memory maps.

#### Scenario: Target resource model remains Linux-oriented
- **WHEN** the resource baseline is reviewed for Raspberry Pi 3B+
- **THEN** the documented limits SHALL use process and filesystem terms such as RSS, threads, files, and buffer pools

### Requirement: Storage Layout And Metadata
The system SHALL define storage roles for Slot A, Slot B, staging, persistent data, and logs/evidence, and boot metadata v1 SHALL be stored as structured files in persistent storage rather than in a database.

#### Scenario: Boot metadata storage choice
- **WHEN** the boot/update subsystem persists `active_slot`, `pending_slot`, or related metadata
- **THEN** that metadata SHALL be written to file-backed persistent storage and SHALL NOT require an embedded database

### Requirement: Monitoring And Constrained Validation
The resource baseline SHALL define monitoring thresholds for memory, CPU, CSP free buffers, staging capacity, and log capacity, and SHALL allow hardware-limited checks to be marked as `Blocked-HW` or `Deferred-RPi` with replacement evidence.

#### Scenario: Hardware-limited storage validation
- **WHEN** a storage or target-platform validation cannot run because Raspberry Pi hardware is unavailable
- **THEN** the validation record SHALL include the correct constrained status and replacement evidence instead of silently dropping the test
