## ADDED Requirements

### Requirement: Official Data Product Runtime Root
The resource-storage baseline SHALL reserve a governed mutable runtime-root path for official F' data product files and catalog state, and that root SHALL remain outside immutable installed release payloads.

#### Scenario: Data product files live under mutable runtime root
- **WHEN** the OBC runtime is launched with a governed runtime root
- **THEN** official F' data product files SHALL be written under a mutable `data-products` runtime-root directory
- **AND** those files SHALL NOT be written into an immutable installed release payload

#### Scenario: Data product catalog state lives with data products
- **WHEN** the official F' data product catalog persists transmission state
- **THEN** the catalog state file SHALL live under the governed data-products runtime-root area

### Requirement: Storage Roles Stay Distinct
Official HK trend data products SHALL remain distinct from the existing housekeeping archive ring/index files, persistent boot/update data, staging data, and logs.

#### Scenario: Housekeeping archive remains separate
- **WHEN** the HK trend data-product path writes files
- **THEN** it SHALL use the official data-products root
- **AND** it SHALL NOT write into or replace the existing `hk/` ring archive root
