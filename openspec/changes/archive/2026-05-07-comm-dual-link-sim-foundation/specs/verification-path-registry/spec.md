## ADDED Requirements

### Requirement: Hosted Dual-Link COMM Simulator Identity Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted dual-link COMM simulator identity and coexistence path once generic COMM node `4`, S-band COMM node `5`, and UHF COMM node `6` have been proven as separate hosted simulator process identities.

#### Scenario: Registry identifies newly proven foundation boundary
- **WHEN** the hosted dual-link COMM simulator foundation path is registered
- **THEN** the registry SHALL identify the newly proven path as hosted OBC node `1` reaching generic `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, and `uhf_comm_csp_node` node `6` over the governed hosted internal CSP substrate
- **AND** it SHALL cite the dual-link foundation evidence record

#### Scenario: Registry keeps older node-4 evidence compatible
- **WHEN** reviewers inspect existing generic COMM node `4` evidence after the dual-link foundation change
- **THEN** the registry SHALL keep that earlier evidence scoped to generic compatibility COMM rather than reclassifying it as S-band or UHF evidence

#### Scenario: Registry excludes future full-link proofs
- **WHEN** reviewers inspect the dual-link foundation registry entry
- **THEN** the entry SHALL state that it does not prove complete S-band GDS path, UHF UART backup path, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, target hardware behavior, or Pi hardware deployment
