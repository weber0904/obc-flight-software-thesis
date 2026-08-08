# Proposal: payload-single-ready-auto-actual-metadata-v1

## Why

The current payload capture contract still mixes two different concerns:

- whether the camera/backend is prepared and ready to capture
- which capture policy (`AUTO`, `DETERMINISTIC`, legacy `STILL` wrapper, or
  `RAW_SENSOR`) is being requested

The active runtime state machine already has a single `READY` state, but
`PayloadOpsController` still gates normal capture on `m_preparedSessionKind`.
That makes `PAYLOAD_PREPARE()` behave as if it were "prepare deterministic"
instead of "prepare camera", and forces operators to think in multiple prepared
states that do not exist at the backend layer.

At the same time, target real-camera `AUTO` capture does not yet report the
actual runtime values chosen by `libcamera`. Current capture metadata records
the requested/controller-resolved settings, but not the actual sensor-side
result. This blocks the intended operator workflow:

1. capture once with `AUTO`
2. inspect the actual auto-selected values
3. capture again with `DETERMINISTIC` using chosen fixed parameters

Finally, current target deterministic support is still narrower than needed.
The target `libcamera` backend currently exposes manual exposure/gain and
preview JPEG quality, but still needs current governed support for
orientation-flip controls so deterministic captures can be repeated without
extra operator-side image correction.

## What Changes

- collapse normal still capture onto a single camera-ready `READY` gate
- keep `RAW_SENSOR` as the only special prepared-session gate
- keep `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` for compatibility, but
  make them deprecated aliases of plain camera prepare
- add actual target capture metadata for `AUTO`:
  - actual exposure
  - actual analogue gain
  - actual AWB result
- extend current target deterministic support to include:
  - `jpegQuality`
  - `hflip`
  - `vflip`
- clarify and document that `jpegQuality` affects only the flight-software
  preview JPEG, not the raw `.bin`

## Impact

- payload operator workflow becomes simpler and closer to the real backend
  state model
- target `AUTO` becomes useful for parameter scouting instead of only for
  "fire and forget" imaging
- target deterministic capture gains a bounded, documented set of repeatable
  controls without trying to close full generic camera-control parity
