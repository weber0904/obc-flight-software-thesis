## ADDED Requirements

### Requirement: Shared ADCS Host Protocol
The ADCS subsystem SHALL define a project-local shared request/response protocol for the hosted simulator and the OBC-side bridge, and that protocol SHALL preserve the formal node `3` and ADCS service-port contract established by the main `adcs-subsystem` specification.

#### Scenario: ADCS request includes a sequence and returns one state payload
- **WHEN** the hosted bridge requests ADCS state
- **THEN** the protocol SHALL carry a sequence field and SHALL return the required ADCS state in a single response payload

### Requirement: Hosted ADCS Simulator
The first ADCS implementation slice SHALL provide a hosted simulator executable that serves state, mode, target, and calibration services over ZMQ request/response transport and maintains deterministic quaternion, angular-rate, pointing-error, and sensor-validity state.

#### Scenario: Mode command changes simulator behavior
- **WHEN** a client issues `ADCS_SET_MODE`
- **THEN** the simulator SHALL update the commanded mode and SHALL reflect that new mode in subsequent status responses

### Requirement: AdcsBridge Public Behavior
`AdcsBridge` SHALL own the `ADCS_*` command, telemetry, and event families, SHALL update telemetry only from valid simulator replies, and SHALL preserve the last valid state when replies are invalid or absent while raising the owned comms or sensor-fault paths.

#### Scenario: Invalid sensor reply preserves the last valid state
- **WHEN** the simulator reports an invalid sensor sample
- **THEN** `AdcsBridge` SHALL emit the sensor-fault behavior and SHALL NOT overwrite the last valid ADCS telemetry with invalid values

### Requirement: ADCS Convergence Signaling
The first ADCS implementation slice SHALL evaluate the mission-constant detumble and pointing thresholds and SHALL surface the owned completion/acquired events when those thresholds are met.

#### Scenario: Detumble threshold is reached
- **WHEN** the simulated angular-velocity norm drops below the detumble threshold
- **THEN** `AdcsBridge` SHALL raise `ADCS_DETUMBLE_COMPLETE`
