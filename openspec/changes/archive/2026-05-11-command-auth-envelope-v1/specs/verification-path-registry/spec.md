## ADDED Requirements

### Requirement: Hosted Command Ingress Authority Path Includes Authenticated Envelope Boundary

The verification-path registry SHALL extend the hosted command ingress authority profile path to distinguish authenticated envelope v1 from earlier authority, metadata, lifecycle, and sequence-only claims.

#### Scenario: Registry identifies authenticated ordering boundary
- **WHEN** the hosted command ingress authority profile path is updated by `command-auth-envelope-v1`
- **THEN** the entry SHALL state that the proved enveloped runtime order is `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
- **AND** it SHALL state that auth-failed traffic does not mutate session or sequence state
- **AND** it SHALL state that auth-pass but authority-denied traffic does not mutate lifecycle or sequence state.

#### Scenario: Registry keeps authenticated scope bounded
- **WHEN** reviewers inspect the hosted command ingress authority registry entry after this change
- **THEN** the entry SHALL state that the path proves authenticated ingress foundation on the current hosted authority ingress port `0`
- **AND** it SHALL state that the path still does not prove full replay protection, persistent secure session or key state, hosted ingress port `1`, simultaneous S-band/UHF routed ingress, file/unknown uplink authority, RF behavior, target hardware, or Pi deployment.
