## ADDED Requirements

### Requirement: External Sequence Execution Requires Repo-Owned Admission

Externally requested sequence execution on the active baseline SHALL be admitted by a repo-owned owner before any official sequence engine runs the file.

#### Scenario: External stock sequence commands are denied directly

- **WHEN** comm-managed ingress submits `SeqDispatcher.RUN`, `SeqDispatcher.RUN_ARGS`, or direct `CmdSequencer` `CS_RUN` / `CS_START` / `CS_STEP` / `CS_AUTO` / `CS_MANUAL` / `CS_JOIN_WAIT` / `CS_CANCEL`
- **THEN** the active command authority path SHALL reject the request before stock sequence execution begins

#### Scenario: Wrapper-controlled sequence admission checks the full file contents

- **WHEN** a caller uses the repo-owned sequence wrapper surface
- **THEN** the admission owner SHALL validate official sequence format and CRC
- **AND** it SHALL deserialize each record into a complete `Fw::CmdPacket`
- **AND** it SHALL verify opcode presence, legal argument serialization, and per-inner-command authority against the caller profile
- **AND** it SHALL reject the sequence if any inner command violates caller authority

### Requirement: Sequence Execution Uses Immutable Admitted Copies

The active sequence wrapper SHALL prevent execution-time dependence on mutable staged files.

#### Scenario: Admitted copy is executed instead of staged source

- **WHEN** a staged sequence file passes admission
- **THEN** the wrapper SHALL copy it into an admission-owned path before execution
- **AND** later run or manual control SHALL reference the admitted copy, not the original staged path

### Requirement: Backup Sequence Control Is Ownership Bound

`uhf-backup` sequence control SHALL remain bounded by both inner-command authority and context ownership.

#### Scenario: Backup can only act on backup-owned admitted contexts

- **WHEN** `uhf-backup` requests validate, run, prepare-manual, start, step, or cancel
- **THEN** the wrapper SHALL allow those operations only for contexts admitted under backup ownership and only when every inner command is backup-allowed
- **AND** it SHALL reject requests that target a context owned by a higher-authority ingress

### Requirement: SystemResources Enable Control Is Not Backup-Writable

The active `SystemResources.ENABLE` surface SHALL be governed as a runtime configuration control, not a backup-writable status surface.

#### Scenario: Backup cannot change SystemResources enable state

- **WHEN** `uhf-backup` attempts to invoke `SystemResources.ENABLE`
- **THEN** the command authority path SHALL reject it
