## ADDED Requirements

### Requirement: EPS Simulator Supports External Runtime SoC Control

The hosted and subsystem EPS simulator SHALL support an optional external
runtime SoC control surface that lets repository-owned probes change simulator
state-of-charge while the simulator remains online.

#### Scenario: External immediate SoC set updates subsequent status
- **WHEN** a repo-owned helper sends a valid external SoC control request with
  `transition-sec = 0` or omitted
- **THEN** the simulator SHALL keep serving on the same EPS CSP node
- **AND** subsequent EPS status responses SHALL reflect the requested SoC

#### Scenario: Timed SoC ramp resolves lazily from monotonic time
- **WHEN** a repo-owned helper sends a valid external SoC control request with
  a positive `transition-sec`
- **THEN** the simulator SHALL preserve the current SoC, target SoC, and
  transition window internally
- **AND** later status responses SHALL report the SoC implied by monotonic time
  across that window without requiring a separate always-running ramp thread

#### Scenario: Invalid control request does not perturb simulator service
- **WHEN** the external control surface receives an invalid SoC control request
- **THEN** the simulator SHALL reject that request
- **AND** it SHALL keep the prior valid SoC trajectory unchanged
- **AND** it SHALL continue serving EPS CSP requests

#### Scenario: Control surface remains separate from OBC command ownership
- **WHEN** reviewers inspect the active runtime SoC stimulation path
- **THEN** the SoC control surface SHALL be simulator-owned and externally
  injected
- **AND** the OBC SHALL NOT gain a new public EPS command for directly setting
  simulator SoC
