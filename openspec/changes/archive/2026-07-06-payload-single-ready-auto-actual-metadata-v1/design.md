# Design: payload-single-ready-auto-actual-metadata-v1

## Summary

This change redefines current payload still-capture semantics around a single
camera-ready state. `AUTO`, `DETERMINISTIC`, and legacy `STILL` remain capture
policies, not hidden prepared states. `RAW_SENSOR` remains the only special
prepared-session path.

The change also extends target metadata so `AUTO` captures expose the actual
runtime values selected by `libcamera`, and it widens current target
deterministic support to include preview JPEG quality plus flip orientation.

## Decisions

### Single camera-ready truth for normal still capture

`PayloadState::PSTATE_READY` remains the only normal still-capture prepared
state. `m_preparedSessionKind` is no longer used to gate:

- `PAYLOAD_CAPTURE_STILL`
- `PAYLOAD_CAPTURE_AUTO`
- `PAYLOAD_CAPTURE_DETERMINISTIC`

These commands require:

- runtime configured
- `PAYLOAD` mode
- no payload work in progress
- `PSTATE_READY`

`RAW_SENSOR` remains special. Raw register commands and raw capture still
require `PAYLOAD_PREPARE_SESSION(RAW_SENSOR)`.

### Prepare compatibility rules

- `PAYLOAD_PREPARE()` becomes the canonical plain camera-prepare surface.
- `PAYLOAD_PREPARE_SESSION(AUTO)` and
  `PAYLOAD_PREPARE_SESSION(DETERMINISTIC)` remain public commands for
  compatibility, but they execute the same backend prepare behavior as
  `PAYLOAD_PREPARE()`.
- These compatibility aliases are documented as deprecated operator-facing
  surfaces.
- `PAYLOAD_PREPARE_SESSION(RAW_SENSOR)` keeps its special meaning.

### Request truth versus actual truth

Current metadata fields continue to mean:

- `requestedSettings`: requested command-side settings
- `appliedSettings`: controller-resolved settings that were sent to the driver
- `requestedMask` / `appliedMask`: request and accepted field masks

New target-side actual fields are added to `PayloadCaptureMetadata`:

- `actualExposureUsec`
- `actualGainX100`
- `actualAwbValid`
- `actualAwbState`
- `actualColourTemperatureKelvin`
- `actualColourGainRedX1000`
- `actualColourGainBlueX1000`

The actual fields are populated by the target `libcamera` backend from the
completed request metadata. The change does not redefine `appliedSettings`
into "actual sensor result", because that would silently rewrite the meaning of
existing evidence and helper code.

### Current actual AUTO acceptance

Current target `AUTO` acceptance requires actual readback of:

- exposure
- analogue gain
- AWB result

The AWB result is represented through current `libcamera` metadata fields:

- AWB state
- colour temperature (when available)
- red/blue colour gains (when available)

This change does not require metering-result readback; metering remains part of
the `AUTO` request policy surface only.

### Target deterministic support boundary

Current target deterministic support after this change is:

- `resolutionPreset`
- `exposureUsec`
- `gainX100`
- `jpegQuality`
- `hflip`
- `vflip`

This change intentionally does not add current support for:

- `brightness`
- `contrast`
- `saturation`
- `sharpness`

### `jpegQuality` meaning

`jpegQuality` only controls the preview JPEG encoded by flight software from
the captured raw frame.

It does not affect:

- raw `.bin` bytes
- raw sensor exposure
- raw sensor gain

The current documented range remains `1..100`, with the runtime default
surfaced as the default preview JPEG quality.

## Interfaces

### Payload metadata and manifest/header surfaces

The following surfaces gain actual-capture fields:

- `PayloadCaptureMetadata`
- payload CSP metadata reply
- payload artifact header/manifest used for indexed replay and decode

This keeps local source artifacts, CSP metadata, and in-process runtime
readback aligned.

### Event/readback surface

`PAYLOAD_CAPTURE_METADATA` is extended so current operator-facing readback of
`PAYLOAD_GET_LAST_CAPTURE_METADATA` includes actual target capture values, not
just request/controller-resolved values.

Mission Console parsers and any structured-event helpers that parse
`PAYLOAD_CAPTURE_METADATA` are updated in the same change.

## Verification

Required verification in this change:

- focused `PayloadOpsController` UT coverage for single-ready behavior,
  compatibility aliases, raw gate retention, and request-vs-actual separation
- focused parser/helper coverage for the expanded metadata surface
- focused target proof showing:
  - one prepare
  - `AUTO` capture
  - actual auto readback
  - deterministic capture without re-prepare
  - deterministic request alignment
  - non-black source artifacts
  - working `hflip/vflip`
  - intact raw-session gate
