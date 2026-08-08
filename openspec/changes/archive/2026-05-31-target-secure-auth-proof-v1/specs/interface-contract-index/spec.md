## ADDED Requirements

### Requirement: Interface Index Distinguishes Hosted And Target Secure Auth Status

The interface contract index SHALL distinguish hosted secure-auth proof from
the target secure-auth proof added by `target-secure-auth-proof-v1`.

#### Scenario: Interfaces show target observability after target proof
- **WHEN** target secure-auth proof evidence is accepted
- **THEN** `docs/interfaces.md` SHALL update secure-auth and secure-command
  observability wording so target/lab status no longer appears unproven for
  the exact S-band and bounded UHF cases exercised by the proof.

#### Scenario: Interfaces keep UHF role boundaries explicit
- **WHEN** `docs/interfaces.md` describes UHF secure auth after this change
- **THEN** it SHALL keep `uhf-backup` and
  `uhf-primary-after-failover` separate
- **AND** it SHALL state that UHF primary staged-upload success remains outside
  the target proof claim.
