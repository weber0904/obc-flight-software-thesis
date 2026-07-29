## ADDED Requirements

### Requirement: Ground TT&C Evidence Keeps S-band And UHF COMM Policy Paths Distinct

The ground TT&C gateway documentation and evidence model SHALL keep the hosted S-band node-`5` and hosted UHF node-`6` COMM policy paths distinct even when one governed change proves them together as part of a COMM policy runtime closure.

#### Scenario: Command-policy evidence names the ingress boundary

- **WHEN** the COMM session-and-downlink QoS hosted probe records a command-policy verdict
- **THEN** the evidence SHALL identify whether the command traversed the S-band node-`5` or UHF node-`6` gateway boundary
- **AND** it SHALL not restate that verdict as a generic single-link ground path

#### Scenario: File/downlink evidence names the egress boundary

- **WHEN** the COMM session-and-downlink QoS hosted probe records a file/downlink verdict
- **THEN** the evidence SHALL identify the active primary file-transfer link used for that transfer
- **AND** it SHALL keep S-band and UHF file/downlink proof language separate
