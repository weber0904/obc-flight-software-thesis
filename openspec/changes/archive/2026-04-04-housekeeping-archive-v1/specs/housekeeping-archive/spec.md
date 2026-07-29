## ADDED Requirements

### Requirement: Periodic Housekeeping Archive Capture
The system SHALL provide a first-version housekeeping archive component that periodically snapshots cached and runtime OBC state into an onboard archive while the flight software runtime is active.

#### Scenario: Periodic capture records a housekeeping snapshot
- **WHEN** the housekeeping archive cadence expires while the hosted OBC runtime is running
- **THEN** the system SHALL append one housekeeping record containing the current cached/runtime snapshot to the active housekeeping archive file

### Requirement: Housekeeping Capture Reuses Existing Cached State
The first-version housekeeping archive SHALL gather subsystem status from existing cached and runtime accessors and SHALL NOT introduce a second transport-poll loop for EPS or ADCS.

#### Scenario: Capture does not duplicate subsystem polling
- **WHEN** the housekeeping archive records a snapshot
- **THEN** it SHALL read existing cached/runtime subsystem values instead of issuing new EPS or ADCS transport requests for the same capture

### Requirement: Ring Slot Rotation With Generation Tracking
The housekeeping archive SHALL use a fixed set of slot files with a per-file size limit, SHALL rotate to the next slot when the active file reaches the configured limit, and SHALL increment the slot generation whenever a slot is reused after wraparound.

#### Scenario: Active file rotates when full
- **WHEN** appending the next housekeeping record would exceed the configured archive-file size limit
- **THEN** the system SHALL seal the current slot, advance to the next slot, and begin a new generation if that slot is being reused

### Requirement: Reviewable Archive Index
The housekeeping archive SHALL maintain a repository-owned index file that records, for each slot, the slot number, generation, file name, start time, end time, record count, file size, and whether the slot is currently active.

#### Scenario: Ground can inspect archive coverage before requesting data
- **WHEN** the housekeeping archive index is read after one or more captures
- **THEN** it SHALL provide enough metadata for an operator to determine which slot file covers a requested time span

### Requirement: Controlled Housekeeping Archive Downlink
The first-version housekeeping archive SHALL expose dedicated command paths for immediate capture, archive-index downlink, and slot-specific archive-file downlink, and it SHALL resolve those commands to governed housekeeping files without accepting an arbitrary on-board file path from the operator-facing command surface.

#### Scenario: Operator requests the index file
- **WHEN** the operator issues the index-downlink command
- **THEN** the housekeeping archive SHALL enqueue the governed archive-index file on the existing file-downlink path

#### Scenario: Operator requests a specific slot and generation
- **WHEN** the operator issues the slot-downlink command with a valid slot and generation tuple
- **THEN** the housekeeping archive SHALL enqueue the matching governed archive file on the existing file-downlink path

#### Scenario: Stale or invalid slot request is rejected
- **WHEN** the operator issues the slot-downlink command with a slot and generation tuple that does not match the current archive index
- **THEN** the housekeeping archive SHALL reject the request instead of downlinking a different file
