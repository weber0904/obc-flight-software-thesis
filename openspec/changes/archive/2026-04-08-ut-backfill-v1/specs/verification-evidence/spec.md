## ADDED Requirements

### Requirement: Targeted Automated Test Backfill For Uneven Slices
The repository SHALL backfill additional automated tests when the formal verification matrix identifies a current capability area as materially uneven across L1/L2/L3/L4 and the affected slice exposes a practical logic or contract seam.

#### Scenario: Weak slices receive the smallest durable automated test
- **WHEN** a current capability area is verified mainly by integration tests, hosted probes, or constrained evidence
- **AND** the implementation exposes a stable contract or logic seam
- **THEN** the repository SHALL add the smallest durable automated test that strengthens reviewability without changing flight behavior

#### Scenario: Classic F prime harnesses are optional for later slices
- **WHEN** a later-phase slice does not map cleanly onto the earlier classic F' component tester pattern
- **THEN** the repository MAY use a smaller contract or pure logic test instead of forcing `register_fprime_ut()`
- **AND** the resulting test SHALL still be inventoried and classified in the formal verification matrix

### Requirement: Remaining Exceptions Stay Explicit
The repository SHALL keep the verification matrix explicit when a currently weak area still relies primarily on integration tests, hosted probes, or constrained evidence after a targeted backfill pass.

#### Scenario: No clean unit seam exists yet
- **WHEN** a targeted area still lacks a clean standalone seam after review
- **THEN** the matrix SHALL keep that area listed as an explicit remaining weak spot or constrained path instead of implying balanced coverage
