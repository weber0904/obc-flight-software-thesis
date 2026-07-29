## Context

The current baseline already converges watchdog and EPS detector requests into `RecoveryExecutor`, but that closure still has two limitations. First, ADCS and COMM are not shared-recovery consumers yet even though `AdcsBridge` already exposes scheduled poll, cached valid state, sensor-invalid, and transport-error behavior, and `CommController` already owns primary-link role, session revoke, and downlink-owner policy. Second, `RecoveryExecutor` still performs external runtime calls while holding its mutex and directly couples to raw component pointers, which becomes riskier as more recovery consumers and actions are added.

This change therefore needs both a bounded behavior expansion and a structural correction. The active baseline should still only cover `EPS`, `ADCS`, and `COMM`, but those three lines need one deterministic incident vocabulary, one shared recovery progression owner, and one truthful reboot-intent metadata path.

## Goals / Non-Goals

**Goals:**

- Expand the active shared recovery path from watchdog plus EPS to bounded `EPS + ADCS + COMM`.
- Keep detector ownership local to `EpsFdirController`, new `AdcsFdirController`, and `CommController` while making `RecoveryExecutor` the only runtime owner allowed to execute subsystem recovery actions, `SAFE` fallback, and reboot intent.
- Refactor `RecoveryExecutor` into short-lock decision plus lock-free action execution so no external runtime calls happen while the internal recovery mutex is held.
- Make ADCS FDIR consume scheduled poll health only, not command-path failures.
- Make COMM detector policy reviewable while removing detector-side failover actuation.
- Preserve truthful boot/recovery metadata by recording reset causes only for reboot-intent closure.
- Leave deterministic evidence for latch, clear, relatch, escalation, and reboot-truth behavior.

**Non-Goals:**

- Building a generic all-subsystem FDIR platform.
- Adding GPS, payload, TTC, scheduler, or storage-health recovery consumers.
- Adding a new COMM reset plane, RF proof, target-hardware reboot proof, or persistent recovery configuration.
- Reviving `MissionExecutive` or moving mission behavior into `Main.cpp`.
- Introducing automatic nominal-link restore after fault-driven COMM failover.

## Decisions

### 1. `RecoveryExecutor` keeps ownership but changes execution structure

`RecoveryExecutor` remains the shared owner for incident normalization, per-incident epoch state, relatch tracking, recovery-level progression, and bounded action gating. However, it will no longer execute mode, boot, EPS, or COMM runtime actions while holding `m_mutex`.

Implementation shape:

- phase 1 under lock:
  - normalize detector request into a typed incident source
  - update incident state and current/highest level
  - decide which bounded action, if any, should be executed next
  - snapshot the action request and any fields needed for telemetry/status updates
- phase 2 outside lock:
  - execute the action through the narrow runtime interface
  - reacquire the lock only to record the outcome, escalation, or close state

Alternatives considered:

- Keep the current raw-pointer plus in-lock external calls and only add ADCS/COMM consumers. Rejected because it scales the deadlock and reentrancy risk.
- Replace `RecoveryExecutor` with pure F' ports in this change. Rejected because the repo already uses narrow runtime interfaces for some internal topology services and the bounded v1 goal is reducing coupling, not rearchitecting every runtime hook.

### 2. Runtime action coupling shrinks to narrow recovery-control interfaces

This change does not broaden raw component pointer ownership inside `RecoveryExecutor`. Existing EPS/mode/boot hooks are refactored behind the same short-lock boundary, and new ADCS/COMM control is exposed through narrow recovery-facing runtime methods rather than by letting the executor reach deeper into detector internals.

Bounded action hooks:

- EPS: existing `resetForRuntime()` path remains the real `R3` action.
- ADCS: no reset plane is added; `R2` remains restart intent only.
- COMM: add `performRecoveryLinkFailoverForRuntime()` plus bounded status access for the current primary link and error-growth state.
- Boot: keep reboot-intent truth in `BootManager`.

Alternative considered:

- Add broad runtime pointers to `AdcsBridge` and `CommController` and let the executor call any internal helper it wants. Rejected because it recreates the same coupling problem that the review flagged.

### 3. Shared incident vocabulary stays bounded and explicit

The change adds these shared incident sources:

- `WATCHDOG_ADCS_FDIR`
- `ADCS_POLL_TRANSPORT`
- `ADCS_POLL_FRESHNESS`
- `COMM_PRIMARY_UNAVAILABLE`
- `COMM_PRIMARY_TRANSPORT`

The shared recovery action vocabulary adds `COMM_LINK_FAILOVER`. `ResetCause` adds only reboot-truth causes for ADCS and COMM. A static index mapping helper plus unit coverage will verify every indexed recovery source, name, initial level, clear path, and reboot-cause mapping.

Alternative considered:

- Leak subsystem-local enums directly into the executor tables. Rejected because it makes shared tests and boot metadata mapping harder to reason about.

### 4. ADCS FDIR reads only scheduled poll health

`AdcsBridge` gains a dedicated `AdcsPollHealthState` updated only by `schedIn` polling. Command handlers may still emit local `ADCS_COMM_ERROR` or `ADCS_SENSOR_FAULT`, but they must not mutate the scheduled FDIR counters.

Detector rules:

- `consecutiveTransportFailures` increments only when the scheduled poll transport fails.
- `consecutiveNoValidRefresh` increments only when the scheduled poll transport succeeds but produces no valid refresh.
- any scheduled valid refresh clears both counters.
- single `sensor_valid = 0` stays a local ADCS event unless it repeats through the scheduled no-valid-refresh path.

Thresholds are fixed in v1:

- latch at `3` consecutive scheduled transport failures
- latch at `3` consecutive scheduled no-valid-refresh cycles
- clear on the first scheduled healthy valid cycle

Alternative considered:

- Let command-path `ADCS_GET_ATTITUDE` or `ADCS_SET_MODE` failures contribute to FDIR. Rejected because operator traffic would then alter subsystem-autonomy fault truth.

### 5. COMM detector stays in `CommController`, but actuation moves out

COMM FDIR remains a bounded helper inside `CommController`. It samples only the current primary link's availability and cumulative `txErrors + rxErrors` growth during `schedIn`.

Detector rules:

- `COMM_PRIMARY_UNAVAILABLE` latches after `3` consecutive scheduled cycles where the current primary link is unavailable.
- `COMM_PRIMARY_TRANSPORT` latches after `3` consecutive scheduled cycles where the current primary link remains available but its cumulative `txErrors + rxErrors` total grows.
- the first scheduled cycle with primary available and no new error growth clears the active COMM detector fault.

Detector-side link-loss actuation is removed. `CommController` will no longer directly:

- switch primary links because of fault detection
- revoke the current primary session because of fault detection
- clear or drop downlink owners because of fault detection

Those actions move into `performRecoveryLinkFailoverForRuntime()`, which returns a structured result containing:

- `switched`
- `alreadyOnHealthyPrimary`
- `noHealthyBackup`
- `sessionsRevoked`
- `ownersCleared`
- `finalPrimaryCommandLink`
- `finalPrimaryTelemetryLink`
- `finalPrimaryFileLink`

`RecoveryExecutor` uses that result to decide whether a COMM incident stops at `R3` or also needs `R5 SAFE`.

Alternative considered:

- Keep current detector-side failover and only notify the executor afterward. Rejected because it breaks the single-owner recovery boundary.

### 6. Recovery progression is per-subsystem and deterministic

Initial levels:

- watchdog incidents, including `WATCHDOG_ADCS_FDIR`: `R2`
- `EPS_TIMEOUT`: `R3`
- `ADCS_POLL_TRANSPORT` and `ADCS_POLL_FRESHNESS`: `R2`
- `COMM_PRIMARY_UNAVAILABLE` and `COMM_PRIMARY_TRANSPORT`: `R3`

Action mapping:

- `EPS_TIMEOUT`: real `R3` EPS reset, then optional `R5 SAFE`
- `ADCS_*`: `R2` restart intent only, then optional `R5 SAFE`
- `COMM_*`: real `COMM_LINK_FAILOVER`, then optional `R5 SAFE` if no healthy backup remains

Escalation rules:

- if the detector clears, the matching incident closes
- if the same incident relatches after clear, the next active epoch escalates to `R6`
- if the incident remains uncleared for `3` executor ticks after its bounded first action, it escalates to `R6`

No incident auto-restores the prior mode or the prior nominal COMM primary link.

### 7. Boot truth remains reboot-only

`BootManager` remains the sole persisted boot-truth owner. `ResetCause`, `lastRecoverySource`, `lastRecoveryLevel`, `consecutiveResetCount`, and `bootSafeFallbackRequired` are updated only when `RecoveryExecutor` records reboot intent at `R6`.

Non-reboot actions:

- `PROCESS_RESTART_INTENT`
- `SUBSYSTEM_INTERFACE_RESET`
- `SAFE_FALLBACK`
- `COMM_LINK_FAILOVER`

update recovery status/event/tlm only. They do not rewrite persisted boot reset truth.

Alternative considered:

- Persist every recovery action into boot metadata. Rejected because it would make boot reset cause untruthful.

### 8. Topology order and watchdog coverage become explicit active-baseline rules

`TopCcsds` fast-group order becomes:

1. `epsBridge`
2. `epsFdirController`
3. `modeSafetyController`
4. `adcsBridge`
5. `adcsFdirController`
6. `commController`
7. `watchdogSupervisor`
8. `recoveryExecutor`

`AdcsFdirController` is added to the watchdog supervised set so the multi-subsystem FDIR path does not leave the new detector itself unsupervised.

Legacy `Top` receives only the minimum build-safe wiring needed to compile and keep shared contracts aligned; active runtime proof remains on `TopCcsds`.

## Risks / Trade-offs

- [Risk] Short-lock refactor could accidentally change current EPS/watchdog behavior. → Mitigation: preserve existing EPS/watchdog tests, rerun the earlier hosted recovery probe, and keep incident mapping tables explicit.
- [Risk] Removing COMM detector-side actuation changes an already-proven runtime behavior surface. → Mitigation: update the COMM delta spec and proof to state that fault-driven switching is now executor-owned and no longer auto-restores nominal S-band.
- [Risk] ADCS scheduled-poll-only counters may diverge from older operator expectations if command-path failures were implicitly treated as health failures. → Mitigation: add explicit tests that command-triggered transport failures do not latch shared ADCS FDIR.
- [Risk] New source/action tables can drift from enum ordering. → Mitigation: add static assertions and direct mapping tests for every indexed source.
- [Risk] Hosted probe could conflate detector-local truth and shared recovery truth. → Mitigation: make the probe assert both local detector status and shared `GET_RECOVERY_STATUS` outcomes as separate observations.

## Migration Plan

1. Write and validate the OpenSpec artifacts plus delta specs for the bounded `EPS + ADCS + COMM` scope.
2. Refactor shared recovery types and `RecoveryExecutor` to the short-lock plus lock-free action model.
3. Add scheduled ADCS poll health plus new `AdcsFdirController`, wire it into `TopCcsds`, watchdog supervision, and shared recovery.
4. Move COMM fault-driven actuation behind executor-owned recovery hooks and add COMM detector normalization into the shared recovery path.
5. Extend boot/recovery metadata truth for ADCS and COMM reboot-intent closure.
6. Add or update component/helper tests, then add the hosted `run_multi_subsystem_fdir_v1_probe.sh` path and evidence.
7. Run fresh verification, focused hosted probes, OpenSpec validation, and update canonical docs.

## Open Questions

- None for v1. Automatic nominal-link restore, broader subsystem coverage, and target-hardware recovery proof remain deliberate future-scope items.
