## Context

`mode-safety-policy-v1` already gives `ModeSafetyController` ownership of cached-EPS SoC fallback for `SAFE -> HELL`, `HELL -> SAFE`, and `IDLE/PAYLOAD/TTC -> SAFE`. `payload-ttc-mode-entry-v1` already routes `MODE_SET` and hosted `mode <...>` through a guarded operator path instead of treating mode changes as arbitrary setters.

The remaining final-design gap is narrower than a new autonomy framework:

- `SAFE -> IDLE` needs cached-EPS SoC admission.
- `IDLE -> PAYLOAD` needs cached-EPS SoC admission.
- `PAYLOAD -> IDLE` needs cached-EPS automatic exit.
- `PAYLOAD -> SAFE` below `40%` must remain stronger than the new `< 60%` exit.
- `IDLE -> TTC` must remain topology/manual only until future pass-scheduler work exists.

The design therefore stays inside the current responsibility split:

- `ModeManager` remains the public mode command/event/telemetry owner.
- `ModeSafetyController` remains the cached-EPS policy owner.
- `MissionExecutive` remains retired.

## Goals / Non-Goals

**Goals**

- Add cached-EPS SoC admission to operator `SAFE -> IDLE` and `IDLE -> PAYLOAD`.
- Add automatic cached-EPS `PAYLOAD -> IDLE` at `< 60%`.
- Preserve strict precedence of `PAYLOAD -> SAFE` at `< 40%`.
- Preserve direct-entry topology guards and unchanged `IDLE -> TTC`.
- Reuse current cache-validity semantics for fail-closed behavior.

**Non-Goals**

- No timestamp freshness or age threshold on EPS cache.
- No new mode engine or `MissionExecutive` replacement.
- No TTC scheduler, TLE/GPS/pass-window logic, ADCS tracking, or payload control.
- No watchdog, subsystem timeout/retry/reset FDIR, authority/session lifecycle, or configurable threshold commands.

## Decisions

### ModeSafetyController Owns Both Admission And Automatic Exit

The existing operator guard interface already lets `ModeSafetyController` decide whether an operator transition is accepted. This change extends that decision logic for:

- `SAFE -> IDLE`: accept only when cached SoC `> 50%`
- `IDLE -> PAYLOAD`: accept only when cached SoC `> 70%`
- `HELL -> SAFE`: keep accept only when cached SoC `> 15%`

Topology-only decisions remain unchanged:

- `IDLE -> TTC` stays allowed with no new SoC threshold
- `SAFE -> PAYLOAD/TTC`, direct `PAYLOAD <-> TTC`, and operator entry to `HELL` stay rejected

The same component also extends autonomous policy for `PAYLOAD -> IDLE` at cached SoC `< 60%`.

### Cached EPS Availability Defines Fail-Closed

This change adopts the current cache contract as the sole stale/unavailable definition:

- `IModeSafetyEpsStatus::getCachedStatusForRuntime(...) == true` means authoritative cached status is available.
- `== false` means unavailable and therefore fail-closed for SoC-guarded admissions.
- A later failed EPS poll already invalidates the cache in `EpsBridge`; that remains the only stale-cache path used here.

No timestamp or age-based freshness is added because the current runtime interface does not carry time metadata.

### Generic Shared SoC Guard Reason Names

Current numeric reason codes `3` and `4` stay stable but are renamed from recovery-specific names to shared names:

- `3`: `SOC_GUARD_UNAVAILABLE`
- `4`: `SOC_GUARD_NOT_MET`

These codes now apply to:

- `HELL -> SAFE` recovery
- `SAFE -> IDLE` admission
- `IDLE -> PAYLOAD` admission

The event already carries `fromMode` and `toMode`, so the blocked transition remains fully reconstructible without multiplying public codes.

### Automatic PAYLOAD Exit Uses Explicit Precedence

`ModeSafetyPolicy` becomes an ordered policy for `PAYLOAD`:

1. If cached SoC `< 40%`, request `SAFE`
2. Else if cached SoC `< 60%`, request `IDLE`
3. Else remain in `PAYLOAD`

Other modes preserve prior behavior:

- `SAFE -> HELL` at `< 10%`
- `HELL -> SAFE` at `> 15%`
- `IDLE -> SAFE` and `TTC -> SAFE` at `< 40%`

This keeps the final-design hysteresis intent without introducing configurable thresholds in v1.

### Internal Apply Source Stays Explicit

`ModeManager` already has an explicit internal apply path. This change extends the source enum with an additional explicit source for automatic PAYLOAD exit so telemetry/tests can distinguish:

- `SafetyFallback`
- `SafetyRecovery`
- `SafetyPayloadExit`
- `TestSetup`

Operator requests still must not use the internal apply path directly.

## Verification Strategy

- L1 helper tests cover new policy thresholds and precedence.
- L2 classic component tests cover SoC-guarded operator admissions, unavailable cache fail-closed behavior, preserved topology, and unchanged `IDLE -> TTC`.
- Focused integration tests cover real `ModeManager` interaction for `PAYLOAD -> IDLE` and `< 40%` precedence.
- Hosted shell regression covers operator-visible behavior through the guarded runtime shell.
- Hosted default CCSDS S-band probe covers `MODE_SET` admissions/rejections and final `SYS_MODE` telemetry on the existing path.

## Risks / Trade-offs

- Keeping stale semantics as cache-validity only does not detect “old but still valid” data. That is intentional scope control for this vertical slice and matches the current interface contract.
- Reusing numeric SoC guard codes while renaming them updates a public name but avoids growing the stable code catalog.
- Adding PAYLOAD automatic exit to `ModeSafetyController` slightly broadens the component, but it still stays inside the same mode/power-safety ownership area and does not become a general mission-execution engine.
