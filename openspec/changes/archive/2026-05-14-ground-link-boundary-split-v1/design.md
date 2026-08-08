## Context

The current active topology has three adjacent but distinct ground-facing link surfaces:

- hosted CCSDS S-band through `sband_comm_csp_node` node `5`
- hosted UHF backup through `uhf_comm_csp_node` node `6`
- direct `OBC -> GDS` TCP adapter for development regression

`CommController` currently samples `GroundLinkDriver` runtime stats directly and treats `connected` plus aggregate TX/RX error growth as its link-health input. That works, but it leaves mission policy coupled to a driver implementation that also owns byte send/receive, backend selection, worker lifecycle, and low-level counters. The review follow-up requires that those responsibilities split before more real modem/radio behavior grows around the same surface.

## Goals / Non-Goals

**Goals:**

- keep `GroundLinkDriver` as the byte/backend owner only
- add an explicit COMM-facing link-health provider component
- define reviewable `COMM_CSP` activity-freshness semantics for node `5` and node `6`
- keep direct TCP as a bounded connected-only fallback
- preserve existing `ComCcsds` byte-stream and `COMM_*` policy behavior except for the new observability surface

**Non-Goals:**

- no new RF, modem control, CCSDS-on-UHF, or target/Pi hardware claim
- no new TCP keepalive or generic reliable-transfer protocol
- no change to watchdog heartbeat meaning; watchdog heartbeat continues to mean owner-cycle liveness, not link activity
- no reopening of generic compatibility node `4` as an active policy surface

## Decisions

### Decision: Add a provider component, not just a helper class

The first split introduces a real passive component `GroundLinkHealthProvider`. It runs on the fast rate group and leaves reviewable telemetry around per-band activity age and availability reasoning. Internal helper structs/classes are allowed inside the implementation, but the public boundary lands as a component so the active topology can schedule and test it explicitly.

### Decision: The provider owns link activity freshness, not watchdog heartbeat

This change uses the term `link activity freshness`, not `heartbeat freshness`.

- watchdog heartbeat remains the existing explicit `watchdogBeatOut` owner-cycle liveness surface
- link activity freshness is a COMM policy signal describing whether the ground link has recent or successfully observed activity

These two signals stay intentionally separate.

### Decision: `COMM_CSP` activity freshness uses observed success

For active node `5` S-band and node `6` UHF `COMM_CSP` paths, the provider treats any of the following as activity:

- successful RX
- successful TX
- successful status observation

The provider runs one health observation per fast-group cycle. `COMM_CSP` backends expose a successful status-observation count so the provider can refresh activity age even during idle command periods.

### Decision: `COMM_CSP` availability is strict and shallow in v1

For `COMM_CSP`, the provider computes:

- `available = connected && activityAgeTicks <= 1`
- `availabilityReason = HEALTHY_ACTIVITY` when the above condition is met
- `availabilityReason = DISCONNECTED` when `connected == false`
- `availabilityReason = STALE_ACTIVITY` when connected but `activityAgeTicks > 1`

The stale threshold is fixed in v1 and not runtime-configurable.

### Decision: Direct TCP is a connected-only fallback

The direct `OBC -> GDS` TCP path remains a development/regression path. It does not get a new keepalive protocol in this change.

For `DIRECT_TCP`, the provider computes:

- `available = connected`
- `availabilityReason = CONNECTED_ONLY_FALLBACK` when connected
- `availabilityReason = DISCONNECTED` when not connected

Idle TCP periods SHALL NOT be marked stale only because no TX/RX occurred.

### Decision: Driver/backend expose observation state upward

`GroundLinkStats` remains the low-level counter surface used for driver telemetry.

This change adds a separate `GroundLinkObservationState` that contains only the provider-facing facts it needs:

- backend mode
- connected
- TX/RX chunk totals
- TX/RX error totals
- successful status-observation total

`GroundLinkDriver` exposes this observation state plus a runtime health-refresh method for the provider. `CommController` SHALL NOT read raw driver stats directly after this change.

### Decision: Fault split stays unchanged

`CommController` keeps the existing fault taxonomy:

- `PRIMARY_UNAVAILABLE` follows provider `available`
- `PRIMARY_TRANSPORT` follows provider `errorGrowthThisCycle`

Stale activity contributes only through `available`, not by opening a new transport-fault kind.

## Runtime Shape

### GroundLinkHealthProvider inputs

- `GroundLinkDriver*` for S-band
- `GroundLinkDriver*` for UHF
- fast-group `schedIn`

### GroundLinkHealthProvider outputs

- per-band `CommLinkHealthView` via runtime getter
- reviewable telemetry for per-band availability, activity age, and availability reason

### `CommLinkHealthView`

Minimum fields:

- `band`
- `backendMode`
- `connected`
- `available`
- `activityAgeTicks`
- `rxAgeTicks`
- `txAgeTicks`
- `errorGrowthThisCycle`
- `availabilityReason`

### `CommRuntimeState` additions

- `sbandActivityAgeTicks`
- `uhfActivityAgeTicks`
- `sbandAvailabilityReason`
- `uhfAvailabilityReason`

Hosted status output also prints those fields.

## Topology / Scheduling

- add `groundLinkHealthProvider` to `TopCcsds`
- schedule it on the fast rate group immediately before `commController.schedIn`
- keep `GroundLinkDriver` outside the rate group as the existing worker-driven byte/backend component
- keep `CommController` as the public COMM summary owner

## Risks / Mitigations

- **[Risk] Connected-but-idle `COMM_CSP` links could flap unavailable if activity is judged too narrowly.** Mitigation: count successful status observations as activity and run one provider-triggered health observation every fast-group cycle.
- **[Risk] Reviewers could confuse link activity freshness with watchdog heartbeat.** Mitigation: rename the concept everywhere in code/docs and preserve existing watchdog behavior unchanged.
- **[Risk] Direct TCP could be misread as an active node-`5`/node-`6` policy path.** Mitigation: keep it explicitly documented and tested as connected-only fallback regression.

## Migration Plan

1. Add the OpenSpec artifacts and `comm-subsystem` delta.
2. Add provider-facing observation types and backend health-refresh support.
3. Add `GroundLinkHealthProvider` and classic component harness.
4. Rewire `CommController` to consume provider views and extend runtime state/status reporting.
5. Update `TopCcsds` scheduling order and runtime configuration defaults as needed to keep the active hosted path aligned with node `5` / node `6`.
6. Refresh docs, run full local verification, and run focused hosted probes for S-band, UHF, and direct-TCP regression.

## Open Questions

- none
