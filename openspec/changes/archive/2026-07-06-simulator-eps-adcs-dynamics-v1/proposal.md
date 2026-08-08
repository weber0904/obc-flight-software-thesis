## Why

The hosted EPS and ADCS simulators currently emit mostly fixed values, which makes mission-console demos, trend plots, and staged route evidence look artificial even when the command and transport paths are working correctly. This change upgrades the simulator behavior to produce seeded, time-continuous curves that remain reviewable and reproducible without changing the OBC public contracts.

## What Changes

- Add simulator-owned EPS runtime load modes `normal` and `high-draw` with seeded pseudo-noise, named PDU channel weights, and time-driven SoC/current/voltage evolution.
- Add simulator-owned ADCS time-continuous mode dynamics so `IDLE`, `DETUMBLE`, and `POINTING` produce bounded but non-static quaternion, angular-rate, and pointing-error behavior.
- Extend the simulator control sockets with minimal new demo controls: EPS `set-load-mode` and ADCS `restart-pointing-pass`.
- Update OpenSpec delta specs, narrative simulator docs, and simulator-focused tests to accept bounded trend/range behavior instead of fixed-value expectations.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `eps-subsystem`: Hosted EPS simulator behavior now includes seeded pseudo-noise, named PDU load semantics, sticky runtime load modes, and a simulator-owned external load-mode control surface.
- `adcs-subsystem`: Hosted ADCS simulator behavior now includes time-continuous mode-aware dynamics, a synthetic pointing-pass profile, and a simulator-owned control to restart the pointing pass.

## Impact

- Affected code: `simulators/eps/*`, `simulators/adcs/*`, simulator control helpers, simulator unit/integration tests, and simulator-facing docs/specs.
- Public OBC APIs stay unchanged: no new `EPS_*` or `ADCS_*` commands, no wire-shape changes in `StatusData` or `StateData`.
- Existing probes and tests that asserted exact fixed EPS/ADCS values will need bounded range/trend assertions instead.
