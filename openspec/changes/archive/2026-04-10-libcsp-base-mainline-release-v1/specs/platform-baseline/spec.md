## ADDED Requirements

### Requirement: Libcsp Integration Base Is Released Through A Mainline Gate
The repository SHALL treat the libcsp integration base as a formal release candidate before it becomes the future mainline development baseline, and that release candidate SHALL pass the shared verification gate plus focused libcsp guardrails before merge to `main`.

#### Scenario: Libcsp base can be reviewed before mainline merge
- **WHEN** the libcsp integration branch is proposed for merge to `main`
- **THEN** reviewers SHALL be able to inspect a release-readiness evidence record that cites the completed CSP foundation, EPS CSP, ADCS CSP, and legacy-ZMQ retirement slices
