## Why

The active baseline still treats `TTC` as a manual mode shell. Operators can enter and exit it through the guarded mode path, but the runtime has no owned policy for when TT&C should begin automatically, when it should end automatically, or how pass-window truth, GPS validity, and existing safety/recovery paths constrain that behavior.

The next bounded baseline slice needs a first-version TTC automation owner that can consume one explicit pass window at runtime and drive `IDLE -> TTC -> IDLE` without turning this PR into a generic scheduler, TLE engine, payload planner, ADCS tracking framework, or target-deployment closure.

## What Changes

- Add a dedicated `TtcPassManager` component as the single runtime owner of TTC pass-window policy.
- Add bounded TT&C config and pass-window control surfaces:
  - `enabled`
  - `loss_of_lock_timeout_sec`
  - one active pass window using `start_unix_sec` and `end_unix_sec`
  - an explicit clear-window command
- Make the runtime auto-enter `TTC` from `IDLE` only when:
  - TTC is enabled
  - a valid pass window is configured
  - cached GPS time is valid and fresh
  - the GPS-derived epoch time is inside the active window
- Make the runtime auto-exit `TTC` back to `IDLE` when:
  - the pass window ends
  - the pass window is cleared or invalidated
  - GPS time validity is lost
  - a bounded COMM loss-of-lock timeout is exceeded
- Keep `ModeSafetyController`, `RecoveryExecutor`, and existing safety/fault paths authoritative over safety and recovery outcomes.
- Keep manual `MODE_SET TTC` / hosted `mode ttc` available, but make any active `TTC` instance subject to the same TTC policy owner.
- Add reviewable TTC policy status/event/telemetry surfaces, focused tests, a hosted probe, OpenSpec deltas, and evidence/docs updates.
- Explicitly defer ADCS pointing, TLE parsing/propagation, generic time-tagged scheduling, payload scheduling, target/Pi closure, and broad COMM redesign.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: extend the active TTC contract from a manual shell to a bounded pass-window-driven TTC policy with explicit config, pass-window, status, command-response, and mode-transition semantics.
- `mission-autonomy`: add `TtcPassManager` as a focused autonomy owner for TTC pass-window policy while keeping `ModeSafetyController` SoC-only and preserving the existing recovery/safety ownership split.
- `verification-evidence`: require reviewable evidence for TTC pass-window policy entry/exit behavior, safety precedence, hosted proof, focused tests, and explicit deferred boundaries.

## Impact

- Affected public/runtime surfaces: TTC runtime commands and hosted shell controls, TTC policy status/telemetry/events, internal mode-source tagging for TTC policy transitions, and runtime service interfaces used by the hosted shell and test/probe code.
- Affected implementation areas: `OBC/Components/TtcPassManager`, bounded helper logic for GPS UTC-to-epoch validity, active topology wiring, runtime service plumbing, hosted runtime shell handling, focused tests, hosted probe scripts, OpenSpec artifacts, evidence/docs, and verification registry updates.
- **BREAKING**: `TTC` is no longer a pure manual shell. Once `TTC` is active, the TTC pass policy may return the system to `IDLE` if TTC is disabled, the pass window is inactive, GPS validity is lost, or the COMM loss timeout expires.
- No generic scheduler, no TLE upload/parser, no ADCS ground tracking, no payload-op scheduling, no COMM architecture redesign, no target/Pi deployment proof, and no persistent event-log framework are added in this change.
