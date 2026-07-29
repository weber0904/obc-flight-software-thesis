## ADDED Requirements

### Requirement: Interface Index Records Current COMM Observability Semantics

`docs/interfaces.md` SHALL include a dedicated COMM observability contract
section for the current baseline that lists each frozen field with owner, units,
freshness or stale rule, unavailable-value semantics, and hosted versus
target-lab truth boundary.

#### Scenario: Reviewers can audit COMM observability field semantics
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see the frozen field set for
  `GroundLinkDriver`, `GroundLinkHealthProvider`, `CommController`,
  `RadioController`, and `UartDriver`
- **AND** each listed field SHALL identify owner, units, freshness or stale
  rule, unavailable semantics, and hosted versus target-lab truth boundary
