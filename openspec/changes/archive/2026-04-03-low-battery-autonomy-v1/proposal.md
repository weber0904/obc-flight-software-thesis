## Why

The repository now has a stable hosted OBC baseline plus a repository-owned scenario replay bridge, but it still does not make any system-level decisions from subsystem state. The next most useful extension is the first autonomy case: when EPS reports low battery, the OBC should enter `LOW_POWER` and command an energy-safe ADCS attitude instead of relying on manual operator action.

## What Changes

- Add a first-version `MissionExecutive` component that observes cached subsystem state and drives system-level autonomy actions.
- Implement the first autonomy case: low battery triggers `SatMode::LOW_POWER` and commands ADCS into a first-version sun-safe pointing profile.
- Reuse the current `ADCS POINTING` mode and a repository-owned fixed sun-safe target quaternion for this slice instead of introducing a new `SUN_TRACKING` enum immediately.
- Extend scenario-driven validation and verification evidence so the low-battery autonomy case is reviewable in hosted testing.
- Keep comm/load shedding, automatic low-power exit, deploy/detumble sequencing, and ground-pass automation out of scope for this slice.

## Capabilities

### New Capabilities

- `mission-autonomy`: first-version mission executive behavior that reacts to subsystem state and issues system-level mode and ADCS pointing commands.

### Modified Capabilities

- `scenario-driven-validation`: add the first low-battery autonomy scenario on top of the existing scenario replay architecture.
- `verification-evidence`: require reviewable evidence for the low-battery autonomy slice and its hosted validation path.

## Impact

- Affected code: `OBC/Components`, `OBC/Top`, and hosted scenario validation tests.
- Affected systems: system-mode changes, ADCS pointing command flow, hosted scenario validation, and autonomy evidence/docs.
- No breaking changes to the direct TCP/GDS baseline, Raspberry Pi target flow, or the existing comm/transparent transport stack.
