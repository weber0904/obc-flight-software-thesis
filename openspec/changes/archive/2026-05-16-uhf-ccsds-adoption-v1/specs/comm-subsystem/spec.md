## ADDED Requirements

### Requirement: Active UHF Node 6 Path Uses A Local CCSDS Stack
The comm subsystem SHALL route the active hosted UHF node `6` path through a repo-local CCSDS stack inside `TopCcsds` instead of the local `OBCComFprime` subtopology.

#### Scenario: Active TopCcsds does not depend on OBCComFprime for UHF
- **WHEN** the default hosted `OBC` deployment is built or run on the active `TopCcsds` path
- **THEN** its UHF driver, command ingress, packet egress, and file/downlink UHF path SHALL use a local CCSDS UHF subtopology
- **AND** the active `TopCcsds` UHF path SHALL NOT depend on `OBCComFprime`

#### Scenario: Legacy ComFprime remains explicit
- **WHEN** old UHF `ComFprime` behavior is needed for regression
- **THEN** it SHALL remain on the explicit legacy deployment or historical regression scripts
- **AND** the change SHALL NOT reinterpret those historical paths as the active UHF baseline

### Requirement: Active UHF CCSDS Uses Shared SCID And Explicit VCID 2
The comm subsystem SHALL keep shared spacecraft identity and APID semantics across S-band and UHF while distinguishing the active UHF CCSDS path with an explicit `VCID = 2`.

#### Scenario: UHF keeps shared packet semantics
- **WHEN** the active UHF CCSDS path sends or receives command, telemetry, event, or file traffic
- **THEN** it SHALL keep `SCID = 0x44`
- **AND** it SHALL keep the stock packet-class APID semantics for command `0`, telemetry `1`, log/event `2`, and file `3`

#### Scenario: UHF link identity is explicit
- **WHEN** the active UHF CCSDS path is framed or deframed
- **THEN** UHF egress SHALL carry `VCID = 2`
- **AND** the active UHF `TcDeframer` SHALL accept only `VCID = 2` for the active path

### Requirement: Active UHF CCSDS Preserves Existing Runtime Semantics
The comm subsystem SHALL preserve the existing active runtime policy semantics for UHF backup, UHF primary-after-switch, and bounded UHF file/downlink while migrating the active UHF framing path to CCSDS.

#### Scenario: UHF backup remains bounded
- **WHEN** UHF is configured as the bounded backup path
- **THEN** authenticated command policy, link-health policy, and command ingress source ownership SHALL remain owned by the active COMM and authority components
- **AND** the migration SHALL NOT broaden UHF authority, RF, or reliable-transfer claims by itself

#### Scenario: UHF primary after switch still supports shared downlink behavior
- **WHEN** active COMM runtime switches UHF into the primary role
- **THEN** the active UHF CCSDS path SHALL continue to support the existing bounded command, event, telemetry, and file/downlink behavior already claimed for the active baseline

## MODIFIED Requirements

### Requirement: CCSDS Hosted Adoption Uses S-band Node 5 Plus Active UHF Node 6
The comm subsystem SHALL treat the active hosted CCSDS baseline as S-band node `5` plus UHF node `6`, with separate adoption evidence and bounded claims for each link.

#### Scenario: S-band adoption uses hosted COMM CSP
- **WHEN** the CCSDS hosted S-band adoption proof runs
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: Active UHF CCSDS uses hosted COMM CSP node 6
- **WHEN** the active hosted UHF CCSDS adoption proof runs
- **THEN** the active UHF path SHALL use `uhf_comm_csp_node(node 6)`
- **AND** it SHALL remain distinct from the S-band node `5` adoption path and from historical UHF `ComFprime` regression paths

#### Scenario: COMM service contract remains unchanged
- **WHEN** CCSDS traffic traverses the COMM path
- **THEN** S-band and UHF SHALL continue to use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT alter existing COMM service ports, node IDs, or request/reply wire layouts

### Requirement: CCSDS Adoption Covers Bounded TT&C, File Downlink, And Decoded Framing
The comm subsystem SHALL prove default hosted CCSDS adoption with bounded command, event, telemetry, bounded file/downlink, gateway raw-byte compatibility, and decoded CCSDS frame/APID observations for the specific active path under test.

#### Scenario: UHF CCSDS adoption proof covers required behavior
- **WHEN** the active UHF CCSDS adoption proof passes
- **THEN** it SHALL include bounded command uplink, event downlink, telemetry downlink, bounded UHF file/downlink, and decoded framing observations
- **AND** it SHALL identify the path as the default hosted `OBC` UHF node `6` path rather than a legacy `ComFprime` executable

#### Scenario: UHF verdict stays bounded
- **WHEN** active UHF CCSDS adoption evidence is recorded
- **THEN** the verdict SHALL NOT claim RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, broader link-authority changes, or arbitrary onboard file downlink
