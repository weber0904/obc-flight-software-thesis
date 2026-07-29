## ADDED Requirements

### Requirement: Target CAN Node-6 Matrix Cells Stay Quiet-Aware

Target CAN node-`6` matrix evidence SHALL preserve the current quiet-UHF
acceptance boundary instead of implying non-quiet background telemetry closure.

#### Scenario: Quiet-UHF target CAN evidence is reviewable
- **WHEN** a target CAN UHF matrix cell passes
- **THEN** the evidence SHALL identify the physical UART southbound path and
  the bounded quiet-UHF acceptance used for that result

### Requirement: Target CAN Failover Evidence Requires Ground-Side Closure

Target CAN failover evidence SHALL prove session continuity from S-band to UHF
primary with ground-side readback rather than local journal evidence alone.

#### Scenario: Failover command continuity is reviewable
- **WHEN** target CAN `failover-command` passes
- **THEN** the evidence SHALL show S-band healthy state, induced loss,
  explicit switch to UHF primary, a reopened session, and successful
  command/readback over the new path
