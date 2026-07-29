## MODIFIED Requirements

### Requirement: Interface Index Uses Explicit Fact Status
`docs/interfaces.md` SHALL classify its current-fact entries as `verified`,
`configured-hosted`, or `TBD`, and it SHALL NOT present unproven target timing
or MTU values as frozen current-baseline facts.

#### Scenario: Hosted-only timing and framing facts stay hosted-scoped
- **WHEN** the index records the current hosted CCSDS frame size or hosted
  rate-group timing profile
- **THEN** it SHALL mark those entries as `configured-hosted` or otherwise make
  the hosted scope explicit
- **AND** it SHALL NOT imply that those values are already frozen target-flight
  truth

#### Scenario: Partially proven target timing facts may be promoted
- **WHEN** the repository has governed evidence for specific target
  service-managed timing facts such as base tick, divisors, nominal rates, or
  zero-slip behavior on a declared workload window
- **THEN** the corresponding interface-index entries MAY be marked `verified`
- **AND** the same section SHALL keep any still-unproven WCET, jitter, or
  broader timing boundary explicit as residual gaps instead of silently
  inferring final target truth

#### Scenario: Unknown target values remain intentionally open
- **WHEN** the repository still lacks governed evidence for a target flightlike
  MTU, WCET bound, jitter limit, or broader timing boundary
- **THEN** the corresponding interface-index entry SHALL remain `TBD` or SHALL
  be called out as an explicit residual gap
- **AND** the document SHALL NOT substitute a guessed or Raspberry-Pi-only
  number as universal target truth
