## Context

Mode-model-v2 established the active spacecraft mode names and explicitly retired the old provisional `MissionExecutive` from topology. That component still contains behavior and public naming tied to retired `LOW_POWER`, sun-safe pointing, and detumble policies. Reusing it as-is would blur the active v2 mode contract and would reintroduce out-of-scope autonomy claims.

This change implements only a safety fallback policy based on cached EPS state of charge. It does not define broader mission autonomy, FDIR, subsystem reset policy, pass scheduling, communication routing, or target hardware behavior.

## Decision

Implement a new passive real F' component named `ModeSafetyController` as the v1 policy owner.

`ModeSafetyController` is chosen over rewriting `MissionExecutive` because:

- The required behavior is narrow: read cached EPS SoC, read the current mode, and request a mode fallback when strict thresholds are crossed.
- The old `MissionExecutive` public surface and policy code are coupled to retired semantics, so rewriting it now would mix safety fallback with a larger MissionExecutive/FDIR redesign that is not in scope.
- A dedicated component gives the safety policy a small interface, focused component coverage, and a clean future migration path if a later governed change introduces a broader policy owner.

## Runtime Binding

The component uses a narrow runtime interface rather than depending on old `MissionExecutive` policy types:

- `IModeSafetyEpsStatus` provides cached EPS status availability and state of charge.
- `IModeSafetyModeControl` provides current-mode read access and mode-set control through the existing `ModeManager` runtime path.

`EpsBridge` implements the cached EPS status interface. `ModeManager` implements the mode control interface, so all transitions still emit normal mode telemetry/events through the existing mode surface. `ModeSafetyController` is instantiated in active topology and configured with those two providers during topology setup.

`EpsBridge` remains the owner of EPS telemetry and EPS alarm events, but it does not own the primary-mode safety fallback. Its critical-battery alarm is a warning event in this slice so hosted and operational runtimes can continue long enough for `ModeSafetyController` to request the governed `SAFE -> HELL` fallback through `ModeManager`.

The old provisional `MissionExecutive` remains uninstantiated, unscheduled, and unbound in active topology.

## Policy Rules

The policy helper is pure C++ and deterministic:

- If cached EPS status is unavailable, no transition occurs.
- `SAFE -> HELL` occurs only when SoC is strictly less than `10%`.
- `HELL -> SAFE` occurs only when SoC is strictly greater than `15%`.
- `IDLE`, `PAYLOAD`, or `TTC -> SAFE` occurs only when SoC is strictly less than `40%`.
- Exact boundary values `10%`, `15%`, and `40%` do not trigger transitions.
- `SAFE -> IDLE` remains manual; high SoC does not automatically recover from `SAFE`.
- If the current mode already matches the target fallback mode, no duplicate transition request is made.

The helper keeps threshold precedence explicit by evaluating `SAFE`, `HELL`, and active modes independently. No rule commands ADCS, EPS load shedding, COMM, scheduler, watchdog, subsystem retry/reset, or other FDIR actions.

## Scheduling

The controller is scheduled from the fast rate group after `epsBridge.schedIn`, so it observes the newest cached EPS state available in that cycle. It is scheduled before later runtime consumers where practical, so the authoritative `ModeManager` mode surface reflects safety fallback early in the cycle.

When EPS status is absent, unavailable, stale after a failed status poll, or not yet cached, the component treats the input as non-authoritative and takes no control action. This avoids feeding simulator truth directly into the OBC runtime and avoids mode oscillation on missing data. If the controller is not configured with both runtime providers, it does not evaluate or publish decision telemetry based on a default assumed mode.

## Simulator And Probe Support

The EPS simulator gains a bounded startup option such as `--initial-soc <pct>`. Hosted probes use that simulator-owned state to produce normal EPS telemetry/cache updates; the OBC runtime still consumes cached EPS status through `EpsBridge`.

The hosted focused probe runs isolated runtime roots and alternate local ports. It exercises separate cases for `SAFE -> HELL`, `HELL -> SAFE`, `IDLE -> SAFE`, `PAYLOAD -> SAFE`, `TTC -> SAFE`, and high-SoC `SAFE` staying `SAFE`. It does not claim target hardware, RF, COMM split-link, CCSDS, scheduler, watchdog, or FDIR validation.

## Failure Behavior

- Missing runtime providers: no mode transition.
- Missing EPS cache or stale cache after a failed EPS poll: no mode transition.
- Missing runtime provider binding: no policy evaluation or decision telemetry based on default mode assumptions.
- Invalid or out-of-range simulator input: simulator argument parsing rejects invalid values or clamps through simulator model behavior before OBC observation.
- Mode set failures are outside this slice because the current `ModeManager::setModeForRuntime` path is synchronous and does not report a failure result; the controller verifies behavior through the normal mode surface in component/integration tests.

## Verification

Verification is layered:

- L1 direct tests cover the pure policy helper and strict comparison behavior.
- L2 classic F' component tests cover `ModeSafetyController` ports, runtime binding, events, telemetry, no-data behavior, threshold boundaries, and duplicate-transition avoidance.
- Focused integration tests use the real `ModeManager` runtime API with fake/model-backed EPS status to prove that fallback requests go through the normal mode surface.
- A hosted probe uses the active OBC topology and EPS simulator initial SoC to prove the policy path in a hosted runtime.
- Full local verification and OpenSpec validation close the change before archive/PR.
