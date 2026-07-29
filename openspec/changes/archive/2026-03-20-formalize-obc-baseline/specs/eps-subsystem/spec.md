## ADDED Requirements

### Requirement: EPS Simulator Scope
The EPS subsystem SHALL model battery, solar, and 8-channel PDU state, SHALL operate as CSP node `2`, and SHALL expose status, PDU, config, and reset services over CSP ports `1` through `4`.

#### Scenario: EPS status fits a single response
- **WHEN** `EpsBridge` requests EPS status
- **THEN** the simulator SHALL return the required summary state in a single CSP response payload that includes a sequence field

### Requirement: EPS Public Contract
The EPS subsystem SHALL own the `EPS_*` command, telemetry, and event families, including `EPS_GET_STATUS`, `EPS_SET_PDU`, `EPS_SET_HEATER`, `EPS_RESET`, the required telemetry fields for battery/solar/PDU state, and the low-battery, over-temperature, PDU-change, and comm-error events.

#### Scenario: EPS command updates observable state
- **WHEN** an operator issues `EPS_SET_PDU`
- **THEN** the subsystem SHALL update the PDU state and surface the change through the owned telemetry and event contracts

### Requirement: EPS Bridge Fallback Behavior
`EpsBridge` SHALL poll for state on a schedule, SHALL update telemetry only from valid replies, and SHALL preserve the last valid state or explicit defaults during comms failure instead of generating random replacement values.

#### Scenario: EPS comms timeout
- **WHEN** the simulator does not respond within the configured timeout window
- **THEN** `EpsBridge` SHALL raise the comms error path and SHALL NOT replace EPS telemetry with arbitrary data
