## 1. OpenSpec Artifacts

- [x] 1.1 Create the `mode-model-v2` OpenSpec change on `feature/mode-model-v2`.
- [x] 1.2 Add proposal, design, delta specs, and task list for the v2 mode model.
- [x] 1.3 Validate the change artifacts before implementation.

## 2. Core Mode Contract

- [x] 2.1 Replace `SatMode` with `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC` using the governed numeric values.
- [x] 2.2 Update `ModeManager` and runtime CLI/help/status parsing to use the v2 mode names.
- [x] 2.3 Remove active topology scheduling/binding for the provisional `MissionExecutive` and decouple `ModeManager` from low-power runtime interfaces.

## 3. Data Surfaces

- [x] 3.1 Update onboard state, live beacon encode/decode, and focused tests for the v2 mode valid set without changing beacon wire size.
- [x] 3.2 Update HK trend and housekeeping snapshot tests/data to use v2 modes.

## 4. Verification And Evidence

- [x] 4.1 Run focused component/helper tests for mode, beacon, snapshot, and HK trend behavior.
- [x] 4.2 Run fresh repository verification and OpenSpec validation.
- [x] 4.3 Record reviewable evidence for mode-model-v2 and archive the change when ready.
