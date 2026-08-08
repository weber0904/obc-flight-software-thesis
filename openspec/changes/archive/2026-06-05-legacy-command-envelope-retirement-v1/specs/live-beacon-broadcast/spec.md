## MODIFIED Requirements

### Requirement: Current Beacon Suppress Wording Prefers Secure Session Ownership

The current baseline SHALL describe UHF beacon suppress ownership in terms of
secure-auth session ownership, while any retained legacy `SESSION_OPEN`
boundary remains compatibility-only wording.

#### Scenario: Current wording does not reuse legacy as the preferred rule
- **WHEN** docs or specs summarize beacon suppress start or refresh behavior
- **THEN** they SHALL describe accepted secure-auth session ownership as the
  preferred baseline
- **AND** any retained legacy `SESSION_OPEN(seq0)` wording SHALL be explicitly
  labeled compatibility-only or follow-up blocker scope.
