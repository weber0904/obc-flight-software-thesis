## ADDED Requirements

### Requirement: Hosted S-band CCSDS Is The Default OBC Ground Path
The comm subsystem SHALL treat the default hosted `OBC` deployment as the governed S-band CCSDS ground path while preserving old `ComFprime` behavior only as an explicit legacy/regression path.

#### Scenario: Default hosted OBC uses CCSDS S-band
- **WHEN** the default hosted `OBC` deployment is built or run without legacy overrides
- **THEN** it SHALL use `ComCcsds` for the ground communication topology
- **AND** it SHALL expose the default command, event, telemetry, and file namespace as `OBCApp`
- **AND** it SHALL use S-band COMM node `5` when exercising the hosted gateway-backed path

#### Scenario: Legacy ComFprime path is explicit
- **WHEN** old hosted `ComFprime` behavior is needed for regression
- **THEN** it SHALL run through an explicitly named legacy deployment rather than the default `OBC` deployment
- **AND** the legacy deployment SHALL NOT be described as the hosted S-band CCSDS default path

#### Scenario: Existing baselines stay scoped
- **WHEN** S-band node `5` or UHF node `6` `ComFprime` baseline records are cited
- **THEN** they SHALL be described as stock `ComFprime` gateway baselines
- **AND** they SHALL NOT be described as CCSDS adoption evidence

### Requirement: CCSDS Hosted Adoption Uses S-band Node 5
The comm subsystem SHALL restrict the default hosted CCSDS adoption path to the S-band node `5` COMM path.

#### Scenario: S-band adoption uses hosted COMM CSP
- **WHEN** the CCSDS hosted adoption proof runs
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: COMM service contract remains unchanged
- **WHEN** CCSDS traffic traverses the COMM path
- **THEN** S-band SHALL continue to use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT alter existing COMM service ports, node IDs, or request/reply wire layouts

#### Scenario: UHF stays out of adoption
- **WHEN** CCSDS hosted adoption results are recorded
- **THEN** UHF node `6` SHALL be listed only as out of scope or future work
- **AND** the change SHALL NOT create UHF CCSDS evidence

### Requirement: CCSDS Adoption Covers Bounded TT&C, File Downlink, And Decoded Framing
The comm subsystem SHALL prove default hosted S-band CCSDS adoption with bounded command, event, telemetry, bounded file/downlink, gateway raw-byte compatibility, and decoded CCSDS frame/APID observations.

#### Scenario: Bounded CCSDS adoption proof covers required behavior
- **WHEN** the CCSDS hosted adoption proof passes
- **THEN** it SHALL include command uplink, event downlink, telemetry downlink, `HK_DOWNLINK_INDEX`, at least two `HK_DOWNLINK_SLOT` files, gateway raw-byte compatibility, and decoded frame/APID observations
- **AND** it SHALL identify the default hosted `OBC` deployment rather than a spike-only executable

#### Scenario: Adjacent futures stay out of the verdict
- **WHEN** CCSDS adoption evidence is recorded
- **THEN** the verdict SHALL NOT claim UHF CCSDS, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, failover policy, pass scheduling, or arbitrary onboard file downlink

## REMOVED Requirements

### Requirement: CCSDS Ground Link Spike Preserves ComFprime Baselines
**Reason**: The spike-only default decision is superseded by formal hosted S-band CCSDS adoption.
**Migration**: Use `Hosted S-band CCSDS Is The Default OBC Ground Path` for the default path and explicit legacy `ComFprime` requirements for regression boundaries.

### Requirement: CCSDS Hosted Proof Uses S-band Node 5
**Reason**: The bounded proof is promoted from spike terminology to hosted adoption terminology.
**Migration**: Use `CCSDS Hosted Adoption Uses S-band Node 5`.

### Requirement: CCSDS Spike Covers Bounded TT&C and File Downlink
**Reason**: The spike recommendation is converted into a default hosted adoption proof with decoded frame/APID evidence.
**Migration**: Use `CCSDS Adoption Covers Bounded TT&C, File Downlink, And Decoded Framing`.
