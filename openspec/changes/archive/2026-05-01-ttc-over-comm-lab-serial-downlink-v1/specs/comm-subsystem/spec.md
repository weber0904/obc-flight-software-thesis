## ADDED Requirements

### Requirement: Physical Lab Serial Downlink Proof
The comm subsystem SHALL provide a governed focused probe that validates bounded ground-visible event and telemetry downlink over the physical lab serial COMM path after bounded physical uplink ingress to hosted OBC has already succeeded in the same run.

#### Scenario: Physical serial downlink requires command readback first
- **WHEN** macOS runs the ground TT&C gateway against the explicit host serial endpoint
- **AND** `subsystem.local` runs native-built `comm_csp_node` on `/dev/serial0` as node `4`
- **AND** hosted OBC runs with `GROUND_LINK_MODE=comm-csp`
- **THEN** the probe SHALL first verify bounded EPS and ADCS command readback through the physical serial COMM path before claiming downlink success

#### Scenario: Events and telemetry are visible through the physical COMM path
- **WHEN** the focused physical lab serial downlink probe reaches its downlink verdict
- **THEN** `fprime-cli events` SHALL observe bounded command event output for the commands sent through the path
- **AND** `fprime-cli channels` SHALL observe `GROUND_LINK_TX_BYTES`
- **AND** the resulting evidence MAY describe the path as bounded physical lab serial TT&C

#### Scenario: COMM service contract remains unchanged
- **WHEN** the physical lab serial downlink probe runs
- **THEN** COMM SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** the physical lab serial downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim file/downlink, RF behavior, target OBC migration, no-preamble first-byte-clean behavior, ScenarioBridge pass state, link availability policy, or COMM shared CAN FD participation
