## MODIFIED Requirements

### Requirement: Node-5 Residual Observability Evidence Distinguishes Truth, Reviewable Proof, And Diagnostics

The verification evidence baseline SHALL record the final node-`5` residual
observability classification so resource truth, reviewable proof surfaces, and
diagnostics-only residuals are not left ambiguous.

#### Scenario: Evidence records the formal resource truth boundary
- **WHEN** the residual cleanup evidence is written
- **THEN** it SHALL identify `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY` as the formal node-`5`
  resource keep-live truth
- **AND** it SHALL explicitly state that `SYS_MEM_RSS_MB` is current resident
  memory and that `SYS_RESOURCE_DEGRADED` / `SYS_LOW_MEMORY` are
  threshold-crossing warnings
- **AND** it SHALL explicitly state that `SystemResources.*` remains
  supplemental diagnostics-only live telemetry.

#### Scenario: Evidence records the formal reviewable proof surfaces that remain required
- **WHEN** the same evidence summarizes node-`5` transport and residual proof
  surfaces
- **THEN** it SHALL identify `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters as formal reviewable observability
- **AND** it SHALL keep those surfaces distinct from both pass-time keep-live
  truth and diagnostics-only residual chatter.

#### Scenario: Evidence separates ambient residual governance from explicit detailed GET requalification
- **WHEN** the same change also repairs representative detailed `GET_*`
  requalification
- **THEN** it SHALL keep that explicit bounded proof distinct from the ambient
  residual-governance inventory
- **AND** it SHALL not treat detailed `GET_*` closure as permission to reopen
  broad non-baseline live packet chatter.

#### Scenario: Hosted dual-GDS evidence proves the corrected RSS semantics
- **WHEN** the hosted manual dual-GDS headless path is rerun after this change
- **THEN** the evidence SHALL compare hosted `SYS_MEM_RSS_MB` with the
  OS-observed current `OBC` process RSS
- **AND** it SHALL show that normal hosted manual dual-GDS operation no longer
  produces spurious repeated `SYS_LOW_MEMORY` warnings from historical peak RSS
  growth alone.
