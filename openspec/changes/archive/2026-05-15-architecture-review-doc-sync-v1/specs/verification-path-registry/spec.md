## MODIFIED Requirements

### Requirement: Historical Physical Lab Serial COMM HK File Downlink Path Is Registered Separately
The verification-path registry SHALL preserve physical lab serial COMM HK
file/downlink as a historical, retired-fallback path only after evidence proves
that stock F' file downlink traversed the gateway-backed physical COMM path and
produced byte-matching files in the ground storage directory.

#### Scenario: Registry names the file/downlink boundary
- **WHEN** the physical lab serial COMM file/downlink probe passes
- **THEN** the registry SHALL identify the historical path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope was housekeeping archive index plus at least two slot files over the existing COMM TT&C path
- **AND** it SHALL state that this path is not the current official `.fdp` mission-history baseline

#### Scenario: Registry keeps file downlink distinct from command/event/channel TT&C
- **WHEN** reviewers inspect the physical lab serial COMM file/downlink registry entry
- **THEN** the registry SHALL keep the bounded command/event/channel TT&C entry as a prerequisite and adjacent path
- **AND** it SHALL NOT treat command/event/channel visibility alone as proof of file/downlink behavior

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary file downlink, RF, target OBC migration, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, and COMM shared CAN FD participation remain unproven by that evidence

### Requirement: Historical Physical COMM SocketCAN HK File Downlink Path Is Registered Separately
The verification-path registry SHALL preserve COMM SocketCAN HK file/downlink as
a historical, retired-fallback path only after evidence proves that stock F'
file downlink traversed COMM node `4` over `subsystem.local:can1` to target OBC
on `obc.local:can0` and produced byte-matching files in the GDS file-storage
directory.

#### Scenario: Registry names the COMM SocketCAN file boundary
- **WHEN** the COMM SocketCAN file/downlink probe passes
- **THEN** the registry SHALL identify the historical path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink over shared SocketCAN -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope was housekeeping archive index plus at least two slot files over the existing SocketCAN-backed COMM TT&C path
- **AND** it SHALL state that this path is not the current official `.fdp` mission-history baseline

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the COMM SocketCAN file/downlink entry
- **THEN** the registry SHALL keep physical lab serial COMM file/downlink and physical COMM SocketCAN command/event/channel TT&C as separate adjacent paths
- **AND** it SHALL NOT treat either adjacent path alone as proof of COMM SocketCAN file/downlink

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM SocketCAN file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary onboard file path downlink, RF, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, dual-bus redundancy, independent COMM hardware, and end-to-end missing-packet retransmission remain unproven by that evidence
