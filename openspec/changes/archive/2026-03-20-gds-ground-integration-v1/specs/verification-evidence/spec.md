## MODIFIED Requirements
### Requirement: First-Version Baseline Scenarios

The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, the boot update prepare/verify/activate/confirm-or-rollback sequence, a hosted end-to-end runtime launch, and a hosted GDS-connected launch that demonstrates the integrated software-only OBC stack can start and attach to the documented ground adapter path.

#### Scenario: Hosted GDS evidence is reviewable
- **WHEN** the hosted ground-integration change completes
- **THEN** reviewers SHALL be able to inspect the GDS launch command, the hosted stack launch command, and the observed connection outcome from the repository evidence tree
