## MODIFIED Requirements

### Requirement: Gateway Relay Boundary Stays Raw During Reliable Transfer

The ground TT&C gateway capability SHALL keep `ground_ttc_gateway` as a raw
relay only when the repository proves bounded reliable transfer on either the
default node-`5` or explicit-switched node-`6` path.

#### Scenario: Hosted and target proofs keep retry ownership out of the gateway

- **WHEN** reviewers inspect the bounded UHF reliable-transfer proof
- **THEN** they SHALL see `ground_ttc_gateway` cited only as the framed-byte
  relay between stock GDS and the current southbound path
- **AND** they SHALL NOT see gateway-owned resend, ARQ, NACK, or CFDP policy
  claimed for either the hosted or target/lab proof

#### Scenario: Node-5 and node-6 reliable slices stay path-distinct

- **WHEN** the same change cites both reliable-transfer paths
- **THEN** it SHALL still identify whether the proof traversed the default
  S-band node-`5` or the explicit-switched UHF node-`6` relay boundary
- **AND** it SHALL NOT collapse those adjacent relay paths into a generic
  simultaneous multi-link gateway claim
