## Why

The current narrative baseline still lags behind the 2026-05-21 payload
convergence and the 2026-05-23 COMM verification-matrix closure, while the
active target-flightlike timing contract is still left as broad `TBD` prose.
That drift now blocks truthful review of what the Raspberry Pi
service-managed baseline actually proves and what timing gaps still remain.

## What Changes

- Refresh current baseline, architecture, target-design, and follow-up docs so
  they reflect the merged payload v2 / helper-backed target payload work and
  the current COMM verification-matrix closure instead of treating
  `payload-ops-contract-v1` as future work.
- Add a repository-owned target timing/WCET proof path for the active
  Raspberry Pi `obc-comm-csp-stack.service` baseline using governed ground/GDS
  ingress and bounded representative COMM/payload activity.
- Freeze the currently provable target timing contract surface: deployed base
  tick, rate-group divisors / nominal rates, and the governed missed-tick/slip
  policy derived from upstream `Svc::ActiveRateGroup` semantics.
- Record evidence-backed current service-managed WCET / jitter observations for
  the declared workload windows, freezing numeric ceilings only where the
  governed evidence is clean enough to justify them and otherwise documenting
  the residual gap explicitly instead of leaving broad timing `TBD`s.
- Package `payload_camera_backend_helper` into the Raspberry Pi release surface
  so the same service-managed target baseline can carry the representative
  payload-active timing workload.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `interface-contract-index`: allow the checked-in current interface index to
  promote proven target timing facts while keeping residual timing gaps
  explicit.
- `platform-baseline`: require the active service-managed target baseline to
  state its deployed base tick, divisors / nominal rates, and governed
  missed-tick policy.
- `verification-evidence`: require reviewable timing/WCET evidence for the
  active service-managed target baseline, including workload labels,
  observation windows, and residual non-claims.

## Impact

- Affected code and packaging:
  - Raspberry Pi release packaging for `payload_camera_backend_helper`
  - new repository-owned target timing/WCET probe entrypoint
- Affected docs:
  - `evidence/records/target-timing-wcet-profile-proof-v1/README.md`
  - `evidence/verification-path-registry.md`
  - stale current-baseline / architecture / target-design narrative layers
- Affected formal specs:
  - `interface-contract-index`
  - `platform-baseline`
  - `verification-evidence`
