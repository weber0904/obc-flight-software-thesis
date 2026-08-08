## ADDED Requirements

### Requirement: Gateway Supports UHF Hosted Serial Southbound Segment
The ground TT&C gateway SHALL support a hosted UHF serial southbound segment while keeping stock F' framing toward `fprime-gds`.

#### Scenario: Gateway connects to UHF serial COMM endpoint
- **WHEN** the hosted UHF UART backup probe runs
- **THEN** `ground_ttc_gateway` SHALL use its configured serial southbound endpoint to exchange stock F' uplink/downlink bytes with `uhf_comm_csp_node` node `6`
- **AND** it SHALL keep the northbound GDS connection as stock F' framing
- **AND** gateway logs/evidence SHALL identify the link identity as `uhf`

#### Scenario: Gateway evidence separates UHF serial from adjacent paths
- **WHEN** UHF gateway evidence is recorded
- **THEN** it SHALL record the GDS IP/TTS ports separately from the hosted UHF serial endpoints
- **AND** it SHALL not describe S-band TCP, direct `GDS -> TCP -> OBC`, or generic node `4` COMM as the UHF verdict

#### Scenario: UHF gateway scope remains bounded
- **WHEN** bounded backup command ingress is validated through the gateway
- **THEN** the evidence SHALL be limited to command ingress, command events, and channel observations over the hosted UHF serial path
- **AND** it SHALL NOT claim full command authority, failover policy, file/downlink, reliable transfer, RF, CCSDS, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation
