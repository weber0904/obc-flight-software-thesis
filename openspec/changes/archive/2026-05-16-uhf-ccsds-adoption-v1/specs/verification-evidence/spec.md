## ADDED Requirements

### Requirement: Active UHF CCSDS Adoption Evidence Is Independent
The verification evidence baseline SHALL record active hosted UHF CCSDS adoption evidence independently from historical UHF `ComFprime` gateway evidence and from the default hosted S-band CCSDS adoption evidence.

#### Scenario: Evidence identifies the active UHF CCSDS path
- **WHEN** active UHF CCSDS adoption evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> default hosted OBC`
- **AND** it SHALL state that the bounded proof traffic occurs only after the active COMM runtime explicitly switches the command, telemetry, and file roles to UHF on the default hosted `OBC` baseline
- **AND** it SHALL identify historical UHF records as `ComFprime` gateway baselines or regression evidence rather than active UHF CCSDS adoption evidence

#### Scenario: Evidence records framing observations
- **WHEN** active UHF CCSDS adoption evidence is recorded
- **THEN** it SHALL record `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=2`, and `frame-size=1024`
- **AND** it SHALL record decoded APID observations for command `0`, telemetry `1`, log/event `2`, and file `3`

#### Scenario: Evidence records bounded UHF behavior
- **WHEN** active UHF CCSDS adoption evidence records a PASS
- **THEN** it SHALL include bounded command readback, command event observations, telemetry observations, and bounded UHF file/downlink byte-match on the active hosted `OBC` path after explicit switch to UHF primary

#### Scenario: Evidence exclusions stay explicit
- **WHEN** active UHF CCSDS adoption evidence is finalized
- **THEN** it SHALL state that the evidence does not prove RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, broader UHF authority expansion, or historical legacy retirement

## MODIFIED Requirements

### Requirement: CCSDS Hosted Adoption Evidence Is Independent
The verification evidence baseline SHALL record default hosted S-band CCSDS adoption evidence independently from historical CCSDS spike evidence, historical UHF `ComFprime` gateway records, and active UHF CCSDS adoption evidence.

#### Scenario: Evidence identifies default adoption path
- **WHEN** CCSDS hosted S-band adoption evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> default hosted OBC`
- **AND** it SHALL identify active UHF CCSDS evidence as a separate node `6` record rather than folding it into the S-band adoption record
- **AND** it SHALL identify existing historical UHF records as `ComFprime` gateway baselines or regression evidence

#### Scenario: Evidence exclusions stay explicit
- **WHEN** CCSDS hosted S-band adoption evidence is finalized
- **THEN** it SHALL state that the evidence does not prove UHF CCSDS outside the separate node `6` adoption record, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, failover policy, pass scheduling, arbitrary onboard file downlink, or HK data-product field alignment
