## ADDED Requirements

### Requirement: Node-5 Transport Uplift Keeps The Same Registry Path Identity
The verification-path registry SHALL treat the node-`5` COMM CSP downlink `v2`
uplift as the same maintained hosted and target node-`5` official file/downlink
path identity rather than as a second official path.

#### Scenario: Hosted node-5 registry wording stays on the same official path
- **WHEN** hosted node-`5` official file/downlink evidence is refreshed after
  the COMM CSP downlink `v2` uplift
- **THEN** the registry SHALL keep the same hosted node-`5` official
  file/downlink path identity
- **AND** it SHALL describe the transport change as a node-`5` transport uplift
  below stock `FileDownlink`, not as a new owner chain or parallel official
  path

#### Scenario: Target node-5 registry wording stays on the same official path
- **WHEN** target node-`5` official file/downlink evidence is refreshed after
  the COMM CSP downlink `v2` uplift
- **THEN** the registry SHALL keep the same governed target node-`5` official
  file/downlink path identity
- **AND** it SHALL state that `v2` preserves stock official ownership while
  changing only the node-local transport acceptance and drain behavior

#### Scenario: Registry keeps v2 non-claims explicit
- **WHEN** reviewers inspect the refreshed node-`5` hosted or target
  file/downlink path wording
- **THEN** the registry SHALL state that the uplift does not prove node-`6`
  migration, reliable-transfer widening, payload-family route replacement, or
  end-to-end reliable delivery after node-`5` commit
