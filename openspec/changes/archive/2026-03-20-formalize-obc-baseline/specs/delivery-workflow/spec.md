## ADDED Requirements

### Requirement: Formal Delivery Governance
The project SHALL treat `obc-dev-spec/` as the narrative source layer, `openspec/specs/` as the formal main-spec layer, and `openspec/changes/` as the only sanctioned change workspace. The bootstrap change `formalize-obc-baseline` SHALL be the only governance-only change; later changes SHALL include implementation and verification work.

#### Scenario: Later capability work includes implementation
- **WHEN** a post-bootstrap capability change is proposed
- **THEN** its task plan SHALL include implementation and verification work rather than only narrative or governance edits

### Requirement: OpenSpec Lifecycle
The official change lifecycle SHALL use the real OpenSpec CLI flow of `new change`, `status`, `instructions`, `validate`, and `archive`, and the project SHALL NOT use manual directory moves as the formal archive mechanism.

#### Scenario: Change completion uses archive command
- **WHEN** a change is ready to sync into the main specs
- **THEN** the project SHALL use `openspec archive` to update `openspec/specs/` instead of manually moving the change directory

### Requirement: Delivery Gates
The delivery workflow SHALL require a build gate, unit/component test coverage, and integration evidence when change scope reaches integration behavior, and pull requests SHALL summarize intent, impact, testing, and any constrained validation evidence.

#### Scenario: Integration-affecting pull request
- **WHEN** a pull request affects simulator, comms, boot, or other integration behavior
- **THEN** the delivery record SHALL include the corresponding integration evidence or constrained-validation record

### Requirement: Follow-On Change Queue
The project SHALL maintain a named follow-on queue for `bootstrap-fprime-platform`, `core-system-contracts-v1`, `eps-subsystem-v1`, `adcs-subsystem-v1`, `comm-subsystem-v1`, `boot-update-v1`, and `verification-ci-v1`, and the shared `resource-storage` capability SHALL be modified from those changes when needed rather than through a standalone implementation change.

#### Scenario: Resource-storage behavior changes later
- **WHEN** a later implementation change needs to alter storage or resource requirements
- **THEN** that change SHALL modify the `resource-storage` capability as part of its own scope instead of creating a separate implementation queue item
