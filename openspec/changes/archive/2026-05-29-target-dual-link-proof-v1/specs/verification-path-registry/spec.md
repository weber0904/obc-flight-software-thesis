## ADDED Requirements

### Requirement: Registry Adds A Distinct Physical Target-Bearing Dual-Link Entry Only After Fresh Proof

The verification-path registry SHALL add a distinct physical target-bearing
dual-link path only when fresh reviewed evidence closes the first
implementation-bearing proof on the target CAN + UHF UART topology.

#### Scenario: Registry entry names the exact physical proof path
- **WHEN** the official target-bearing dual-link proof is finalized
- **THEN** the registry SHALL identify the path as the physical target CAN +
  UHF UART family built from:
  - default target node-`5` primary truth
  - non-quiet node-`6` `uhf-backup` adjunct
  - explicit switched `uhf-primary-after-failover` non-quiet truth
- **AND** it SHALL keep target TCP comparator and direct-target command-path
  evidence as adjacent citations rather than part of the new path itself

#### Scenario: Registry entry is branch-scoped
- **WHEN** the official run proves only one successful outcome branch
- **THEN** the registry entry SHALL describe only that exact branch
- **AND** it SHALL name whether the governing evidence landed as
  `operator-observability=PASS` or `DEGRADED`
- **AND** it SHALL name quiet rescue only if that rescue was actually used in
  the official successful run

#### Scenario: Registry entry keeps residual non-claims explicit
- **WHEN** the new target-bearing entry is registered
- **THEN** it SHALL keep explicit non-claims for symmetric dual-authority
  commands, one-GDS heterogeneous upstream handling, one-gateway multiplexing,
  RF closure, and generic clean non-quiet operator observability beyond the
  exact branch proven
