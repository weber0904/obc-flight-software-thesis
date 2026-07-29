## MODIFIED Requirements

### Requirement: Hosted Command Ingress Authority Profile Path Is Registered

The verification-path registry SHALL identify the hosted command ingress authority profile proof path and distinguish configured authority, envelope metadata, and active envelope sequence-enforcement evidence from broader link-security claims.

#### Scenario: Registry identifies active envelope sequence-enforcement boundary
- **WHEN** the hosted command ingress authority profile path is updated by `command-session-sequence-v1`
- **THEN** the entry SHALL state that the path proves active duplicate/lower sequence rejection only for valid command envelope v1 packets on current authority ingress port `0`
- **AND** it SHALL state that legacy non-envelope commands remain supported and are not sequence gated
- **AND** it SHALL state that authority-denied envelopes do not consume sequence state.
