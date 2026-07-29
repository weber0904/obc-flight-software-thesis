## ADDED Requirements

### Requirement: Interface Index Records The Exact Node-5 Residual Inventory

`docs/interfaces.md` SHALL record the current node-`5` residual live inventory
with explicit owner/component mapping and final governance bucket for each
current surface under review.

#### Scenario: Reviewers can audit resource, transport, queue, owner, egress, and COMM residuals
- **WHEN** a reviewer inspects the node-`5` observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL identify the reviewed `SystemResources`,
  `GroundLinkDriver`, `GroundLinkHealthProvider`, `ComCcsds` / `OBCComCcsds`,
  `CspRuntimeOwner`, `CommEgressMux`, `UartDriver`, and `CommController`
  surfaces
- **AND** it SHALL state the final bucket, replacement surface if any, and the
  owner/component for each reviewed item

### Requirement: Interface Index Distinguishes Keep-Live Truth From Reviewable Proof Observability

`docs/interfaces.md` SHALL distinguish formal node-`5` pass-time truth from
formal reviewable proof / transport / policy observability instead of placing
both into the same live bucket.

#### Scenario: Reviewers can see which surfaces remain formal pass-time truth
- **WHEN** the index describes current node-`5` operator truth
- **THEN** it SHALL identify `SYS_*` resource keep-live truth and the
  operator-facing `CommController` transition/state surfaces that remain
  pass-time truth

#### Scenario: Reviewers can see which residual surfaces remain reviewable but not keep-live summary
- **WHEN** the index describes supporting node-`5` transport and policy
  evidence surfaces
- **THEN** it SHALL identify `GROUND_LINK_TX_BYTES`, transport-error growth,
  `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`, `CSP_OWNER_TIMEOUT`,
  `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band `CommEgressMux` counters as formal
  reviewable observability
- **AND** it SHALL keep the remaining residual surfaces diagnostics-only unless
  another governed change promotes them

#### Scenario: Reviewers can distinguish current exposure from implementation action
- **WHEN** the index records the current node-`5` residual inventory
- **THEN** each reviewed surface SHALL include the currently observed live
  exposure together with the intended implementation action
- **AND** that action SHALL distinguish `docs-only reclassification`,
  `proof/oracle repair`, `runtime behavior repair`, and explicit same-change
  deferral
