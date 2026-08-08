## MODIFIED Requirements

### Requirement: Registry Exposes Stable Current And Historical Status
The repository SHALL present current verification coverage in
`docs/verification.md` and SHALL retain the detailed path ledger under
`evidence/verification-path-registry.md` for engineering traceability.

#### Scenario: Reviewer selects a validation path
- **WHEN** a reviewer starts from the verification overview
- **THEN** maintained build, test, hosted, and target/lab evidence SHALL be
  discoverable by capability
- **AND** readers needing exact path identities SHALL be routed to the detailed
  evidence ledger
