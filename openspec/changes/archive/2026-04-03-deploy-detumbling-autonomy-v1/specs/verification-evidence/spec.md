## ADDED Requirements

### Requirement: Detumbling Autonomy Evidence
The verification evidence tree SHALL record the commands, scenario inputs, observed detumble trigger, observed ADCS mode behavior, observed rate-norm convergence, and final verdict for the first detumbling autonomy slice.

#### Scenario: Detumbling autonomy slice is reviewable
- **WHEN** the detumbling autonomy change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the replayed high-rate condition, and the observed `DETUMBLE` plus convergence outcome from the repository evidence tree
