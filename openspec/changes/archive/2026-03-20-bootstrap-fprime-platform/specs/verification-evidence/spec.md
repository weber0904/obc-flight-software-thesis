## ADDED Requirements

### Requirement: Bootstrap Gate Evidence
The bootstrap phase SHALL include a minimum verification gate consisting of `fprime-util generate` and `fprime-util build` executed from the generated project virtual environment, and the results SHALL be captured as reviewable evidence in the repository.

#### Scenario: Bootstrap gate passes
- **WHEN** the F' project tree has been populated successfully
- **THEN** the project SHALL run `generate` and `build` from the generated virtual environment and SHALL record the summary outcomes as bootstrap evidence

### Requirement: Bootstrap Evidence Storage
Bootstrap-phase evidence SHALL be stored under a project documentation path that is discoverable by later implementation changes and review workflows.

#### Scenario: Later changes look up the initial baseline evidence
- **WHEN** a later capability change needs to reference the initial platform bootstrap result
- **THEN** it SHALL be able to find the bootstrap evidence from the repository documentation tree
