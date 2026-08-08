## ADDED Requirements

### Requirement: Quiet And Non-Quiet Target Node-6 Boundaries Stay Distinct

The verification-path registry SHALL keep target/lab quiet node-`6` proof and
target/lab non-quiet node-`6` diagnosis as separate boundaries unless fresh
reviewed evidence proves a reusable non-quiet path.

#### Scenario: Registry does not widen quiet node-6 truth by adjacency
- **WHEN** reviewers inspect the existing target/lab quiet node-`6` entries
- **THEN** those entries SHALL continue to state that they do not prove general
  non-quiet serial stability under background telemetry

#### Scenario: Diagnosis evidence does not automatically become a reusable path
- **WHEN** `target-nonquiet-background-tm-stability-v1` records only an
  oracle-only, mixed, or other partial-closure result
- **THEN** the registry SHALL keep that result in the evidence tree without
  promoting it to a reusable non-quiet node-`6` validation path

#### Scenario: Registry promotion requires explicit non-quiet proof
- **WHEN** the change produces reviewed evidence that a target/lab non-quiet
  node-`6` boundary is reusable
- **THEN** the registry SHALL identify the exact proven path, oracle boundary,
  and residual exclusions explicitly
- **AND** it SHALL keep quiet-path and debug-only side-channel evidence distinct
