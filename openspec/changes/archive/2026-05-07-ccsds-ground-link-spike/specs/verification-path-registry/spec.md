## ADDED Requirements

### Requirement: CCSDS Path Registration Requires Passing Hosted Proof
The verification path registry SHALL register a reusable CCSDS hosted ground-link path only after the bounded S-band node `5` hosted proof passes.

#### Scenario: Passing proof registers CCSDS path
- **WHEN** the CCSDS hosted S-band node `5` proof passes command, event, telemetry, bounded file/downlink, and gateway raw-byte compatibility checks
- **THEN** the registry SHALL add a CCSDS hosted ground-link path that references the CCSDS spike evidence record
- **AND** the path SHALL name `space-packet-space-data-link`, SCID `0x44`, VCID `1`, TM frame size `1024`, S-band node `5`, and the CCSDS spike executable

#### Scenario: Failed proof does not register reusable path
- **WHEN** any CCSDS proof area fails or remains inconclusive
- **THEN** the registry SHALL NOT add a reusable CCSDS hosted ground-link path
- **AND** the evidence SHALL record blockers and the selected recommendation

#### Scenario: Existing paths remain distinct
- **WHEN** the registry is updated for this change
- **THEN** existing stock `ComFprime` S-band and UHF paths SHALL remain separately named
- **AND** the registry SHALL NOT merge direct TCP, S-band-through-COMM, UHF UART backup, and CCSDS hosted paths into a single validation path
