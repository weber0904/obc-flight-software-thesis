## MODIFIED Requirements

### Requirement: First-Version Baseline Scenarios

The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, the boot update prepare/verify/activate/confirm-or-rollback sequence, a hosted end-to-end runtime launch, a hosted GDS-connected launch that demonstrates the integrated software-only OBC stack can start and attach to the documented ground adapter path, a Raspberry Pi native build and integrated stack launch, and a target-side boot metadata restart flow that demonstrates persisted confirm/rollback state on actual target hardware.

#### Scenario: Hosted GDS evidence is reviewable
- **WHEN** the hosted ground-integration change completes
- **THEN** reviewers SHALL be able to inspect the GDS launch command, the hosted stack launch command, and the observed connection outcome from the repository evidence tree

#### Scenario: Raspberry Pi target evidence is reviewable
- **WHEN** the Raspberry Pi target-integration change completes
- **THEN** reviewers SHALL be able to inspect the target sync/build/run commands, the observed target launch outcome, and the boot-state restart evidence from the repository documentation tree

### Requirement: Constrained Validation Status
The project SHALL use `Blocked-HW` only for tests blocked by unavailable hardware, cables, or devices, SHALL use `Deferred-RPi` only for tests that require the Raspberry Pi target integration phase, and SHALL update previously deferred records once the relevant Raspberry Pi target validation has been executed. Each constrained test SHALL include the original goal, the reason it is constrained, replacement evidence, summary results, and the condition to clear the constraint.

#### Scenario: Hardware-limited comms test
- **WHEN** real UART validation cannot run because the hardware path is unavailable
- **THEN** the record SHALL use `Blocked-HW` and SHALL include PTY, mock, or equivalent replacement evidence

#### Scenario: Raspberry Pi target execution clears a deferred item
- **WHEN** a previously `Deferred-RPi` behavior is exercised on the Raspberry Pi target and reviewable evidence is captured
- **THEN** the related evidence record SHALL no longer describe that behavior as deferred and SHALL either mark it passed or leave only the remaining constrained sub-cases explicit
