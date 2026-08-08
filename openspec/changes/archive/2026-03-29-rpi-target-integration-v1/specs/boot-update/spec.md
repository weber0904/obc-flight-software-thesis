## ADDED Requirements

### Requirement: Target-Configurable Storage Roots
The boot/update subsystem SHALL allow the runtime entrypoint or launch helpers to configure the staging and persistent metadata roots so that the same `BootManager` implementation can run under hosted and Raspberry Pi profiles without recompilation.

#### Scenario: Integ-rpi launcher sets target storage roots
- **WHEN** the Raspberry Pi target profile launches the OBC runtime
- **THEN** `BootManager` SHALL resolve relative staged-image paths against the configured target staging root and SHALL persist metadata under the configured target persistent-data root

### Requirement: Target Restart Preserves Pending Confirmation
The boot/update subsystem SHALL reload file-backed metadata after a target-side process restart, SHALL preserve pending-slot and confirmation-window state across that restart, and SHALL allow the operator to complete the flow through `BOOT_CONFIRM` or `BOOT_ROLLBACK` after the target process comes back up.

#### Scenario: Raspberry Pi restart resumes the confirm window
- **WHEN** a staged image has been activated on the Raspberry Pi target and the OBC process restarts before confirmation
- **THEN** the restarted runtime SHALL reload the pending-slot state from persistent metadata, resume the confirmation window, and keep the confirm-or-rollback decision observable through the public boot contract

## MODIFIED Requirements

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL remain persisted as structured files in persistent storage rather than in a database and SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, and error fields. Communication and framing paths MAY continue to use CRC-32, but staged-image verification SHALL use lowercase SHA-256 hex without requiring the project to switch the global F' `Utils::Hash` backend.

#### Scenario: Staged-image verification uses a dedicated SHA-256 path
- **WHEN** the boot/update subsystem compares a staged image against the operator-supplied digest
- **THEN** it SHALL compute SHA-256 for the staged file through the boot/update implementation itself, compare the lowercase hex result against the persisted expected digest, and SHALL NOT require first-version signature validation
