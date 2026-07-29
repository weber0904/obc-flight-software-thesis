## MODIFIED Requirements

### Requirement: Bounded First-Version Root Scope
The current storage health slice SHALL observe only the governed runtime roots
for persistent data, staging data, logs, and official data products, and it
SHALL report those roots separately rather than as one aggregated filesystem
total.

#### Scenario: Scan reports four-root statistics
- **WHEN** a storage scan completes successfully
- **THEN** the cached storage health state SHALL distinguish
  `persistent-data`, `staging`, `logs`, and `data-products` root statistics
  individually
- **AND** it SHALL NOT expose a retired `hk` root in the active baseline

### Requirement: Data Products Root Observability
Storage health SHALL scan the governed `<runtime-root>/data-products/` root
alongside persistent, staging, and logs roots.

#### Scenario: Data products root contributes cached state
- **WHEN** a storage scan completes
- **THEN** the cached storage state SHALL include `data-products` existence,
  scan status, file count, byte count, and error code
- **AND** warning and degraded masks SHALL use the re-based four-root numbering
  for `persistent-data`, `staging`, `logs`, and `data-products`

#### Scenario: Data products root events use stable root kind
- **WHEN** the `data-products` root is missing, cannot be scanned, or exceeds
  the configured warning threshold
- **THEN** storage-health root events SHALL report root kind `DATA_PRODUCTS`
