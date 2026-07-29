## ADDED Requirements

### Requirement: Derived Storage Capabilities Reuse The Shared Baseline
The resource-storage baseline SHALL remain the governing source for storage roles, mutable runtime-root boundaries, and constrained-validation terminology even when later dedicated capabilities such as storage observability or archive management are introduced.

#### Scenario: Storage-oriented capability extends the baseline without redefining roots
- **WHEN** a later capability adds storage-oriented behavior such as runtime-root observability or archive indexing
- **THEN** that capability SHALL reuse the shared storage roles from `resource-storage` instead of redefining staging, persistent-data, logs, or governed runtime-root ownership independently
