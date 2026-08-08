## ADDED Requirements

### Requirement: Future Omitted-RF TT&C Path Is Registered Separately
When the repository proves a gateway-backed omitted-RF TT&C path, the verification-path registry SHALL record that path separately from both the direct `GDS -> TCP -> OBC` development path and the existing external comm mock or UART baselines.

#### Scenario: Registry keeps gateway-backed TT&C distinct from direct GDS
- **WHEN** the first gateway-backed omitted-RF TT&C path becomes formally proven
- **THEN** the registry SHALL identify it as a distinct path and SHALL keep direct GDS and current external-comm baseline entries separate

### Requirement: Future Shared CAN FD Internal CSP Path Is Registered Separately
When the repository proves a spacecraft-side shared `CAN FD` carrier for future CSP-facing subsystems, the verification-path registry SHALL record that path separately from hosted `ZMQHUB` CSP baselines and from omitted-RF ground ingress paths.

#### Scenario: Registry keeps shared CAN FD proof separate from hosted ZMQHUB proof
- **WHEN** the first shared `CAN FD` internal CSP path becomes formally proven
- **THEN** the registry SHALL identify that path as distinct from the hosted or split-host `ZMQHUB/TCP/IP` CSP baselines

### Requirement: Future Gateway And Shared-Bus Paths Stay Distinct From Existing Live GPS UART Proof
When the repository later proves gateway-backed TT&C or shared `CAN FD` CSP paths, the verification-path registry SHALL keep those entries distinct from the already-governed live `GPS -> OBC` UART path.

#### Scenario: Registry does not collapse GPS proof into later TT&C or CAN FD proof
- **WHEN** a reviewer checks future gateway-backed TT&C or shared `CAN FD` entries
- **THEN** the registry SHALL continue to identify the governed live GPS UART entry as a separate direct sensor path
