## MODIFIED Requirements

### Requirement: First-Version Baseline Scenarios

The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, and the boot update prepare/verify/activate/confirm-or-rollback sequence.

#### Scenario: Comm transport evidence captures both software-only replacement paths
- **WHEN** real UART hardware is unavailable for the first comm subsystem slice
- **THEN** the evidence SHALL include both TCP mock and PTY-backed replacement coverage plus any remaining `Blocked-HW` gaps
