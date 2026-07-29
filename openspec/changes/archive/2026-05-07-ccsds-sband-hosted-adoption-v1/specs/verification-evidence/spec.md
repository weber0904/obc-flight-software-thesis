## ADDED Requirements

### Requirement: CCSDS Hosted Adoption Evidence Is Independent
The verification evidence baseline SHALL record default hosted S-band CCSDS adoption evidence independently from historical CCSDS spike evidence and existing stock `ComFprime` S-band and UHF gateway records.

#### Scenario: Evidence identifies default adoption path
- **WHEN** CCSDS hosted adoption evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> default hosted OBC`
- **AND** it SHALL identify existing S-band and UHF records as `ComFprime` gateway baselines, not CCSDS adoption evidence
- **AND** it SHALL identify `ccsds-ground-link-spike-v1` as historical spike evidence rather than the default hosted adoption evidence

#### Scenario: Evidence records framing and decoded APID observations
- **WHEN** CCSDS hosted adoption evidence is recorded
- **THEN** it SHALL record `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=1`, and `frame-size=1024`
- **AND** it SHALL record decoded frame/APID observations for command `0`, telemetry `1`, log/event `2`, and file `3`
- **AND** it SHALL record observed sequence counts where available from decoded frame inspection without treating those counts as reliable-transfer evidence

#### Scenario: Evidence records bounded TT&C and file proof
- **WHEN** CCSDS hosted adoption evidence records a PASS
- **THEN** it SHALL include bounded command readback for `EPS_SET_PDU` and `ADCS_SET_MODE`
- **AND** it SHALL include command event observations, `GROUND_LINK_TX_BYTES` telemetry observations, `HK_DOWNLINK_INDEX`, and at least two `HK_DOWNLINK_SLOT` file downlinks
- **AND** it SHALL state that received housekeeping files were compared byte-for-byte against OBC runtime source snapshots

#### Scenario: Evidence exclusions stay explicit
- **WHEN** CCSDS hosted adoption evidence is finalized
- **THEN** it SHALL state that the evidence does not prove UHF CCSDS, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, failover policy, pass scheduling, arbitrary onboard file downlink, or HK data-product field alignment

## REMOVED Requirements

### Requirement: CCSDS Spike Evidence Is Independent
**Reason**: The spike evidence remains historical, but new evidence for this change is default hosted S-band CCSDS adoption evidence.
**Migration**: Use `CCSDS Hosted Adoption Evidence Is Independent`.
