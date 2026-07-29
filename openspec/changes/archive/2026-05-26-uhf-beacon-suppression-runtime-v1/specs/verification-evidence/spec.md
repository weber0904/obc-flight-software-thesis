## ADDED Requirements

### Requirement: UHF Beacon Suppress Runtime Evidence States What Was Newly Proven

The verification evidence for `uhf-beacon-suppression-runtime-v1` SHALL record
the exact governed path under test, what was newly proven, and what remains
outside the claim boundary.

#### Scenario: Evidence names the exact new runtime claim
- **WHEN** the change records its final evidence
- **THEN** it SHALL identify that accepted authenticated UHF
  `SESSION_OPEN(seq0)` starts suppress, same-session accepted UHF activity
  refreshes the bounded window, and inactivity timeout or explicit invalidation
  resumes beaconing
- **AND** it SHALL state the owner as `CommController`
- **AND** it SHALL preserve explicit non-claims for simultaneous dual-link
  arbitration, UHF reliable transfer, RF closure, and broader handshake state

### Requirement: UHF Beacon Suppress Runtime Evidence Includes Hosted And Target Proof

The verification evidence for `uhf-beacon-suppression-runtime-v1` SHALL
include both a hosted governed node-`6` proof and a target/lab quiet-UHF
node-`6` proof.

#### Scenario: Hosted proof records start, hold, resume, and a negative case
- **WHEN** the hosted node-`6` suppress/runtime probe passes
- **THEN** the evidence SHALL identify the hosted CCSDS S-band plus UHF node-`6`
  path, baseline beacon visibility before suppress, accepted UHF
  `SESSION_OPEN(seq0)` start, accepted same-session UHF read/status refresh,
  inactivity-timeout resume, and at least one invalid or non-qualifying
  no-suppress result

#### Scenario: Target quiet-UHF proof stays bounded
- **WHEN** the target/lab quiet node-`6` suppress/runtime probe passes
- **THEN** the evidence SHALL identify the physical quiet-UHF path, probe-owned
  beacon capture markers, journal or event evidence for suppress transitions,
  and the bounded quiet-UHF acceptance used for that result
- **AND** it SHALL state that the target proof does not prove non-quiet
  background-TM stability, simultaneous dual-link runtime, UHF reliable
  transfer, or RF behavior
