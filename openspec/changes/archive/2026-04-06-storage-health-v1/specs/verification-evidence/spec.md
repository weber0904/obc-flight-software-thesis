## ADDED Requirements

### Requirement: Storage Health Hosted Validation Evidence
The verification evidence tree SHALL record the runtime roots used, the representative synthetic root contents, the configured warning condition, the observed storage telemetry or event behavior, and the final verdict for the first hosted storage health slice.

#### Scenario: Storage health hosted slice is reviewable
- **WHEN** the first storage health change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the synthetic runtime-root contents, and the observed storage cached-state outcomes from the repository evidence tree

### Requirement: Storage Hardware Scope Boundary Remains Explicit
The first storage health evidence SHALL explicitly distinguish hosted governed-root validation from any still-unimplemented Raspberry Pi target disk-health or retention-policy validation.

#### Scenario: Hosted storage validation does not imply target disk coverage
- **WHEN** the first hosted storage health validation passes
- **THEN** the evidence SHALL mark only the hosted governed-root storage path as passed and SHALL keep Raspberry Pi target disk behavior, cleanup policy, and retention policy explicit as future scope or `Deferred-RPi`, whichever applies
