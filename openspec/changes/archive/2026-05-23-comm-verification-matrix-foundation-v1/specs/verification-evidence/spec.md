## ADDED Requirements

### Requirement: Matrix Foundation Metadata Is Stable

The formal communication verification matrix SHALL record a stable
machine-readable case metadata contract so later follow-on changes can close
cells without redefining summary semantics.

#### Scenario: Required case metadata stays present
- **WHEN** a matrix case finishes with pass, fail, or blocked status
- **THEN** its metadata SHALL include the case id, environment, carrier kind,
  verdict, blocker class when applicable, rerun-safety value, and artifact
  root
- **AND** malformed or incomplete metadata SHALL be treated as a harness bug

### Requirement: Matrix Wrapper Intent Is Reviewable

The matrix evidence SHALL distinguish reused probes, narrowed probes, and
bounded blockers so a green cell is not confused with an unimplemented one.

#### Scenario: Blocked and reused cells are distinguishable
- **WHEN** reviewers inspect the matrix summaries or wrapper metadata
- **THEN** they SHALL be able to tell whether the case reused an existing probe,
  narrowed a larger proof, or stayed blocked awaiting a dedicated harness
