## MODIFIED Requirements

### Requirement: Evidence Capture
The project SHALL preserve reviewable evidence for both automated and manual testing, including test identity, execution time, result summary, commands or steps used, expected outcomes, observed outcomes, and the final verdict.

#### Scenario: Manual GDS validation
- **WHEN** a behavior is validated by manual GDS interaction
- **THEN** the test record SHALL capture the commands or operations used and the summary of visible telemetry or event outcomes

## ADDED Requirements

### Requirement: Evidence Names The Exact Path Under Test
Each evidence record SHALL name the exact validation path under test, including the transport or interface layering that the verdict applies to, instead of describing only the feature name.

#### Scenario: Evidence states the validated path
- **WHEN** a change records manual or integration evidence
- **THEN** the evidence SHALL identify the path being proven, such as direct `OBC -> GDS` TCP connectivity, `fprime-cli -> GDS` command dispatch, or transparent UART framing

### Requirement: Evidence Distinguishes Adjacent Paths
When two neighboring paths could be confused with one another, the evidence SHALL explicitly say which neighboring paths are not covered by the current verdict.

#### Scenario: GDS command-path evidence distinguishes TCP adapter coverage
- **WHEN** a change validates a `fprime-cli -> GDS` command path
- **THEN** the evidence SHALL state whether direct `OBC -> GDS` TCP adapter connectivity is merely a prerequisite or is also being revalidated in the same record

### Requirement: Evidence Cites The Governing Baseline
If a change reuses an already proven validation path, its evidence SHALL cite the governing repository baseline or verification-path registry entry instead of assuming the reader will infer that reuse from framework conventions alone.

#### Scenario: Change reuses an established path
- **WHEN** a later change depends on a path already proven by an earlier archived slice
- **THEN** the evidence SHALL cite the archived evidence or verification-path registry entry that establishes that path before building new conclusions on top of it

### Requirement: Debugging Lessons Preserve Path-Selection Mistakes
When a verification failure was caused by choosing the wrong path or by conflating adjacent paths, the repository debugging-lessons document SHALL record that mistake and the corrected path-selection rule.

#### Scenario: Wrong-path lesson becomes repository guidance
- **WHEN** a verification effort discovers that a failing result was caused by using the wrong command or transport path
- **THEN** the debugging-lessons record SHALL describe the mistaken assumption, the corrected path distinction, and the future rule that prevents the same error
