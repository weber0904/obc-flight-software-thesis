## ADDED Requirements

### Requirement: Node-5 COMM CSP Downlink V2 Stages Before Drain
The comm subsystem SHALL provide a node-`5`-only parallel downlink transport
`v2` below stock `FileDownlink` that stages one upper-layer send stream in
memory, commits the full stream atomically into a bounded drain queue, and
drains committed bytes to the external link asynchronously.

#### Scenario: Node-5 v2 stages and commits one stream atomically
- **WHEN** target OBC sends one file/downlink buffer to COMM node `5` through
  the `v2` path
- **THEN** non-final chunks SHALL enter a bounded private staging stream only
- **AND** the shared drain queue SHALL remain unchanged until the final chunk is
  accepted
- **AND** the final chunk SHALL commit the full staged stream into the shared
  drain queue atomically before success is reported upstream

#### Scenario: Node-5 v2 retries final commit without replaying staged chunks
- **WHEN** node `5` has already staged non-final chunks for one active stream
- **AND** the final chunk cannot commit because drain credit is temporarily
  exhausted
- **THEN** node `5` SHALL return a bounded no-credit result without discarding
  the staged stream
- **AND** the sender SHALL retry only the final chunk until commit succeeds or
  the bounded wait expires

#### Scenario: Node-5 v2 rejects malformed or out-of-order traffic without mutation
- **WHEN** node `5` receives a `v2` stage request with the wrong stream id,
  wrong chunk sequence, invalid first/last shape, or an unexpected new-stream
  sequence
- **THEN** it SHALL reject the request as invalid
- **AND** it SHALL NOT mutate staged bytes, committed drain bytes, or queue
  accounting

#### Scenario: Node-5 v2 abort and timeout clear only uncommitted staging
- **WHEN** the sender aborts an active staged stream or node `5` reaches the
  bounded staging timeout without final commit
- **THEN** node `5` SHALL clear only the uncommitted staged stream
- **AND** it SHALL preserve already committed drain bytes
- **AND** it SHALL make the stage credit available for the next stream

#### Scenario: Node-5 v2 external-link failure purges committed bytes explicitly
- **WHEN** node `5` loses the external link or encounters a write failure while
  draining committed `v2` bytes
- **THEN** it SHALL purge the staged stream and committed drain queue
- **AND** it SHALL account for committed-but-undrained bytes as dropped
- **AND** it SHALL leave retry ownership to the existing stock upper-layer
  file/downlink path

### Requirement: Node-5 COMM CSP Downlink V2 Preserves Stock Upper-Layer Ownership
The comm subsystem SHALL keep the current official `DpCatalog -> CommController
-> FileDownlink` ownership boundary while allowing the node-`5` COMM CSP
backend to probe and use the downlink transport `v2` path.

#### Scenario: Node-5 transport uplift does not replace stock file owner chain
- **WHEN** an official `.fdp` file is downlinked on the current node-`5`
  maintained path
- **THEN** `DpCatalog`, `CommController`, and stock `FileDownlink` SHALL remain
  the selecting, scheduling, and file-packet ownership surfaces
- **AND** the `v2` transport SHALL operate strictly below that ownership
  boundary

#### Scenario: Node-5 backend falls back cleanly to v1
- **WHEN** the OBC-side COMM CSP backend targets node `5`
- **AND** the bounded `v2` status probe fails or returns an invalid reply
- **THEN** the backend SHALL emit a bounded fallback diagnostic
- **AND** it SHALL continue the send on the existing `v1` `DOWNLINK_WRITE`
  transport without widening scope to other nodes

## MODIFIED Requirements

### Requirement: Service-Managed Target S-band Uses Node 5
The comm subsystem SHALL support a service-managed target/lab S-band path where `subsystem.local` hosts `sband_comm_csp_node` as COMM node `5` and `obc.local` uses that path as the default active COMM profile.

#### Scenario: Target S-band service owns node 5
- **WHEN** the target/lab S-band COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `sband_comm_csp_node` as COMM node `5`
- **AND** it SHALL expose the configured TCP listener plus the existing COMM
  services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** it SHALL additionally expose node-`5`-only downlink transport `v2`
  services `34 DOWNLINK_STAGE_V2`, `35 DOWNLINK_STATUS_V2`, and
  `36 DOWNLINK_ABORT_V2`

#### Scenario: Target OBC default path uses node 5
- **WHEN** the target/lab default COMM profile runs
- **THEN** target OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`
