# Tasks: payload-single-ready-auto-actual-metadata-v1

## 1. Change skeleton and contract updates

- [x] 1.1 Add delta specs for payload operations, verification evidence, and verification-path registry.
- [x] 1.2 Update the design/proposal/tasks artifacts to reflect the single-ready semantics and actual AUTO metadata scope.

## 2. Payload runtime semantics

- [x] 2.1 Remove `AUTO`/`DETERMINISTIC` prepared-session exclusivity from normal still-capture gating while preserving `RAW_SENSOR` special gating.
- [x] 2.2 Make `PAYLOAD_PREPARE()` the canonical plain camera prepare and turn `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` into deprecated compatibility aliases.
- [x] 2.3 Keep `PAYLOAD_CAPTURE_STILL(...)` as a deterministic compatibility wrapper.

## 3. Target metadata and backend capability updates

- [x] 3.1 Extend payload metadata structures, artifact header/manifest handling, and CSP metadata replies with actual target capture fields.
- [x] 3.2 Populate actual exposure/gain/AWB result fields from target `libcamera` completed-request metadata.
- [x] 3.3 Extend target deterministic support to include `jpegQuality`, `hflip`, and `vflip`, keeping `brightness/contrast/saturation/sharpness` out of scope.

## 4. Verification assets

- [x] 4.1 Update or add focused `PayloadOpsController` UT coverage for single-ready capture, raw-session gate retention, compatibility aliases, and request-vs-actual metadata.
- [x] 4.2 Update parser/helper tests for the expanded `PAYLOAD_CAPTURE_METADATA` surface.
- [x] 4.3 Add or update a governed target focused proof for `AUTO -> actual readback -> DETERMINISTIC` on one camera-ready window.

## 5. Documentation and evidence

- [x] 5.1 Update interface/registry docs so `AUTO/DETERMINISTIC/STILL` are capture-policy families, not parallel prepared states/routes.
- [x] 5.2 Document `jpegQuality` as preview-JPEG-only and note its numeric range/default.
- [x] 5.3 Update the relevant payload evidence record with current actual AUTO readback and operator workflow guidance.
