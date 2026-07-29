## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the active UHF beacon suppress and
resume runtime contract in `docs/interfaces.md`, including owner, trigger,
refresh semantics, timeout units, reviewable state, and explicit non-claims.

#### Scenario: Reviewers can audit suppress and resume semantics in one place
- **WHEN** a reviewer inspects the COMM sections of `docs/interfaces.md`
- **THEN** they SHALL be able to see that `CommController` owns UHF beacon
  suppress and resume runtime truth
- **AND** they SHALL be able to see that accepted authenticated UHF
  `SESSION_OPEN(seq0)` is the suppress-start boundary
- **AND** they SHALL be able to see that accepted authenticated same-session
  UHF read/status or command traffic refreshes the bounded active window
- **AND** they SHALL be able to see that `BeaconPublisher` stays session-agnostic

### Requirement: Interface Index Records Current COMM Observability Semantics

`docs/interfaces.md` SHALL include the UHF beacon suppress runtime fields in
the dedicated COMM observability contract section, with owner, units,
freshness, unavailable semantics, and hosted versus target-lab truth boundary.

#### Scenario: Reviewers can audit suppress runtime observability semantics
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see suppress-active state, owner ingress,
  owner role, owner session, last accepted sequence, remaining ticks, and
  timeout ticks
- **AND** each listed field SHALL identify owner, units, freshness or stale
  rule, unavailable semantics, and hosted versus target-lab truth boundary
