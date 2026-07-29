## ADDED Requirements

### Requirement: Mission Console Consumes The Maintained Manual COMM Operator Surface

The COMM subsystem capability SHALL allow a repo-owned Mission Console to
consume the maintained manual secure auth, secure-v2 command, governed staged
upload, and governed `SEQ_*` operator surface without redefining stock GDS as
the secure command plane.

#### Scenario: Mission Console does not redefine GDS UI authority
- **WHEN** the Mission Console is used on the maintained hosted or target manual
  dual-GDS surface
- **THEN** stock `fprime-gds` SHALL remain a reviewable observation surface
- **AND** Mission Console operator actions SHALL still run through the
  repo-owned helper authority rather than through stock GDS UI interception.

### Requirement: Mission Console Ensure-Auth Respects Existing Session Invalidation

The maintained COMM operator path SHALL allow Mission Console `ensure-auth`
behavior that re-establishes auth only when no valid session exists and that
still treats band switch, timeout, manual clear, or surface restart as session
invalidation boundaries.

#### Scenario: Mission Console re-auths after invalidation but not before
- **WHEN** a Mission Console action requests `ensure-auth`
- **THEN** the Gateway SHALL establish auth only if the stored session is
  missing, invalidated, expired, or tied to an old surface identity
- **AND** an explicit `COMM_SET_ACTIVE` transition SHALL still force the old
  band session to become invalid and require re-auth on the new band.

### Requirement: Mission Console Negative Packet Demo Stays Diagnostic-Only

The maintained COMM operator path SHALL permit a bounded Mission Console
negative packet demo surface that sends selected malformed or replayed packets
through the existing TTS path for lab diagnosis without broadening the formal
operator authority.

#### Scenario: Negative packet demo does not become a second command plane
- **WHEN** Mission Console injects replay, stale-session, duplicate-sequence,
  tampered-sequence, or tampered-MAC packets
- **THEN** it SHALL send them only through the existing `gdsTtsPort` path using
  the current secure packet format or captured raw bytes
- **AND** it SHALL remain explicitly diagnostic-only rather than a general
  operator command surface or broad fuzzing framework.
