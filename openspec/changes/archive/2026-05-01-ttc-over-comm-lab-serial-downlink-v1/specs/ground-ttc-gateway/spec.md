## ADDED Requirements

### Requirement: Physical Lab Serial Gateway Downlink Validation
The ground TT&C gateway SHALL support a focused physical lab serial validation flow where bounded downlink proof is registered only after gateway-mediated physical serial uplink has produced OBC command readback and ground-side tooling observes bounded event and telemetry output.

#### Scenario: Gateway downlink proof follows physical ingress
- **WHEN** the repository validates the gateway over the physical macOS-to-`subsystem.local` serial link
- **THEN** the downlink-focused probe SHALL first verify that gateway-origin command bytes traverse the serial ingress into COMM and produce bounded OBC readback

#### Scenario: Full physical TT&C claim requires ground-side observations
- **WHEN** the physical lab serial downlink probe observes both command events through `fprime-cli events` and telemetry through `fprime-cli channels`
- **THEN** the evidence MAY describe the result as bounded physical lab serial TT&C
- **AND** without those observations it SHALL NOT describe the result as full physical lab serial TT&C

#### Scenario: Gateway framing and preamble behavior stay explicit
- **WHEN** the physical lab serial gateway validation uses a gateway-origin acquisition preamble
- **THEN** `ground_ttc_gateway` SHALL emit that preamble only when explicitly configured
- **AND** the downlink evidence SHALL record the preamble line count and delay before claiming useful TT&C traffic
