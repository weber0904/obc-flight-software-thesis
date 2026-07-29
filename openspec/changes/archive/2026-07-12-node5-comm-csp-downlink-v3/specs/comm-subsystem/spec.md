## ADDED Requirements

### Requirement: Node-5 COMM CSP Downlink V3 Uses Bulk Data Frames Below GroundLinkDriver
The comm subsystem SHALL provide a node-`5`-only COMM CSP downlink transport
`v3` below `GroundLinkDriver` that sends one-way bulk data frames and uses
bounded control polling plus atomic commit-to-drain acceptance.

#### Scenario: Node-5 v3 accepts a full send after committed in-memory enqueue
- **WHEN** target OBC sends one serialized downlink buffer to COMM node `5`
  through the `v3` path
- **THEN** node `5` SHALL stage the incoming `v3` data frames in memory
- **AND** it SHALL report success upstream only after the full buffer has been
  committed into the bounded drain queue
- **AND** it SHALL NOT wait for external-link flush completion before that
  success is reported

#### Scenario: Node-5 v3 sender does not stop after each frame
- **WHEN** the OBC-side backend sends one upper-layer buffer on the `v3` path
- **THEN** it SHALL send one-way `v3` data frames without waiting for
  per-frame request/reply completion
- **AND** it SHALL use bounded control polling to learn contiguous received
  frames, contiguous received bytes, and available credit

#### Scenario: Node-5 v3 resends only unacknowledged tail frames
- **WHEN** the node-`5` receiver reports no progress or a bounded ACK timeout
  occurs before the full stream is committed
- **THEN** the sender SHALL resume from the receiver-reported contiguous frame
  boundary
- **AND** it SHALL NOT resend already acknowledged prefix frames

#### Scenario: Node-5 v3 begin and commit are idempotent
- **WHEN** node `5` receives a duplicate `BEGIN` or duplicate `COMMIT` for the
  same `streamId`, `totalFrames`, and `totalBytes`
- **THEN** it SHALL return the cached acceptance result
- **AND** it SHALL NOT append duplicate bytes to the shared drain queue

#### Scenario: Node-5 v3 disconnect purges staged and committed queue state
- **WHEN** node `5` loses the external link or a drain write fails during a
  committed `v3` stream
- **THEN** it SHALL purge the active staging stream and committed drain queue
- **AND** it SHALL account for committed-but-undrained bytes as dropped
- **AND** it SHALL leave retry ownership to the existing stock upper-layer
  downlink path

### Requirement: Node-5 COMM CSP Downlink V3 Preserves Stock Official File Ownership
The comm subsystem SHALL keep the current official `DpCatalog ->
CommController -> FileDownlink -> CommEgressMux -> GroundLinkDriver` ownership
boundary while allowing node-`5` to use the `v3` bulk transport below that
boundary.

#### Scenario: Node-5 v3 does not replace FileDownlink ownership
- **WHEN** an official `.fdp` file is downlinked on the maintained node-`5`
  path
- **THEN** `DpCatalog`, `CommController`, stock `FileDownlink`, and
  `CommEgressMux` SHALL remain the selecting and packetizing owner surfaces
- **AND** `v3` SHALL operate strictly below `GroundLinkDriver.send()`

#### Scenario: Node-5 backend falls back directly from v3 to v1
- **WHEN** the OBC-side COMM CSP backend targets node `5`
- **AND** the bounded `v3` status probe fails or returns an invalid reply
- **THEN** the backend SHALL continue on the existing `v1` `DOWNLINK_WRITE`
  path
- **AND** it SHALL NOT require `v2` as an intermediate runtime fallback

## MODIFIED Requirements

### Requirement: Service-Managed Target S-band Uses Node 5
The comm subsystem SHALL support a service-managed target/lab S-band path where
`subsystem.local` hosts `sband_comm_csp_node` as COMM node `5` and
`obc.local` uses that path as the default active COMM profile.

#### Scenario: Target S-band service exposes node-5 v3 downlink services
- **WHEN** the target/lab S-band COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `sband_comm_csp_node` as COMM node `5`
- **AND** it SHALL expose the existing COMM services `30 UPLINK_POLL`,
  `31 DOWNLINK_WRITE`, `32 LINK_STATUS`, `34 DOWNLINK_STAGE_V2`,
  `35 DOWNLINK_STATUS_V2`, `36 DOWNLINK_ABORT_V2`, `37 RELIABLE_TRANSFER_DATA`,
  and `38 RELIABLE_TRANSFER_CONTROL`
- **AND** it SHALL additionally expose node-`5` downlink `v3` services
  `39 DOWNLINK_CONTROL_V3` and `40 DOWNLINK_DATA_V3`

#### Scenario: Governed target baseline scopes CAN FD to COMM V3 traffic
- **WHEN** the maintained target/lab COMM baseline is prepared for node-`5`
  V3 operation
- **THEN** the OBC, S-band node `5`, and UHF node `6` services SHALL enable
  SocketCAN CAN FD for destination nodes `5,6` and destination port `40`
- **AND** the baseline manager SHALL own installation, restart, and readiness
  verification of that profile
- **AND** the functional probe SHALL only verify and consume the profile
- **AND** EPS/ADCS services SHALL NOT receive the COMM CAN FD override
- **AND** this requirement SHALL NOT imply BRS or measured data-phase-rate
  closure
