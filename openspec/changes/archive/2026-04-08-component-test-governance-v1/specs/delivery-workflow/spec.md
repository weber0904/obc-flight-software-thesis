## MODIFIED Requirements

### Requirement: Delivery Gates

The delivery workflow SHALL require a build gate, unit/component test coverage, and integration evidence when change scope reaches integration behavior, and pull requests SHALL summarize intent, impact, testing, and any constrained validation evidence.

#### Scenario: Repository CI calls the shared verification gate
- **WHEN** the repository CI workflow validates the baseline gate
- **THEN** it SHALL invoke one repo-local verification script that runs the documented build, test, and OpenSpec checks instead of duplicating those commands independently in workflow YAML

#### Scenario: Real repository components require classic F' L2 coverage
- **WHEN** a formal change introduces or modifies a real repository component under `OBC/Components/` that derives from `*ComponentBase`
- **THEN** the change SHALL include a classic F' component harness using `register_fprime_ut()`, generated `TesterBase`/`GTestBase`, and interface-level command, event, telemetry, or port assertions unless a later governed exception explicitly approves a different shape

## ADDED Requirements

### Requirement: Component-Test Baseline Checker
The delivery workflow SHALL provide a repo-local checker that enumerates real repository components and fails when one lacks the required classic F' component harness registration.

#### Scenario: New component without classic UT fails the checker
- **WHEN** a real component derived from `*ComponentBase` is present in `OBC/Components/` but its module does not register classic F' UT
- **THEN** the repo-local component-test-baseline checker SHALL fail and report the offending component name

#### Scenario: Shared gate runs the component-test checker
- **WHEN** the repository executes the shared baseline gate
- **THEN** that gate SHALL run the component-test-baseline checker before declaring the baseline verification complete
