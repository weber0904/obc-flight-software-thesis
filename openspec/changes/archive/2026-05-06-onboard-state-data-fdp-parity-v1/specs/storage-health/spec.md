## ADDED Requirements

### Requirement: Data Products Root Observability
Storage health SHALL scan the governed `<runtime-root>/data-products/` root alongside HK, persistent, staging, and logs roots.

#### Scenario: Data products root contributes cached state
- **WHEN** a storage scan completes
- **THEN** the cached storage state SHALL include `data-products` existence, scan status, file count, byte count, and error code
- **AND** warning and degraded masks SHALL use bit 4 for the `data-products` root without changing the mask width from `U8`

#### Scenario: Data products root events use stable root kind
- **WHEN** the `data-products` root is missing, cannot be scanned, or exceeds the configured warning threshold
- **THEN** storage-health root events SHALL report root kind `DATA_PRODUCTS`

### Requirement: Observe Only Data Products Policy Fields
Storage health SHALL expose first-slice policy visibility for `data-products` without deleting or cleaning files.

#### Scenario: Quota and retention policy are visible
- **WHEN** the `data-products` root is scanned
- **THEN** its root stats SHALL include quota bytes, watermark bytes, quota status, and retention status
- **AND** quota status SHALL distinguish not configured, ok, over quota, and unavailable when the root scan fails
- **AND** retention status SHALL identify observe-only behavior

#### Scenario: No destructive retention
- **WHEN** the `data-products` root exceeds any configured quota or watermark
- **THEN** the scanner SHALL report status through cached state, telemetry, and events
- **AND** it SHALL NOT delete or rewrite any data-product file
