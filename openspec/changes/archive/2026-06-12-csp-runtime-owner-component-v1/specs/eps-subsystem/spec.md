## MODIFIED Requirements

### Requirement: EpsBridge Public Behavior
`EpsBridge` SHALL own the `EPS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, SHALL preserve the last valid state on comms failure while raising the EPS comms error path, and SHALL maintain deterministic runtime poll-health state for repeated poll failure handling.

#### Scenario: Deployed EPS CSP transport uses owner-managed runtime access
- **WHEN** the deployed `EpsBridge` binds its owned CSP transport
- **THEN** it SHALL use the topology-owned runtime owner rather than directly binding to the global shared runtime
- **AND** that ownership change SHALL NOT alter the existing `EPS_*` public contract or poll-health semantics by itself
