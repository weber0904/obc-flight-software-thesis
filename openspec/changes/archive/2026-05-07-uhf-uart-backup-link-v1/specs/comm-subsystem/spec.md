## ADDED Requirements

### Requirement: UHF Hosted UART Backup Uses COMM Node 6
The comm subsystem SHALL support a hosted UHF UART/RS485/USB/macOS stand-in backup command ingress path through `uhf_comm_csp_node` as CSP node `6` while preserving generic COMM node `4` compatibility, S-band node `5` behavior, and the existing COMM CSP command/downlink service contract.

#### Scenario: UHF executable owns the hosted serial endpoint
- **WHEN** the hosted UHF UART backup probe runs
- **THEN** `uhf_comm_csp_node` SHALL run as link identity `uhf` with CSP node `6`
- **AND** it SHALL use the configured hosted serial endpoint and baudrate for backup ingress
- **AND** the probe SHALL NOT use `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, S-band TCP, or direct `GDS -> TCP -> OBC` as UHF evidence

#### Scenario: OBC reaches GDS through UHF COMM rather than direct TCP
- **WHEN** the hosted UHF UART backup probe validates command, event, or telemetry behavior
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=6`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: UHF backup ingress keeps COMM services unchanged
- **WHEN** UHF backup command ingress traverses COMM
- **THEN** UHF SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add command/downlink service ports or alter the existing request/reply wire layouts

### Requirement: UHF Node-6 Beacon Side Channel
The comm subsystem SHALL support bounded BeaconV1 emission through `uhf_comm_csp_node` node `6` as a UHF link-local beacon side channel that remains distinct from command/downlink chunk services.

#### Scenario: Beacon path traverses UHF node 6
- **WHEN** the hosted UHF beacon probe runs
- **THEN** BeaconV1 frames SHALL be sent from hosted OBC to `uhf_comm_csp_node` node `6`
- **AND** `uhf_comm_csp_node` SHALL emit the beacon frame on the configured hosted beacon serial endpoint for capture

#### Scenario: Beacon side channel does not alter command services
- **WHEN** UHF BeaconV1 frames are emitted
- **THEN** services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS` SHALL remain unchanged
- **AND** beacon emission SHALL NOT require ground acknowledgement, command response semantics, file/downlink behavior, reliable transfer, CCSDS framing, or replacement of `ComFprime`

#### Scenario: UHF v1 claims stay bounded
- **WHEN** UHF UART backup evidence is recorded
- **THEN** the verdict SHALL NOT claim full UHF command authority, failover policy, arbitrary file downlink, S-band TCP behavior, direct GDS TCP behavior, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation
