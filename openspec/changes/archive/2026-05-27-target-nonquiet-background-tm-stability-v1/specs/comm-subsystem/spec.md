## ADDED Requirements

### Requirement: Target Node-6 Non-Quiet Diagnosis Stays Separate From Quiet Proof

The comm subsystem SHALL keep target/lab quiet node-`6` proof distinct from any
later governed non-quiet target node-`6` diagnosis until the repository has
fresh evidence for the non-quiet boundary.

#### Scenario: Quiet proof keeps current meaning
- **WHEN** reviewers inspect the existing target/lab quiet node-`6` proof
- **THEN** that proof SHALL remain bounded to the current quiet-path command,
  file, sequence, failover, and beacon-suppress records
- **AND** it SHALL NOT be reinterpreted as proof of general non-quiet
  background-telemetry stability

#### Scenario: Non-quiet diagnosis keeps owner boundary explicit
- **WHEN** the repository adds a target node-`6` non-quiet diagnosis wrapper
- **THEN** the diagnosis SHALL treat `ground_ttc_gateway` as a raw relay only
- **AND** it SHALL keep runtime or egress ownership on the OBC COMM boundary
  unless the evidence proves a narrower owner

### Requirement: Oracle-Only Node-6 Findings Do Not Force Runtime Change

The comm subsystem SHALL allow target/lab node-`6` residual issues to close at
the proof/oracle boundary when fresh evidence shows correct target-side command
truth with polluted ground observability only.

#### Scenario: Oracle contamination stays a tooling boundary
- **WHEN** quiet control passes
- **AND** non-quiet target node-`6` command truth remains correct in target
  journal or equivalent target-local readback
- **AND** ground event/channel acceptance is the only degraded surface
- **THEN** the repository SHALL treat the issue as oracle-only
- **AND** it SHALL NOT claim that nominal runtime policy or relay ownership
  needed to change just to make the oracle convenient

### Requirement: Mixed Node-6 Findings Freeze The Runtime Residual Narrowly

The comm subsystem SHALL keep mixed target/lab node-`6` findings bounded to the
proven physical coexistence surface until a later change narrows the minimal
runtime owner further.

#### Scenario: Mixed node-6 diagnosis does not blame GDS or gateway
- **WHEN** quiet control passes or partially passes
- **AND** fresh target CAN node-`6` non-quiet evidence shows command ingress
  degrading before target acceptance while byte captures confirm the command
  left ground
- **AND** a supporting comparator indicates the higher-level target command
  policy is still healthy on an adjacent non-CAN southbound
- **THEN** the repository SHALL classify the residual as mixed
- **AND** it SHALL keep `ground_ttc_gateway` as a raw relay rather than the
  chosen runtime owner
- **AND** it SHALL NOT claim a second GDS is the required fix
