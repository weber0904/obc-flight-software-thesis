## ADDED Requirements

### Requirement: CspBridge Selects The Active Runtime Carrier
`CspBridge` SHALL keep owning libcsp runtime bring-up and diagnostics, and that ownership SHALL include selecting and initializing the configured internal CSP carrier or interface backend for the OBC process.

#### Scenario: CspBridge initializes node 1 through the configured carrier
- **WHEN** the OBC process starts the governed internal CSP runtime
- **THEN** `CspBridge` SHALL initialize libcsp node `1` through the configured carrier/backend, publish runtime counters from the real libcsp state, and keep subsystem business traffic ownership with the subsystem bridges
