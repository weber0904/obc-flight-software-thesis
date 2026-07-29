## ADDED Requirements

### Requirement: Target Dual-Link Proof Keeps Gateway Surfaces In The Operator Verdict Only

The ground TT&C gateway capability SHALL keep gateway-owned artifacts for the
first target-bearing dual-link proof limited to operator observability support.

#### Scenario: Gateway artifacts do not override target claim truth
- **WHEN** the first implementation-bearing target dual-link proof records
  gateway captures, ground events, or ground channels
- **THEN** those surfaces SHALL contribute only to the
  `operator-observability` verdict
- **AND** they SHALL NOT overturn passing target-side command truth by
  themselves

#### Scenario: Gateway boundary keeps its non-claims explicit
- **WHEN** the same proof is documented in current docs or evidence
- **THEN** it SHALL keep explicit non-claims for one-GDS heterogeneous
  upstream handling and one-gateway simultaneous S-band/UHF multiplexing
- **AND** it SHALL NOT restate the target-bearing proof as new gateway-owned
  policy or ownership
