## ADDED Requirements

### Requirement: COMM SocketCAN File Downlink Validation
The comm subsystem SHALL provide a governed validation path where existing housekeeping archive file/downlink commands traverse COMM node `4` over the shared SocketCAN carrier to target OBC and return received files to GDS file storage.

#### Scenario: File downlink uses existing COMM SocketCAN path
- **WHEN** the COMM SocketCAN file/downlink probe runs
- **THEN** `comm_csp_node` SHALL run as COMM node `4` through `CSP_TRANSPORT=socketcan` on `subsystem.local:can1`
- **AND** target OBC SHALL run on `obc.local:can0` with `GROUND_LINK_MODE=comm-csp`
- **AND** EPS and ADCS SHALL remain reachable through `subsystem.local:can0`

#### Scenario: Existing file command surface is reused
- **WHEN** housekeeping archive file/downlink is validated over COMM SocketCAN
- **THEN** the path SHALL use `HK_CAPTURE_NOW`, `HK_DOWNLINK_INDEX`, and `HK_DOWNLINK_SLOT`
- **AND** the change SHALL NOT add a generic arbitrary-file command, COMM service port, or wire layout

#### Scenario: File packets stay bounded for the physical COMM carrier
- **WHEN** housekeeping archive file/downlink is validated over COMM SocketCAN
- **THEN** the project SHALL keep stock F' `FileDownlink` while bounding `FW_FILE_BUFFER_MAX_SIZE` so file data packets remain below the generic COM buffer size for the constrained physical COMM path
- **AND** the verdict SHALL still require byte-matched files instead of treating bounded command success as sufficient proof

#### Scenario: File verdict requires byte matches
- **WHEN** `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` complete over the SocketCAN-backed COMM path
- **THEN** GDS file storage SHALL contain `hk-index.csv` and at least two selected `hk-slot-*.bin` files
- **AND** each received file SHALL match its target OBC runtime source snapshot byte-for-byte

#### Scenario: Adjacent futures stay out of the verdict
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim arbitrary onboard file path downlink, RF behavior, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge/pass automation, dual-bus redundancy, independent COMM hardware beyond the `subsystem.local:can1` controller, or end-to-end missing-packet retransmission under packet loss
