## ADDED Requirements

### Requirement: S-band TCP Ground Link Uses COMM Node 5
The comm subsystem SHALL support a hosted S-band simulated TCP ground-link path through `sband_comm_csp_node` as CSP node `5` while preserving generic COMM node `4` compatibility, UHF node `6` foundation behavior, and the existing COMM CSP service contract.

#### Scenario: S-band executable owns the TCP endpoint
- **WHEN** the hosted S-band TCP ground-link probe runs
- **THEN** `sband_comm_csp_node` SHALL run as link identity `sband` with CSP node `5`
- **AND** it SHALL listen on the configured S-band simulated TCP endpoint
- **AND** the probe SHALL NOT use `comm_csp_node` node `4` as S-band evidence

#### Scenario: OBC reaches GDS through COMM rather than direct TCP
- **WHEN** the hosted S-band TCP ground-link probe validates command, event, telemetry, or file/downlink behavior
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: COMM service contract remains unchanged
- **WHEN** S-band TCP traffic traverses COMM
- **THEN** S-band SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or alter the existing request/reply wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** S-band TCP ground-link evidence is recorded
- **THEN** the verdict SHALL NOT claim UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, pass scheduling, or arbitrary onboard file downlink
