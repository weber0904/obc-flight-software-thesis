## MODIFIED Requirements

### Requirement: Future Shared CAN FD Internal CSP Path Is Registered Separately
When the repository proves a spacecraft-side shared `CAN FD` carrier for future CSP-facing subsystems, the verification-path registry SHALL record that path separately from hosted `ZMQHUB` CSP baselines and from omitted-RF ground ingress paths.

#### Scenario: First physical internal CSP proof records logical-node limits explicitly
- **WHEN** the first governed CAN FD-capable SocketCAN path is registered
- **THEN** the registry SHALL identify `EPS` and `ADCS` as separate logical CSP nodes while also stating that the proof does not imply independent physical EPS and ADCS controllers

#### Scenario: First physical internal CSP proof records reserved-channel isolation separately
- **WHEN** the first governed CAN FD-capable SocketCAN path is registered
- **THEN** the registry SHALL record the active shared-bus path separately from the reserved subsystem CAN channel evidence and SHALL not treat the reserved channel as an active COMM path
