## ADDED Requirements

### Requirement: Physical COMM SocketCAN TT&C Path Is Registered Separately
The verification-path registry SHALL register COMM SocketCAN command/event/channel TT&C as a distinct path only after evidence proves gateway-backed TT&C through COMM node `4` over `subsystem.local:can1` to target OBC on `obc.local:can0`.

#### Scenario: Registry names the COMM SocketCAN boundary
- **WHEN** the COMM SocketCAN TT&C probe passes
- **THEN** the registry SHALL identify the newly proven path as `GDS -> ground_ttc_gateway -> lab serial ingress -> COMM node 4 on subsystem.local:can1 -> shared SocketCAN bus -> obc.local:can0 -> OBC -> COMM downlink -> GDS`
- **AND** it SHALL state that EPS/ADCS remain on `subsystem.local:can0`

#### Scenario: Registry keeps prior paths distinct
- **WHEN** reviewers inspect the COMM SocketCAN TT&C entry
- **THEN** the registry SHALL keep EPS/ADCS SocketCAN foundation, physical lab serial COMM TT&C, and COMM file/downlink as separate adjacent paths
- **AND** it SHALL not treat any of those entries alone as proof of COMM SocketCAN TT&C

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM SocketCAN TT&C path is registered
- **THEN** the registry SHALL explicitly state that file/downlink, RF, no-preamble first-byte-clean behavior, dual-bus redundancy, and independent COMM hardware remain unproven by that evidence
