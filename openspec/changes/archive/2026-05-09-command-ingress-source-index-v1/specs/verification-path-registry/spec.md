## MODIFIED Requirements

### Requirement: Registry Includes Hosted Command Ingress Authority Profile Path
The verification-path registry SHALL include a distinct entry for the hosted command ingress authority profile proof path once `command-ingress-authority-v1` is verified.

#### Scenario: Registry identifies configured source-index boundary
- **WHEN** the hosted command ingress authority profile path is updated by `command-ingress-source-index-v1`
- **THEN** the entry SHALL state that current hosted default CCSDS and legacy ComFprime topologies wire authority ingress index `0` only
- **AND** it SHALL state that source identity is configured by authority ingress port index
- **AND** it SHALL state that `COMMAND_AUTHORITY_REJECTED` evidence includes ingress port and link identity fields.

#### Scenario: Registry keeps source-index proof bounded
- **WHEN** reviewers inspect the hosted command ingress authority profile registry entry
- **THEN** the entry SHALL state that hosted proof for authority ingress port `1` and simultaneous S-band/UHF routed command ingress remain out of scope
- **AND** it SHALL continue to reject claims of trusted source, per-packet provenance, physical UHF provenance, full link authority, full uplink authority, crypto authentication, active session enforcement, or replay protection.
