## ADDED Requirements

### Requirement: Gateway Relays CCSDS Framed Bytes Transparently
The ground TT&C gateway SHALL support CCSDS spike compatibility only as transparent raw-byte relay between GDS and the S-band COMM endpoint.

#### Scenario: GDS uses CCSDS framing
- **WHEN** the CCSDS hosted proof runs
- **THEN** `fprime-gds` SHALL run with `--framing-selection space-packet-space-data-link`
- **AND** it SHALL use SCID `0x44`, VCID `1`, and TM frame size `1024`

#### Scenario: Gateway does not parse CCSDS
- **WHEN** CCSDS-framed traffic traverses `ground_ttc_gateway`
- **THEN** the gateway SHALL relay raw bytes between its northbound GDS TCP connection and southbound S-band TCP connection
- **AND** it SHALL NOT parse or validate CCSDS Space Packets, TC frames, TM frames, APIDs, SCID, VCID, sequence counts, commands, events, telemetry, or files

#### Scenario: Gateway verdict remains bounded
- **WHEN** gateway CCSDS compatibility is recorded
- **THEN** the verdict SHALL state that gateway compatibility proves transparent byte movement only
- **AND** it SHALL NOT claim gateway-level CCSDS semantic compatibility, reliable transfer, RF compatibility, command authority, or failover policy
