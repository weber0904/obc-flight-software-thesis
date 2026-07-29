## ADDED Requirements

### Requirement: Interface Index Records Frozen Future Target-Bearing Dual-Link Boundary

`docs/interfaces.md` SHALL summarize the frozen future target-bearing
simultaneous dual-link boundary without restating it as a currently proven path.

#### Scenario: Interface index records the future path family and verdict split
- **WHEN** reviewers inspect the current COMM boundary notes in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that the frozen future target-bearing
  family uses:
  - default node-`5` primary truth
  - non-quiet node-`6` `uhf-backup` concurrent adjunct
  - explicit switched `uhf-primary-after-failover` UHF command truth
  - quiet node-`6` adjunct rescue
- **AND** they SHALL be able to see that `target-claim` and
  `operator-observability` are separate formal verdicts

#### Scenario: Interface index preserves current non-claim status
- **WHEN** the same summary describes the frozen future boundary
- **THEN** it SHALL state that the boundary is clarified future work rather
  than a currently proven simultaneous target path
- **AND** it SHALL keep non-claims explicit for one-GDS aggregation, one-gateway
  multiplexing, simultaneous full-authority commands on both links, and
  mandatory file/downlink continuity in the main PASS
