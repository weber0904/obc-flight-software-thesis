## MODIFIED Requirements

### Requirement: Change Completion Checkpoints
The delivery workflow SHALL distinguish a `local-ready` checkpoint from formal completion. A change SHALL be considered `local-ready` only after the implementation is finished, the relevant local verification passes, the OpenSpec change has been archived, the worktree is clean, and the work has been collected into a Conventional Commit.

#### Scenario: Change reaches the local-ready checkpoint
- **WHEN** a change has been implemented and locally verified
- **THEN** it SHALL NOT be reported as ready to push until archive, clean-worktree state, and a Conventional Commit are also complete

### Requirement: Push And CI Gate
The delivery workflow SHALL treat push as a separate step after the `local-ready` checkpoint, and a change SHALL NOT be treated as formally complete until the pushed commit has passed the required GitHub CI workflow.

#### Scenario: Change waits for developer confirmation before push
- **WHEN** a change has reached the `local-ready` checkpoint
- **THEN** the workflow SHALL wait for explicit developer confirmation before pushing the commit to GitHub

#### Scenario: Change becomes formally complete only after CI green
- **WHEN** a change has been pushed to GitHub
- **THEN** it SHALL remain in a not-yet-complete state until the relevant CI workflow reports success for that pushed commit

### Requirement: Release And Tag Timing
The delivery workflow SHALL create releases and tags only after the relevant pushed commit has completed the required CI workflow successfully.

#### Scenario: Release work waits for CI green
- **WHEN** a change is being considered for release or tag creation
- **THEN** the workflow SHALL wait until the relevant pushed commit has passed CI before creating the release or tag

## ADDED Requirements

### Requirement: Formal Work Starts On A Dedicated Branch
The delivery workflow SHALL begin each formal repository change from a dedicated branch named for its intent, using `feature/*`, `fix/*`, `docs/*`, or `hotfix/*`, and SHALL avoid direct feature development on `main`.

#### Scenario: New feature work starts from a feature branch
- **WHEN** a developer begins a new capability or integration slice
- **THEN** the work SHALL start on a dedicated `feature/*` branch instead of committing the new feature directly on `main`

#### Scenario: Documentation or workflow repair starts from a docs or fix branch
- **WHEN** a developer begins a documentation-only or bug-fix slice
- **THEN** the work SHALL start on a dedicated `docs/*` or `fix/*` branch whose name reflects the change intent

### Requirement: Repository Evidence Governs Validation Path Reuse
The delivery workflow SHALL require developers to consult repository evidence before reusing a validation path, and it SHALL NOT treat generic upstream F' expectations as sufficient proof that a repository-specific path is already established.

#### Scenario: Generic framework knowledge is not enough
- **WHEN** a developer believes a path is already proven because it is common in generic F' usage
- **THEN** the workflow SHALL require them to cite repository evidence or the verification-path registry before treating that path as established in this repository

#### Scenario: Adjacent validation paths stay distinct
- **WHEN** a change validates one path that sits next to another path in the same ground or transport stack
- **THEN** the workflow SHALL keep those paths distinct unless archived evidence proves each one separately
