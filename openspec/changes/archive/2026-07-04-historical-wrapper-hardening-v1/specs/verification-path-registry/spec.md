## ADDED Requirements

### Requirement: Historical Or Retired Wrapper Entrypoints Self-Identify

The verification-path registry SHALL require any script entrypoint that is
currently classified as supplemental historical or retired historical rather
than a maintained closeout gate to self-identify that status at execution time.

#### Scenario: Supplemental historical wrapper requires explicit opt-in
- **WHEN** a repository-owned wrapper is preserved only as supplemental
  historical reference and a developer invokes it without explicit override
- **THEN** the wrapper SHALL refuse to proceed
- **AND** it SHALL explain that the wrapper is not part of the current
  maintained gate set
- **AND** it SHALL state the explicit override required to run it intentionally.

#### Scenario: Retired historical wrapper fails closed
- **WHEN** a repository-owned wrapper is classified as retired historical and a
  developer invokes it
- **THEN** the wrapper SHALL exit before attempting runtime setup or proof work
- **AND** it SHALL direct the developer to the current registry or runbook
  authority instead of behaving like a maintained gate.

### Requirement: Historical Wrapper Messaging Matches Governing Docs

The verification-path registry, runbooks, and wrapper entrypoints SHALL use
consistent current-baseline wording for historical or retired probe families so
developers do not see conflicting authority signals.

#### Scenario: Wrapper messaging stays aligned with registry wording
- **WHEN** reviewers inspect a hardened historical or retired wrapper together
  with the governing docs
- **THEN** the wrapper message SHALL match the same classification vocabulary
  used by the current registry/runbook layer
- **AND** it SHALL not imply that the wrapper is a current maintained closeout
  dependency.
