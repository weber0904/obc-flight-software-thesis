## ADDED Requirements

### Requirement: COMM File Downlink Evidence Is Reviewable
The verification evidence tree SHALL record the commands, endpoints, archive slot identifiers, received file paths, source file paths, byte-comparison results, and final verdict for COMM TT&C file/downlink validation.

#### Scenario: Hosted regression evidence is captured
- **WHEN** the hosted PTY COMM file/downlink probe passes
- **THEN** the evidence SHALL record the hosted GDS ports, runtime root, GDS file-storage directory, command sequence, selected archive files, and byte-comparison verdict

#### Scenario: Physical file evidence includes TT&C prerequisite
- **WHEN** the physical lab serial COMM file/downlink probe passes
- **THEN** the evidence SHALL include the command/event/channel TT&C prerequisite result before the file/downlink verdict
- **AND** it SHALL record host serial device, subsystem serial device, baudrate, CSP hub ports, GDS ports, preamble settings, and command pacing settings

#### Scenario: File comparison evidence is explicit
- **WHEN** the evidence describes a file/downlink PASS
- **THEN** it SHALL identify the downlinked `hk-index.csv` and at least two downlinked `hk-slot-*.bin` files
- **AND** it SHALL state that each received file was compared byte-for-byte against the OBC runtime source file

#### Scenario: Evidence keeps adjacent paths separate
- **WHEN** COMM file/downlink evidence is recorded
- **THEN** it SHALL cite the prior bounded command/event/channel TT&C evidence as a reused prerequisite
- **AND** it SHALL keep RF, target OBC migration, no-preamble first-byte-clean behavior, arbitrary file downlink, archive wraparound, ScenarioBridge, and COMM shared CAN FD outside the proven verdict
