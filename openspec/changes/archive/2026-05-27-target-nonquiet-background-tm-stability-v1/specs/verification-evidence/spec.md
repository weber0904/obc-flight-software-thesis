## ADDED Requirements

### Requirement: Target Node-6 Non-Quiet Diagnosis Evidence Is Multi-Oracle

The verification evidence tree SHALL record reviewable target node-`6`
non-quiet diagnosis evidence that keeps target truth, ground truth, and byte
transport observations distinct.

#### Scenario: Evidence records quiet control and non-quiet case together
- **WHEN** `target-nonquiet-background-tm-stability-v1` records target
  diagnosis evidence
- **THEN** it SHALL include a quiet target CAN node-`6` control result
- **AND** it SHALL include at least one non-quiet target CAN node-`6` case
- **AND** it SHALL keep those two verdicts adjacent instead of folding them
  into one generic node-`6` claim

#### Scenario: Evidence records all diagnosis truth surfaces
- **WHEN** the non-quiet diagnosis runs
- **THEN** the evidence SHALL record:
  - target journal observations
  - ground `fprime-cli events` observations
  - ground `fprime-cli channels` observations
  - gateway byte-capture artifacts
  - node-`6` beacon/debug capture artifacts when used

#### Scenario: Evidence classifies the residual honestly
- **WHEN** the diagnosis evidence is finalized
- **THEN** it SHALL state whether the residual issue is `oracle`, `runtime`, or
  `mixed`
- **AND** it SHALL name the chosen isolation boundary
- **AND** it SHALL state what was changed
- **AND** it SHALL keep any remaining non-quiet node-`6` non-claims explicit

#### Scenario: Evidence records a degraded oracle case when present
- **WHEN** a ground-only oracle would misclassify or fail the target node-`6`
  non-quiet case
- **THEN** the evidence SHALL record that degraded oracle outcome explicitly
- **AND** it SHALL distinguish that from target-side command truth
