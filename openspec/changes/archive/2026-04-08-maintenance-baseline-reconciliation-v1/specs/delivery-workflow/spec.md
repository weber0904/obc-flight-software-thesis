## MODIFIED Requirements

### Requirement: Follow-On Change Queue

The project SHALL maintain a named **initial baseline queue** for `bootstrap-fprime-platform`, `core-system-contracts-v1`, `eps-subsystem-v1`, `adcs-subsystem-v1`, `comm-subsystem-v1`, `boot-update-v1`, `verification-ci-v1`, `deployment-runtime-v1`, and `gds-ground-integration-v1`, and the shared `resource-storage` capability SHALL be modified from those changes when needed rather than through a standalone implementation change. Later archived changes MAY extend the repository beyond that initial queue, but those governed expansions SHALL remain visible in the repository's reconciliation trail rather than being treated as undocumented drift.

#### Scenario: Initial baseline queue remains reviewable after later expansions
- **WHEN** reviewers inspect the repository after later capabilities such as autonomy, archive, GPS, or storage observability have been added
- **THEN** the delivery workflow SHALL still identify the original baseline queue while also making the later governed expansions reviewable as formal archived history

#### Scenario: GDS integration change remains part of the initial baseline queue
- **WHEN** the hosted stack gains a documented `fprime-gds` ground path
- **THEN** that work SHALL remain visible in the initial baseline queue history and SHALL not be folded into earlier archived runtime work

## ADDED Requirements

### Requirement: Baseline Reconciliation Matrix
The repository SHALL provide a checked-in baseline reconciliation matrix that maps the original initial baseline queue, the archived change history, the current main-spec capability set, and the reviewable evidence or exception trail for each archived change.

#### Scenario: Archived change maps to capability and evidence trail
- **WHEN** a reviewer inspects an archived change in the repository history
- **THEN** the reconciliation matrix SHALL identify the archived change's primary capability coverage and SHALL cite either reviewable evidence paths or an explicit rationale for why that change is a governance or follow-up exception

#### Scenario: Later governed expansion stays distinct from the initial queue
- **WHEN** a capability is introduced after the original baseline queue has already been completed
- **THEN** the reconciliation matrix SHALL classify that change as a governed expansion instead of pretending it was part of the original queue

### Requirement: Repository Consistency Checks
The delivery workflow SHALL provide a repo-local consistency check that fails when a main spec still carries placeholder Purpose text, when an archived change is missing from the reconciliation matrix, or when the matrix cites a nonexistent capability or evidence path.

#### Scenario: Placeholder main-spec purpose fails the check
- **WHEN** a main spec still contains placeholder purpose text such as archive carry-over or `TBD`
- **THEN** the repo-local consistency check SHALL fail and report the affected spec path

#### Scenario: Archived change is missing from the matrix
- **WHEN** a change with `tasks.md` exists under `openspec/changes/archive/` but is not represented in the reconciliation matrix
- **THEN** the repo-local consistency check SHALL fail and report the missing archived change name
