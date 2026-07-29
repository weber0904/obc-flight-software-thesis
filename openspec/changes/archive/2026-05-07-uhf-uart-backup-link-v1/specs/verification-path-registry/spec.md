## ADDED Requirements

### Requirement: Hosted UHF Serial Backup TT&C Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted UHF serial gateway-backed COMM command/event/channel ingress path once it is proven through `uhf_comm_csp_node` node `6`.

#### Scenario: Registry identifies UHF backup ingress boundary
- **WHEN** the hosted UHF serial backup TT&C path is registered
- **THEN** the registry SHALL identify the newly proven path as bounded command/event/channel traffic through `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> internal CSP -> hosted OBC`
- **AND** it SHALL cite the UHF UART backup evidence record

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the hosted UHF serial backup TT&C registry entry
- **THEN** the entry SHALL state that it does not prove S-band TCP, direct `GDS -> TCP -> OBC`, generic node `4` COMM, full UHF command authority, failover policy, file/downlink behavior, CCSDS behavior, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Hosted UHF Node-6 Beacon Path Is Registered
The verification-path registry SHALL include a distinct entry for bounded BeaconV1 capture over the hosted UHF node-6 beacon side channel once the captured frame is decoded and recorded as formal evidence.

#### Scenario: Registry identifies UHF beacon boundary
- **WHEN** the hosted UHF beacon path is registered
- **THEN** the registry SHALL identify the newly proven path as `OBC BeaconPublisher -> UHF node-6 beacon side channel -> uhf_comm_csp_node(node 6) -> hosted beacon serial -> capture/decode`
- **AND** it SHALL require a captured BeaconV1 binary artifact and decoded JSON artifact before the entry is cited as evidence

#### Scenario: Registry keeps UHF beacon scope bounded
- **WHEN** reviewers inspect the hosted UHF beacon registry entry
- **THEN** the entry SHALL state that it does not prove command response behavior, file/downlink, full UHF command authority, failover policy, reliable transfer, CCSDS behavior, RF behavior, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation
