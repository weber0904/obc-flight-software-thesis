## ADDED Requirements

### Requirement: Hosted S-band TCP TT&C Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted S-band TCP gateway-backed COMM command/event/channel TT&C path once it is proven through `sband_comm_csp_node` node `5`.

#### Scenario: Registry identifies S-band TCP TT&C boundary
- **WHEN** the hosted S-band TCP TT&C path is registered
- **THEN** the registry SHALL identify the newly proven path as bounded command/event/channel traffic through `fprime-cli -> fprime-gds -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> internal CSP -> hosted OBC`
- **AND** it SHALL cite the S-band TCP ground-link evidence record

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the hosted S-band TCP TT&C registry entry
- **THEN** the entry SHALL state that it does not prove direct `GDS -> TCP -> OBC`, UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, target hardware, Raspberry Pi deployment, or arbitrary onboard file downlink

### Requirement: Hosted S-band TCP File Downlink Path Is Registered
The verification-path registry SHALL include a distinct entry for bounded housekeeping archive file/downlink over the hosted S-band TCP gateway-backed COMM path once received files byte-match OBC runtime source snapshots.

#### Scenario: Registry identifies S-band TCP file/downlink boundary
- **WHEN** the hosted S-band TCP file/downlink path is registered
- **THEN** the registry SHALL identify the newly proven path as `HK_DOWNLINK_* -> FileDownlink -> S-band TCP COMM downlink -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL require byte-matched `hk-index.csv` and selected `hk-slot-*.bin` files before the file/downlink entry is cited as evidence

#### Scenario: Registry keeps S-band file scope bounded
- **WHEN** reviewers inspect the hosted S-band TCP file/downlink registry entry
- **THEN** the entry SHALL cite the S-band TCP TT&C path as a prerequisite
- **AND** it SHALL state that the entry does not prove arbitrary onboard file path downlink, reliable transfer, packet-loss recovery, RF behavior, target hardware, Raspberry Pi deployment, CCSDS, or UHF backup behavior
