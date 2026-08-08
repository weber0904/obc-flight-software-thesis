## ADDED Requirements

### Requirement: Boot-After-Recovery Safe Fallback Is Truthful
The boot/update subsystem SHALL preserve enough recovery boot metadata to clamp the next boot into `SAFE` after repeated recovery-triggered resets, and that truth SHALL remain reviewable through the owned boot contract.

#### Scenario: Repeated recovery-triggered reboot clamps boot into SAFE
- **WHEN** the persisted boot metadata reports repeated consecutive recovery-triggered resets at or above the configured threshold
- **THEN** the next runtime startup SHALL mark boot-after-recovery safe fallback as required
- **AND** the active runtime SHALL request `SAFE` through the normal internal mode path instead of auto-restoring a higher mode

#### Scenario: Stable runtime clears the repeated-reset clamp
- **WHEN** the runtime survives the bounded stable window with no active recovery incident and no pending reboot intent
- **THEN** the boot/update owner SHALL clear the persisted repeated-reset clamp state
- **AND** later boots SHALL report that the safe-fallback clamp is no longer pending

## MODIFIED Requirements

### Requirement: Boot Manager Public Contract

The subsystem SHALL own the `BOOT_*` command, telemetry, and event families, the reviewable `GET_RESET_CAUSE` and `GET_BOOT_COUNT` status commands, the staged-image lifecycle status/prepare/verify/activate/confirm/rollback behavior, and the active/pending/confirmed/error/recovery progress signals required to monitor the boot and recovery state machine.

#### Scenario: Hosted slice owns staged-image lifecycle and boot recovery status
- **WHEN** the active `BootManager` implementation is reviewed after `recovery-executors-v1`
- **THEN** it SHALL provide `BOOT_STATUS`, `BOOT_PREPARE_UPDATE`, `BOOT_VERIFY_STAGED_IMAGE`, `BOOT_ACTIVATE_STAGED_IMAGE`, `BOOT_CONFIRM`, `BOOT_ROLLBACK`, `GET_RESET_CAUSE`, and `GET_BOOT_COUNT`
- **AND** it SHALL expose owned active/pending/confirmed/error telemetry plus boot-count/reset-cause/repeated-reset review surfaces

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL remain persisted as structured files in persistent storage rather than in a database, SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, error, reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields, and SHALL rewrite the persisted file cleanly when field values shrink so stale trailing bytes are not left behind. Communication and framing paths MAY continue to use CRC-32, but staged-image verification SHALL use lowercase SHA-256 hex without requiring the project to switch the global F' `Utils::Hash` backend.

#### Scenario: Recovery metadata remains file-backed
- **WHEN** `RecoveryExecutor` records reboot intent or the next boot reports reset truth
- **THEN** the related reset-cause, boot-count, and recovery-summary fields SHALL be written into the existing file-backed boot metadata store
- **AND** the runtime SHALL NOT require a database or parallel recovery metadata store

#### Scenario: Rollback rewrites metadata without stale trailing bytes
- **WHEN** rollback or recovery-state clearing rewrites a previously populated persisted field such as `staged_path` or the bounded recovery-summary fields
- **THEN** the next saved `metadata-v1.txt` SHALL parse cleanly as a complete file by a fresh metadata reader
- **AND** it SHALL NOT retain stale bytes from the earlier longer serialization

### Requirement: Target Restart Preserves Pending Confirmation
The boot/update subsystem SHALL reload file-backed metadata after a target-side process restart, SHALL preserve pending-slot and confirmation-window state across that restart, and SHALL keep recovery-triggered reset truth observable through the public boot contract after the target process comes back up.

#### Scenario: Raspberry Pi restart resumes the confirm window
- **WHEN** a staged image has been activated on the Raspberry Pi target and the OBC process restarts before confirmation
- **THEN** the restarted runtime SHALL reload the pending-slot state from persistent metadata, resume the confirmation window, and keep the confirm-or-rollback decision observable through the public boot contract

#### Scenario: Recovery-triggered restart reloads reset truth
- **WHEN** the OBC process restarts after a recovery-triggered reboot intent on the hosted or Raspberry Pi profile
- **THEN** the restarted runtime SHALL reload the persisted reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields from boot metadata
- **AND** it SHALL keep those fields observable through `GET_RESET_CAUSE`, `GET_BOOT_COUNT`, and the owned boot status surfaces
