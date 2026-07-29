## Context

The repository already has one governed payload path:

- ground issues payload commands through the normal command path
- `PayloadOpsController` owns the only public payload contract
- `DpCatalog -> CommController -> FileDownlink` owns official stored/downlink
  delivery

What is missing is a payload model that matches camera-payload operations.
Ground wants a smaller preview product first and a larger raw product later,
without reopening arbitrary-file governance or inventing a second delivery
plane. The implementation must also correct the current target truth that local
`.jpg` naming is not enough to guarantee valid JPEG bytes.

## Goals / Non-Goals

**Goals**
- Capture one authoritative raw frame and derive one preview JPEG from that
  same frame.
- Store both artifacts locally using deterministic `PIC%02X.bin/.jpg` names.
- Keep payload `.fdp` as the only official delivery artifact family while
  distinguishing preview versus raw at the record/header layer.
- Auto-publish preview JPEG immediately after capture.
- Let ground later request official raw publication through one explicit
  payload command without adding a browse/list/download family.
- Keep official raw bounded to `VGA` and `HD` under the existing `2 MiB`
  payload family ceiling.
- Increase the stock file packet budget now so payload proof does not keep
  running against an obviously undersized `256`-byte file buffer.

**Non-Goals**
- No generic payload manager, multi-payload abstraction, or mission scheduler.
- No second delivery owner outside `DpCatalog`.
- No token-bucket shaping or new file QoS profile in this change.
- No official `FULL` raw downlink claim.
- No use of `rpicam-jpeg` as the primary flight implementation.

## Decisions

### Keep one local raw artifact plus one local preview JPEG
The flight path will treat the raw frame as authoritative capture output and
derive the preview JPEG from the same frame. This ensures the raw and preview
artifacts represent the same exposure and avoids taking two separate captures.

Alternatives considered:
- Preview-only official model: rejected because it cannot support delayed raw
  delivery.
- Raw-only local storage plus on-demand preview generation: rejected because it
  makes immediate operator review and proof slower and more fragile.

### Keep official `.fdp` ownership but split artifacts by kind, not by local file extension
The official delivery surface remains payload `.fdp`, but the payload header
will identify `PREVIEW_JPEG` versus `RAW_FRAME`. Raw and preview will not be
packed into the same official `.fdp`; otherwise downloading a preview would
still force raw bytes across the link.

Alternatives considered:
- Put both raw and preview in one `.fdp`: rejected because it defeats later
  operator choice.
- Publish local files directly through a new payload file command: rejected
  because it reopens arbitrary-file governance.

### Add one explicit raw/preview publication command
`PAYLOAD_PUBLISH_CAPTURE(captureIndex, artifactKind)` is the minimum command
surface that lets ground choose which artifact becomes official later without
adding a general browse/list/download family. Capture itself still auto-publishes
the preview JPEG because that is the primary first-look artifact.

### Retire sidecar `.json` from the formal contract
The current `.json` sidecar duplicates information that belongs either in
payload readback or the official payload `.fdp` header. This change removes the
sidecar from the formal contract and keeps the metadata model in one governed
place.

### Keep current `2 MiB` payload family ceiling
`FULL` raw frames exceed the current payload family model because the
controller publishes from in-memory bytes and the TopCcsds payload DP bin is
currently sized around that ceiling. Raising `FW_FILE_BUFFER_MAX_SIZE` does not
solve that problem. Therefore:

- preview JPEG may still claim `FULL` if it fits
- raw official publication is bounded to `VGA` and `HD`
- `FULL` raw remains explicit non-claim

### Raise stock file/downlink packet sizing now
The current `FW_FILE_BUFFER_MAX_SIZE = 256` is materially too small for the
current payload and HK `.fdp` story. This change raises:

- `FW_FILE_BUFFER_MAX_SIZE = 2048`
- `ComCfg::TmFrameFixedSize = 4096`
- S-band `commsFileBuffSize = 4096`
- S-band `commsFileBuffCount = 32`

No token bucket is added; the goal is to stop using the current obviously tiny
packet size first.

## Data Model

### Local storage
- `persistent-data/payload/camera/PIC%02X.bin`
- `persistent-data/payload/camera/PIC%02X.jpg`

Reusing the same `captureIndex` overwrites the existing pair.

### Runtime metadata/readback
`PayloadCaptureMetadata` becomes artifact-aware and records:
- `captureIndex`
- `captureTimeSec` / `captureTimeUsec`
- `imageWidth` / `imageHeight`
- `pixelFormat`
- `rawRelativePath`
- `previewRelativePath`
- `rawBytes`
- `previewJpegBytes`
- `previewDataProductRelativePath`
- `rawDataProductRelativePath`
- `previewDataProductPublished`
- `rawDataProductPublished`
- the minimal applied capture settings that matter for decode/review

### Official payload data-product contract
The old JPEG-only record pair is replaced by:
- one artifact-oriented header record
- one variable-size payload-bytes record

The header records the artifact kind, minimal capture parameters, and the
artifact-specific publication identity. The bytes record contains either the
preview JPEG bytes or the raw frame bytes for that product.

The extractor remains backward-compatible with the archived JPEG-only payload
`.fdp` family so earlier evidence stays decodable.

## Driver Strategy

### Target driver
The target `libcamera` backend must stop treating `.jpg` naming as proof of
JPEG correctness. The driver will:
- negotiate one bounded raw-capable stream format
- reject unsupported formats for this change
- write the raw capture to `.bin`
- software-encode the preview JPEG from the same frame

Supported input formats for this change:
- `YUYV`
- `UYVY`
- `NV12`
- `RGB888`
- `BGR888`

If the negotiated format is outside that set, capture fails cleanly.

### Hosted stub driver
The stub driver will also write a deterministic raw `.bin` plus a deterministic
preview JPEG so hosted proof covers the same artifact model.

## Operator / Proof Flow

### Capture flow
1. Ground issues `PAYLOAD_CAPTURE_*` with `captureIndex`
2. controller captures one raw frame
3. driver writes `PIC%02X.bin`
4. driver derives and writes `PIC%02X.jpg`
5. controller auto-publishes preview JPEG as official payload `.fdp`
6. final success/failure is surfaced on payload state/event/tlm/readback

### Raw promotion flow
1. Ground issues `PAYLOAD_PUBLISH_CAPTURE(captureIndex, RAW_FRAME)`
2. controller loads local `PIC%02X.bin`
3. controller publishes raw official `.fdp` if the artifact fits the bounded
   payload family budget
4. ground uses existing `BUILD_CATALOG` / `START_XMIT_CATALOG`

### Proof boundary
- hosted proof must validate preview and raw official `.fdp` decode/parity for
  `VGA` and `HD`
- target governed node-`5` proof must at minimum validate preview official
  downlink plus `VGA` raw official downlink
- `FULL` raw remains explicit non-claim

## Risks / Trade-offs

- [Software JPEG encoding adds a new dependency surface]  
  Mitigation: keep the input-format set small and add focused unit/probe
  coverage plus `rpicam-jpeg` oracle comparison during development only.

- [The helper protocol and CSP metadata reply both widen]  
  Mitigation: keep the new metadata fields minimal and aligned with real active
  backend behavior instead of exposing dormant settings.

- [Raising file buffer size affects the general file/downlink path]  
  Mitigation: pair the change with the required TM frame-size uplift and rerun
  focused stock `DpCatalog -> CommController -> FileDownlink` proof instead of
  only payload-specific tests.

## Migration Plan

1. Add OpenSpec artifacts and delta specs.
2. Extend FPP/runtime data model for `captureIndex`, artifact kind, and
   artifact-aware payload metadata.
3. Implement local raw + preview-JPEG capture in the drivers and controller.
4. Replace the official payload `.fdp` record family with the artifact-aware
   header/bytes model while keeping legacy extractor support.
5. Add the explicit raw-publish command and extend payload readback/CSP
   surfaces.
6. Raise file/downlink buffer sizing and rerun focused file-path checks.
7. Update hosted/target payload proofs and active docs, then validate the
   OpenSpec change.

## Open Questions

- None for this change. The bounded raw/non-claim boundary, naming rule,
  `captureIndex` width, and transport uplift were fixed by the approved plan.
