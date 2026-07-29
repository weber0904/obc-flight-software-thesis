## Context

The current active-baseline split is already meaningful and should stay explicit:

- `ModeSafetyController` owns cached-EPS SoC policy and operator safety guards only.
- `EpsFdirController` owns EPS timeout retry/latch/SAFE escalation only.
- `CommController` owns COMM link-role and shared downlink runtime policy only.
- `HealthMonitor` currently owns only hosted CPU/RSS resource sampling and related `HEALTH_*` / `SYS_*` resource surfaces.

What is still missing is a single runtime owner for liveness supervision across the active fast-rate-group control owners. The target design points toward a process supervisor at FDIR L1 and a separate hardware-watchdog layer above it; the roadmap also says this must stay separate from EPS timeout FDIR and SoC mode policy.

## Goals / Non-Goals

**Goals**

- Add one explicit watchdog owner on the active baseline.
- Supervise a bounded source set with deterministic explicit-heartbeat freshness rules.
- Escalate beyond telemetry-only behavior into reviewable fault, `SAFE` request, and feed-suppression behavior.
- Absorb `HealthMonitor` so the repo no longer keeps a “maybe watchdog, maybe not” health owner.
- Keep the public watchdog surface aligned with the target design where the current runtime can truthfully support it.
- Add hosted evidence that proves healthy, stale, escalated, suppressed, and recovered behavior without over-claiming target reset or boot recovery.

**Non-Goals**

- No broad all-subsystem fault-policy framework.
- No real Raspberry Pi hardware watchdog reset claim.
- No boot-safe-image or post-reset recovery engine.
- No process restart executor, subsystem reset executor, or persistent event store.
- No expansion of the supervised set into slow/data-group owners or `ModeManager` in v1.
- No attempt to replace or repurpose upstream `CdhCore.health` as the watchdog owner.

## Decisions

### `WatchdogSupervisor` Is The Single Owner

This change adds one new passive component, `WatchdogSupervisor`, as the active-baseline watchdog owner.

Reasons:

- the user explicitly chose a single `WatchdogSupervisor` owner;
- `ModeSafetyController` and `EpsFdirController` already have deliberately narrow contracts;
- broadening either one would blur the already-governed FDIR layering;
- using `SystemSupervisor` now would encourage scope creep into a broader FDIR rewrite that the roadmap explicitly defers.

The owner split becomes:

- `WatchdogSupervisor`: heartbeat freshness, watchdog config/status, escalation, recovery clear, resource-threshold monitoring, feed eligibility/suppression.
- `ModeSafetyController`: unchanged SoC policy owner.
- `EpsFdirController`: unchanged EPS timeout owner.
- `CommController`: unchanged COMM runtime-policy owner.

### `HealthMonitor` Is Absorbed, Not Retained

The existing `HealthMonitor` component is removed. Its CPU/RSS threshold logic migrates into `WatchdogSupervisor`, and the repo stops carrying a second narrow health owner that looks adjacent to watchdog policy.

Migration rules:

- keep the existing resource-monitoring behavior and thresholds;
- move the public owner path from `healthMonitor` to `watchdogSupervisor`;
- update authority catalog, topology instances, tests, and docs together;
- do not leave a compatibility wrapper that keeps the old owner alive.

### Upstream F' Health Pattern Is Reference, Not The Main Mechanism

The implementation will reference upstream F' `Svc::Health`, `Svc::Ping`, and `Svc::WatchDog` layering, but it will not use upstream active-component ping as the primary watchdog signal for v1.

Reasons:

- the supervised source set chosen for v1 is composed of repo-local passive components scheduled by active rate groups;
- upstream `Svc::Health` is already present through `CdhCore.health` and remains useful for framework active-service responsiveness;
- passive scheduled owners need a direct “I finished this cycle” signal, not queue-ping semantics.

So the final layering is:

- keep `CdhCore.health` unchanged for upstream active-service health;
- use repo-local explicit heartbeat beats for `WatchdogSupervisor`;
- expose a supervisor-side `Svc::WatchDog` feed/suppress contract for future target binding.

### V1 Supervised Set Is Bounded And Fast-Group Only

The supervised set is fixed in v1:

- `EpsBridge`
- `EpsFdirController`
- `ModeSafetyController`
- `CommController`

These sources are chosen because they are real active-baseline owners, run every fast rate-group cycle, and already sit on the current FDIR / mode / COMM control line.

Explicit exclusions:

- no `ModeManager`, because it is not a scheduled owner in the current topology;
- no slow-group or data-group owners in v1;
- no attempt to supervise all components simply because they exist in the topology.

### Heartbeat Model Uses Explicit Beat-After-Cycle

Each supervised owner emits an explicit beat to `WatchdogSupervisor` after completing its `schedIn_handler` work for that cycle.

Rules:

- heartbeat means “the owner executed this cycle,” not “the owner’s subsystem operation succeeded”;
- EPS poll failure, unavailable link state, or “no mode change needed” still count as a valid liveness beat;
- watchdog freshness is judged from the last accepted beat tick count, not from downstream business results;
- watchdog does not infer freshness only from expected scheduling.

This keeps liveness and subsystem-failure policy separate, which is required because `EpsFdirController` already owns EPS failure policy.

### Tick-Based Freshness Thresholds Are Fixed-Shape But Configurable

`WatchdogSupervisor` uses fast-tick counters rather than wall-clock timestamps.

Per source config in v1:

- `enabled`
- `warningTicks`
- `safeTicks`
- `suppressTicks`

Default policy:

- warning at tick age `2`
- latch / `SAFE` escalation at tick age `3`
- feed suppression at tick age `5`

The shape is fixed in v1:

- warning threshold must be less than or equal to safe threshold;
- safe threshold must be less than or equal to suppress threshold;
- config is runtime-memory only, not persistent across reboot;
- source-specific numeric defaults may all start equal across the four sources unless testing shows one source needs a slightly wider window.

### Escalation Contract Is Deterministic And Layered

Per-source escalation:

1. `HEALTHY`
   - beats arrive within the configured warning window.
2. `WARNING`
   - first freshness miss crossing `warningTicks`;
   - emit reviewable warning or degraded evidence only.
3. `LATCHED_FAULT`
   - first crossing of `safeTicks`;
   - latch source fault;
   - if current mode is `IDLE`, `PAYLOAD`, or `TTC`, request `SAFE` exactly once through the normal internal mode-control path using a distinct watchdog apply source;
   - if current mode is `SAFE` or `HELL`, record the fault, report aggregate recovery level as latched-fault only, and defer that one `SAFE` request until a later requestable mode is observed while the same fault epoch remains latched.
4. `FEED_SUPPRESSED`
   - first crossing of `suppressTicks`;
   - mark watchdog feed ineligible;
   - emit reviewable restart-intent or feed-suppressed evidence;
   - do not claim real hardware reset in this change.

Aggregate behavior:

- source state rolls up into aggregate fault mask and aggregate recovery level;
- aggregate recovery level distinguishes fault latched without `SAFE` request from fault latched after a real watchdog `SAFE` request;
- a second stale source while already latched does not re-request `SAFE`;
- a later source crossing into suppress while aggregate is already suppressed only expands the source mask and emits source-level evidence.

### Recovery Clears On First Beat, But Does Not Auto-Restore Mode

Per source:

- the first later valid beat clears that source’s stale/fault/suppressed state immediately.

Aggregate:

- aggregate `feedEligible` becomes true only when all enabled supervised sources are again below the suppress threshold;
- aggregate latched fault clears only when no enabled source remains faulted or suppressed;
- counters stay cumulative and do not reset on recovery;
- watchdog recovery does not auto-restore the pre-fault mode.

### Hardware Watchdog Hook Is Supervisor-Side Only

`WatchdogSupervisor` will export a future-friendly watchdog service hook and feed-eligibility state, but the claim is bounded:

- hosted proof covers only the supervisor deciding whether feed is eligible or suppressed;
- the repo may connect that decision to a `Svc.WatchDog` port or equivalent project-local feed adapter;
- the change does not claim true Raspberry Pi watchdog stroking, reset cause persistence, or boot recovery closure.

### Public Surface Stays Bounded

The public watchdog command surface in v1 is:

- `GET_WATCHDOG_STATUS`
- `SET_WATCHDOG_CONFIG`

Not included:

- `FORCE_PROCESS_RESTART`
- `FORCE_SUBSYSTEM_RESET`

because the current baseline has no truthful runtime executor for those actions.

Status/config surface requirements:

- `GET_WATCHDOG_STATUS` must expose aggregate state plus per-source freshness/trip data through reviewable telemetry and/or dedicated status events;
- `SET_WATCHDOG_CONFIG` must support bounded runtime config changes for the v1 per-source thresholds and enable flags;
- config changes require validation and reviewable update evidence.

## Runtime Contract

### `WatchdogSupervisor` Interfaces

The component will expose:

- existing migrated resource-monitoring command and telemetry surface;
- watchdog status/config commands;
- one `schedIn` port for watchdog evaluation on the active fast group;
- one heartbeat input port carrying `source` and the current tick/key;
- one watchdog-feed output contract for future `Svc.WatchDog` stroking;
- optional project-local runtime getter used by hosted status/probe surfaces.

### Supervised Source Contract

Each supervised owner will:

- keep its existing business behavior;
- emit one heartbeat after every completed fast-group cycle;
- avoid embedding watchdog policy locally;
- remain responsible for its own domain behavior, e.g. EPS timeout or COMM policy.

### Schedule Order

The active `TopCcsds` fast-group order becomes:

1. `epsBridge`
2. `epsFdirController`
3. `modeSafetyController`
4. `adcsBridge`
5. `radioController`
6. `uartDriver`
7. `commController`
8. `watchdogSupervisor`

This guarantees the watchdog evaluates after all v1 supervised sources have had a chance to beat in the same cycle.

### Hosted/Probe Control Boundary

The repository-owned hosted probe needs a deterministic way to create stale watchdog sources without falsifying unrelated subsystem behavior. That is done through a bounded hosted/probe-only beat-suppression hook.

Rules:

- it is only for repository-owned probe control;
- it is not a flight public command surface;
- it suppresses or drops heartbeat submission for selected sources while leaving the rest of the runtime alive;
- the official evidence must still exercise the real runtime owner path, not a unit-test-only mock.

## Verification Strategy

- classic F' L2 watchdog component coverage:
  - healthy beats keep feed eligible;
  - first warning crossing logs degraded only;
  - first safe crossing latches once and requests `SAFE` once;
  - first suppress crossing disables feed once;
  - `SAFE` and `HELL` remain fault-only cases;
  - recovery clears source and aggregate state correctly;
  - migrated resource-monitoring commands and threshold warnings still behave truthfully.
- direct helper tests:
  - per-source threshold ordering and validation;
  - aggregate roll-up and clear rules;
  - config update semantics;
  - no-persistence assumptions.
- affected owner regression:
  - keep `ModeSafetyController`, `EpsFdirController`, and relevant hosted runtime tests green after heartbeat-wiring changes.
- hosted probe:
  - healthy case stays feed eligible;
  - one-source warning case stays below SAFE request;
  - one-source stale case reaches latched fault, reports truthful `LATCHED_FAULT` vs `SAFE_REQUESTED`, and requests `SAFE` at most once for the fault epoch;
  - continued stale reaches feed-suppressed state;
  - resumed beat clears according to the defined contract without auto-exiting `SAFE`;
  - supervisor-side feed enable/suppress state is observable even though real target reset is not claimed.

## Risks / Trade-offs

- Removing `HealthMonitor` changes owner names and dictionary paths, but leaving it alive would preserve exactly the ownership ambiguity this change is supposed to close.
- Tick-based thresholds depend on stable fast-group cadence; this is acceptable in v1 because the supervised set is intentionally limited to one rate group.
- Probe-only beat suppression adds a test seam, but it is preferable to inventing false subsystem failures or trying to freeze the whole runtime just to prove watchdog freshness rules.
