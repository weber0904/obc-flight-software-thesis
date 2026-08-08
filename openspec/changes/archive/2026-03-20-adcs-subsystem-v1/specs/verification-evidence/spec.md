## ADDED Requirements

### Requirement: ADCS Implementation Evidence
The first ADCS implementation slice SHALL record its build, bridge-unit-test, and host transport integration results under `evidence/records/adcs-subsystem-v1/`.

#### Scenario: ADCS evidence is reviewable after implementation
- **WHEN** the ADCS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: ADCS Hardware Gaps Are Explicit
Any ADCS behavior that still depends on unavailable real hardware, physical closed-loop validation, or real calibration equipment SHALL be called out explicitly in the ADCS evidence record using the `Blocked-HW` status term.

#### Scenario: Host verification does not cover real ADCS hardware
- **WHEN** the ADCS change completes with simulator and host-side verification only
- **THEN** the remaining real-hardware validation gap SHALL be labeled `Blocked-HW`
