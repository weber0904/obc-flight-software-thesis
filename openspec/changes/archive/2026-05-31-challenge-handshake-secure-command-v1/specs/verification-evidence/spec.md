## MODIFIED Requirements

### Requirement: Authenticated Command Envelope Evidence Is Reviewable

Verification evidence for the secure command path SHALL distinguish challenge
authorization proof from post-auth secure command proof and SHALL keep legacy
v1 evidence reviewable as a separate compatibility path.

#### Scenario: Secure auth evidence proves the full hosted bootstrap sequence
- **WHEN** `challenge-handshake-secure-command-v1` evidence is cited
- **THEN** evidence SHALL list coverage for:
  - `ReqAuth -> Challenge -> Response -> Authenticated`
  - malformed handshake rejection
  - wrong service rejection
  - wrong response rejection
  - timeout clearing
  - reboot clearing
  - UHF role invalidation clearing

#### Scenario: Secure command evidence proves post-auth gating
- **WHEN** the same evidence cites secure command v2
- **THEN** it SHALL list coverage for:
  - no active auth state rejection
  - auth-granted runtime open synthesis
  - first accepted secure command at `sequenceNumber = 1`
  - increasing sequence acceptance
  - duplicate or lower sequence rejection
  - bad MAC rejection
  - UHF backup authenticated read/status continuity
  - UHF failover-primary re-auth before higher-authority command acceptance

#### Scenario: Legacy compatibility evidence remains separate
- **WHEN** the same change cites the retained legacy v1 path
- **THEN** it SHALL state legacy coverage separately from the new secure path
- **AND** it SHALL NOT present legacy `SESSION_OPEN` acceptance as proof that
  the secure auth bootstrap path was exercised.
