## ADDED Requirements

### Requirement: Physical Lab Serial Gateway Validation Is Staged
The ground TT&C gateway SHALL support a staged physical lab serial validation flow that separates gateway-to-COMM uplink ingress from full bidirectional TT&C downlink proof.

#### Scenario: Gateway physical ingress is tested before full TT&C
- **WHEN** the repository validates the gateway over the physical macOS-to-`subsystem.local` serial link
- **THEN** the probe SHALL first verify that gateway-origin command bytes traverse the serial ingress into COMM and produce bounded OBC readback

#### Scenario: Physical serial acquisition preamble is explicit
- **WHEN** the physical lab serial gateway validation requires a gateway-origin acquisition preamble
- **THEN** `ground_ttc_gateway` SHALL only emit that preamble when explicitly configured
- **AND** the probe evidence SHALL record the preamble line count and delay before claiming useful TT&C traffic

#### Scenario: Full TT&C claim requires event and telemetry visibility
- **WHEN** the staged probe also observes command events through `fprime-cli events` and telemetry through `fprime-cli channels`
- **THEN** the evidence MAY describe the result as bounded physical lab serial TT&C
- **AND** without those observations it SHALL NOT describe the result as full TT&C
