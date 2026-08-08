## ADDED Requirements

### Requirement: Shared Verification Gate Script

The repository SHALL provide a repo-local verification gate script that records per-step logs and a summary while running the baseline build, unit/integration test, and OpenSpec validation commands.

#### Scenario: Local developer reuses the same gate as CI
- **WHEN** a developer wants to run the baseline verification flow before or during a change
- **THEN** they SHALL be able to use the same repo-local script that the CI workflow invokes

### Requirement: Evidence Template

The repository SHALL provide a reusable markdown template for change-level evidence records under `evidence/records/templates/`.

#### Scenario: Later change needs a consistent evidence structure
- **WHEN** a later capability change records automated or constrained-validation evidence
- **THEN** it SHALL be able to start from the checked-in template instead of inventing a new record structure

### Requirement: Verification CI Evidence

The first verification CI slice SHALL record the shared script, workflow coverage, and local verification result under `evidence/records/verification-ci-v1/`.

#### Scenario: CI baseline is reviewable after implementation
- **WHEN** the verification CI change completes
- **THEN** reviewers SHALL be able to inspect the workflow entrypoint, local run summary, and remaining CI scope limits from the repository documentation tree
