## ADDED Requirements

### Requirement: Complex Change Commit Boundaries
The delivery workflow SHALL allow complex or high-risk local development to keep meaningful intermediate commits as rollback points, but before the first push to origin the workflow SHALL still collect that local history into the single reviewable Conventional Commit required for the `local-ready` checkpoint. After the branch has been pushed, review follow-ups SHOULD use additional focused fix commits instead of rewriting published history by default.

#### Scenario: Complex local development keeps rollback checkpoints
- **WHEN** a complex, high-risk, or multi-step change is still being developed locally
- **THEN** the workflow MAY keep meaningful intermediate commits as rollback points instead of relying on one large unstaged diff all the way through

#### Scenario: First push still uses one reviewable Conventional Commit
- **WHEN** a change is about to be reported as `local-ready` and pushed to origin for the first time
- **THEN** its local development history SHALL be collected into the single reviewable Conventional Commit required by the workflow

#### Scenario: Published branch accepts focused fix commits
- **WHEN** a review requests changes after the branch has already been pushed
- **THEN** the workflow SHOULD use additional focused fix commits instead of rewriting published history by default
