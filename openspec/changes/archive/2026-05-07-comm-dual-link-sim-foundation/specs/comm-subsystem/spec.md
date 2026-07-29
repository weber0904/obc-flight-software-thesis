## ADDED Requirements

### Requirement: Dual-Link COMM Simulator Foundation
The comm subsystem SHALL provide explicit hosted simulator process identities for generic compatibility COMM, S-band COMM, and UHF COMM while preserving the existing COMM CSP service contract.

#### Scenario: Executables define distinct link identities
- **WHEN** the hosted COMM simulator executables are built
- **THEN** `comm_csp_node` SHALL remain the generic compatibility COMM executable with default node `4`
- **AND** `sband_comm_csp_node` SHALL provide the S-band COMM executable identity with default node `5`
- **AND** `uhf_comm_csp_node` SHALL provide the UHF COMM executable identity with default node `6`

#### Scenario: Link identities reuse the existing COMM services
- **WHEN** generic, S-band, or UHF COMM simulator identities handle CSP ground-link requests
- **THEN** each identity SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or alter the existing request/reply wire layouts

#### Scenario: Shared implementation keeps executable boundaries visible
- **WHEN** the COMM simulator identities share `CommNodeServer` or `CommSimModel` implementation
- **THEN** each executable's startup logs, launcher/probe logs, and evidence SHALL identify the executable name, link identity, node id, serial endpoint, baudrate, and CSP interface name

#### Scenario: Foundation scope excludes complete link paths
- **WHEN** the dual-link simulator foundation evidence is recorded
- **THEN** the evidence SHALL NOT claim complete S-band GDS path, UHF UART backup path, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, or Pi hardware validation
