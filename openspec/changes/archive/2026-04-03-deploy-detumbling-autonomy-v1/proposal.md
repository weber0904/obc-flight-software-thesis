## Why

The repository now has a first-version `MissionExecutive` plus scenario-seeded ADCS initial angular rates, but it still lacks the other natural autonomy case discussed earlier: when the spacecraft starts with deployment-like high angular rates, the OBC should recognize that condition and command ADCS into `DETUMBLE` without requiring an operator to notice and intervene manually.

## What Changes

- Extend the first-version `MissionExecutive` so it can react to cached ADCS state in addition to cached EPS state.
- Implement the first deployment-style autonomy case: high ADCS angular-rate norm triggers `ADCS_SET_MODE(DETUMBLE)`.
- Add hosted validation showing scenario-seeded initial angular rates drive the new detumble autonomy path and that the rate norm converges below the mission detumble threshold.
- Keep deploy phase management, post-detumble mission transitions, and richer phase sequencing out of scope for this slice.

## Capabilities

### Modified Capabilities

- `mission-autonomy`: add the first deployment-style detumbling response on top of the existing low-battery autonomy baseline.
- `scenario-driven-validation`: add a hosted replay case where scenario-seeded initial angular rates trigger detumbling autonomy.
- `verification-evidence`: require reviewable evidence for the high-angular-rate detumbling autonomy slice.

## Impact

- Affected code: `OBC/Components/MissionExecutive`, `OBC/Components/AdcsBridge`, and hosted autonomy validation tests.
- Affected systems: mission autonomy policy ordering, ADCS runtime-control helpers, and hosted scenario-driven validation.
- No breaking changes to Raspberry Pi target flow, comm/transparent transport, boot/update, or the existing low-battery autonomy behavior.
