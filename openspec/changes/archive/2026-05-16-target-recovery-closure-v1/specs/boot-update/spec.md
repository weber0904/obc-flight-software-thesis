## MODIFIED Requirements

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL remain persisted as structured files in persistent storage rather than in a database, SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, error, reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields, and SHALL rewrite the persisted file cleanly when field values shrink so stale trailing bytes are not left behind. Communication and framing paths MAY continue to use CRC-32, but staged-image verification SHALL use lowercase SHA-256 hex without requiring the project to switch the global F' `Utils::Hash` backend. Recovery-triggered managed process restart truth MAY be recorded through the same bounded recovery-summary fields without treating that action as hardware reset, Linux reboot, bootloader handoff, or power-loss recovery.

#### Scenario: Non-reboot recovery actions do not pollute boot truth
- **WHEN** shared recovery executes EPS reset, `SAFE` fallback, COMM failover, or a historical restart-intent observation that does not request runtime exit
- **THEN** the persisted boot metadata SHALL preserve the prior reset-cause truth
- **AND** it SHALL NOT record those non-exit actions as though a reset had already occurred

#### Scenario: R2 process restart records recovery truth before exit
- **WHEN** shared recovery executes a managed `R2_RESTART_SOFTWARE_COMPONENT` process restart
- **THEN** `BootManager` SHALL persist the recovery reset-cause, last-recovery-source, and last-recovery-level before the runtime exits
- **AND** the restarted runtime SHALL reload that recovery truth while keeping the action distinguishable from `R6_OBC_REBOOT`

### Requirement: Target Restart Preserves Pending Confirmation
The boot/update subsystem SHALL reload file-backed metadata after a target-side process restart, SHALL preserve pending-slot and confirmation-window state across that restart, and SHALL keep recovery-triggered reset or process-restart truth observable through the public boot contract after the target process comes back up.

#### Scenario: Raspberry Pi restart resumes the confirm window
- **WHEN** a staged image has been activated on the Raspberry Pi target and the OBC process restarts before confirmation
- **THEN** the restarted runtime SHALL reload the pending-slot state from persistent metadata, resume the confirmation window, and keep the confirm-or-rollback decision observable through the public boot contract

#### Scenario: Recovery-triggered restart reloads reset truth
- **WHEN** the OBC process restarts after a recovery-triggered reboot intent on the hosted or Raspberry Pi profile
- **THEN** the restarted runtime SHALL reload the persisted reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields from boot metadata
- **AND** it SHALL keep those fields observable through `GET_RESET_CAUSE`, `GET_BOOT_COUNT`, and the owned boot status surfaces

#### Scenario: Recovery-triggered process restart reloads R2 truth
- **WHEN** the OBC process restarts after a managed `R2_RESTART_SOFTWARE_COMPONENT` process restart on the hosted or Raspberry Pi profile
- **THEN** the restarted runtime SHALL reload the persisted reset-cause, boot-count, consecutive-reset, and bounded recovery-summary fields from boot metadata
- **AND** it SHALL expose `lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT` rather than reporting that action as `R6_OBC_REBOOT`
