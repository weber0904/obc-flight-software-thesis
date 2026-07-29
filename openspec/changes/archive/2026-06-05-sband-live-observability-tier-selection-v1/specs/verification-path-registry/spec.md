## ADDED Requirements

### Requirement: Hosted Node-5 Tier-Selection Proof Registers Curated Summary And Explicit Detail Readback

The verification-path registry SHALL register the hosted node-`5`
observability proof as a curated-summary path, not merely an auth-gated
access-gating path.

#### Scenario: Registry states the hosted curated-summary boundary
- **WHEN** the hosted node-`5` tier-selection proof passes
- **THEN** the registry SHALL state that the path proves post-auth curated live
  summary on node `5`
- **AND** it SHALL state that representative detailed family telemetry remains
  absent from ambient live visibility until explicit `GET_*` readback opens it
  in bounded form.

### Requirement: Target Node-5 Tier-Selection Proof Registers Curated Summary And Explicit Detail Readback

The verification-path registry SHALL register the target node-`5`
observability proof with the same curated-summary versus bounded-detail
distinction.

#### Scenario: Registry states the target curated-summary boundary
- **WHEN** the target node-`5` tier-selection proof passes
- **THEN** the registry SHALL state that the path proves post-auth curated live
  summary on the maintained target node-`5` path
- **AND** it SHALL state that representative detailed family telemetry remains
  absent from ambient live visibility until explicit `GET_*` readback opens it
  in bounded form.

#### Scenario: Registry keeps residual live chatter out of current baseline claims
- **WHEN** reviewers inspect the updated hosted or target node-`5` entry
- **THEN** the entry SHALL keep any remaining transport, queue, driver, or
  other residual runtime chatter outside the current node-`5` operator
  baseline by marking it `non-baseline live`
- **AND** it SHALL NOT widen the proof into generic telemetry filtering, UHF
  command-paced observability, or a new command plane claim.
