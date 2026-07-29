## MODIFIED Requirements

### Requirement: Reliable transfer remains bounded to current baseline

The comm subsystem SHALL keep reliable transfer bounded to the current official
HK `.fdp` whole-file family and to exactly two current formal paths:

- default S-band node-`5`
- explicit-switched `uhf-primary-after-failover` node-`6`

#### Scenario: UHF reliable transfer stays explicit-switch-only

- **WHEN** `uhf-reliable-transfer-v1` is cited
- **THEN** it SHALL mean reliable transfer on the current official HK `.fdp`
  family only after an explicit switch onto `uhf-primary-after-failover`
- **AND** it SHALL NOT be interpreted as `uhf-backup` reliable transfer or as
  automatic failover-to-UHF reliable transfer

#### Scenario: Active transfer contexts do not migrate across path changes

- **WHEN** a reliable transfer is already active and the primary file path
  changes
- **THEN** the in-flight transfer SHALL stay bound to its start-time path and
  target node
- **AND** the role change SHALL abort or fail that transfer boundedly instead
  of migrating it across paths

#### Scenario: Non-claims remain explicit

- **WHEN** `uhf-reliable-transfer-v1` is cited
- **THEN** it SHALL NOT be interpreted as proof of RF closure,
  restart-persistent resume, broad CFDP adoption, one-GDS aggregation,
  one-gateway multiplexing, generic arbitrary-file authority redesign, or
  broader simultaneous dual-link runtime arbitration
