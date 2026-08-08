## MODIFIED Requirements

### Requirement: Repository Evidence Governs Validation Path Reuse
Current evidence SHALL distinguish official `.fdp` / `DpCatalog` history-path
proof from historical HK ring behavior and SHALL NOT continue treating HK ring
artifacts as active-baseline evidence once HK retirement is merged.

#### Scenario: Current evidence uses official `.fdp` only
- **WHEN** the current baseline records history-path evidence after HK
  retirement
- **THEN** that evidence SHALL validate official `.fdp` generation,
  `DpCatalog` build/transmit, and received-file checks as the current path
- **AND** it SHALL NOT require `runtime/hk`, `hk/index.csv`, or `HK_*`
  commands as current-baseline proof
