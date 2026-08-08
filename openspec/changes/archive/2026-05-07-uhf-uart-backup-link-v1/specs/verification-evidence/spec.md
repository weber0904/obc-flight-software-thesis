## ADDED Requirements

### Requirement: UHF UART Backup Link Evidence Is Reviewable
The verification evidence baseline SHALL record the commands, launcher scripts, endpoints, observed bounded traffic, BeaconV1 capture/decode artifacts, and final verdict for the hosted UHF UART backup/beacon path.

#### Scenario: Evidence records UHF process identity and command path
- **WHEN** the hosted UHF UART backup probe passes
- **THEN** the evidence SHALL record `uhf_comm_csp_node` as link identity `uhf` and CSP node `6`
- **AND** it SHALL record the command ingress path as `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> OBC`

#### Scenario: Evidence records bounded backup command ingress
- **WHEN** the evidence describes UHF backup command ingress PASS
- **THEN** it SHALL include bounded command readback from OBC
- **AND** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Evidence records UHF BeaconV1 capture
- **WHEN** the evidence describes UHF beacon PASS
- **THEN** it SHALL identify the captured BeaconV1 binary artifact and decoded JSON artifact
- **AND** it SHALL record that the captured frame was decoded with expected wire size, schema version, CRC, and representative populated state fields
- **AND** it SHALL identify the beacon path as UHF node-6 side-channel capture rather than file/downlink or command response behavior

#### Scenario: Evidence exclusions stay explicit
- **WHEN** UHF UART backup evidence is recorded
- **THEN** it SHALL state that the evidence does not prove S-band TCP, direct `GDS -> TCP -> OBC`, full UHF command authority, failover policy, arbitrary file downlink, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation
