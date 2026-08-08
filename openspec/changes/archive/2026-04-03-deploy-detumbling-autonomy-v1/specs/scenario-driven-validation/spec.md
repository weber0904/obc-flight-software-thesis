## ADDED Requirements

### Requirement: Deployment-Style Detumbling Autonomy Scenario Replay
The scenario-driven validation capability SHALL support a hosted replay case in which scenario-seeded deployment-style initial angular rates trigger the mission-autonomy detumbling response and converge below the mission detumble threshold.

#### Scenario: Replay case triggers detumbling autonomy
- **WHEN** a hosted replay initializes the ADCS simulator with deployment-style high angular rates
- **THEN** the validation flow SHALL be able to demonstrate the `DETUMBLE` autonomy command and subsequent rate-norm convergence below the mission threshold
