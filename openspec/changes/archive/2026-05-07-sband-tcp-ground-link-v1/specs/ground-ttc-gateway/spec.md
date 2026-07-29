## ADDED Requirements

### Requirement: Gateway Supports S-band TCP Southbound Segment
The ground TT&C gateway SHALL support a hosted S-band simulated TCP southbound segment while keeping stock F' framing toward `fprime-gds`.

#### Scenario: Gateway connects to S-band TCP COMM endpoint
- **WHEN** the hosted S-band TCP ground-link probe runs
- **THEN** `ground_ttc_gateway` SHALL connect southbound to the configured S-band TCP endpoint owned by `sband_comm_csp_node`
- **AND** it SHALL keep the northbound GDS connection as stock F' framing

#### Scenario: Gateway evidence separates GDS TCP from S-band TCP
- **WHEN** S-band TCP gateway evidence is recorded
- **THEN** it SHALL record the GDS IP/TTS ports separately from the S-band simulated TCP endpoint
- **AND** it SHALL not describe direct `GDS -> TCP -> OBC` as the S-band-through-COMM verdict

### Requirement: Gateway-Backed S-band TCP File Downlink Validation
The ground TT&C gateway SHALL support bounded housekeeping archive file/downlink validation over the hosted S-band TCP COMM path.

#### Scenario: File proof follows S-band TT&C readiness
- **WHEN** the hosted S-band TCP file/downlink probe runs
- **THEN** it SHALL first verify bounded command/event/channel TT&C over the same S-band TCP path before claiming file/downlink success

#### Scenario: Ground storage receives bounded archive files
- **WHEN** `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` are sent through the S-band TCP COMM path
- **THEN** the configured GDS file-storage directory SHALL receive the housekeeping index and at least two selected slot files

#### Scenario: S-band file evidence records byte matches
- **WHEN** hosted S-band TCP file/downlink evidence is recorded
- **THEN** it SHALL record source snapshots, received file paths, byte counts, and byte-comparison verdicts for `hk-index.csv` and the selected `hk-slot-*.bin` files
- **AND** the verdict SHALL NOT claim arbitrary file downlink or reliable retransmission under packet loss
