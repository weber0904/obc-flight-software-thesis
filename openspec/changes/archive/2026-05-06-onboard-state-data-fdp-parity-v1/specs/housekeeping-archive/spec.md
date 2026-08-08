## ADDED Requirements

### Requirement: Transitional HK Ring Storage Schema
The housekeeping archive SHALL remain available as a bounded transitional fallback and SHALL record the expanded storage-health root schema when its binary record shape changes.

#### Scenario: Archive stores fifth root
- **WHEN** a housekeeping archive record is serialized after data-products root observability is added
- **THEN** the record SHALL include storage-health data for HK, persistent, staging, logs, and data-products roots
- **AND** each root entry SHALL include existence, scan status, file count, bytes, error code, quota bytes, watermark bytes, quota status, and retention status

#### Scenario: Archive version increments
- **WHEN** the archive binary record layout changes to include the fifth storage root and policy fields
- **THEN** the repository-owned archive file header SHALL publish a newer file-format version than the current v2 layout
