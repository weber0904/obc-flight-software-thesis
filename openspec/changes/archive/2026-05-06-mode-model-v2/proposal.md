## Why

The current primary spacecraft mode model still reflects the early baseline (`SAFE`, `NOMINAL`, `LOW_POWER`, `DEBUG`, `UPDATE`) and now conflicts with the selected final-design mode vocabulary. The next governed integration step is to replace that public mode contract before adding later autonomy, COMM, CCSDS, storage, FDIR, or pass-scheduling behavior.

## What Changes

- **BREAKING** Replace the primary `SatMode` enum with `SAFE=0`, `IDLE=1`, `HELL=2`, `PAYLOAD=3`, and `TTC=4`.
- **BREAKING** Remove `NOMINAL`, `LOW_POWER`, `DEBUG`, and `UPDATE` from the primary mission mode enum and update command, telemetry, event, dictionary, runtime status, HK trend, and beacon decode surfaces to the v2 names.
- Retire the current provisional low-battery `MissionExecutive` behavior from the active runtime baseline; complete HELL/SAFE/IDLE threshold policy, FDIR, and MissionExecutive redesign remain deferred to later changes.
- Keep `PAYLOAD` and `TTC` as commandable mode shells in this change without payload/camera behavior, ground-pass automation, TLE scheduling, COMM split-link behavior, CCSDS migration, or storage-policy changes.
- Preserve historical evidence and archived specs as historical records, while preventing the active main specs from continuing to claim `LOW_POWER` autonomy as current behavior.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: replace the shared `SatMode` public contract and keep core mode command/telemetry surfaces aligned with the v2 enum.
- `mission-autonomy`: retire the provisional active `LOW_POWER` autonomy claim and state that the full MissionExecutive/FDIR policy will be rebuilt in later governed changes.
- `scenario-driven-validation`: remove active validation claims that scenario replay must demonstrate `LOW_POWER` entry in the current baseline.
- `onboard-data-products-and-live-beacon`: update reduced-state and beacon mode validity to the v2 enum without changing the beacon wire size.
- `hk-data-products`: update HK trend mode field expectations to the v2 enum while preserving the existing product record identity and version.
- `verification-evidence`: require reviewable evidence for hosted mode-model, HK trend, and beacon decode compatibility while excluding COMM, CCSDS, storage-policy, FDIR, and hardware claims.

## Impact

- Affected public API: `SatMode`, `ModeManager.MODE_SET`, `SYS_MODE`, `SYS_MODE_CHANGE`, runtime CLI mode parsing/status text, HK trend `mode`, and live beacon mode encode/decode validation.
- Affected implementation areas: `OBC/Types`, `OBC/Components/ModeManager`, `OBC/Main.cpp`, onboard state data helpers, HK trend tests, beacon tests, topology scheduling/bindings for the provisional `MissionExecutive`, and focused hosted mode smoke coverage.
- No new external dependencies, no COMM node or service changes, no CCSDS wire-protocol changes, no storage retention changes, and no target hardware validation claims.
