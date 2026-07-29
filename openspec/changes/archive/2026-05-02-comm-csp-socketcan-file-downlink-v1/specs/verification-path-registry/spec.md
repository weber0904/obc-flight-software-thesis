## ADDED Requirements

### Requirement: Physical COMM SocketCAN File Downlink Path Is Registered Separately
The verification-path registry SHALL register COMM SocketCAN file/downlink as a distinct path only after evidence proves that stock F' file downlink can traverse COMM node `4` over `subsystem.local:can1` to target OBC on `obc.local:can0` and produce byte-matching files in the GDS file-storage directory.

#### Scenario: Registry names the COMM SocketCAN file boundary
- **WHEN** the COMM SocketCAN file/downlink probe passes
- **THEN** the registry SHALL identify the newly proven path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink over shared SocketCAN -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope is housekeeping archive index plus at least two slot files over the existing SocketCAN-backed COMM TT&C path

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the COMM SocketCAN file/downlink entry
- **THEN** the registry SHALL keep physical lab serial COMM file/downlink and physical COMM SocketCAN command/event/channel TT&C as separate adjacent paths
- **AND** it SHALL NOT treat either adjacent path alone as proof of COMM SocketCAN file/downlink

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM SocketCAN file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary onboard file path downlink, RF, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, dual-bus redundancy, independent COMM hardware, and end-to-end missing-packet retransmission remain unproven by that evidence
