# boot-update Specification

## Purpose
Define the first-version A/B update path, boot metadata rules, and the public confirm/rollback behavior for `BootManager`.
## Requirements
### Requirement: File-Based A/B Update Path
The boot/update subsystem SHALL perform updates by receiving an image into a staging area, validating that staged file, selecting the inactive slot as pending, and rebooting into that slot. The subsystem SHALL NOT use command-payload chunk uploads for full images.

#### Scenario: Chunk upload remains disallowed
- **WHEN** a first-version update interface is reviewed
- **THEN** the accepted interface SHALL use staging files and SHALL reject command-chunk upload as the formal update path

### Requirement: Boot Manager Public Contract
The subsystem SHALL own the `BOOT_*` command, telemetry, and event families, the reviewable `GET_RESET_CAUSE` and `GET_BOOT_COUNT` status commands, the staged-image lifecycle status/prepare/verify/activate/confirm/rollback behavior, and the active/pending/confirmed/error/recovery progress signals required to monitor the boot and recovery state machine.

#### Scenario: Reboot truth covers bounded subsystem recovery sources
- **WHEN** the active runtime restarts after a shared recovery reboot intent from `EPS`, `ADCS`, or `COMM`
- **THEN** `BootManager` SHALL reload the persisted reset-cause, boot-count, consecutive-reset-count, last-recovery-source, and last-recovery-level fields
- **AND** it SHALL keep those fields observable through the owned boot contract

### Requirement: Confirm And Rollback Defaults

The subsystem SHALL use a default `BOOT_CONFIRM` timeout of 60 seconds, SHALL allow configuration within the range of 30-180 seconds, and SHALL roll back when confirmation is missing, health checks fail repeatedly, metadata is invalid, or a rollback command is issued.

#### Scenario: Hosted activation starts the confirm window immediately
- **WHEN** the first hosted implementation activates a verified staged image
- **THEN** it SHALL switch to the inactive slot in the modeled state, mark that slot pending, and begin the default 60-second confirm window that can end in either `BOOT_CONFIRM` or rollback

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

### Requirement: Target-Configurable Storage Roots
The boot/update subsystem SHALL allow the runtime entrypoint or launch helpers to configure the staging and persistent metadata roots so that the same `BootManager` implementation can run under hosted and Raspberry Pi profiles without recompilation.

#### Scenario: Integ-rpi launcher sets target storage roots
- **WHEN** the Raspberry Pi target profile launches the OBC runtime
- **THEN** `BootManager` SHALL resolve relative staged-image paths against the configured target staging root and SHALL persist metadata under the configured target persistent-data root

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

### Requirement: Governed Post-Reboot Startup Path
The first Raspberry Pi boot integration slice SHALL provide a governed post-reboot startup path that relaunches the installed `current` release after the target OS boots, and that path SHALL keep boot/update state observable through the existing public boot contract without claiming firmware- or partition-level handoff support.

#### Scenario: System reboot relaunches the installed current release
- **WHEN** the Raspberry Pi target reboots after the governed autostart service has been installed and enabled
- **THEN** the installed `current` release SHALL come back through the governed service path and SHALL continue to expose `BOOT_STATUS` state from file-backed metadata

### Requirement: Signed Boot Manifest Verification

The boot/update subsystem SHALL require a signed boot manifest or equivalent signed boot metadata before a staged image can be treated as verified for activation.

#### Scenario: Staged image verification requires manifest trust decision
- **WHEN** `BOOT_VERIFY_STAGED_IMAGE` evaluates a staged image
- **THEN** `BootManager` SHALL verify the staged file size, SHA-256 digest, target slot, signer identity, key slot, signature, and software version from the manifest before setting the staged image as verified
- **AND** digest or size match alone SHALL NOT be sufficient to set the staged image as verified.

#### Scenario: Adjacent manifest is the v1 runtime contract
- **WHEN** `BootManager` receives a staged image path for verification
- **THEN** it SHALL resolve the v1 manifest as an adjacent `<stagingPath>.manifest-v1` file under the configured staging root.

### Requirement: Boot Trust Anchor Model

The boot/update subsystem SHALL verify boot manifests against a repo-controlled runtime/config-backed trust anchor model that binds signer identity, key slot, signature algorithm, and verification key material.

#### Scenario: Trusted signer and key slot are configured locally
- **WHEN** a boot manifest declares a signer and key slot
- **THEN** `BootManager` SHALL accept that identity only if runtime configuration provides a matching trusted signer, key slot, algorithm, and key material
- **AND** it SHALL reject manifest-provided signer or key claims that are not present in the configured trust anchor model.

#### Scenario: HMAC-SHA256 is the v1 manifest algorithm
- **WHEN** boot trust v1 verifies a manifest signature
- **THEN** it SHALL use `HMAC-SHA256` over the canonical manifest fields
- **AND** it SHALL keep the trust-anchor lookup boundary narrow enough for a later provider replacement.

### Requirement: Monotonic Boot Version Policy

The boot/update subsystem SHALL enforce a persisted monotonic software-version floor before staged image activation.

#### Scenario: Verification rejects downgrade candidates
- **WHEN** a manifest software version is less than or equal to the persisted last accepted version
- **THEN** `BootManager` SHALL reject the staged image as a downgrade and SHALL NOT mark it trusted or stage-verified.

#### Scenario: Activation advances accepted version floor
- **WHEN** a trusted staged image is activated into the pending-confirm window
- **THEN** `BootManager` SHALL persist that image version as the new last accepted version
- **AND** later rollback SHALL NOT lower the accepted version floor.

### Requirement: Trust Rejection Is Observable

The boot/update subsystem SHALL make each boot trust rejection reason explicit through runtime state, persisted metadata, events, telemetry, and operator status.

#### Scenario: Trust failure cases are distinguishable
- **WHEN** signature verification fails, signer/key slot is unknown, the manifest is malformed, the manifest version is a downgrade, or the staged image digest mismatches the manifest
- **THEN** `BootManager` SHALL reject activation progress
- **AND** it SHALL expose a distinct trust rejection reason instead of reporting a generic or ambiguous success state.

### Requirement: Trust Decision Is Compatible With Confirm And Rollback

The boot/update subsystem SHALL preserve the existing activation, pending-confirm, confirm, timeout rollback, manual rollback, and metadata reload lifecycle while making those states truthful under the boot trust decision.

#### Scenario: Invalid trust state cannot activate
- **WHEN** the current staged image has not passed the complete boot trust decision
- **THEN** `BOOT_ACTIVATE_STAGED_IMAGE` SHALL reject the command and SHALL NOT enter a pending slot.

#### Scenario: Restart preserves truthful pending trust state
- **WHEN** `BootManager` reloads metadata during an active pending-confirm window
- **THEN** it SHALL preserve the pending slot, trusted manifest identity, staged software version, and remaining confirm behavior needed for `BOOT_CONFIRM` or `BOOT_ROLLBACK`.

#### Scenario: Old ambiguous pending metadata fails closed
- **WHEN** existing metadata predates boot trust fields and claims a pending or stage-verified image
- **THEN** `BootManager` SHALL treat that state as untrusted, roll back to the last-known-good slot, and rewrite metadata in the supported schema.

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

### Requirement: BootManager Records Persistent Boot Breadcrumbs
The boot/update subsystem SHALL append bounded persistent fault ring
breadcrumbs for boot-observed startup and recovery-boot-ack stabilization while
keeping boot metadata as the authoritative boot and reset truth.

#### Scenario: Boot-observed breadcrumb reuses persisted boot truth
- **WHEN** the runtime starts and `BootManager` reloads persisted boot metadata
- **THEN** `BootManager` SHALL append a boot-observed breadcrumb that reflects
  the loaded reset-cause, boot-count, consecutive-reset-count,
  last-recovery-source, and last-recovery-level truth
- **AND** the breadcrumb SHALL remain derived from rather than replace the owned
  boot metadata file

#### Scenario: Recovery boot acknowledgement stays additive
- **WHEN** `BootManager` later clears the bounded repeated-recovery restart
  clamp after a stable runtime window
- **THEN** it SHALL append a recovery-boot-ack breadcrumb to the persistent
  fault ring
- **AND** it SHALL preserve the existing `BOOT_STATUS`, `GET_RESET_CAUSE`, and
  `GET_BOOT_COUNT` truth surfaces instead of moving them into the ring

