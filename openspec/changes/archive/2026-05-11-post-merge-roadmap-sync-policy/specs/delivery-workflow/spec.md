## ADDED Requirements

### Requirement: Mainline Narrative Documentation Reconciliation
The delivery workflow SHALL distinguish branch-verifiable documentation from post-merge mainline reconciliation. Documentation whose truth can be reviewed on the originating branch, including formal specs, product behavior descriptions, architecture truth, verification evidence, and operator runbooks, SHALL be updated on that branch before review. `docs/roadmap/*` MAY be updated either on the originating branch when the expected post-merge state is already stable or after merge on `main` when the truthful wording depends on the already-merged mainline state. Any post-merge direct edit on `main` SHALL remain limited to `docs/roadmap/*` progress, dependency, ordering, or handoff reconciliation and SHALL NOT be used for formal workflow, evidence, architecture-truth, or operator-runbook changes.

#### Scenario: Branch-verifiable truth stays with the originating change
- **WHEN** a change alters formal workflow, product behavior, architecture truth, verification evidence, or operator procedures
- **THEN** the relevant documentation SHALL be updated on the originating branch before review
- **AND** the workflow SHALL NOT defer those updates solely because the branch has not merged yet

#### Scenario: Stable roadmap update can ship in the original PR
- **WHEN** the expected post-merge roadmap, progress, or next-work wording is already stable at review time
- **THEN** the originating branch MAY include the `docs/roadmap/*` update in the same PR

#### Scenario: Roadmap reconciliation waits for merged main when necessary
- **WHEN** truthful `docs/roadmap/*` wording depends on the actual merged `main` state, updated merge ordering, or freshly synced mainline progress
- **THEN** the maintainer MAY reconcile `docs/roadmap/*` directly on `main` after merge
- **AND** the workflow SHALL NOT require a second follow-on PR solely to restate that merged roadmap state

#### Scenario: Post-merge shortcut stays narrow
- **WHEN** a post-merge documentation update touches any file outside `docs/roadmap/*` or changes formal workflow, verification evidence, architecture truth, or operator instructions
- **THEN** that update SHALL use the normal branch and review flow
- **AND** it SHALL NOT use the roadmap reconciliation shortcut on `main`
