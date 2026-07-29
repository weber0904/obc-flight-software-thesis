## MODIFIED Requirements

### Requirement: Registry Distinguishes Current Secure Baseline From Legacy Compatibility Evidence

The verification-path registry SHALL keep historical legacy command-envelope
records reviewable without presenting them as the preferred current baseline.

#### Scenario: Legacy proof families are historical compatibility citations
- **WHEN** the registry mentions `command-envelope-metadata-v1`,
  `command-session-sequence-v1`, `command-session-lifecycle-v1`,
  `command-auth-envelope-v1`, or `persistent-command-freshness-v1`
- **THEN** it SHALL label them as historical compatibility evidence or exact
  later cleanup surfaces
- **AND** it SHALL NOT present them as the preferred current secure-command
  authority.
