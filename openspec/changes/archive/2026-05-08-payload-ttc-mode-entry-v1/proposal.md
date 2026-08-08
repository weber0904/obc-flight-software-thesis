## Why

The active mode-model-v2 baseline exposes `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC`, but `ModeManager.MODE_SET` still behaves like an arbitrary mode setter. The next governed slice needs a first-version operator transition policy so `PAYLOAD` and `TTC` can be entered and exited manually without making `HELL` an ordinary operator mode or moving SoC fallback ownership out of `ModeSafetyController`.

## What Changes

- Add a v1 operator transition guard for `ModeManager.MODE_SET` and the hosted runtime `mode <...>` shell path.
- Allow manual recovery and manual first-version mission shells:
  - `SAFE -> IDLE`
  - `IDLE -> PAYLOAD`
  - `PAYLOAD -> IDLE`
  - `IDLE -> TTC`
  - `TTC -> IDLE`
  - `IDLE/PAYLOAD/TTC -> SAFE`
- Keep same-mode operator requests as `OK` no-ops.
- Reject unsafe or undefined operator transitions such as `SAFE -> PAYLOAD`, `SAFE -> TTC`, `PAYLOAD -> TTC`, `TTC -> PAYLOAD`, and any ordinary operator entry into `HELL`.
- Allow operator `HELL -> SAFE` only through the `ModeSafetyController` guard when cached EPS SoC is strictly greater than `15%`.
- Add a rejection event with stable `U32` reason codes while preserving the existing `SYS_MODE_CHANGE(mode)` event shape.
- Rename the neutral internal mode-apply API so safety fallback and test setup cannot be confused with guarded operator requests.
- Update hosted probes and evidence to cover the shell path and the default hosted CCSDS S-band F Prime command path.
- Explicitly defer SoC admission for `SAFE -> IDLE`, `IDLE -> PAYLOAD`, and `IDLE -> TTC`; scheduler/pass automation; payload control; link authority; CCSDS route changes; HK data-product changes; and broader FDIR.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: change the public `MODE_SET` / hosted mode shell contract from arbitrary mode set to the v1 guarded operator transition matrix, with deterministic command response, event, and telemetry semantics.
- `mission-autonomy`: extend `ModeSafetyController` ownership from autonomous cached-EPS SoC fallback to the narrow operator transition guard decisions that depend on cached EPS, while preserving autonomous fallback ownership and keeping broader autonomy deferred.
- `verification-evidence`: require reviewable evidence for the operator transition matrix, shell parser regression, default hosted CCSDS S-band command/event/telemetry path, and explicit deferred boundaries.

## Impact

- Affected public/runtime surfaces: `ModeManager.MODE_SET`, `SYS_MODE`, `SYS_MODE_CHANGE`, the new `SYS_MODE_TRANSITION_REJECTED` event, hosted runtime `mode <...>` shell output, and runtime mode-control interfaces used by `ModeSafetyController`.
- Affected implementation areas: `OBC/Components/ModeManager`, `OBC/Components/ModeSafetyController`, `OBC/Runtime/HostedRuntime.*`, focused tests, hosted probe scripts, OpenSpec artifacts, verification registry/evidence, and pending planning docs.
- **BREAKING**: operator mode setting is no longer arbitrary. Existing scripts or probes that set `HELL` directly or jump directly from `SAFE` to `PAYLOAD`/`TTC` must use a valid operator sequence or an explicit internal safety/test fixture.
- No PAYLOAD subsystem behavior, TTC scheduler/pass-window behavior, link authority/auth/session behavior, CCSDS default route changes, UHF failover, HK data-product field changes, or broader FDIR/watchdog behavior is introduced.
