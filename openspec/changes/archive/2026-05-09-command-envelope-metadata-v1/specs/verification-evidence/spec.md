## ADDED Requirements

### Requirement: Command Envelope Metadata Evidence Is Reviewable
The verification evidence SHALL prove command envelope metadata behavior without claiming active session enforcement or trusted source provenance.

#### Scenario: Unit and component tests prove envelope behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list helper and classic component tests covering legacy command compatibility, valid envelope unwrap, context preservation, envelope metadata observation, malformed envelope fail-closed behavior, and UHF backup denial of enveloped high-authority commands.

#### Scenario: Hosted proof covers the routed CCSDS command path
- **WHEN** hosted probe evidence is recorded
- **THEN** it SHALL show legacy command dispatch still works through `fprime-cli`
- **AND** it SHALL show a repo-owned envelope injector sending an enveloped command through the existing hosted CCSDS S-band routed command path
- **AND** it SHALL show envelope observed/rejected events from ground-side event capture.

#### Scenario: Evidence scope remains bounded
- **WHEN** evidence cites `command-envelope-metadata-v1`
- **THEN** it SHALL state that the proof covers current hosted authority ingress port `0` only
- **AND** it SHALL NOT claim active session enforcement, replay protection, authentication, crypto, physical UHF provenance, dual-link simultaneous proof, file authority, unknown packet authority, or reliable transfer.
