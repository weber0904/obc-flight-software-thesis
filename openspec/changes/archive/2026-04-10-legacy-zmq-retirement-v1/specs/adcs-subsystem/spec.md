## MODIFIED Requirements

### Requirement: ADCS CSP Protocol Owns The Active Wire Contract
The ADCS subsystem SHALL use ADCS-owned CSP request/reply payload definitions as the active hosted wire contract, SHALL keep ADCS runtime state DTOs in ADCS-owned code, and SHALL NOT depend on a shared `simulators/common/protocol.h` wire authority or direct-ZMQ compatibility transport.

#### Scenario: ADCS hosted path has no direct-ZMQ fallback
- **WHEN** the default ADCS bridge transport is constructed
- **THEN** it SHALL target ADCS CSP node `3` over the internal libcsp runtime
- **AND** it SHALL NOT accept or interpret a direct ZeroMQ endpoint as the ADCS hosted business path
