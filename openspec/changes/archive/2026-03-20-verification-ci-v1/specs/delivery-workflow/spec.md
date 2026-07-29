## MODIFIED Requirements

### Requirement: Delivery Gates

The delivery workflow SHALL require a build gate, unit/component test coverage, and integration evidence when change scope reaches integration behavior, and pull requests SHALL summarize intent, impact, testing, and any constrained validation evidence.

#### Scenario: Repository CI calls the shared verification gate
- **WHEN** the repository CI workflow validates the baseline gate
- **THEN** it SHALL invoke one repo-local verification script that runs the documented build, test, and OpenSpec checks instead of duplicating those commands independently in workflow YAML

### Requirement: Follow-On Change Queue

The project SHALL maintain a named follow-on queue for `bootstrap-fprime-platform`, `core-system-contracts-v1`, `eps-subsystem-v1`, `adcs-subsystem-v1`, `comm-subsystem-v1`, `boot-update-v1`, and `verification-ci-v1`, and the shared `resource-storage` capability SHALL be modified from those changes when needed rather than through a standalone implementation change.

#### Scenario: Initial queue item becomes historical after archive
- **WHEN** one of the named initial follow-on changes is archived
- **THEN** the project SHALL preserve that name as part of the documented implementation history while treating the queue item itself as complete
