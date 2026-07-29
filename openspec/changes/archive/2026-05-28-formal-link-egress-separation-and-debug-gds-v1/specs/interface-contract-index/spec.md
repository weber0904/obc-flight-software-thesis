## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the current UHF backup and UHF
primary-after-failover role split, and it SHALL record that session quiet on
the formal UHF path suppresses live packet egress without redefining official
file/data-product downlink as non-formal traffic.

#### Scenario: Reviewers can audit current UHF role semantics in one place
- **WHEN** reviewers inspect the current COMM role boundary in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that `uhf-backup` remains bounded
  allowlisted backup ingress rather than beacon-only wording
- **AND** they SHALL be able to see that `uhf-primary-after-failover` is the
  explicit-switch clean-UHF runtime role

#### Scenario: Reviewers can audit packet-versus-file quiet semantics in one place
- **WHEN** reviewers inspect the current COMM egress ownership section in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that diagnostic quiet may suppress packet
  and file egress
- **AND** they SHALL be able to see that UHF session quiet suppresses live
  packet egress only and preserves official file/data-product routing
