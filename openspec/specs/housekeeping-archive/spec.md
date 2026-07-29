# housekeeping-archive Specification

## Purpose
Record that the legacy housekeeping archive fallback has been retired from the active baseline in favor of the canonical onboard-state snapshot plus official `.fdp` data-product path.
## Requirements
### Requirement: Housekeeping Archive Runtime Surface Is Retired
The active baseline SHALL NOT instantiate or rely on the legacy housekeeping archive runtime surface.

#### Scenario: Runtime does not create archive artifacts
- **WHEN** the hosted or target runtime starts from the active baseline
- **THEN** it SHALL NOT instantiate `HousekeepingArchive`
- **AND** it SHALL NOT create `runtime/hk`, `hk/index.csv`, or HK slot files as active mission-history artifacts

### Requirement: Housekeeping Archive Command Surface Is Retired
The active baseline SHALL NOT expose the old operator-facing HK archive commands.

#### Scenario: Retired commands are absent from the active command dictionary
- **WHEN** operators inspect the active command dictionary or authority catalog
- **THEN** `housekeepingArchive.HK_CAPTURE_NOW`, `housekeepingArchive.HK_DOWNLINK_INDEX`, and `housekeepingArchive.HK_DOWNLINK_SLOT` SHALL NOT be part of the active operator surface

### Requirement: Official Data Products Replace Archive History
The retired HK archive SHALL NOT be treated as the current history or catalog path for onboard state.

#### Scenario: Official `.fdp` products are the active history path
- **WHEN** the active baseline records or downlinks onboard state history
- **THEN** it SHALL use the canonical state snapshot feeding official `HkTrendRecord` `.fdp` files and `DpCatalog`
- **AND** it SHALL NOT require legacy HK ring semantics for current operator or evidence flows
