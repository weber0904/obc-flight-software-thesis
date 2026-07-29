## ADDED Requirements

### Requirement: Low-Battery Autonomy Evidence
The verification evidence tree SHALL record the commands, scenario inputs, observed mode transition, observed ADCS pointing command behavior, and final verdict for the first low-battery autonomy slice.

#### Scenario: Low-battery autonomy slice is reviewable
- **WHEN** the low-battery autonomy change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the replayed low-battery condition, and the observed `LOW_POWER` plus ADCS pointing outcome from the repository evidence tree
