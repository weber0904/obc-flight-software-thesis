## ADDED Requirements

### Requirement: Scenario-Driven EPS Replay Inputs
The hosted EPS simulator SHALL accept scenario-driven sunlight and battery state-of-charge inputs from the repository-owned scenario bridge while preserving the existing PDU, heater, and status command behavior.

#### Scenario: Scenario replay updates EPS environment state
- **WHEN** the scenario bridge applies a replay sample with new sunlight or battery state-of-charge values
- **THEN** the hosted EPS simulator SHALL reflect those values in subsequent status responses without removing the existing commandable PDU and heater behavior
