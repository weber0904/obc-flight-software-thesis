## ADDED Requirements

### Requirement: Current Operational Visibility Distinguishes Curated Live Summary From Bounded Detailed Readback

The onboard data-products and live-beacon capability SHALL describe current
live operational visibility as curated post-auth live summary plus bounded
explicit detailed readback rather than broad family chatter by default.

#### Scenario: Live summary and detailed readback stay distinct
- **WHEN** the repository describes current operational visibility for onboard
  state families
- **THEN** it SHALL keep beacon as the no-ACK reduced-state broadcast
- **AND** it SHALL describe node-`5` auth-gated scheduled family summary as
  current live visibility
- **AND** it SHALL describe explicit detailed `GET_*` readback as bounded
  operator readback rather than broad ambient live telemetry.

#### Scenario: Residual runtime chatter is not rewritten as baseline operator truth
- **WHEN** reviewers encounter additional runtime live surfaces outside the
  curated summary families
- **THEN** the current documentation SHALL be able to mark them as
  `non-baseline live`
- **AND** it SHALL NOT collapse those residual surfaces back into the current
  operator baseline.
