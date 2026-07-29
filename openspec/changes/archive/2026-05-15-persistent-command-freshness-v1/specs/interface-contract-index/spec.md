## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries
`docs/interfaces.md` SHALL summarize the active baseline's command-envelope
surface, ingress roles and admission boundary, CCSDS framing identifiers and
APID mappings, rate-group timing profiles, official `.fdp` history boundary,
persistent fault readback surface, and governed sequencing/resource surfaces.

#### Scenario: Current command-session epoch contract is reviewable in one place
- **WHEN** a reviewer inspects the command-ingress section of `docs/interfaces.md`
- **THEN** the document SHALL describe active-path `session_id` as a monotonic
  reopen epoch per source epoch rather than as a runtime-only session label
- **AND** it SHALL state that the active ingress order remains
  `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
- **AND** it SHALL keep the bounded non-claims explicit, including no nonce
  replay window, no persistent secure key store, and no hardware-backed secure
  boot claim from this change.
