## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries
`docs/interfaces.md` SHALL summarize the active baseline's command-envelope
surface, ingress roles and admission boundary, CCSDS framing identifiers and
APID mappings, rate-group timing profiles, official `.fdp` history boundary,
persistent fault readback surface, governed sequencing or resource surfaces,
the current `uhf-backup` versus `uhf-primary-after-failover` distinction, the
bounded ground-side retry boundary, `ground_ttc_gateway` non-claims, and the
live-versus-stored-versus-diagnostic observability split.

#### Scenario: Current command-session epoch contract is reviewable in one place
- **WHEN** a reviewer inspects the command-ingress section of
  `docs/interfaces.md`
- **THEN** the document SHALL describe active-path `session_id` as a monotonic
  reopen epoch per source epoch rather than as a runtime-only session label
- **AND** it SHALL state that the active ingress order remains
  `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
- **AND** it SHALL keep the bounded non-claims explicit, including no nonce
  replay window, no persistent secure key store, and no hardware-backed secure
  boot claim from this change

#### Scenario: COMM policy boundaries are reviewable in one place
- **WHEN** a reviewer inspects the COMM sections of `docs/interfaces.md`
- **THEN** they SHALL be able to see that accepted `SESSION_OPEN(seq0)` is the
  current UHF command-session policy boundary
- **AND** they SHALL be able to see that `ground_ttc_gateway` is not the
  authority owner, dual-link multiplexer, or reliable-transfer engine
- **AND** they SHALL be able to see which current surfaces are nominal live
  visibility, stored history, or diagnostics-only
