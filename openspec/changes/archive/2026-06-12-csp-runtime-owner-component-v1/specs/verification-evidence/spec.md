## MODIFIED Requirements

### Requirement: Layered Verification Model
The project SHALL organize verification into L1 unit tests, L2 component tests, L3 integration tests, and L4 system or mission-scenario tests, and each layer SHALL have a distinct purpose rather than replacing another layer.

#### Scenario: Runtime-owner refactor carries both local and target evidence
- **WHEN** a change centralizes deployed CSP runtime ownership and alters how COMM, EPS, or ADCS clients reach the runtime
- **THEN** that change SHALL provide local build or test evidence for the owner-injected topology
- **AND** it SHALL keep any target secure-auth proof separate and reviewable as system-level evidence rather than claiming architecture closure from unit tests alone
