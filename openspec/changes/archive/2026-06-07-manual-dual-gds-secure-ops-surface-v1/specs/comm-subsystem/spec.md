## ADDED Requirements

### Requirement: Manual Secure Ops Helper Stays Separate From GDS UI

The current COMM baseline SHALL expose manual secure auth, secure-v2 command,
and governed sequence/file operator actions through a repo-owned helper surface
that remains distinct from the stock GDS UI.

#### Scenario: GDS UI is not treated as the secure command plane
- **WHEN** operators use the maintained manual dual-GDS surface
- **THEN** stock `fprime-gds` SHALL remain the reviewable TT&C observation
  surface
- **AND** secure auth, secure-v2 command send, governed staged upload, and
  governed `SEQ_*` actions SHALL run through the repo-owned helper CLI instead
  of by intercepting stock GDS UI commands.

### Requirement: Manual Secure Ops Helper Uses The Current Secure Baseline

The maintained manual secure-ops helper SHALL use the current secure-auth plus
secure-v2 command path and SHALL keep legacy `SESSION_OPEN` out of the default
operator flow.

#### Scenario: Helper derives secure session from the tracked keystore
- **WHEN** the manual helper establishes auth on the maintained hosted or
  target COMM path
- **THEN** it SHALL use the tracked command-auth keystore material for the
  selected secure service
- **AND** it SHALL derive and persist the session using the current secure
  handshake and secure-v2 packet format.

#### Scenario: Helper scope stays governed
- **WHEN** an operator uses the maintained helper for file or sequence actions
- **THEN** it SHALL permit only governed `.sequence-staging/<leaf>` upload and
  `SequenceAdmissionController` wrapper commands
- **AND** it SHALL NOT present arbitrary file-uplink destinations, raw
  `SeqDispatcher.RUN`, or raw `CmdSequencer` controls as the maintained default
  operator surface.

### Requirement: Manual Secure Session State Has Explicit Invalidation

The maintained manual helper SHALL persist its auth/session state under the
owned surface root and SHALL invalidate that state on explicit band changes,
timeout, manual clear, or surface restart.

#### Scenario: Session invalidation is reviewable
- **WHEN** an operator inspects the maintained manual helper status
- **THEN** the helper SHALL report whether an active secure session exists,
  which band/service owns it, the next secure sequence, and whether the stored
  state has been invalidated by timeout or band transition.

### Requirement: Target Manual Auth Uses A Provenance Gate

The maintained target manual helper SHALL verify the installed release keystore
and active service root before completing manual auth establishment.

#### Scenario: Target helper rejects stale installed auth material
- **WHEN** target manual auth is requested
- **THEN** the helper SHALL compare the repo-tracked keystore SHA with the
  installed release keystore SHA
- **AND** it SHALL verify that the active OBC service still points at the
  governed current release root
- **AND** it SHALL refuse to claim successful auth establishment when that
  provenance gate fails.
