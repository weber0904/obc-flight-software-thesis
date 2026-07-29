## ADDED Requirements

### Requirement: Node-5 Residual Observability Buckets Stay Explicit

The comm subsystem SHALL distinguish node-`5` post-auth pass-time keep-live
truth, reviewable transport/policy proof observability, bounded fresh
readback, and diagnostics-only residual live chatter instead of treating the
entire residual surface as one undifferentiated `non-baseline live` bucket.

#### Scenario: Resource truth does not fall back to SystemResources
- **WHEN** current docs or runbooks describe node-`5` post-auth resource truth
- **THEN** they SHALL use `WatchdogSupervisor` `SYS_*` surfaces as the formal
  resource keep-live truth
- **AND** they SHALL treat `SystemResources.*` as supplemental diagnostics-only
  live telemetry rather than pass-time operator truth

#### Scenario: Transport reviewable surfaces remain formal without becoming keep-live summary
- **WHEN** current docs, runbooks, or verification artifacts describe node-`5`
  transport facts
- **THEN** `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error
  growth, and `GROUND_LINK_HEALTH_S_BAND_*` SHALL remain formal reviewable
  observability
- **AND** those surfaces SHALL NOT be reclassified as the pass-time keep-live
  summary

#### Scenario: Queue and owner residuals are not flattened into one label
- **WHEN** current docs or evidence describe node-`5` queue and owner
  residuals
- **THEN** `QueueOverflow`, `CSP_OWNER_TIMEOUT`, and
  `CSP_OWNER_TOTAL_TIMEOUTS` SHALL remain formal reviewable proof surfaces
- **AND** queue-depth, owner-success, UART, and similar low-level counters
  SHALL remain diagnostics-only unless a later governed change promotes them

### Requirement: Node-5 COMM Policy Truth Stays Separate From Raw Transport And COMM Internals

The comm subsystem SHALL keep current node-`5` operator-facing policy truth,
reviewable policy observability, and diagnostics-only COMM internals as
separate layers.

#### Scenario: Policy-facing state remains formal pass-time truth
- **WHEN** current docs or proofs describe node-`5` live operator truth
- **THEN** `COMM_BAND_SWITCH`, `COMM_PRIMARY_LINK_CHANGED`,
  `COMM_LINK_AVAILABILITY_CHANGED`, `COMM_DOWNLINK_STATE_CHANGED`,
  `COMM_RECOVERY_FAILOVER_RESULT`, and
  `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED` SHALL remain formal pass-time
  truth
- **AND** current primary-band state surfaces SHALL remain aligned with that
  operator-facing truth

#### Scenario: Supporting counters stay reviewable without promoting reliable-transfer internals
- **WHEN** current docs or proofs describe node-`5` supporting COMM counters
- **THEN** availability, activity-age, downlink-owner, and failover counters
  SHALL remain reviewable policy observability
- **AND** reliable-transfer internals, pass counters, and unrelated UHF
  suppress internals SHALL remain diagnostics-only for this node-`5` cleanup

### Requirement: Representative Detailed GET Readback Drift Is Resolved Inside This Same Change

The comm subsystem SHALL keep the representative authenticated detailed
`GET_*` readback path aligned with the intended summary/detail split during the
same governed node-`5` residual cleanup.

#### Scenario: Proof drift is diagnosed before runtime policy is changed
- **WHEN** a fresh hosted or target node-`5` observability rerun shows that a
  representative detailed `GET_*` field is no longer proven by the current
  packetized oracle
- **THEN** the same change SHALL classify the drift as proof-oracle drift,
  packetized delivery-path drift, or component-owned summary/detail drift
- **AND** it SHALL prefer proof-path repair before changing component
  publication or auth-gated live policy when the detailed publication still
  exists by design
