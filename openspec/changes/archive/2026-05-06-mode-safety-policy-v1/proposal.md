## Why

The active mode-model-v2 baseline now exposes the final `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC` mode vocabulary, but it intentionally retired the provisional `MissionExecutive` autonomy behavior that was coupled to `LOW_POWER`, sun-safe pointing, and detumble semantics. The next governed slice is a narrow safety fallback policy that reacts only to cached EPS state of charge and commands the existing `ModeManager` mode surface.

## What Changes

- Add a new narrow `ModeSafetyController` real F' component as the v1 policy owner for cached-EPS SoC safety fallback.
- Implement deterministic strict-threshold fallback behavior:
  - `SAFE -> HELL` when SoC is `< 10%`.
  - `HELL -> SAFE` when SoC is `> 15%`.
  - `IDLE`, `PAYLOAD`, or `TTC -> SAFE` when SoC is `< 40%`.
- Keep `SAFE -> IDLE` manual; SoC recovery above 50% only remains a future configurability boundary and does not trigger automatic mode recovery.
- Keep the old provisional `MissionExecutive` retired from active topology. Do not revive `LOW_POWER`, sun-safe pointing, ADCS detumble, EPS load shedding, COMM, CCSDS, scheduler, watchdog, or FDIR timeout/reset behavior.
- Add focused component/helper/integration and hosted evidence proving the new SoC-driven safety path against the active topology.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-autonomy`: replace the deferred-autonomy baseline with a narrow SoC-driven mode safety fallback policy owned by `ModeSafetyController`, while leaving full MissionExecutive/FDIR redesign deferred.
- `verification-evidence`: require reviewable evidence for the new component, helper, integration, and hosted safety-policy path, with explicit exclusions for adjacent unfinished work.

## Impact

- Affected public/runtime surfaces: active topology scheduling, mode runtime binding, EPS cached status access, `SYS_MODE_CHANGE` through the existing `ModeManager` path, and focused hosted probe support.
- Affected implementation areas: `OBC/Components`, `OBC/Top`, EPS simulator startup options, focused tests, hosted probe script, and evidence records.
- No enum/dictionary-breaking mode changes, no COMM split-link work, no CCSDS work, no PAYLOAD/TTC scheduler or pass automation, no FDIR timeout/retry/reset/watchdog expansion, and no target hardware or RF validation claims.
