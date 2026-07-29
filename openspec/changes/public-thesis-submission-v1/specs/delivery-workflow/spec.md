## ADDED Requirements

### Requirement: Public Thesis Release Uses A Separate Governed Repository
The thesis release SHALL be assembled on a dedicated branch of the public
repository from a fixed source commit and SHALL use PR, CI, merge, annotated
tag, and GitHub Release gates.

#### Scenario: Public release reaches local-ready
- **WHEN** implementation, documentation, evidence, OpenSpec sync/archive, and
  clean-clone validation are complete
- **THEN** the branch SHALL remain local until explicit push approval
- **AND** the eventual PR SHALL be ready-for-review rather than draft

#### Scenario: Public release reaches tag-ready
- **WHEN** the public PR is merged and required CI is green
- **THEN** the exact merged commit SHALL be the only allowed tag target

## REMOVED Requirements

### Requirement: Repository Agent Onboarding Entrypoint
**Reason**: The public portfolio distribution uses standard contributor
documentation rather than agent-specific onboarding.

**Migration**: Preserve formal workflow rules in `CONTRIBUTING.md` and the
delivery-workflow spec; retain the historical requirement in archived OpenSpec.
