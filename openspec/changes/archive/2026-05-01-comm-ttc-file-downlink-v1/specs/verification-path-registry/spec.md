## ADDED Requirements

### Requirement: Physical Lab Serial COMM File Downlink Path Is Registered Separately
The verification-path registry SHALL register physical lab serial COMM file/downlink only after evidence proves that stock F' file downlink can traverse the gateway-backed physical COMM path and produce byte-matching files in the ground storage directory.

#### Scenario: Registry names the file/downlink boundary
- **WHEN** the physical lab serial COMM file/downlink probe passes
- **THEN** the registry SHALL identify the newly proven path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope is housekeeping archive index plus at least two slot files over the existing COMM TT&C path

#### Scenario: Registry keeps file downlink distinct from command/event/channel TT&C
- **WHEN** reviewers inspect the physical lab serial COMM file/downlink registry entry
- **THEN** the registry SHALL keep the bounded command/event/channel TT&C entry as a prerequisite and adjacent path
- **AND** it SHALL NOT treat command/event/channel visibility alone as proof of file/downlink behavior

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary file downlink, RF, target OBC migration, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, and COMM shared CAN FD participation remain unproven by that evidence
