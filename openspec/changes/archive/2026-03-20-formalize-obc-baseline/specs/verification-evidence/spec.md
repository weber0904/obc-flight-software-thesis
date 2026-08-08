## ADDED Requirements

### Requirement: Layered Verification Model
The project SHALL organize verification into L1 unit tests, L2 component tests, L3 integration tests, and L4 system or mission-scenario tests, and each layer SHALL have a distinct purpose rather than replacing another layer.

#### Scenario: Integration behavior requires higher-level evidence
- **WHEN** a change modifies cross-component or simulator behavior
- **THEN** that change SHALL include L3 or L4 evidence in addition to any lower-level tests

### Requirement: Evidence Capture
The project SHALL preserve reviewable evidence for both automated and manual testing, including test identity, execution time, result summary, commands or steps used, expected outcomes, observed outcomes, and the final verdict.

#### Scenario: Manual GDS validation
- **WHEN** a behavior is validated by manual GDS interaction
- **THEN** the test record SHALL capture the commands or operations used and the summary of visible telemetry or event outcomes

### Requirement: Constrained Validation Status
The project SHALL use `Blocked-HW` only for tests blocked by unavailable hardware, cables, or devices, and SHALL use `Deferred-RPi` only for tests that require the Raspberry Pi target integration phase. Each constrained test SHALL include the original goal, the reason it is constrained, replacement evidence, summary results, and the condition to clear the constraint.

#### Scenario: Hardware-limited comms test
- **WHEN** real UART validation cannot run because the hardware path is unavailable
- **THEN** the record SHALL use `Blocked-HW` and SHALL include PTY, mock, or equivalent replacement evidence

### Requirement: First-Version Baseline Scenarios
The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, and the boot update prepare/verify/activate/confirm-or-rollback sequence.

#### Scenario: Bootstrap verification checklist
- **WHEN** the project reviews first-version readiness
- **THEN** those baseline scenarios SHALL be available as the minimum shared verification checklist
