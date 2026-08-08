## MODIFIED Requirements

### Requirement: Verification Evidence Separates Current Secure Proof From Legacy Compatibility Proof

The verification evidence tree SHALL keep legacy command-envelope proof
families reviewable as history while requiring current secure-baseline claims
to cite the secure-auth and secure-command proof family.

#### Scenario: Legacy proof records stay historical
- **WHEN** evidence cites `command-envelope-metadata-v1`,
  `command-session-sequence-v1`, `command-session-lifecycle-v1`,
  `command-auth-envelope-v1`, or `persistent-command-freshness-v1`
- **THEN** it SHALL describe those records as legacy compatibility evidence
  rather than preferred current secure-baseline proof.

#### Scenario: Current secure claims cite the secure-auth family
- **WHEN** evidence claims the preferred current comm-managed command-session
  baseline
- **THEN** it SHALL cite the secure-auth and secure-command proof family
  rather than a legacy `SESSION_OPEN` proof family.
