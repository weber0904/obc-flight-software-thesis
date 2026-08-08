## ADDED Requirements

### Requirement: CCSDS Ground Link Spike Preserves ComFprime Baselines
The comm subsystem SHALL support a bounded CCSDS ground-link spike without migrating the default `OBC` `ComFprime` topology or reclassifying existing `ComFprime` evidence as CCSDS evidence.

#### Scenario: Default topology remains ComFprime
- **WHEN** the default `OBC` deployment is built or run
- **THEN** it SHALL continue to use the existing `ComFprime` topology path
- **AND** the CCSDS spike SHALL use a separate spike-only topology, executable, or build target
- **AND** the change SHALL NOT silently migrate command, event, telemetry, file uplink, or file downlink for the default topology to `ComCcsds`

#### Scenario: Existing S-band and UHF baselines stay scoped
- **WHEN** S-band node `5` or UHF node `6` baseline records are cited
- **THEN** they SHALL be described as stock `ComFprime` gateway baselines
- **AND** they SHALL NOT be described as CCSDS-compatible evidence unless this spike records new CCSDS proof for the specific behavior

### Requirement: CCSDS Hosted Proof Uses S-band Node 5
The comm subsystem SHALL restrict the CCSDS hosted proof to the S-band node `5` COMM path.

#### Scenario: S-band proof uses hosted COMM CSP
- **WHEN** the CCSDS hosted proof runs
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: COMM service contract remains unchanged
- **WHEN** CCSDS traffic traverses the COMM path
- **THEN** S-band SHALL continue to use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT alter existing COMM service ports, node IDs, or request/reply wire layouts

#### Scenario: UHF stays analysis-only
- **WHEN** CCSDS spike results are recorded
- **THEN** UHF node `6` SHALL be listed only as a comparison or risk item
- **AND** the change SHALL NOT create UHF CCSDS evidence

### Requirement: CCSDS Spike Covers Bounded TT&C and File Downlink
The comm subsystem SHALL compare or prototype `ComCcsds` feasibility for command, event, telemetry, bounded file/downlink, and gateway compatibility risk.

#### Scenario: Bounded CCSDS proof covers required behavior
- **WHEN** the CCSDS hosted proof passes
- **THEN** it SHALL include command uplink, event downlink, telemetry downlink, `HK_DOWNLINK_INDEX`, at least two `HK_DOWNLINK_SLOT` files, and gateway raw-byte compatibility
- **AND** it SHALL include a recommendation of `adopt now`, `defer with blockers`, or `keep ComFprime with CCSDS-aligned semantics`

#### Scenario: Adjacent futures stay out of the verdict
- **WHEN** CCSDS spike evidence is recorded
- **THEN** the verdict SHALL NOT claim topology-wide migration, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, failover policy, or arbitrary onboard file downlink
