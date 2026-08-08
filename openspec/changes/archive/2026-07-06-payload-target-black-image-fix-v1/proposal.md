## Why

Current target payload captures can complete, publish artifacts, and even pass
Route 1 downlink/hash closure while the source-side preview JPEG and raw frame
are still effectively black. Direct `rpicam-still` on `obc.local` proves the
camera hardware and CSI path are working, so the remaining defect is in the
active target `libcamera` capture sequencing and in the lack of a bounded
source-image validity oracle.

## What Changes

- Add bounded warm-up capture behavior to the target `libcamera` payload
  backend so target `AUTO` and `DETERMINISTIC` captures no longer persist the
  first cold-start frame as the formal result.
- Add a repo-owned target payload capture sanity proof that validates onboard
  source artifacts for `AUTO` and `DETERMINISTIC` VGA captures on the governed
  A/B/C target path.
- Add a source-image content sanity oracle based on raw-frame luma statistics,
  and update evidence/docs so current Route 1 target closure remains a
  transport/downlink claim rather than an implicit source-image-quality claim.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `payload-operations`: The current target `libcamera` backend now requires
  bounded warm-up before final still capture persistence, and target backend
  closure now includes source-image validity for `AUTO` and `DETERMINISTIC`
  captures.
- `verification-evidence`: Current payload target evidence now needs a
  dedicated real-camera sanity record that distinguishes onboard source
  artifacts from ground/downlink artifacts and records image-content validity.
- `verification-path-registry`: The payload validation-path registry now needs
  a distinct target real-camera capture sanity path that stays separate from
  Route 1 official `.fdp` downlink closure.

## Impact

- Affected code: `OBC/Components/PayloadOpsController/LibcameraPiCameraDriver.cpp`,
  payload-focused tests, and a new target payload sanity proof script/helper.
- Affected docs/evidence: new target capture sanity test record plus current
  notes in payload and Chapter 5 records.
- Public command/FPP surface stays unchanged: no new payload operator command,
  no new mode semantics, and no Route 1 rerun requirement in this change.
