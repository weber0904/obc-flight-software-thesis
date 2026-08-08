## Context

The official sequencing resources probe already proves that hosted `TopCcsds`
sequencing is alive, but the matrix needs a narrower harness that proves a
specific capability family:

- same-path sequence upload
- non-reject admission
- `SEQ_RUN(..., WAIT)` success
- EPS and ADCS readback returning through the same ground path

## Design

### Shared Sequence Helper

The change extracts a shared helper from the existing official sequencing probe
instead of cloning probe logic into multiple wrappers. The helper governs:

- sequence source generation
- binary compilation and staging
- file-uplink through the active ground path
- `SEQ_VALIDATE` as a non-reject preflight only
- `SEQ_RUN(..., WAIT)` as the pass oracle

### Fixed Sequence Shape

The matrix-owned sequence shape remains stable:

- `EPS_GET_STATUS`
- `ADCS_GET_ATTITUDE`
- an optional harmless marker command when needed for correlation

No mutating subsystem action is required for this first shared harness.

### Hosted First

This change closes only hosted sequence cells. Target CAN and target TCP reuse
the same helper later once their carrier-specific launchers exist.

## Boundaries

- No target runtime topology changes
- No failover logic
- No registry update unless the hosted sequence cells finish with passing
  governed evidence
