## Why

The current payload public surface still mixes non-RAW prepare truth with
capture-policy truth. That leaves stale compatibility commands
(`PAYLOAD_SET_DEFAULTS`, `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)`) on the
maintained surface, creates real runtime regression risk in current Route 1
payload flows, and keeps operator-facing names such as `PSESSION_AUTO`
misaligned with the single-ready model already documented for current payload
operations.

## What Changes

- **BREAKING** retire `PAYLOAD_SET_DEFAULTS` from the current public payload
  surface.
- **BREAKING** retire `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` from the
  current public payload surface and replace the raw prepare path with an
  explicit `PAYLOAD_PREPARE_RAW_SENSOR`.
- Add a shared non-RAW session-defaults command,
  `PAYLOAD_SET_CAMERA_DEFAULTS(resolutionPreset, jpegQuality, hflip, vflip)`,
  so session-level truth is no longer hidden inside AUTO or DETERMINISTIC
  policy defaults.
- Split the current mixed `PayloadSessionKind` truth into:
  - ready-state truth for controller/runtime status
  - capture-policy truth for metadata, manifests, CSP replies, and events
- Narrow current policy-defaults truth so:
  - AUTO owns only AUTO policy fields
  - DETERMINISTIC owns only deterministic manual fields
- Migrate maintained payload wrappers, proofs, parsers, and current docs/specs
  to the converged surface and classify old `SET_DEFAULTS`,
  `PREPARE_SESSION(AUTO|DETERMINISTIC)`, and `PSESSION_*` records as
  historical/superseded.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `payload-operations`: retire stale compatibility commands, add shared
  session-defaults plus raw-only prepare truth, and split ready-state truth
  from capture-policy truth.
- `verification-path-registry`: update maintained payload proof selection so
  current records and wrappers no longer cite retired compatibility commands or
  `PSESSION_*` public truth.
- `verification-evidence`: require current payload records to distinguish
  shared non-RAW ready truth, capture-policy truth, and historical superseded
  compatibility surfaces.

## Impact

- Affected code:
  - `OBC/Components/PayloadOpsController/`
  - payload CSP/metadata reply surfaces
  - Mission Console payload command catalog/parsers
  - maintained payload target/hosted proof scripts
- Affected public APIs:
  - payload FPP command surface
  - payload metadata/event/telemetry naming
  - payload manifest/artifact-header readback semantics
- Affected records/docs:
  - current payload interface docs
  - verification-path registry
  - current payload proof/test records
