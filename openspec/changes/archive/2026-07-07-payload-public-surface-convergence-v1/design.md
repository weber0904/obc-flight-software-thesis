# Design: payload-public-surface-convergence-v1

## Context

The previous payload slices already converged normal still capture onto one
shared non-RAW `READY` window and retired `PAYLOAD_CAPTURE_STILL`. However,
they stopped short of removing the older compatibility surfaces and older
public naming. That leaves three current problems:

1. `PAYLOAD_SET_DEFAULTS -> PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_DETERMINISTIC`
   can now fail because prepare truth still depends on AUTO defaults while the
   deterministic route can request a different resolution later.
2. current docs/specs over-claim `jpegQuality/hflip/vflip` as deterministic
   per-capture operator controls even though they are not on the current
   capture command surface.
3. `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` is documented as a plain
   alias, but runtime still gives those choices real prepare-time meaning.

The fix is to stop treating this as a compatibility-preserving cleanup and
instead make the public payload surface match the true operator model.

## Goals / Non-Goals

**Goals**

- Remove stale compatibility commands from the maintained payload surface.
- Establish one explicit shared non-RAW session-defaults surface.
- Preserve `RAW_SENSOR` as a special prepare path with explicit naming.
- Split prepare truth from capture-policy truth across runtime, metadata,
  manifests, telemetry, events, and proofs.
- Keep persistent-session behavior and actual AUTO metadata readback intact.
- Preserve on-disk manifest backward compatibility for older captures.

**Non-Goals**

- No Route 1 full formal rerun; only focused maintained payload-route reruns.
- No new generic target parity claim for
  `brightness/contrast/saturation/sharpness`.
- No new payload downlink path or new data-product family.
- No attempt to preserve operator compatibility for retired payload commands.

## Decisions

### Current payload public surface is explicit

Maintained non-RAW still capture now uses:

- `PAYLOAD_SET_CAMERA_DEFAULTS`
- `PAYLOAD_PREPARE`
- `PAYLOAD_SET_AUTO_DEFAULTS`
- `PAYLOAD_SET_DETERMINISTIC_DEFAULTS`
- `PAYLOAD_CAPTURE_AUTO`
- `PAYLOAD_CAPTURE_DETERMINISTIC`
- `PAYLOAD_GET_LAST_CAPTURE_METADATA`
- `PAYLOAD_SHUTDOWN`

Maintained raw-only preparation now uses:

- `PAYLOAD_PREPARE_RAW_SENSOR`
- `PAYLOAD_CAPTURE_RAW`
- raw sensor register commands

Removed current public commands:

- `PAYLOAD_SET_DEFAULTS`
- `PAYLOAD_PREPARE_SESSION`

### Shared ready truth is not capture-policy truth

Replace the mixed current payload session model with two explicit enums:

- `PayloadReadyKind`
  - `READY_NON_RAW`
  - `READY_RAW_SENSOR`
- `PayloadCapturePolicy`
  - `CAPTURE_AUTO`
  - `CAPTURE_DETERMINISTIC`
  - `CAPTURE_RAW_SENSOR`

Usage boundary:

- controller status, active-ready telemetry, and internal prepare gate use
  `PayloadReadyKind`
- capture metadata, artifact headers/manifests, CSP metadata replies, and
  `PAYLOAD_CAPTURE_METADATA` use `PayloadCapturePolicy`

Old `sessionKind` / `preparedSessionKind` wording is removed from current
operator-facing truth.

### Shared non-RAW session defaults are first-class

`PAYLOAD_SET_CAMERA_DEFAULTS` becomes the sole shared non-RAW session-level
surface. It owns:

- `resolutionPreset`
- preview-only `jpegQuality`
- preview-only `hflip`
- preview-only `vflip`

Those fields are no longer duplicated across AUTO and DETERMINISTIC defaults.

### Policy defaults are narrowed

`PAYLOAD_SET_AUTO_DEFAULTS` keeps only AUTO policy fields:

- `awbMode`
- `meteringMode`
- `evCompX100`

`PAYLOAD_SET_DETERMINISTIC_DEFAULTS` keeps only manual deterministic fields:

- `exposureUsec`
- `gainX100`

Current maintained DETERMINISTIC truth does not continue to expose
`awbMode/meteringMode/evComp` as public manual-capture controls in this slice.

`brightness/contrast/saturation/sharpness` remain out of current target
support and are not added to any new shared surface.

### Prepare semantics become exact

- `PAYLOAD_PREPARE` prepares the shared non-RAW session using
  `PAYLOAD_SET_CAMERA_DEFAULTS`
- `PAYLOAD_PREPARE_RAW_SENSOR` prepares the raw-only session
- `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC` both require an
  active `READY_NON_RAW`
- `PAYLOAD_CAPTURE_RAW` and sensor register control require an active
  `READY_RAW_SENSOR`

Session mismatch detection only checks the session-level signature:

- non-RAW versus raw
- shared non-RAW resolution

AUTO or DETERMINISTIC policy defaults no longer decide prepare truth.

### Manifest compatibility stays load-compatible

Older capture manifests must continue to load through the current
`PAYLOAD_GET_LAST_CAPTURE_METADATA` and `PAYLOAD_PUBLISH_CAPTURE` paths.

Implementation rule:

- new headers/manifests use the new capture-policy naming
- legacy serialized payload manifests still deserialize into the new runtime
  metadata model through an explicit compatibility path

### Proof and parser migration is part of the change

Current maintained wrappers, records, parsers, and operator documentation are
part of the public surface and must migrate in the same change.

That includes:

- Route 1 current target payload wrapper
- payload target sanity proof
- payload persistent-session hosted proof
- Mission Console payload catalog/parsers
- payload FDP extract helper expectations where current maintained naming is
  asserted

Historical records are not rewritten; they gain current-note/superseded
guidance only.

## Risks / Trade-offs

- **[Risk] Public command retirement can break nearby maintained wrappers** →
  current Route 1 target and current maintained payload proofs must be migrated
  in the same change, then rerun fresh.
- **[Risk] Enum/type split can break parsers or metadata replay helpers** →
  update the event parser, payload helper protocol conversion, and manifest
  compatibility tests together in the first implementation slice.
- **[Risk] Narrowing deterministic defaults could unintentionally remove a
  relied-on manual control** → audit current maintained wrappers first and only
  preserve public fields that still have current maintained proof value.

## Migration Plan

1. Create the formal change and delta specs.
2. Implement the FPP/API/type split plus runtime behavior changes.
3. Add or refresh focused UT/parser coverage and manifest compatibility checks.
4. Commit the runtime/API slice.
5. Migrate maintained wrappers/proofs/docs/specs/records to the new surface.
6. Rerun focused maintained payload proofs.
7. Validate OpenSpec and reach local-ready.

## Open Questions

None. Command retirement, type split, and shared-session-default ownership are
fixed by this change.
