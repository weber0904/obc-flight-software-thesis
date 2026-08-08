## Why

The active baseline already proves hosted CCSDS S-band node `5`, hosted UHF node `6`, and direct `OBC -> GDS` TCP development connectivity, but `GroundLinkDriver` still mixes byte/backend behavior with the runtime signals that `CommController` consumes as mission link-state truth. That coupling was acceptable for early hosted integration, but it now makes link-health policy, failover reviewability, and future modem/radio growth harder to reason about and easier to regress.

This change narrows `GroundLinkDriver` back to a byte/backend owner and introduces an explicit link-health provider for the active COMM policy runtime. It keeps the current byte-stream contracts and proof boundaries intact while making link availability, activity freshness, and transport-fault inputs reviewable as a COMM-facing policy surface instead of as driver internals.

## What Changes

- Add a passive `GroundLinkHealthProvider` component that runs in the fast rate group before `CommController` and computes per-band health views for S-band and UHF.
- Keep `GroundLinkDriver` focused on backend configuration, worker lifecycle, byte send/receive, buffer handoff, and driver-level counters/events/telemetry.
- Add a runtime observation surface between driver/backend and provider so link-health logic no longer depends on direct driver internals.
- Make `CommController` consume per-band health views instead of reading `GroundLinkDriver` stats directly.
- Define `COMM_CSP` activity freshness for the active node-`5` and node-`6` paths from successful RX, successful TX, or successful status observation.
- Keep direct `OBC -> GDS` TCP as a connected-only fallback path without introducing a new keepalive protocol.
- Extend reviewable COMM runtime state and hosted status output so activity age and availability reasons are visible during verification.
- Add focused component and integration coverage for stale-activity, recovery, transport-growth, and direct-TCP fallback behavior.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: define a provider-owned link-health surface, activity-freshness policy, and direct-TCP fallback semantics while preserving existing byte-stream and active node-`5`/node-`6` proof boundaries.

## Impact

- Affected code:
  - `OBC/Components/GroundLinkDriver/*`
  - new `OBC/Components/GroundLinkHealthProvider/*`
  - `OBC/Components/CommController/*`
  - `OBC/TopCcsds/*`
  - hosted runtime status/reporting paths
  - `simulators/comm/GroundLinkBackend.*`
- Affected interfaces:
  - `GroundLinkDriver` gains a runtime observation/health-refresh surface for the provider
  - `CommController` runtime configuration shifts from direct driver stats to provider-owned health views
  - `CommRuntimeState` gains per-band activity-age and availability-reason fields
- Affected verification:
  - new classic harness for `GroundLinkHealthProvider`
  - updated classic harnesses for `GroundLinkDriver` and `CommController`
  - focused hosted regressions for S-band node `5`, UHF node `6`, and direct-TCP fallback behavior
