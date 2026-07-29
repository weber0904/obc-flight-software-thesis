## ADDED Requirements

### Requirement: Gateway-Backed COMM SocketCAN File Downlink Validation
The ground TT&C gateway SHALL support governed file/downlink validation where stock F' file downlink traffic traverses lab serial ingress into COMM node `4`, reaches target OBC over the shared SocketCAN carrier, and returns files to the configured GDS file-storage directory.

#### Scenario: File proof follows SocketCAN TT&C readiness
- **WHEN** the COMM SocketCAN file/downlink probe runs
- **THEN** it SHALL first verify gateway-backed command/event/channel TT&C readiness over the same SocketCAN-backed COMM path before claiming file/downlink success

#### Scenario: Ground storage receives bounded archive files
- **WHEN** `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` are sent through the SocketCAN-backed COMM path
- **THEN** the configured GDS file-storage directory SHALL receive the housekeeping index and selected slot files

#### Scenario: Gateway accepts bounded transfer retries
- **WHEN** a received file does not byte-match its target OBC runtime source snapshot during bounded SocketCAN-backed file/downlink validation
- **THEN** the probe MAY retry the same existing `HK_DOWNLINK_*` command within the configured attempt limit
- **AND** success SHALL be claimed only for the final received file that byte-matches the snapshot

#### Scenario: Gateway retry boundary stays whole-command only
- **WHEN** SocketCAN-backed COMM file/downlink validation uses bounded retries
- **THEN** those retries SHALL be treated as whole-command retries
- **AND** the verdict SHALL NOT claim missing file-packet retransmission, NACK/ARQ recovery, or reliable file transfer under packet loss

#### Scenario: Gateway file evidence records SocketCAN transport settings
- **WHEN** the SocketCAN-backed COMM file/downlink evidence is recorded
- **THEN** it SHALL record gateway serial endpoint settings, preamble settings, GDS ports, GDS file-storage directory, COMM node id, CAN interface mapping, CAN timing, and selected slot identifiers

#### Scenario: Gateway contract remains stock F' northbound
- **WHEN** SocketCAN-backed file/downlink validation runs
- **THEN** the gateway SHALL keep stock F' framing toward `fprime-gds`
- **AND** the change SHALL NOT require a custom GDS communication plugin
