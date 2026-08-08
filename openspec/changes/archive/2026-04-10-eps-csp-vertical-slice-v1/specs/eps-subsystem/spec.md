## MODIFIED Requirements

### Requirement: Shared EPS Host Protocol
The EPS subsystem SHALL define EPS-owned CSP service request and response payloads for the hosted simulator and OBC-side bridge, and those payloads SHALL preserve the formal node `2` contract while using EPS application ports `10` through `13` so libcsp reserved ports `0` through `3` remain available for built-in management services.

#### Scenario: EPS request includes a sequence and returns one status payload
- **WHEN** the hosted bridge requests EPS status over libcsp
- **THEN** the EPS CSP service reply SHALL carry a sequence field and SHALL return the full EPS status in a single response payload

### Requirement: Hosted EPS Simulator
The first EPS CSP vertical slice SHALL provide a hosted simulator executable that serves the EPS status, PDU, config, and reset services as libcsp node `2` over the hosted ZMQHUB-backed internal CSP substrate while maintaining deterministic battery, solar, heater, and 8-channel PDU state.

#### Scenario: PDU command updates simulator state
- **WHEN** a client issues `EPS_SET_PDU` through the hosted internal CSP path
- **THEN** the simulator SHALL update the requested PDU channel and SHALL reflect the new aggregate PDU state in the next status response

## ADDED Requirements

### Requirement: EpsBridge Uses Internal libcsp Transport
`EpsBridge` SHALL use the repository's internal libcsp substrate as its default hosted transport to EPS node `2`, while preserving the public `EPS_*` F' command, telemetry, event, cached-state, and autonomy-facing behavior.

#### Scenario: EPS status is fetched over libcsp
- **WHEN** `EpsBridge` polls EPS status in the hosted profile
- **THEN** it SHALL request status from libcsp node `2`, EPS status port `10`, and apply telemetry only from a valid EPS CSP reply

#### Scenario: EPS PDU command uses libcsp service
- **WHEN** an operator issues `EPS_SET_PDU`
- **THEN** `EpsBridge` SHALL send the request to EPS node `2`, EPS PDU port `11`, and SHALL map the CSP service reply into the existing command response, telemetry, and event behavior

### Requirement: Legacy EPS Direct-ZMQ Path Retires From Active Baseline
The legacy direct ZMQ request/response EPS path SHALL no longer be the active hosted EPS architecture once the EPS CSP vertical slice passes; any retained files SHALL be treated as temporary legacy support only.

#### Scenario: Verification reports the active EPS path
- **WHEN** EPS hosted integration evidence is recorded after this change
- **THEN** it SHALL identify the libcsp EPS path as the active proven path and SHALL NOT describe direct ZMQ request/response as the current EPS baseline
