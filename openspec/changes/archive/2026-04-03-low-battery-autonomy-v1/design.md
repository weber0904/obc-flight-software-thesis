## Context

The project already has the subsystem pieces needed to stage a low-battery autonomy case: `EpsBridge` can detect low-battery conditions, `ModeManager` already supports `LOW_POWER`, `AdcsBridge` already supports `POINTING` and target quaternions, and the recent scenario bridge can replay battery state-of-charge and sunlight transitions into the hosted simulators. What is still missing is the system-level decision layer that can watch subsystem state and command a coordinated response.

The previous design discussion also established two important constraints for this first autonomy slice:

- do not feed STK or scenario truth directly into the OBC runtime
- do not expand this slice into full mission scheduling, comm pass automation, or housekeeping/downlink

This design therefore introduces the smallest useful `MissionExecutive` that can react to low battery without pretending to solve the full mission executive problem in one change.

## Goals / Non-Goals

**Goals:**

- add a first-version `MissionExecutive` component to the OBC topology
- let that component react to cached EPS status rather than issuing extra transport polls every cycle
- enter `LOW_POWER` automatically when EPS state-of-charge crosses the existing low-battery threshold
- command ADCS into a first-version sun-safe pointing profile using the current `POINTING` mode and a repository-owned fixed target quaternion
- verify the autonomy policy locally with automated tests and hosted scenario-driven evidence

**Non-Goals:**

- automatic exit from `LOW_POWER`
- comm-band switching, radio disable, or broader load shedding
- a new `SUN_TRACKING` enum or a true scenario-driven sun-vector target
- deploy/detumble phase logic
- ground-pass-aware operation, housekeeping archive, or file downlink

## Decisions

### Decision: Introduce a minimal MissionExecutive now instead of folding the policy into an existing bridge

The low-battery response is a system-level decision, not an EPS-only or ADCS-only behavior. Introducing a small dedicated `MissionExecutive` now keeps that boundary explicit and gives later autonomy cases a stable home instead of scattering policy across subsystem bridges.

Alternative considered:

- put the low-battery response directly inside `EpsBridge`
  - rejected because it would mix subsystem transport/state ownership with system-level policy

### Decision: Reuse cached bridge state through lightweight runtime bindings

The current component set does not yet expose project-owned control or status ports suitable for autonomy orchestration. For this first slice, `MissionExecutive` will bind to `ModeManager`, `EpsBridge`, and `AdcsBridge` through narrow runtime interfaces and consume cached bridge state, avoiding redundant transport calls while keeping the implementation small.

Alternative considered:

- introduce a new family of FPP control/status ports first
  - rejected because it would broaden the change beyond the first autonomy case

### Decision: Model first-version sun-tracking as `POINTING` toward a fixed sun-safe target

The current ADCS public contract already supports `POINTING` and target quaternions. The simulator still lacks a true scenario-driven sun vector target, so this slice uses a repository-owned fixed quaternion as a first-version sun-safe target. This gives the autonomy path a concrete, reviewable behavior now while leaving true sun-vector tracking and a future `SUN_TRACKING` mode for a later change.

Alternative considered:

- add a new `SUN_TRACKING` mode immediately
  - rejected because it requires a larger ADCS contract and simulator expansion before the first autonomy case is proven useful

### Decision: Keep the first low-battery response latched once entered

Once the low-battery autonomy policy activates, it remains active for this first slice rather than automatically restoring `NOMINAL` when SOC recovers. This is the safer policy default and avoids inventing recovery hysteresis and re-entry semantics before the first entry case is validated.

Alternative considered:

- add automatic recovery to `NOMINAL`
  - rejected because recovery thresholds and mission policy are not yet aligned and would enlarge the change unnecessarily

## Risks / Trade-offs

- [Risk] The fixed sun-safe target is only a proxy for real sun-tracking. → Mitigation: keep the spec and evidence explicit that this slice uses `POINTING` plus a repository-owned target rather than a true sun-vector follower.
- [Risk] Runtime bindings between components are less formal than dedicated control/status ports. → Mitigation: keep the interfaces narrow and isolate them to the new mission-executive slice.
- [Risk] A latched low-power response may be too conservative for later mission scenarios. → Mitigation: make the lack of automatic recovery explicit and leave recovery policy to a later change.
- [Risk] Readers may assume comm/load shedding is already handled when low battery occurs. → Mitigation: keep that work explicitly out of scope in specs and evidence.

## Migration Plan

1. Add the new `mission-autonomy` capability and delta specs for scenario validation and evidence.
2. Introduce the `MissionExecutive` component and runtime interface hooks for cached EPS/ADCS state access.
3. Add unit and hosted validation tests for low-battery autonomy.
4. Archive the change after local verification and evidence updates.

## Open Questions

- When the project later adds ground-pass-aware operation, should `MissionExecutive` own comm pass activation directly or delegate to a separate mission-phase layer?
- When the project later expands ADCS semantics, should `SUN_TRACKING` become a new `AdcsMode` or remain a policy-level specialization of `POINTING`?
