## MODIFIED Requirements

### Requirement: AdcsBridge Public Behavior
`AdcsBridge` SHALL own the `ADCS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, and SHALL preserve the last valid state when replies are invalid or absent while raising the owned comms or sensor-fault paths.

#### Scenario: Deployed ADCS CSP transport uses owner-managed runtime access
- **WHEN** the deployed `AdcsBridge` binds its owned CSP transport
- **THEN** it SHALL use the topology-owned runtime owner rather than directly binding to the global shared runtime
- **AND** that ownership change SHALL NOT alter the existing `ADCS_*` public contract or scheduled poll-health semantics by itself
