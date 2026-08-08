## MODIFIED Requirements

### Requirement: Secure Auth Completion Is The Preferred UHF Command Boundary

The comm subsystem SHALL present accepted UHF secure-auth completion as the
preferred current UHF operational command-session boundary. Any retained
legacy `SESSION_OPEN(seq0)` boundary SHALL be scoped to compatibility-only
traffic or explicit historical cleanup surfaces.

#### Scenario: Legacy wording no longer leads current COMM summaries
- **WHEN** current COMM docs, runbooks, or specs summarize UHF command-session
  behavior
- **THEN** they SHALL lead with auth-success session ownership on the secure
  path
- **AND** any legacy `SESSION_OPEN(seq0)` mention SHALL be explicitly labeled
  compatibility-only or historical-cleanup scoped.
