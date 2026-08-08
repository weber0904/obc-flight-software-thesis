## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the active handshake secure-command
boundary in addition to the UHF primary packet-quiet and beacon suppress
runtime contracts.

#### Scenario: Reviewers can audit the new secure auth boundary in one place
- **WHEN** a reviewer inspects the COMM and command-security sections of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that the handshake family uses APID
  `0x00FE`
- **AND** they SHALL be able to see that `ServiceID = 1` maps to S-band and
  `ServiceID = 2` maps to UHF
- **AND** they SHALL be able to see that accepted UHF auth completion, not
  wire-level `SESSION_OPEN`, is the new secure-session suppress-start boundary
  for the active secure path.

#### Scenario: Reviewers can audit legacy and secure paths separately
- **WHEN** reviewers inspect the command ingress boundary in `docs/interfaces.md`
- **THEN** they SHALL be able to see that legacy command envelope v1 still uses
  `source_id`, `key_slot`, `session_id`, and explicit `SESSION_OPEN`
- **AND** they SHALL be able to see that secure command v2 omits those fields
  and relies on prior auth grant plus per-session `sequence_number`.

### Requirement: Interface Index Records Current COMM Observability Semantics

`docs/interfaces.md` SHALL describe the new secure auth/session observability
surface with owner, freshness, timeout, and invalidation semantics.

#### Scenario: Reviewers can audit secure auth observability
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see the active secure service ID, suppress
  owner role, last accepted secure sequence, and inactivity timeout semantics
- **AND** they SHALL be able to see that UHF backup and failover-primary share
  the same secure service while remaining distinct runtime roles.
