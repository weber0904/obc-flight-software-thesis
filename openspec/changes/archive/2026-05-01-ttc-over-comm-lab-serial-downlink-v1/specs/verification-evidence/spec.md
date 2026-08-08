## ADDED Requirements

### Requirement: Lab Serial Downlink Evidence Is Reviewable
The verification evidence tree SHALL record physical lab serial downlink evidence with a distinct verdict from the prior physical lab serial uplink-ingress-only record.

#### Scenario: Downlink evidence includes the physical path and prerequisite readback
- **WHEN** the physical lab serial downlink probe passes
- **THEN** reviewers SHALL be able to inspect the host serial device, subsystem serial device, baudrate, GDS ports, CSP hub ports, gateway launch, remote COMM node launch, OBC launch mode, commands issued, and OBC readback used as the prerequisite for the verdict

#### Scenario: Downlink evidence includes event and telemetry observations
- **WHEN** the evidence describes bounded physical lab serial TT&C
- **THEN** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Downlink evidence keeps adjacent paths separate
- **WHEN** the physical lab serial downlink evidence is recorded
- **THEN** it SHALL cite the prior uplink ingress record as a reused/supporting boundary rather than replacing it
- **AND** it SHALL keep file/downlink, RF, target OBC, no-preamble first-byte-clean behavior, and COMM shared CAN FD outside the proven verdict
