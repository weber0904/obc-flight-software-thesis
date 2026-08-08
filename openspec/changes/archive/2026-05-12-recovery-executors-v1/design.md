## Context

The current active baseline has two separate detector slices:

- `WatchdogSupervisor` owns explicit heartbeat freshness, warning/fault/suppress thresholds, `SAFE` request, and watchdog-feed suppression over a bounded source set.
- `EpsFdirController` owns EPS-only timeout detection, failures `1-2` retry-only behavior, fault latch at failure `3`, one-shot `SAFE` request, and first-success clear.

Both slices currently stop before a shared action owner exists. There is no common incident model, no shared recovery-level progression, no truthful reboot/reset metadata closure, and no runtime path that can explain which recovery action caused a restart-equivalent flow. `ModeSafetyController` is intentionally SoC-only and must stay that way. `BootManager` already owns file-backed boot metadata and is the narrowest truthful place to persist reset/boot information, but it does not yet carry recovery metadata or boot counters.

The change therefore needs one new runtime owner and bounded changes across mission autonomy, boot metadata, topology wiring, hosted runtime services, component tests, and hosted evidence.

## Goals / Non-Goals

**Goals:**

- Introduce a dedicated `RecoveryExecutor` component as the single shared owner for recovery incidents, recovery-level progression, and bounded recovery action execution.
- Route watchdog stale faults and EPS timeout faults into the same executor path while preserving each detector's bounded detection logic.
- Persist truthful boot/recovery metadata across restart-equivalent flows and expose that metadata through reviewable status surfaces.
- Provide deterministic clear, relatch, and repeated-failure escalation rules that move recovery forward instead of repeating the same no-op action forever.
- Prove the new closure on the hosted active baseline with a repository-owned probe that relaunches the same runtime root after a reboot-equivalent exit.

**Non-Goals:**

- Building a broad all-subsystem FDIR framework or introducing non-EPS subsystem reset policies.
- Reviving `MissionExecutive`, broadening `HealthMonitor`, or moving recovery ownership into `ModeSafetyController` or `BootManager`.
- Adding generic `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET` commands in v1.
- Claiming Raspberry Pi hardware watchdog reset, target hardware reboot proof, persistent event log infrastructure, or full secure boot redesign.

## Decisions

### 1. `RecoveryExecutor` owns progression as well as action execution

`RecoveryExecutor` will not be a passive dispatcher. It will own:

- incident normalization from bounded detector inputs
- per-incident epoch and relatch tracking
- recovery-level progression
- single-shot action gating
- action outcome/status surfaces

This keeps `WatchdogSupervisor` and `EpsFdirController` narrow and avoids splitting progression policy across multiple detectors that would otherwise duplicate escalation rules and make later consumers harder to add.

Alternative considered:
- Keep progression in each detector and let the new owner only execute actions. Rejected because it would still leave two different recovery frameworks in active runtime and would make repeated-failure/reboot policy inconsistent.

### 2. Detectors stay detector-only and retain local truth

`WatchdogSupervisor` keeps:

- heartbeat freshness
- warning/fault/suppress thresholds
- aggregate/per-source watchdog status
- feed-eligibility truth

`EpsFdirController` keeps:

- EPS consecutive poll-failure evaluation
- local retry-only state for failures `1-2`
- first-fault latch
- first-success clear

Neither detector will directly call `ModeManager` for `SAFE` escalation after this change. Both emit shared recovery requests and explicit clear notifications into `RecoveryExecutor`.

Alternative considered:
- Fold watchdog or EPS recovery ownership into the new executor and simplify the detectors further. Rejected because the detectors already own meaningful local status contracts that operators review directly.

### 3. Bounded action set mixes intent-only and real execution

The v1 action set is:

- `R2 RestartSoftwareComponent`: reviewable intent only. No truthful process-restart hook exists yet, so this change records intent and progression but does not claim actual process restart execution.
- `R3 ResetSubsystemInterface`: real execution for EPS only, implemented via `EpsBridge.resetForRuntime()`.
- `R5 ModeFallback`: real execution via the normal internal mode-control path.
- `R6 OBCReboot`: real hosted reboot-equivalent execution by persisting metadata and invoking a new runtime reboot hook that terminates the runtime process.

`R4 PowerCycleSubsystem` and `R7 EnterHELL` remain defined in the shared type vocabulary but unused in v1.

Alternative considered:
- Skip `R2` entirely and begin at `SAFE`. Rejected because the plan explicitly requires a visible progression that shows restart intent before reboot.

### 4. Recovery progression is deterministic and incident-specific

Incident model:

- bounded incident keys: `WATCHDOG_<source>` and `EPS_TIMEOUT`
- per-incident state: epoch, current level, highest level, actions-issued mask, relatch count, last outcome, awaiting-clear

Progression policy:

- Watchdog first fault opens at `R2`, may advance once to `R5` if mode is requestable, and advances to `R6` when detector-side feed suppression proves continued stale failure.
- EPS does not enter the executor on failures `1-2`; failure `3` opens at `R3`, may also issue `R5`, and advances to `R6` on relatch or bounded failure-to-clear.
- Detector clear closes only the matching active incident epoch; no automatic pre-fault mode restore is allowed.

Alternative considered:
- Use a single global recovery ladder for all incidents. Rejected because watchdog and EPS already have different detector semantics and the v1 scope only needs two bounded consumers.

### 5. Boot truth stays in `BootManager` file-backed metadata

`BootManager` and `BootMetadataStore` remain the single source of truth for persisted boot state. The schema will expand in-place to include:

- `boot_count`
- `reset_cause`
- `consecutive_reset_count`
- `last_recovery_source`
- `last_recovery_level`
- `boot_safe_fallback_required`

Startup flow:

- reload metadata
- increment `boot_count`
- if prior reset cause came from `RecoveryExecutor` reboot intent, increment `consecutive_reset_count`, otherwise clear it
- if `consecutive_reset_count >= 3`, request `SAFE` clamp on boot and report that truth

Steady-state closure:

- once the runtime has been stable for a fixed bounded window with no active incidents or pending reboot intent, `RecoveryExecutor` acks stability through `BootManager`, which clears the consecutive-reset path.

Alternative considered:
- Persist recovery metadata directly in `RecoveryExecutor`. Rejected because it would create a parallel boot-truth store next to `BootManager`.

### 6. Public surface stays bounded to status

This change adds:

- `BootManager.GET_RESET_CAUSE`
- `BootManager.GET_BOOT_COUNT`
- `RecoveryExecutor.GET_RECOVERY_STATUS`

It intentionally does not add:

- generic `FORCE_PROCESS_RESTART`
- generic `FORCE_SUBSYSTEM_RESET`

EPS keeps its existing subsystem-specific `EPS_RESET` command, which remains the only operator-triggered real subsystem reset surface in v1.

### 7. Hosted reboot-equivalent proof is the formal evidence boundary

The new hosted probe will prove:

- watchdog and EPS share the same executor path
- repeated failures escalate to reboot instead of staying at warning/SAFE forever
- reboot intent persists truthful boot metadata
- a restarted runtime on the same runtime root reports the correct boot/reset state and `SAFE` clamp truth

The probe will not claim target hardware reboot, Pi hardware watchdog, or power-loss resilience.

## Risks / Trade-offs

- [Risk] New shared owner could blur detector-local truth. → Mitigation: keep `GET_WATCHDOG_STATUS` and EPS detector-local telemetry unchanged in purpose; only move recovery execution ownership.
- [Risk] Reboot-equivalent probe may be flaky if it reuses stale ports or runtime roots. → Mitigation: follow the repo's isolated hosted-probe pattern with fresh runtime roots, dedicated ports, and explicit relaunch sequencing.
- [Risk] Boot metadata schema expansion could break older files. → Mitigation: keep backward-compatible parsing defaults and cover schema upgrade/rewrite behavior with direct L1 tests and `BootManager` L2 coverage.
- [Risk] Repeated-failure policy could over-escalate if detector clear timing is ambiguous. → Mitigation: make clear/relatch rules epoch-based and bounded by explicit detector events rather than inferred timing alone.
- [Risk] Legacy `Top` and authority-policy surfaces could fail to build after contract changes. → Mitigation: update both active and legacy topologies plus authority catalog/policy in the same change.

## Migration Plan

1. Create and validate the OpenSpec artifacts and delta specs before runtime changes.
2. Add the new shared recovery types/interfaces and the `RecoveryExecutor` component with unit coverage.
3. Rewire `WatchdogSupervisor`, `EpsFdirController`, `BootManager`, runtime services, and both topologies onto the new shared path.
4. Update status/authority/onboard-state surfaces to use boot metadata truth instead of `ModeManager`'s in-memory reboot counter.
5. Add the hosted recovery probe and evidence record.
6. Run fresh verification, focused regressions, OpenSpec validation, and then archive/sync the change.

## Open Questions

- None for v1. The remaining deferred questions are deliberate future-scope items: target hardware reboot/watchdog closure, generic `FORCE_*` surfaces, and non-EPS subsystem consumers.
