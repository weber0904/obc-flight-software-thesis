## ADDED Requirements

### Requirement: Persistent Command Freshness Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local, hosted, and
bounded Raspberry Pi proof for `persistent-command-freshness-v1`.

#### Scenario: Focused tests cover store and session contracts
- **WHEN** `persistent-command-freshness-v1` completes focused local
  verification
- **THEN** the evidence SHALL identify tests covering first-boot empty load,
  persisted-floor advance on accepted open, restart replay rejection, fresh
  higher reopen acceptance, stale old-session rejection before dispatch,
  per-source independence, newer-copy corruption fallback, and both-invalid
  fail-closed behavior.

#### Scenario: Hosted probe proves active-path reboot-safe reopen boundary
- **WHEN** `persistent-command-freshness-v1` records hosted proof
- **THEN** the evidence SHALL include a repository-owned active-path probe that
  opens a valid session, restarts the same runtime root, rejects replayed old
  envelopes, accepts a fresh higher `SESSION_OPEN`, and proves successful
  follow-on command dispatch
- **AND** it SHALL describe that proof as same-runtime-root restart persistence
  rather than as target power-loss proof.

#### Scenario: Raspberry Pi evidence proves bounded persistence coexistence
- **WHEN** `persistent-command-freshness-v1` records Raspberry Pi target proof
- **THEN** the evidence SHALL show that the active target path preserves both
  boot-trust metadata and command-freshness state across target restart or
  reboot on the same governed active package path
- **AND** it SHALL keep stale-replay functional closure formally grounded in
  hosted proof unless separate target behavior is also demonstrated.
