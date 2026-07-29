## Context

The repository currently exposes `SatMode` through shared FPP types, `ModeManager` commands/events/telemetry, runtime CLI parsing, onboard state snapshots, live beacon encode/decode, and official HK trend products. The existing provisional `MissionExecutive` also depends on `LOW_POWER`, but the selected final mode model replaces that concept with a later HELL/SAFE threshold policy rather than a direct rename.

## Goals / Non-Goals

**Goals:**

- Replace the primary public mode vocabulary with `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC`.
- Preserve the existing numeric range and keep `SAFE=0`, `IDLE=1`, `HELL=2`, `PAYLOAD=3`, `TTC=4`.
- Keep HK trend product identity and live beacon wire size stable while bumping their schema versions to distinguish v2 mode semantics.
- Remove the current provisional `MissionExecutive` low-power/detumble behavior from the active topology so later autonomy work can rebuild it around the final mode model.

**Non-Goals:**

- No HELL/SAFE/IDLE hysteresis policy, load shedding, FDIR, watchdog, or scheduler behavior.
- No PAYLOAD execution behavior beyond a commandable mode shell.
- No TTC pass automation, TLE/GPS pass scheduling, COMM link split, CCSDS migration, reliable transfer, or storage retention change.
- No new target hardware validation path.

## Decisions

- Use `SAFE=0`, `IDLE=1`, `HELL=2`, `PAYLOAD=3`, `TTC=4` instead of severity ordering. This keeps the old safe/default adjacent values close to existing behavior while still removing the retired names from the public enum.
- Treat `HELL` as a new mode value, not a direct implementation rename of `LOW_POWER`. The SoC thresholds and hysteresis belong to `mode-autonomy-policy-v2`.
- Retire active `MissionExecutive` scheduling and `ModeManager` low-power interfaces in this change. Keeping a component that still commands a retired enum would leave the runtime in a misleading state; deleting historical evidence is not required.
- Keep the beacon frame size unchanged and bump the beacon schema version to `2`. Existing mode bytes are still one byte, but pre-v2 captures must not be decoded as the v2 mode set.
- Keep the `HkTrendRecord` product record name and id unchanged, but bump the payload type/version to `HkTrendRecordV3` / `version = 3` so pre-mode-model-v2 V2 `.fdp` files are not treated as compatible with the regenerated dictionary.

## Risks / Trade-offs

- `MissionExecutive` behavior disappears from the active runtime until the later autonomy redesign. Mitigation: specs and evidence explicitly mark this as intentional scope, and mode command/HK/beacon coverage remains in place.
- Historical evidence records still mention `LOW_POWER`. Mitigation: preserve them as historical records and update only active specs/evidence claims.
- Mode enum changes are dictionary-breaking for consumers expecting retired names. Mitigation: call out the breaking public API change and verify generated dictionary/build/test paths before archive.

## Migration Plan

- Create the OpenSpec change and delta specs first.
- Update shared FPP types and generated dictionary surfaces through the normal F' generate/build flow.
- Update runtime CLI/help/status, component tests, onboard state helper tests, HK trend tests, and any integration tests that construct snapshots with retired modes.
- Archive only after local verification and OpenSpec validation pass.

## Open Questions

- None for this change. The policy thresholds, manual/automatic `SAFE -> IDLE` recovery, and full MissionExecutive/FDIR architecture are assigned to later governed changes.
