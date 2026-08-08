## ADDED Requirements

### Requirement: Default CCSDS S-band Path Registration Requires Passing Hosted Adoption Proof
The verification path registry SHALL register a reusable default hosted CCSDS S-band ground-link path only after the hosted adoption proof passes.

#### Scenario: Passing adoption proof registers default CCSDS path
- **WHEN** the default CCSDS hosted S-band node `5` adoption proof passes command, event, telemetry, bounded file/downlink, gateway raw-byte compatibility, and decoded frame/APID observation checks
- **THEN** the registry SHALL add a default hosted CCSDS S-band ground-link path that references the `ccsds-sband-hosted-adoption-v1` evidence record
- **AND** the path SHALL name `space-packet-space-data-link`, SCID `0x44`, VCID `1`, TM frame size `1024`, S-band node `5`, and the default hosted `OBC` executable

#### Scenario: Failed adoption proof does not register reusable path
- **WHEN** any CCSDS adoption proof area fails or remains inconclusive
- **THEN** the registry SHALL NOT add a reusable default CCSDS hosted ground-link path
- **AND** the evidence SHALL record blockers instead of claiming adoption

#### Scenario: Existing paths remain distinct
- **WHEN** the registry is updated for this change
- **THEN** existing stock `ComFprime` S-band and UHF paths SHALL remain separately named
- **AND** historical `ccsds-ground-link-spike-v1` evidence SHALL remain distinct from default hosted adoption evidence
- **AND** the registry SHALL NOT merge direct TCP, S-band-through-COMM, UHF UART backup, spike CCSDS, and default CCSDS hosted paths into a single validation path

## REMOVED Requirements

### Requirement: CCSDS Path Registration Requires Passing Hosted Proof
**Reason**: The spike path registration rule is superseded by a default hosted adoption registration rule.
**Migration**: Use `Default CCSDS S-band Path Registration Requires Passing Hosted Adoption Proof`.
