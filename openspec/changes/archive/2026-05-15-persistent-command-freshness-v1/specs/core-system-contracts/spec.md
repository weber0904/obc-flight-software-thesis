## MODIFIED Requirements

### Requirement: Session Open Is The V1 Lifecycle And Recovery Surface

`SESSION_OPEN` SHALL be the only v1 command-session open, replace, and resync
surface.

#### Scenario: Fresh open establishes active session and advances persisted floor
- **WHEN** a source epoch receives an authority-allowed valid `SESSION_OPEN`
  envelope with `sequence_number = 0`
- **AND** the envelope `session_id` is strictly greater than the persisted
  source-epoch freshness floor
- **THEN** the gate SHALL persist that new session epoch before reporting
  success
- **AND** it SHALL establish that envelope `session_id` as the active session
  for the source epoch
- **AND** it SHALL establish sequence baseline `0`
- **AND** it SHALL synthesize exactly one `Fw::CmdResponse::OK` without
  forwarding the inner command to `Svc::CmdDispatcher`.

#### Scenario: Replay reopen fails closed across restart
- **WHEN** a source epoch receives an authority-allowed valid `SESSION_OPEN`
  envelope whose `session_id` is less than or equal to the persisted
  source-epoch freshness floor
- **THEN** the gate SHALL reject that reopen before `Svc::CmdDispatcher`
- **AND** it SHALL leave the current in-memory session closed or unchanged
- **AND** it SHALL expose that rejection as stale replay rather than as a
  generic open failure.

### Requirement: Session Lifecycle Evidence Is Dedicated And Bounded

Session lifecycle SHALL expose dedicated rejection and open evidence without
expanding broader security claims.

#### Scenario: Session lifecycle exposes persistent freshness health
- **WHEN** `CommandIngressAuthority` loads or updates persisted freshness state
- **THEN** it SHALL emit dedicated reviewable telemetry and/or event evidence
  for persistent-store health, active source-epoch floor, and load or save
  faults
- **AND** it SHALL keep that evidence distinct from `BootManager` boot metadata
  truth.

#### Scenario: Lifecycle claims stay bounded after persistence
- **WHEN** `persistent-command-freshness-v1` is cited
- **THEN** it SHALL be valid to claim reboot-safe monotonic session reopen
  enforcement on the active path
- **AND** it SHALL NOT be cited as nonce-window replay protection, persistent
  secure key storage, hardware-backed secure boot, bootloader handoff proof, or
  legacy retirement.

### Requirement: Reboot Requires Fresh Session Open

Session lifecycle state SHALL require a fresh reopen after runtime restart while
preserving the persisted source-epoch freshness floor.

#### Scenario: Restart clears active session but preserves reopen floor
- **WHEN** the hosted runtime or equivalent topology process restarts
- **THEN** all active in-memory session lifecycle state and in-memory sequence
  state SHALL be cleared
- **AND** the persisted source-epoch freshness floor SHALL remain available to
  reject replayed or lower/equal `SESSION_OPEN` values
- **AND** non-lifecycle enveloped traffic SHALL fail closed until a fresh
  higher `SESSION_OPEN` is accepted.

## ADDED Requirements

### Requirement: Persistent Freshness Store Is Dedicated And Fail-Closed

The active command-security path SHALL persist command freshness in a dedicated
`CommandIngressAuthority` store rather than in `BootManager` metadata.

#### Scenario: Dual-copy store falls back on single-copy corruption
- **WHEN** the persistent freshness store has one invalid or corrupt copy and
  one valid copy
- **THEN** `CommandIngressAuthority` SHALL load the newest valid copy
- **AND** it SHALL continue enforcing the persisted source-epoch floors without
  silently resetting them.

#### Scenario: Both-invalid persistent state fails closed
- **WHEN** both freshness-store copies are invalid or unavailable for a
  comm-managed ingress path
- **THEN** `CommandIngressAuthority` SHALL reject enveloped lifecycle and
  non-lifecycle traffic that depends on persisted freshness truth
- **AND** it SHALL expose a distinct persistent-state-unavailable rejection
  reason instead of silently reopening the source epoch.
