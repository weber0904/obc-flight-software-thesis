## ADDED Requirements

### Requirement: Physical Lab Serial Uplink Ingress
The comm subsystem SHALL provide a governed staged probe that can validate bounded macOS-origin TT&C uplink ingress through the physical lab serial link into `subsystem.local` COMM node `4` and onward to hosted OBC over the existing COMM CSP service contract.

#### Scenario: Physical serial ingress reaches OBC command readback
- **WHEN** macOS runs the ground TT&C gateway against the explicit host serial endpoint
- **AND** `subsystem.local` runs native-built `comm_csp_node` on `/dev/serial0` as node `4`
- **AND** hosted OBC runs with `GROUND_LINK_MODE=comm-csp`
- **THEN** bounded GDS commands SHALL be able to change OBC EPS and ADCS readback before the repository claims physical lab serial uplink ingress

#### Scenario: COMM contract remains unchanged
- **WHEN** the lab serial ingress probe runs
- **THEN** the COMM node SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Downlink does not gate uplink ingress verdict
- **WHEN** bounded uplink commands reach OBC readback but events or telemetry are not visible through `fprime-cli`
- **THEN** the repository MAY record physical lab serial uplink ingress as the formal verdict
- **AND** it SHALL record downlink or full TT&C as diagnostic or not proven

