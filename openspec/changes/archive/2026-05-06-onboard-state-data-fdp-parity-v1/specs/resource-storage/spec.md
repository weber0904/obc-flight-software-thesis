## ADDED Requirements

### Requirement: Data Products Runtime Root Policy Surface
The governed runtime-root storage model SHALL include `<runtime-root>/data-products/` as the official F' data-product file and catalog-state location with observe-only quota visibility.

#### Scenario: Data products root is governed storage
- **WHEN** the OBC topology configures official F' data products
- **THEN** `DpWriter` output and `DpCatalog` state SHALL remain under `<runtime-root>/data-products/`
- **AND** storage-health SHALL scan that same root rather than a hard-coded `runtime/data-products` path

#### Scenario: Quota configuration is non-destructive
- **WHEN** `OBC_DATA_PRODUCTS_QUOTA_BYTES` is set
- **THEN** storage-health SHALL expose the configured quota and over-quota status for `<runtime-root>/data-products/`
- **AND** the system SHALL NOT delete data-product files as part of this change
