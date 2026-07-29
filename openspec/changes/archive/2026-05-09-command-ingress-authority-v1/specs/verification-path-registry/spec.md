## ADDED Requirements

### Requirement: Hosted Command Ingress Authority Profile Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted command ingress authority profile proof path once `command-ingress-authority-v1` is verified.

#### Scenario: Registry identifies configured-profile authority boundary
- **WHEN** the hosted command ingress authority profile path is registered by `command-ingress-authority-v1`
- **THEN** the registry SHALL identify the path as the default hosted CCSDS S-band routed `Fw.Com` command path with explicit `CommandIngressAuthority` profiles
- **AND** it SHALL state that `sband-primary` and `uhf-backup` are configured OBC authority roles for the proof, not physical-link provenance inferred from `Fw.Com.context` or gateway metadata
- **AND** it SHALL cite the command ingress authority evidence record.

#### Scenario: Registry keeps authority scope bounded
- **WHEN** reviewers inspect the hosted command ingress authority profile registry entry
- **THEN** the entry SHALL state that the authority claim is limited to routed `Fw.Com` command packets before `Svc::CommandDispatcher`
- **AND** it SHALL NOT claim hosted UHF serial node-6 path authority, file packet uplink authority, unknown packet authority, full link authority, full uplink authority, crypto/session authentication, dynamic UHF primary failover, reliable transfer, target hardware, physical USB serial hardware, or physical RS485 electrical validation.
