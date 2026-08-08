## ADDED Requirements

### Requirement: Hosted UHF Beacon Suppress Runtime Path Is Registered Separately

The verification-path registry SHALL register the hosted UHF beacon
suppress/runtime proof as a path distinct from the ordinary hosted UHF node-`6`
beacon side-channel capture path and from hosted UHF command-path proofs.

#### Scenario: Registry names the hosted suppress/runtime boundary
- **WHEN** hosted beacon suppress/runtime evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay or serial southbound) -> uhf_comm_csp_node(node 6) -> CSP -> OBC CommController -> BeaconPublisher suppress gate -> uhf_comm_csp_node beacon side channel -> hosted beacon serial capture`
- **AND** it SHALL state that the path proves suppress start, same-session
  refresh, timeout resume, and at least one negative no-suppress case

### Requirement: Target Quiet-UHF Beacon Suppress Runtime Path Is Registered Separately

The verification-path registry SHALL register the target/lab quiet node-`6`
beacon suppress/runtime proof as a path distinct from ordinary quiet-UHF
command/readback, file/downlink, and failover continuity proofs.

#### Scenario: Registry keeps quiet-UHF suppress/runtime scope bounded
- **WHEN** target quiet-UHF beacon suppress/runtime evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC CommController -> BeaconPublisher suppress gate -> subsystem.local beacon capture`
- **AND** it SHALL state that the proof remains quiet-path only
- **AND** it SHALL keep simultaneous dual-link runtime, UHF reliable transfer,
  RF closure, and broader handshake/runtime claims outside the registered path
