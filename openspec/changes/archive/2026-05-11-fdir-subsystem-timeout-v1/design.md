## Context

The current runtime split is already narrow and should stay narrow:

- `EpsBridge` owns EPS transport polling, cache validity, and EPS-owned telemetry/events.
- `ModeSafetyController` owns cached-EPS SoC policy only.
- `ModeManager` owns mode state and `SYS_MODE_CHANGE`.
- `HealthMonitor` remains CPU/RSS threshold monitoring only.

What is missing is a deterministic owner for `EPS poll timeout -> bounded retry -> escalate -> recover`. The target design allows broader subsystem watchdog and multi-level FDIR later, but the current roadmap explicitly calls for one narrow subsystem-timeout slice first.

## Goals / Non-Goals

**Goals**

- Detect repeated EPS poll failures in the active runtime.
- Tolerate transient failures with bounded retry.
- Latch an EPS fault at three consecutive failures.
- Escalate once through the normal internal mode-control path to `SAFE` for active modes.
- Emit explicit recovery evidence and clear the fault on first success.
- Keep the behavior deterministic and easy to prove in unit, integration, and hosted-probe layers.

**Non-Goals**

- No broad all-subsystem FDIR framework.
- No timestamp freshness or age-based stale policy.
- No EPS reset, interface reset, or power cycle.
- No watchdog, persistent event store, process restart, or hardware action.
- No `MissionExecutive` restoration.
- No second subsystem slice.

## Decisions

### A Separate EpsFdirController Owns Timeout FDIR

This change adds a new passive component, `EpsFdirController`, instead of extending `ModeSafetyController`.

Reasons:

- `ModeSafetyController` currently has a clean SoC-only contract and its spec explicitly excludes subsystem timeout, retry, reset, and broader FDIR actions.
- `HealthMonitor` is intentionally narrow CPU/RSS monitoring and should stay out of subsystem fault policy.
- A separate component keeps this slice evolvable toward a future broader FDIR layer without forcing that framework now.

The ownership split becomes:

- `EpsBridge`: poll, cache-validity, transport outcome accounting, EPS comm-error visibility.
- `EpsFdirController`: thresholding, fault latching, one-shot escalation, recovery clear.
- `ModeManager`: unchanged mode owner and internal-apply target.
- `ModeSafetyController`: unchanged SoC policy owner.

### Consecutive Poll Failures Are The Only Timeout Signal In V1

The timeout input is the scheduled `EpsBridge` poll outcome only:

- successful poll: reset consecutive failure count to `0`, mark last poll success
- failed poll: increment consecutive failure count and invalidate cached EPS status as already defined

No timestamp freshness or age threshold is added because:

- the current runtime EPS cache contract has no timestamp
- the user explicitly constrained this change away from a broader framework
- consecutive failure alone is sufficient to prove the intended runtime closure

### Retry And Escalation Thresholds Are Fixed

Threshold policy is fixed and not configurable in v1:

- failure count `0`: `HEALTHY`
- failure counts `1-2`: `RETRYING`
- failure count `>= 3`: `FAULTED`

Escalation occurs only on the first transition into `FAULTED`. Later failures while fault remains latched do not re-escalate.

### Recovery Is First Success After Fault

Recovery boundary:

- if the controller is fault-latched and a later EPS poll succeeds, it emits recovered/fault-cleared evidence and clears the latch immediately
- recovery does not auto-restore a pre-fault mode; the system remains in whatever mode it reached, including `SAFE`

This keeps the behavior deterministic and avoids sneaking mission-mode automation into this slice.

### Escalation Uses Existing Internal Mode Control

When `EpsFdirController` first enters `FAULTED`:

- if current mode is `IDLE`, `PAYLOAD`, or `TTC`, request `SAFE` through `ModeManager::applyModeForInternalSource(...)`
- if current mode is `SAFE` or `HELL`, record the fault and skip a duplicate mode request

Add a new internal apply source such as `FdirSubsystemFault` so the path stays distinct from:

- `SafetyFallback`
- `SafetyRecovery`
- `SafetyPayloadExit`
- `TestSetup`

This keeps `SYS_MODE_CHANGE` as the normal outward-facing mode surface while preserving internal source clarity for tests and later observability.

### Deterministic Schedule Order Must Be Explicit

The active `TopCcsds` rate-group order already places:

1. `epsBridge.schedIn`
2. `modeSafetyController.schedIn`

This change inserts `epsFdirController.schedIn` between them in both `TopCcsds` and legacy `Top`:

1. `epsBridge.schedIn`
2. `epsFdirController.schedIn`
3. `modeSafetyController.schedIn`

That guarantees the FDIR decision always consumes the latest EPS poll result before SoC mode-safety runs.

## Runtime Contract

### EPS Runtime Health Provider

Add a runtime EPS health snapshot exposed by `EpsBridge`, with at least:

- `cacheValid`
- `lastPollSucceeded`
- `consecutivePollFailures`
- `cumulativePollErrors`

Contract details:

- cache validity remains the authoritative stale/unavailable signal for downstream consumers
- failed poll increments consecutive failures and cumulative errors
- successful poll resets consecutive failures to zero and sets `lastPollSucceeded = true`
- unsuccessful poll sets `lastPollSucceeded = false`

### EpsFdirController State Machine

Controller-visible states:

- `HEALTHY`
- `RETRYING`
- `FAULTED`

Transitions:

1. `HEALTHY -> RETRYING` on failure count `1`
2. `RETRYING` remains while failure count is `2`
3. `RETRYING/HEALTHY -> FAULTED` on failure count `3`
4. `FAULTED` remains latched while failures continue
5. `FAULTED -> HEALTHY` on first success

Observability:

- event when retry state begins or advances
- event when fault latches and whether escalation requested `SAFE`
- event when recovery clears the fault
- telemetry for current fault-latched state, last observed failure count, and escalation count

The controller should avoid noisy repeated retry/fault events on every cycle once the state is unchanged.

## Verification Strategy

- `EpsBridge` classic component UT proves poll-health accounting and reset-on-success semantics.
- `EpsFdirController` classic component UT proves thresholding, one-shot escalation, no duplicate escalation, recovery clear, and `SAFE`/`HELL` no-op mode action.
- direct helper test proves the pure threshold/latch state machine independent of F' plumbing.
- focused integration test proves real `EpsBridge` health output feeds real `EpsFdirController` and requests mode changes through the normal mode-control surface.
- hosted probe proves real hosted `TopCcsds` runtime behavior using simulator stop/restart, with deterministic no-escalation-before-third-failure, one-shot `SAFE` fallback at threshold, and recovered/fault-cleared evidence after restart.

## Risks / Trade-offs

- No timestamp freshness means this slice cannot detect “old but still cached” EPS data; that is deferred by design.
- A new component adds topology/test wiring overhead, but that is preferable to mixing responsibilities into existing owners.
- The first-success recovery rule is intentionally simple and may be too optimistic for future hardware instability; later broader FDIR work can refine it with persistence or hysteresis once there is a concrete need.
