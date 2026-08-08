## ADDED Requirements

### Requirement: Node-5 Residual Observability Evidence Distinguishes Truth, Reviewable Proof, And Diagnostics

The verification evidence baseline SHALL record the final node-`5` residual
observability classification so resource truth, reviewable proof surfaces, and
diagnostics-only residuals are not left ambiguous.

#### Scenario: Evidence records the formal resource truth boundary
- **WHEN** the residual cleanup evidence is written
- **THEN** it SHALL identify `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY` as the formal node-`5`
  resource keep-live truth
- **AND** it SHALL explicitly state that `SystemResources.*` remains
  supplemental diagnostics-only live telemetry

#### Scenario: Evidence records the formal reviewable proof surfaces that remain required
- **WHEN** the same evidence summarizes node-`5` transport and residual proof
  surfaces
- **THEN** it SHALL identify `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters as formal reviewable observability
- **AND** it SHALL keep those surfaces distinct from both pass-time keep-live
  truth and diagnostics-only residual chatter

#### Scenario: Evidence separates ambient residual-governance proof from explicit detailed readback requalification
- **WHEN** the same change also repairs or requalifies representative detailed
  authenticated `GET_*` observation
- **THEN** the evidence SHALL record ambient post-auth keep-live/reviewable
  observations separately from the explicit bounded detailed readback checks
- **AND** it SHALL state whether the original drift was diagnosed as oracle,
  packetized path, or component-publication drift before closeout
