## ADDED Requirements

### Requirement: Gateway-Backed COMM SocketCAN TT&C Validation
The ground TT&C gateway SHALL support governed validation where bounded command/event/channel TT&C traverses lab serial ingress into COMM node `4`, then reaches target OBC over the shared SocketCAN carrier.

#### Scenario: Gateway path continues through SocketCAN COMM
- **WHEN** the COMM SocketCAN TT&C probe runs
- **THEN** `ground_ttc_gateway` SHALL remain the stock F' framing adapter between GDS and the lab serial ingress
- **AND** COMM SHALL forward the ground-link chunks to OBC through SocketCAN rather than hosted ZMQHUB

#### Scenario: Gateway verdict requires downlink observations
- **WHEN** the probe claims bounded TT&C over COMM SocketCAN
- **THEN** it SHALL observe command events through `fprime-cli events`
- **AND** it SHALL observe `GROUND_LINK_TX_BYTES` through `fprime-cli channels`

#### Scenario: Gateway evidence records both ingress and internal carrier
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** it SHALL record the serial ingress endpoint settings, COMM node id, CAN interface mapping, CAN timing, GDS ports, and active CAN health
