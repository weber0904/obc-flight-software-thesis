## ADDED Requirements

### Requirement: COMM SocketCAN TT&C Participation
The comm subsystem SHALL provide a governed validation path where COMM node `4` participates on the spacecraft-side SocketCAN carrier through `subsystem.local:can1` while preserving the existing gateway-backed TT&C service contract.

#### Scenario: COMM node joins the shared SocketCAN carrier
- **WHEN** the COMM SocketCAN TT&C probe runs
- **THEN** `comm_csp_node` SHALL run as COMM node `4` through `CSP_TRANSPORT=socketcan` on `subsystem.local:can1`
- **AND** EPS and ADCS SHALL remain reachable through `subsystem.local:can0`

#### Scenario: COMM service contract remains unchanged
- **WHEN** COMM participates over SocketCAN
- **THEN** COMM SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or wire layouts

#### Scenario: Gateway-backed TT&C reaches OBC over SocketCAN
- **WHEN** macOS sends bounded GDS commands through `ground_ttc_gateway` and the lab serial ingress
- **THEN** the commands SHALL traverse COMM node `4` over the shared SocketCAN carrier and produce bounded OBC readback, command events, and `GROUND_LINK_TX_BYTES`

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** the verdict SHALL NOT claim file/downlink behavior, RF behavior, no-preamble first-byte-clean behavior, target OBC migration beyond this target OBC run, dual-bus redundancy, or independent COMM hardware beyond the `subsystem.local:can1` controller
