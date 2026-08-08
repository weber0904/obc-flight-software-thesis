## ADDED Requirements

### Requirement: GPS Hosted Validation Evidence
The verification evidence tree SHALL record the source mode, representative supported GPS sentences, malformed or no-fix inputs, observed telemetry or event behavior, and final verdict for the first GPS hosted validation slice.

#### Scenario: GPS hosted slice is reviewable
- **WHEN** the first GPS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the fake or replay GPS inputs used, and the observed GPS cached-state outcomes from the repository evidence tree

### Requirement: GPS Hardware Scope Boundary Remains Explicit
The first GPS evidence SHALL explicitly distinguish hosted fake/replay validation from any still-unimplemented Raspberry Pi UART hardware bring-up or live-sky reception validation.

#### Scenario: Hosted GPS validation does not imply target hardware integration
- **WHEN** the first GPS hosted validation passes
- **THEN** the evidence SHALL mark only the fake or replay-backed hosted GPS path as passed and SHALL keep Raspberry Pi UART wiring, live receiver integration, and live-fix validation explicit as future scope or `Blocked-HW`, whichever applies
