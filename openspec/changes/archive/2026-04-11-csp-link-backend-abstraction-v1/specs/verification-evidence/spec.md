## ADDED Requirements

### Requirement: Carrier Abstraction Evidence Keeps Development Carrier Separate From Future Hardware Buses
The verification evidence tree SHALL record the commands, selected carrier/backend, observed libcsp startup outcome, and final verdict for any internal CSP carrier-abstraction change, and that evidence SHALL keep the active development carrier separate from future hardware-bus validation claims.

#### Scenario: ZMQHUB refactor evidence stays bounded
- **WHEN** the repository validates a carrier-abstraction refactor while `zmqhub` remains the only active carrier
- **THEN** the evidence SHALL identify `zmqhub` as the validated carrier, cite the rerun CSP/EPS/ADCS checks, and SHALL NOT describe the result as proof of future `UART`, `CAN`, `RS485`, or other physical-bus behavior
