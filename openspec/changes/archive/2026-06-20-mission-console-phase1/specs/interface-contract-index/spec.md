## ADDED Requirements

### Requirement: Interface Index Records The Mission Console Companion Reference

`docs/interfaces.md` and adjacent current-baseline documentation SHALL treat
`docs/roadmap/mission-console-phase1-handoff.md` as the companion reference for
Mission Console Phase 1 implementation assumptions and authority reuse.

#### Scenario: Reviewers can find the Mission Console planning reference
- **WHEN** reviewers inspect the Mission Console or manual-ops related sections
  of the interface index and current roadmap docs
- **THEN** they SHALL be able to see that
  `mission-console-phase1-handoff.md` is the companion planning reference
- **AND** the docs SHALL NOT silently describe a conflicting baseline or helper
  dependency order.

### Requirement: Interface Index Records Mission Console Readback Categories

`docs/interfaces.md` SHALL record the Mission Console distinction between
surface/lifecycle truth, keep-live summary, operator-facing transition events,
and explicit detailed readback.

#### Scenario: Reviewers can audit Mission Console data tiers
- **WHEN** reviewers inspect the Mission Console-related interface sections
- **THEN** they SHALL be able to see which current surfaces belong to dashboard
  summary, transition/event review, and explicit detailed readback
- **AND** the wording SHALL keep bounded `GET_*` or status-driven readback
  distinct from broad ambient live packet chatter.

### Requirement: Interface Index Records Packet-Lab Diagnostic Boundaries

`docs/interfaces.md` SHALL describe the Mission Console packet-lab surface as a
bounded diagnostic/demo feature with parsed packet summaries and explicit
non-claims.

#### Scenario: Packet-lab docs stay bounded and reviewable
- **WHEN** reviewers inspect the packet-lab-related interface wording
- **THEN** the docs SHALL identify the supported replay/tamper cases, the
  bounded field-summary intent, and the expected evidence model for failure
  observation
- **AND** they SHALL NOT describe the surface as a generic fuzzing framework,
  flight operator plane, or alternate secure authority.
