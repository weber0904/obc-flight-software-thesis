## Why

The active mode-model-v2 baseline already has the final `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC` mode shell, plus cached-EPS SoC fallback through `ModeSafetyController` and guarded manual PAYLOAD/TTC topology entry through `ModeManager`. What is still missing from the final design is the SoC admission and automatic PAYLOAD exit behavior that sits between those two completed slices.

This change closes that gap without creating a new mode engine and without reviving the retired provisional `MissionExecutive`. It extends the existing `ModeManager` guarded operator path and the existing `ModeSafetyController` cached-EPS policy path to add final-design SoC admission and PAYLOAD exit behavior as one mode/power-safety vertical slice.

## What Changes

- Guard operator/manual `SAFE -> IDLE` on cached EPS SoC `> 50%`.
- Guard operator/manual `IDLE -> PAYLOAD` on cached EPS SoC `> 70%`.
- Keep operator/manual `IDLE -> TTC` free of any new SoC admission threshold.
- Add automatic `PAYLOAD -> IDLE` when cached EPS SoC is `< 60%`.
- Keep existing `PAYLOAD -> SAFE` at cached EPS SoC `< 40%` and make it higher priority than `PAYLOAD -> IDLE`.
- Preserve `SAFE -> HELL` at `< 10%`, `HELL -> SAFE` at `> 15%`, direct-entry topology rejection, and operator entry to `HELL` rejection.
- Reuse the current cached EPS validity contract for fail-closed admission semantics. This change does not add timestamp freshness.
- Rename the public SoC guard rejection reason names to generic shared names so they cover both recovery and admission without adding new numeric codes.
- Add focused tests, hosted shell evidence, and default hosted CCSDS S-band `MODE_SET` evidence for the new SoC admission and PAYLOAD exit behavior.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-autonomy`: extend `ModeSafetyController` from cached-EPS fallback and `HELL -> SAFE` recovery guard into final-design SoC admission and `PAYLOAD -> IDLE` exit ownership.
- `core-system-contracts`: refine guarded operator transition semantics so `SAFE -> IDLE` and `IDLE -> PAYLOAD` become cached-EPS guarded admits instead of unconditional admits, and rename shared SoC guard rejection reason names.
- `verification-evidence`: require reviewable evidence for guarded SoC admissions, automatic PAYLOAD exit precedence, unchanged `IDLE -> TTC`, and the closeout validation commands.

## Impact

- Affected public/runtime surfaces: `ModeManager.MODE_SET`, hosted runtime `mode <...>` behavior, `SYS_MODE_TRANSITION_REJECTED.reasonCode`, `ModeSafetyController` telemetry/events, and default hosted CCSDS S-band command-path evidence.
- Affected implementation areas: `OBC/Components/ModeSafetyController`, `OBC/Components/ModeManager`, focused tests, hosted probes, OpenSpec artifacts, verification registry/evidence, and local pending notes.
- No new mode manager, no `MissionExecutive` restoration, no TTC pass scheduler, no ADCS ground tracking, no payload/camera control, no watchdog, no subsystem-timeout FDIR, no command-auth/session lifecycle work, and no configurable threshold commands.
