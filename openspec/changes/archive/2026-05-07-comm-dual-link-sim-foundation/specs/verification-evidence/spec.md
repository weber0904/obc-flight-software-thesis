## ADDED Requirements

### Requirement: Dual-Link COMM Simulator Foundation Evidence
The verification evidence baseline SHALL record bounded evidence for the hosted dual-link COMM simulator foundation without over-claiming later S-band or UHF transport paths.

#### Scenario: Evidence records explicit process identities
- **WHEN** the dual-link COMM simulator foundation probe passes
- **THEN** the evidence SHALL record the generic `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, and `uhf_comm_csp_node` node `6` executable identities
- **AND** it SHALL record distinct logs or output excerpts for each executable identity

#### Scenario: Evidence records bounded CSP service behavior
- **WHEN** the dual-link foundation probe exercises COMM services
- **THEN** the evidence SHALL show that each hosted COMM identity responds on services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the evidence SHALL show that OBC can reach nodes `4`, `5`, and `6` over the governed hosted internal CSP substrate

#### Scenario: Evidence exclusions stay explicit
- **WHEN** dual-link foundation evidence is recorded
- **THEN** it SHALL state that the evidence does not prove complete S-band GDS behavior, UHF UART/RS485/USB/macOS backup behavior, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, file/downlink behavior, target hardware behavior, or Pi hardware deployment
