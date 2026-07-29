## MODIFIED Requirements

### Requirement: Periodic Housekeeping Archive Capture
The current active baseline SHALL NOT provide a housekeeping archive component
that periodically snapshots cached/runtime OBC state into ring files while the
runtime is active.

#### Scenario: HK ring is retired from current baseline
- **WHEN** the current hosted or target-facing baseline is configured
- **THEN** it SHALL NOT instantiate `HousekeepingArchive`
- **AND** it SHALL NOT create `runtime/hk`, `hk/index.csv`, or HK slot files as
  part of the current history path

### Requirement: Controlled Housekeeping Archive Downlink
The current active baseline SHALL NOT expose dedicated HK capture or HK
slot/index downlink commands as operator-facing current-history surface.

#### Scenario: HK operator commands are absent
- **WHEN** operators use the current baseline command surface
- **THEN** `housekeepingArchive.HK_CAPTURE_NOW`,
  `housekeepingArchive.HK_DOWNLINK_INDEX`, and
  `housekeepingArchive.HK_DOWNLINK_SLOT` SHALL NOT be part of the active
  baseline
- **AND** official history retrieval SHALL instead use cadence-driven `.fdp`
  generation plus `DpCatalog` catalog/downlink commands
