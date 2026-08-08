## ADDED Requirements

### Requirement: Published Verification Entrypoints Are Manifest Governed
Every source executable SHALL have a publication status, and every shipped
verification entrypoint SHALL cite a current registry path or operator surface.

#### Scenario: Script inventory is checked
- **WHEN** the source executable inventory and public tree are compared
- **THEN** maintained and support/internal scripts SHALL be the only shipped
  executables
- **AND** historical, retired, deprecated, and alias-only scripts SHALL be
  absent

### Requirement: Registry Exposes Stable Current And Historical Status
The public registry SHALL provide a concise current-path index while retaining
historical entries under stable identifiers and explicit status.

#### Scenario: Reviewer selects a validation path
- **WHEN** a reviewer looks up a capability
- **THEN** the maintained path, environment, proof owner, evidence record, and
  non-claims SHALL be discoverable without selecting a historical wrapper
