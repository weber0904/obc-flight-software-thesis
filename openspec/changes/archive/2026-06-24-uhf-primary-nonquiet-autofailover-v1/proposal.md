## Why

The current maintained target/lab COMM baseline still treats UHF primary live
`event/tlm` packet egress as quiet-by-default and still proves current UHF
primary behavior through explicit `COMM_SET_ACTIVE(UHF)` switching. That no
longer matches the desired Chapter 5 route truth or the intended spacecraft
recovery model, where S-band loss should autonomously promote UHF to primary
and ground operators must still be able to observe bounded live readback on
that path.

## What Changes

- Remove UHF primary packet quiet from the current maintained COMM baseline.
- Keep UHF beacon suppress as a separate COMM-owned mechanism that still starts
  only after accepted UHF secure auth.
- Promote `COMM_PRIMARY_UNAVAILABLE` plus executor-owned failover as the only
  current autonomous `S-band -> UHF` failover truth for target/lab closure.
- Add a governed target helper that creates a bounded node-`5` unavailable
  window by stopping and restoring `subsystem-sband-csp.service`.
- Add new repository-owned target probes for:
  - non-quiet UHF primary runtime stability
  - autonomous failover to UHF primary with re-auth and bounded readback
- Retire quiet/manual-switch Chapter 5 dependencies from current Route 2/3
  target closure and mark adjacent historical evidence as superseded.

## Capabilities

### New Capabilities

- `target-autonomous-uhf-failover`: governed target/lab autonomous failover
  proof and bounded shared-service unavailable-window trigger ownership

### Modified Capabilities

- `comm-subsystem`: current UHF primary live-packet semantics and current
  failover semantics change from quiet/manual-switch truth to
  non-quiet/autonomous-failover truth
- `mission-autonomy`: current `COMM_PRIMARY_UNAVAILABLE` recovery wording now
  becomes the maintained autonomous UHF promotion truth used by current route
  closure
- `verification-evidence`: evidence requirements change for current UHF
  primary runtime, autonomous failover proof, and watchdog non-quiet target
  closure
- `verification-path-registry`: current route and target COMM registry wording
  changes from quiet/manual-switch paths to non-quiet/autonomous-failover
  paths

## Impact

- Affected code:
  - `OBC/Components/CommController/`
  - `OBC/Components/CommEgressMux/`
  - target proof helpers under `scripts/comm_verification/lib/`
  - target wrappers under `scripts/chapter5_routes/target/`
- Affected docs:
  - `docs/interfaces.md`
  - `docs/roadmap/current-baseline.md`
  - `evidence/verification-path-registry.md`
  - `evidence/records/`
- Affected current proof surfaces:
  - target UHF primary live benchmark family
  - target failover proof family
  - Chapter 5 Route 2 / Route 3 target closure
