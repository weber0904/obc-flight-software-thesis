## ADDED Requirements

### Requirement: Registry Records Residual-Observability Buckets On The Node-5 Proof Paths

The verification-path registry SHALL describe the hosted and target node-`5`
observability paths using explicit resource-truth, reviewable proof, and
diagnostics-only residual boundaries.

#### Scenario: Hosted and target node-5 registry entries keep resource truth explicit
- **WHEN** reviewers inspect the current hosted or target node-`5`
  observability-governance entries
- **THEN** the registry SHALL identify `SYS_*` as the formal node-`5`
  resource keep-live truth
- **AND** it SHALL identify `SystemResources.*` as supplemental
  diagnostics-only live telemetry

#### Scenario: Hosted and target node-5 registry entries keep reviewable transport and proof surfaces explicit
- **WHEN** the same entries describe the supported reviewable path
- **THEN** they SHALL keep `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters in the formal reviewable scope
- **AND** they SHALL keep diagnostics-only residual chatter outside the formal
  operator baseline without claiming it has been fully removed from runtime
  output

#### Scenario: Detailed GET requalification keeps the same path identity
- **WHEN** the same hosted or target node-`5` observability path is rebuilt to
  requalify representative authenticated detailed `GET_*` readback
- **THEN** the registry SHALL keep the existing path identity and wrapper name
  instead of inventing a parallel proof family
- **AND** it SHALL describe the detailed readback work as same-change
  requalification on top of the current residual-governance path
