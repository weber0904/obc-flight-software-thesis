## Why

The current payload baseline closes capture acceptance, helper-backed target
hardening, and official payload `.fdp` delivery for one JPEG-like artifact, but
it still fails an operationally useful camera-payload model in two ways:

- the target backend does not guarantee that the local `.jpg` is a real JPEG
- the payload contract cannot preserve both a raw science frame and a smaller
  preview artifact that ground can choose between later

This change moves payload to a bounded dual-artifact baseline that keeps one
local raw frame plus one local preview JPEG per capture, auto-publishes the
preview into the official payload `.fdp` path, and lets ground explicitly
promote the raw frame later through the existing `DpCatalog -> CommController ->
FileDownlink` path.

## What Changes

- Add a deterministic `captureIndex: U8` to every `PAYLOAD_CAPTURE_*` command
  and store local artifacts as `PIC%02X.bin` and `PIC%02X.jpg` under the
  governed payload runtime root.
- Replace the JPEG-specific official payload `.fdp` family with an
  artifact-oriented payload `.fdp` family that can carry either
  `PREVIEW_JPEG` or `RAW_FRAME`.
- Auto-publish `PREVIEW_JPEG` after each successful capture and add a new
  `PAYLOAD_PUBLISH_CAPTURE(captureIndex, artifactKind)` command so ground can
  promote the raw frame later without introducing a payload-specific browse or
  download family.
- Keep the current `2 MiB` payload family ceiling and explicitly bound official
  raw publication to `VGA` and `HD`; `FULL` raw remains a non-claim in this
  change.
- Raise `FW_FILE_BUFFER_MAX_SIZE` to `2048`, raise the CCSDS TM frame budget to
  `4096`, and set S-band file buffers to `4096 x 32` so the stock file-downlink
  path stops running on the current undersized packet budget.
- Reconcile active docs, verification records, and repo-owned decode tooling so
  official payload proof distinguishes raw versus preview artifacts and records
  the `FULL` raw deferred boundary explicitly.

## Capabilities

### New Capabilities
- none

### Modified Capabilities
- `payload-data-products`: official payload `.fdp` publication becomes
  artifact-oriented and supports both preview JPEG and bounded raw-frame
  products
- `payload-operations`: capture commands add `captureIndex`, local storage
  becomes dual-artifact, and payload readback gains artifact-specific identity
  and publication state
- `comm-subsystem`: the stock payload `.fdp` path keeps `DpCatalog` ownership
  while the file/downlink packet budget is raised to a governed larger size
- `resource-storage`: the local payload capture root becomes `PIC%02X.bin/.jpg`
  only and sidecar `.json` metadata is retired from the formal contract
- `verification-path-registry`: hosted and governed target payload proof paths
  now distinguish preview and raw official artifacts while keeping `FULL` raw
  explicitly out of claim

## Impact

- Affected code: `PayloadOpsController`, `PayloadOpsRuntime`,
  `PayloadBackendHelperProtocol`, `PayloadCspProtocol`, payload extractor/probe
  tooling, `LibcameraPiCameraDriver`, `StubPiCameraDriver`, and TopCcsds file
  buffer configuration
- Affected systems: payload local storage layout, payload `.fdp` header/record
  contract, official payload delivery proof, and stock file/downlink packet
  sizing
- Public behavior: capture commands now require a ground-chosen `captureIndex`,
  preview JPEG is auto-published as the formal first artifact, raw official
  publication becomes explicit, and `FULL` raw official publication is bounded
  out of scope
