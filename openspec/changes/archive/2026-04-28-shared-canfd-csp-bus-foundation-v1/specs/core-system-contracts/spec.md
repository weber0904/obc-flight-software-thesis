## MODIFIED Requirements

### Requirement: CspBridge Selects The Active Runtime Carrier
`CspBridge` SHALL keep owning libcsp runtime bring-up and diagnostics, and that ownership SHALL include selecting and initializing the configured internal CSP carrier or interface backend for the OBC process.

#### Scenario: Linux OBC runtime binds through SocketCAN
- **WHEN** the OBC process runs with `CSP_TRANSPORT=socketcan`
- **THEN** `CspBridge` SHALL initialize node `1` through the governed SocketCAN backend, SHALL require an explicit `CSP_CAN_DEVICE`, and SHALL continue to keep subsystem business traffic ownership with the subsystem bridges

#### Scenario: SocketCAN init fails fast on invalid interface state
- **WHEN** a later change or operator points the OBC runtime at a missing, non-CAN, or down Linux network device
- **THEN** the runtime SHALL fail before later CSP traffic attempts and SHALL not silently degrade into an ambiguous ping or request timeout
