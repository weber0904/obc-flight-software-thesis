## ADDED Requirements

### Requirement: EPS Implementation Evidence
The first EPS implementation slice SHALL record its build, bridge-unit-test, and host transport integration results under `evidence/records/eps-subsystem-v1/`.

#### Scenario: EPS evidence is reviewable after implementation
- **WHEN** the EPS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: EPS Hardware Gaps Are Explicit
Any EPS behavior that still depends on unavailable real hardware or physical power characterization SHALL be called out explicitly in the EPS evidence record using the `Blocked-HW` status term.

#### Scenario: Host verification does not cover real EPS hardware
- **WHEN** the EPS change completes with simulator and host-side verification only
- **THEN** the remaining real-hardware validation gap SHALL be labeled `Blocked-HW`
