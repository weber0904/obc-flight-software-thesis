## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the current UHF backup and UHF
primary-after-failover role split, and it SHALL record that UHF primary packet
quiet on the formal UHF path is primary-band-driven while UHF beacon
suppress/runtime remains accepted-session-driven.

#### Scenario: Reviewers can audit current UHF role semantics in one place
- **WHEN** reviewers inspect the current COMM role boundary in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that `uhf-backup` remains bounded
  allowlisted backup ingress rather than beacon-only wording
- **AND** they SHALL be able to see that `uhf-primary-after-failover` is the
  explicit-switch clean-UHF runtime role

#### Scenario: Reviewers can audit packet quiet versus beacon suppress in one place
- **WHEN** reviewers inspect the current COMM egress ownership section in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that formal UHF primary packet quiet is
  triggered directly by the current UHF primary-band state
- **AND** they SHALL be able to see that UHF beacon suppress still starts only
  after accepted qualifying UHF `SESSION_OPEN(seq0)` and same-session accepted
  activity

#### Scenario: Reviewers can audit packet-versus-file quiet semantics in one place
- **WHEN** reviewers inspect the current COMM egress ownership section in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that diagnostic quiet may suppress packet
  and file egress
- **AND** they SHALL be able to see that UHF primary packet quiet suppresses
  live packet egress only and preserves official file/data-product routing
