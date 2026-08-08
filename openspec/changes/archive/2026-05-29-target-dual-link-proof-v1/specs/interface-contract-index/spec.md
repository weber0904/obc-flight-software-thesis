## ADDED Requirements

### Requirement: Interface Index Records The First Implementation-Bearing Target Dual-Link Branch

`docs/interfaces.md` SHALL summarize the first implementation-bearing
target-bearing dual-link proof as an exact proven branch, not only as a frozen
future boundary.

#### Scenario: Reader can see the exact proven branch
- **WHEN** readers inspect `docs/interfaces.md` after the proof lands
- **THEN** they SHALL be able to see:
  - the default node-`5` primary truth
  - the non-quiet node-`6` `uhf-backup` adjunct
  - the explicit switched `uhf-primary-after-failover` non-quiet truth
  - whether quiet rescue was used
  - whether the official run landed as `target-claim=PASS` with
    `operator-observability=PASS` or `DEGRADED`

#### Scenario: Interface index does not overstate unrun branches
- **WHEN** the official run proves only one successful outcome branch
- **THEN** `docs/interfaces.md` SHALL describe only that exact branch as
  proven
- **AND** it SHALL keep any unrun rescue or degraded branch as a non-claim or
  future possibility rather than current proof
