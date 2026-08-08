## ADDED Requirements

### Requirement: COMM TT&C File Downlink Proof
The comm subsystem SHALL provide a governed validation path that proves existing housekeeping archive file/downlink behavior over the COMM TT&C path without adding new COMM service ports, wire layouts, or generic arbitrary-file commands.

#### Scenario: File downlink uses existing housekeeping commands
- **WHEN** the COMM file/downlink probe runs
- **THEN** it SHALL use `HK_CAPTURE_NOW`, `HK_DOWNLINK_INDEX`, and `HK_DOWNLINK_SLOT`
- **AND** it SHALL NOT add a new operator command for arbitrary onboard file paths

#### Scenario: Multiple archive slots are downlinked
- **WHEN** the COMM file/downlink proof reaches its file verdict
- **THEN** the probe SHALL have produced at least two occupied housekeeping archive slots through paced ground commands over the same GDS/COMM path
- **AND** it SHALL downlink the archive index plus at least two slot files

#### Scenario: Received files match OBC runtime sources
- **WHEN** housekeeping archive files are received by the ground-side file-storage directory
- **THEN** the probe SHALL compare the received index and slot files against the OBC runtime source files
- **AND** the formal file/downlink verdict SHALL require byte-for-byte matches for the selected files

#### Scenario: COMM service contract remains unchanged
- **WHEN** the COMM file/downlink probe runs
- **THEN** COMM SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** COMM file/downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim RF behavior, target OBC migration, no-preamble first-byte-clean behavior, ScenarioBridge pass state, link availability policy, arbitrary file downlink, archive generation wraparound, or COMM shared CAN FD participation
