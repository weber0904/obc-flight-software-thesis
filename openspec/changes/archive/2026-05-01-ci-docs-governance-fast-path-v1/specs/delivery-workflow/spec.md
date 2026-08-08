## MODIFIED Requirements

### Requirement: Delivery Gates
The delivery workflow SHALL keep a single required GitHub CI job named `baseline-gate`, SHALL require build and test coverage for product-impacting changes, SHALL require OpenSpec spec validation and core repository traceability checks for all governed changes, and SHALL require pull requests to summarize intent, OpenSpec status, verification, and any constrained validation evidence.

#### Scenario: Required gate focuses on product and traceability risk
- **WHEN** the repository CI workflow validates the baseline gate for a product-impacting or otherwise unknown change scope
- **THEN** it SHALL run the shared verification script for F' generate/build, unit test generation/build, `fprime-util check --all`, OpenSpec spec validation, and core traceability checks
- **AND** it SHALL NOT fail the PR solely for agent-link wording or verification-matrix prose drift

## ADDED Requirements

### Requirement: Verification Scope Classification
The delivery workflow SHALL classify each pull request verification scope as `full` or `lightweight` from the changed-file set. The classifier SHALL use a whitelist strategy: only explicitly listed documentation and governance paths may use `lightweight`; empty, unknown, product, build, workflow, script, or active OpenSpec change-workspace paths SHALL use `full`.

#### Scenario: Documentation and governance-only changes use lightweight verification
- **WHEN** a PR changes only files within the lightweight-eligible set such as documentation prose, governance specs, archived changes, test records, reconciliation surfaces, or agent skill Markdown
- **THEN** the CI gate SHALL skip F' generate/build, unit test generation/build, and `fprime-util check --all`
- **AND** it SHALL still run `check_repo_consistency.py`, `check_component_test_baseline.py`, `check_legacy_zmq_retired.py`, and `openspec validate --specs`

#### Scenario: Unknown or product files trigger full verification
- **WHEN** a PR changes any file outside the lightweight-eligible set, including source code, build configuration, CI workflow files, scripts, active OpenSpec change workspaces, or unknown new paths
- **THEN** the CI gate SHALL run the complete verification gate including F' generate/build, unit test generation/build, `fprime-util check --all`, and all static governance checks

#### Scenario: Required check remains present
- **WHEN** a PR qualifies for lightweight verification
- **THEN** the workflow SHALL still start the required `baseline-gate` job and complete it successfully only after the lightweight static checks pass
- **AND** the workflow SHALL NOT use workflow-level path filters to skip the required check
