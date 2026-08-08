## ADDED Requirements

### Requirement: Node-5 Live Visibility Keeps Keep-Live Truth, Reviewable Proof, And Diagnostics Separate

The onboard state and live-visibility documentation SHALL keep node-`5`
pass-time keep-live truth, reviewable proof / transport observability, bounded
fresh readback, and diagnostics-only residuals as distinct current surfaces.

#### Scenario: Node-5 live wording does not collapse reviewable proof into keep-live summary
- **WHEN** current docs describe node-`5` live operational visibility
- **THEN** they SHALL keep curated pass-time summary and operator-facing
  `CommController` state separate from reviewable transport, queue, owner, and
  egress proof surfaces
- **AND** they SHALL NOT imply that `GROUND_LINK_TX_BYTES`, `QueueOverflow`,
  or similar reviewable proof surfaces are part of the small pass-time summary

#### Scenario: Resource wording does not collapse supplemental SystemResources back into live truth
- **WHEN** the same docs describe current resource observability
- **THEN** they SHALL identify `SYS_*` as the formal node-`5` resource
  keep-live truth
- **AND** they SHALL keep `SystemResources.*` in the supplemental
  diagnostics-only bucket
