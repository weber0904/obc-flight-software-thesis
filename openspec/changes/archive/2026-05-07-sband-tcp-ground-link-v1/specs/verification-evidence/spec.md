## ADDED Requirements

### Requirement: S-band TCP Ground Link Evidence Is Reviewable
The verification evidence baseline SHALL record the commands, launcher scripts, endpoints, observed bounded traffic, and final verdict for the hosted S-band TCP through-COMM path.

#### Scenario: Evidence records S-band process identity and path
- **WHEN** the hosted S-band TCP ground-link probe passes
- **THEN** the evidence SHALL record `sband_comm_csp_node` as link identity `sband` and CSP node `5`
- **AND** it SHALL record the path as `fprime-cli -> fprime-gds -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC`

#### Scenario: Evidence records bounded TT&C proof
- **WHEN** the evidence describes S-band TCP command/event/channel PASS
- **THEN** it SHALL include bounded command readback from OBC
- **AND** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Evidence records bounded file/downlink proof
- **WHEN** the evidence describes S-band TCP file/downlink PASS
- **THEN** it SHALL identify the downlinked `hk-index.csv` and at least two downlinked `hk-slot-*.bin` files
- **AND** it SHALL state that each received file was compared byte-for-byte against an OBC runtime source snapshot

#### Scenario: Evidence exclusions stay explicit
- **WHEN** S-band TCP ground-link evidence is recorded
- **THEN** it SHALL state that the evidence does not prove direct `GDS -> TCP -> OBC`, UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, pass scheduling, or arbitrary onboard file downlink
