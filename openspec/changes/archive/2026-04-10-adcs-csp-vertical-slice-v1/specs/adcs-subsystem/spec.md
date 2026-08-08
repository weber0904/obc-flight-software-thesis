## MODIFIED Requirements

### Requirement: ADCS CSP Host Protocol
The ADCS subsystem SHALL define ADCS-owned CSP service request and response payloads for the hosted simulator and the OBC-side bridge, and those payloads SHALL preserve the formal node `3` contract while avoiding libcsp reserved service ports `0` through `3`.

#### Scenario: ADCS request includes a sequence and returns one state payload
- **WHEN** the hosted bridge requests ADCS state
- **THEN** the protocol SHALL carry a sequence field and SHALL return the required ADCS state in a single CSP response payload

### Requirement: Hosted ADCS Simulator
The hosted ADCS implementation SHALL provide a simulator executable that serves state, mode, target, and calibration services as libcsp node `3` over the hosted ZMQHUB-backed internal CSP substrate while maintaining deterministic quaternion, angular-rate, pointing-error, and sensor-validity state.

#### Scenario: Mode command changes simulator behavior
- **WHEN** a client issues `ADCS_SET_MODE`
- **THEN** the simulator SHALL update the commanded mode and SHALL reflect that new mode in subsequent status responses

#### Scenario: Target command changes simulator target state
- **WHEN** a client issues `ADCS_SET_TARGET`
- **THEN** the simulator SHALL update the target vector and SHALL reflect the requested target through subsequent state responses

#### Scenario: Calibration command is served over internal CSP
- **WHEN** a client issues an ADCS calibration request over the hosted internal CSP substrate
- **THEN** the simulator SHALL acknowledge the request and preserve deterministic state reporting after the calibration service returns
