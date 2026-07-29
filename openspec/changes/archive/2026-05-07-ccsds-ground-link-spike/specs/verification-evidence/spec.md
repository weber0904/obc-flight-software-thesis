## ADDED Requirements

### Requirement: CCSDS Spike Evidence Is Independent
The verification evidence baseline SHALL record CCSDS spike evidence independently from existing stock `ComFprime` S-band and UHF gateway records.

#### Scenario: Evidence identifies CCSDS proof path
- **WHEN** CCSDS spike evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC CCSDS spike executable`
- **AND** it SHALL identify existing S-band and UHF records as `ComFprime` gateway baselines, not CCSDS evidence

#### Scenario: Evidence records framing and APID observations
- **WHEN** CCSDS spike evidence is recorded
- **THEN** it SHALL record `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=1`, and `frame-size=1024`
- **AND** it SHALL record APID observations for command `0`, telemetry `1`, log/event `2`, and file `3`, including sequence-count observations where available from logs or decoded frame inspection

#### Scenario: Evidence records bounded TT&C and file proof
- **WHEN** CCSDS spike evidence records a PASS
- **THEN** it SHALL include bounded command readback for `EPS_SET_PDU` and `ADCS_SET_MODE`
- **AND** it SHALL include command event observations, `GROUND_LINK_TX_BYTES` telemetry observations, `HK_DOWNLINK_INDEX`, and at least two `HK_DOWNLINK_SLOT` file downlinks
- **AND** it SHALL state that received housekeeping files were compared byte-for-byte against OBC runtime source snapshots

#### Scenario: Evidence records recommendation and blockers
- **WHEN** CCSDS spike evidence is finalized
- **THEN** it SHALL record one recommendation: `adopt now`, `defer with blockers`, or `keep ComFprime with CCSDS-aligned semantics`
- **AND** if any proof area fails, it SHALL list blockers instead of registering a reusable CCSDS path
