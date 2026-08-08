## ADDED Requirements

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
