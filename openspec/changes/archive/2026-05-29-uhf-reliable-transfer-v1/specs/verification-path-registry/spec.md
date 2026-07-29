## ADDED Requirements

### Requirement: Reliable-transfer Node-6 Boundaries Are Registered Adjacent To Existing Node-6 Paths

The verification-path registry SHALL register bounded UHF reliable-transfer
proofs as distinct reliable-transfer boundaries adjacent to existing hosted and
target/lab node-`6` paths instead of widening those earlier paths by
implication.

#### Scenario: Hosted switched-UHF reliable transfer is registered separately

- **WHEN** hosted switched-UHF reliable-transfer evidence passes
- **THEN** the registry SHALL identify the path as one shared hosted OBC
  runtime with distinct stock S-band and UHF ground stacks, explicit switch to
  `uhf-primary-after-failover`, and node-`6` reliable-transfer receiver output
- **AND** it SHALL keep existing hosted node-`6` file/downlink and packet-quiet
  entries as separate adjacent boundaries

#### Scenario: Target quiet switched node-6 reliable transfer is registered separately

- **WHEN** target/lab quiet switched node-`6` reliable-transfer evidence passes
- **THEN** the registry SHALL identify the path as the explicit-switched quiet
  node-`6` reliable-transfer boundary
- **AND** it SHALL state that quiet mode remained probe-owned and was removed
  after the proof
- **AND** it SHALL keep target quiet command/file baseline, suppress/runtime,
  and non-quiet diagnosis entries separate from this reliable-transfer entry

#### Scenario: Registry keeps the new node-6 reliable slice bounded

- **WHEN** reviewers inspect either new node-`6` reliable-transfer entry
- **THEN** the entry SHALL keep `uhf-backup` reliable transfer, RF closure,
  restart-persistent resume, broad CFDP, one-GDS aggregation, one-gateway
  multiplexing, and generic simultaneous closure outside the registered path
