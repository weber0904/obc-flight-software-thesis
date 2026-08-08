## MODIFIED Requirements
### Requirement: First-Version Baseline Scenarios

The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, the boot update prepare/verify/activate/confirm-or-rollback sequence, and a hosted end-to-end runtime launch that demonstrates the integrated software-only OBC stack can be started and operated.

#### Scenario: Hosted runtime evidence is reviewable
- **WHEN** the integrated software-only OBC runtime change completes
- **THEN** reviewers SHALL be able to inspect the launch command, exercised runtime operations, and outcome summary from the repository evidence tree
