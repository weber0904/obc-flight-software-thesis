## ADDED Requirements

### Requirement: Active UHF CCSDS Node 6 Path Registration Requires Passing Adoption Proof
The verification path registry SHALL register a reusable active hosted UHF CCSDS ground-link path only after the node `6` UHF adoption proof passes.

#### Scenario: Passing UHF adoption proof registers active path
- **WHEN** the active hosted UHF CCSDS node `6` adoption proof passes bounded command, event, telemetry, file/downlink, and decoded framing checks
- **THEN** the registry SHALL add an active hosted UHF CCSDS path that references the `uhf-ccsds-adoption-v1` evidence record
- **AND** the path SHALL name `space-packet-space-data-link`, `SCID 0x44`, `VCID 2`, `TM frame size 1024`, node `6`, and the default hosted `OBC` executable

#### Scenario: Historical UHF path remains distinct
- **WHEN** the registry is updated for this change
- **THEN** the historical hosted UHF serial backup `ComFprime` path SHALL remain separately named
- **AND** the registry SHALL NOT merge historical UHF `ComFprime`, active UHF CCSDS, direct TCP, or S-band CCSDS into a single validation path

## MODIFIED Requirements

### Requirement: Hosted UHF Serial Backup TT&C Path Is Registered
The verification-path registry SHALL keep the historical hosted UHF serial gateway-backed `ComFprime` TT&C path distinct from the active hosted UHF CCSDS node `6` path.

#### Scenario: Registry identifies historical UHF backup ingress boundary
- **WHEN** the historical hosted UHF serial backup TT&C path is cited
- **THEN** the registry SHALL identify the path as bounded command/event/channel traffic through `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> internal CSP -> hosted OBC_ComFprimeLegacy`
- **AND** it SHALL cite the historical UHF UART backup evidence record rather than the active UHF CCSDS adoption record

#### Scenario: Registry keeps adjacent UHF paths distinct
- **WHEN** reviewers inspect the historical hosted UHF serial backup TT&C registry entry
- **THEN** the entry SHALL state that it does not prove active UHF CCSDS behavior, S-band TCP, direct `GDS -> TCP -> OBC`, generic node `4` COMM, full UHF command authority, failover policy, file/downlink behavior, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation
