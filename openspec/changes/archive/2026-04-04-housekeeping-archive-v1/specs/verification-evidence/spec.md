## ADDED Requirements

### Requirement: Housekeeping Archive Evidence
The verification evidence tree SHALL record the capture cadence used, the archive-slot limits, the observed rotation behavior, the generated index contents, the downlink commands exercised, and the final verdict for the first housekeeping archive slice.

#### Scenario: Housekeeping archive slice is reviewable
- **WHEN** the housekeeping archive change completes
- **THEN** reviewers SHALL be able to inspect hosted validation steps covering archive capture, slot rotation, index generation, and commanded index or slot downlink from the repository evidence tree
