## ADDED Requirements

### Requirement: Future Target-Bearing Dual-Link Claim Stays Above Hosted Layers And One-Southbound Gateway

The future target-bearing simultaneous dual-link claim SHALL remain separate
from the current hosted layer-1 per-band stock baseline, the hosted layer-2
orchestration owner, and the one-southbound-per-process `ground_ttc_gateway`
boundary.

#### Scenario: Hosted layer-1 and layer-2 remain adjacent prerequisites only
- **WHEN** the repository describes the future target-bearing dual-link claim
- **THEN** it SHALL treat the hosted per-band stock stacks and the hosted
  orchestration owner as adjacent prerequisite layers only
- **AND** it SHALL NOT restate hosted layer-1 or hosted layer-2 proof as the
  target-bearing simultaneous claim itself

#### Scenario: Future target claim does not imply one-GDS or one-gateway closure
- **WHEN** reviewers inspect the same future target-bearing claim boundary
- **THEN** they SHALL see that one stock `fprime-gds` heterogeneous upstream
  aggregation and one `ground_ttc_gateway` simultaneous S-band/UHF multiplexing
  remain explicit non-claims
- **AND** they SHALL NOT treat a future target-bearing claim as proof that the
  current gateway or stock-GDS boundary already solved those ground-software
  problems
