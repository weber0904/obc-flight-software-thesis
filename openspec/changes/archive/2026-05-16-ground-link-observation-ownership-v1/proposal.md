## Why

`ground-link-boundary-split-v1` moved mission link-health policy out of `GroundLinkDriver` and into `GroundLinkHealthProvider`, but three residual ownership issues remain. The provider-facing observation contract is still defined by `simulators/comm`, `CommController` still carries unused `GroundLinkDriver*` type coupling, and generic COMM node `4` compatibility semantics still leak into provider policy as raw node-id checks.

This change finishes that cleanup without reopening the larger 03 boundary split or starting a naming-heavy transport/probe refactor. It makes the runtime observation contract OBC-owned, makes node `4` a connected-only compatibility path, and leaves active `COMM_CSP` health policy scoped to node `5` S-band and node `6` UHF.

## What Changes

- Add an OBC-owned `GroundLinkObservationRuntime.hpp` contract for provider-facing link observation types.
- Move `GroundLinkBackendMode` and `GroundLinkObservationState` out of `simulators/comm/GroundLinkBackend.hpp` while keeping their existing symbol names.
- Add `GroundLinkHealthSemantics` so backends publish policy semantics explicitly instead of forcing the provider to inspect raw CSP node IDs.
- Treat COMM CSP node `5` and node `6` as `ACTIVE_COMM_CSP`, and generic node `4` as `CONNECTED_ONLY_FALLBACK`.
- Update `GroundLinkHealthProvider` to use `healthSemantics` only for availability and transport-growth policy.
- Remove the unused direct `GroundLinkDriver*` coupling from `CommController`.
- Refresh docs and tests for the narrowed ownership boundary.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: provider-facing ground-link observation is now an OBC-owned runtime contract; generic node `4` compatibility no longer participates in active COMM CSP stale/activity or transport-growth policy.

## Impact

- Affected code:
  - `OBC/Components/GroundLinkDriver/*`
  - `OBC/Components/GroundLinkHealthProvider/*`
  - `OBC/Components/CommController/*`
  - `OBC/Top*/*`
  - `simulators/comm/GroundLinkBackend.*`
- Affected interfaces:
  - `GroundLinkObservationState` drops `cspTargetNode` and gains explicit `healthSemantics`
  - `CommController::configureRuntime` no longer accepts ground-link driver pointers
- Affected verification:
  - update GroundLinkDriver, GroundLinkHealthProvider, CommController, and simulator/backend coverage
  - rerun active S-band, active UHF, and generic node `4` compatibility probes after a fresh build
