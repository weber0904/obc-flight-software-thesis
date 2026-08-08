## ADDED Requirements

### Requirement: Shared EPS Host Protocol
The EPS subsystem SHALL define a project-local shared request/response protocol for the hosted simulator and the OBC-side bridge, and that protocol SHALL preserve the formal node `2` and service port `1` through `4` contract established by the main `eps-subsystem` specification.

#### Scenario: EPS request includes a sequence and returns one status payload
- **WHEN** the hosted bridge requests EPS status
- **THEN** the protocol SHALL carry a sequence field and SHALL return the full EPS status in a single response payload

### Requirement: Hosted EPS Simulator
The first EPS implementation slice SHALL provide a hosted simulator executable that serves the EPS status, PDU, config, and reset services over ZMQ request/response transport and maintains deterministic battery, solar, heater, and 8-channel PDU state.

#### Scenario: PDU command updates simulator state
- **WHEN** a client issues `EPS_SET_PDU`
- **THEN** the simulator SHALL update the requested PDU channel and SHALL reflect the new aggregate PDU state in the next status response

### Requirement: EpsBridge Public Behavior
`EpsBridge` SHALL own the `EPS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, and SHALL preserve the last valid state on comms failure while raising the EPS comms error path.

#### Scenario: EPS timeout preserves last valid state
- **WHEN** `EpsBridge` fails to receive a valid simulator response within the configured timeout
- **THEN** it SHALL emit the comms error behavior and SHALL NOT overwrite EPS telemetry with arbitrary replacement values

### Requirement: EPS Threshold Event Mapping
`EpsBridge` SHALL map simulator status into the subsystem-owned alarm events, including low battery, critical battery, over-temperature, and PDU-change reporting.

#### Scenario: Low battery status raises the subsystem event
- **WHEN** a valid EPS status reports state-of-charge below `20%`
- **THEN** `EpsBridge` SHALL raise `EPS_LOW_BATTERY`
