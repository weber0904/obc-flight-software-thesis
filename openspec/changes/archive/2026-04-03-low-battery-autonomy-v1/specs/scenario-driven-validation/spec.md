## ADDED Requirements

### Requirement: Low-Battery Autonomy Scenario Replay
The scenario-driven validation capability SHALL support a hosted replay case in which scenario-driven EPS state-of-charge crosses the low-battery threshold and drives the first mission-autonomy response.

#### Scenario: Replay case triggers the first autonomy behavior
- **WHEN** a hosted replay drives EPS state-of-charge below the low-battery threshold
- **THEN** the validation flow SHALL be able to demonstrate entry into `LOW_POWER` and the first-version ADCS sun-safe pointing command
