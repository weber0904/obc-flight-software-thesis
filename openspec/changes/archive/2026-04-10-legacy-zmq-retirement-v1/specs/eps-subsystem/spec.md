## MODIFIED Requirements

### Requirement: EPS CSP Protocol Owns The Active Wire Contract
The EPS subsystem SHALL use EPS-owned CSP request/reply payload definitions as the active hosted wire contract, SHALL keep EPS runtime state DTOs in EPS-owned code, and SHALL NOT depend on a shared `simulators/common/protocol.h` wire authority or direct-ZMQ compatibility transport.

#### Scenario: EPS hosted path has no direct-ZMQ fallback
- **WHEN** the default EPS bridge transport is constructed
- **THEN** it SHALL target EPS CSP node `2` over the internal libcsp runtime
- **AND** it SHALL NOT accept or interpret a direct ZeroMQ endpoint as the EPS hosted business path
