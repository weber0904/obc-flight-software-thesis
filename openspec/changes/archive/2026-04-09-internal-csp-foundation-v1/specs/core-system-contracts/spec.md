## MODIFIED Requirements

### Requirement: Core Control Interface
The system SHALL expose the `MODE_*`, `HEALTH_*`, and `CSP_*` core command families and SHALL provide the `SYS_*` and `CSP_*` telemetry required to observe system state, uptime, resource pressure, and real libcsp connectivity.

#### Scenario: Core CSP operation is diagnosable
- **WHEN** an operator issues `CSP_INIT` followed by `CSP_PING`
- **THEN** the deployment SHALL initialize the local libcsp node, attempt the ping over the governed internal CSP substrate, and surface enough telemetry to diagnose the connectivity result

## ADDED Requirements

### Requirement: CspBridge Owns Runtime Foundation
`CspBridge` SHALL own the repository's internal libcsp runtime foundation for the OBC process, including local node initialization, interface binding, diagnostic ping, bounded raw debug send, and runtime counter publication.

#### Scenario: CspBridge initializes hosted node 1
- **WHEN** the hosted OBC runtime starts the governed internal CSP foundation path
- **THEN** `CspBridge` SHALL initialize libcsp node `1`, bind the hosted interface, and publish runtime counters from the real libcsp state

#### Scenario: CspBridge remains diagnostic rather than subsystem business transport
- **WHEN** later subsystem bridges use the internal CSP network for business traffic
- **THEN** `CspBridge` SHALL remain the owner of runtime bring-up and diagnostics while the subsystem-specific business requests stay owned by the corresponding subsystem bridges
