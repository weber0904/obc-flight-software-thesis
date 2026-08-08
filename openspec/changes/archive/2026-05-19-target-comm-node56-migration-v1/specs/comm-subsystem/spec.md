## ADDED Requirements

### Requirement: Service-Managed Target S-band Uses Node 5
The comm subsystem SHALL support a service-managed target/lab S-band path where `subsystem.local` hosts `sband_comm_csp_node` as COMM node `5` and `obc.local` uses that path as the default active COMM profile.

#### Scenario: Target S-band service owns node 5
- **WHEN** the target/lab S-band COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `sband_comm_csp_node` as COMM node `5`
- **AND** it SHALL expose the configured TCP listener plus the existing COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`

#### Scenario: Target OBC default path uses node 5
- **WHEN** the target/lab default COMM profile runs
- **THEN** target OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`

### Requirement: Service-Managed Target UHF Uses Node 6
The comm subsystem SHALL support a service-managed target/lab UHF path where `subsystem.local` hosts `uhf_comm_csp_node` as COMM node `6` on the physical serial ingress while preserving the existing COMM service contract.

#### Scenario: Target UHF service owns node 6
- **WHEN** the target/lab UHF COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `uhf_comm_csp_node` as COMM node `6`
- **AND** it SHALL use the configured lab serial ingress device and baudrate plus the existing COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`

#### Scenario: Target UHF profiles select authority role without changing node identity
- **WHEN** target/lab UHF runs as `uhf-primary` or `uhf-backup`
- **THEN** both profiles SHALL keep `COMM_CSP_NODE=6`
- **AND** only the `COMMAND_AUTHORITY_PROFILE` SHALL differ between those bounded operator modes

### Requirement: Quiet-Mode Node 6 Proof Stays Bounded
The comm subsystem SHALL keep target node-`6` proof bounded to quiet-mode operational validation until general non-quiet serial stability is separately proven.

#### Scenario: Node 6 proof uses probe-owned quiet mode
- **WHEN** target node-`6` command/readback validation is recorded
- **THEN** the proof SHALL be allowed to use the existing probe-owned quiet-egress override
- **AND** the verdict SHALL NOT claim general non-quiet serial/background-TM closure

### Requirement: Target/Lab COMM Primary Unavailable Uses Subsystem Responsiveness
The comm subsystem SHALL treat target/lab `COMM_PRIMARY_UNAVAILABLE` as repeated no-response from the current primary COMM subsystem stand-in, not as absence of an attached ground gateway.

#### Scenario: Node 5 S-band default path ignores detached ground tooling
- **WHEN** target/lab runs with node `5` as the default active COMM path
- **AND** the local probe-owned `ground_ttc_gateway` is stopped or absent
- **AND** OBC can still successfully reach node `5` through bounded internal CSP ping
- **THEN** the active runtime SHALL keep the primary COMM path available
- **AND** it SHALL NOT latch `COMM_PRIMARY_UNAVAILABLE` from gateway detachment alone

#### Scenario: Repeated node 5 or node 6 CSP ping failure latches unavailable
- **WHEN** the current primary target/lab COMM profile selects node `5` or node `6`
- **AND** OBC observes `3` consecutive scheduled internal CSP ping failures to that selected node
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE`
- **AND** the first scheduled successful responsiveness check SHALL clear that fault

#### Scenario: Ground-link transport observability stays separate
- **WHEN** target/lab evaluates COMM detector truth for node `5` or node `6`
- **THEN** ground-facing `GROUND_LINK_UP/DOWN` and transport-error growth SHALL remain reviewable observability
- **AND** those signals SHALL NOT redefine `COMM_PRIMARY_UNAVAILABLE` back into a ground-station-attachment fault
- **AND** those signals SHALL NOT by themselves submit reboot-class target/lab COMM FDIR while the primary subsystem responsiveness check remains healthy
