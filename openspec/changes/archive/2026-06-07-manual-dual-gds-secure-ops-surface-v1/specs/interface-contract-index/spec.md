## ADDED Requirements

### Requirement: Interface Index Records The Manual Operator Surface Contract

`docs/interfaces.md` SHALL summarize the maintained manual operator manifest
and session-state contract for the hosted and target dual-GDS surfaces.

#### Scenario: Reviewers can audit the manual helper contract in one place
- **WHEN** a reviewer inspects the COMM/manual-ops section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see the required manifest fields, the hosted
  versus target surface split, and the stored auth/session-state fields
- **AND** they SHALL be able to see the invalidation triggers for manual auth
  state without having to infer them from helper code alone.
