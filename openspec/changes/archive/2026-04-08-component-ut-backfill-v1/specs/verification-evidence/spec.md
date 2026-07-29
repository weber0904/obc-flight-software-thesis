## ADDED Requirements

### Requirement: Later Real Components Recover The Classic L2 Layer
When later repository work has introduced a real F' component without the repository's classic component harness, the recovery change SHALL add that harness while preserving the existing helper and integration evidence for the same slice.

#### Scenario: Classic harness backfill keeps the helper layer
- **WHEN** the repository backfills classic F' L2 coverage for a component such as `GpsBridge`, `StorageHealthBridge`, or `HousekeepingArchive`
- **THEN** the change SHALL retain the existing helper-level tests and integration evidence instead of replacing them with only the classic harness

#### Scenario: Classic harness backfill is reviewable
- **WHEN** the classic harness backfill change completes
- **THEN** reviewers SHALL be able to inspect the focused component tester commands and outcomes together with the retained helper and integration layers from the repository evidence tree
