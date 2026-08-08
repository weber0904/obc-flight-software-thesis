## Why

The payload surface already converged to one shared non-RAW `READY` state at
the controller layer, but the active target `libcamera` backend still behaves
like a single-shot workaround: each capture starts the stream, drains a bounded
warm-up set, persists one frame, and stops again. That makes the operator flow
look simpler than the real runtime behavior, keeps repeated captures paying the
same cold-start cost, and leaves current docs/specs overstating the meaning of
`camera-ready`.

At the same time, `PAYLOAD_CAPTURE_STILL` no longer owns a distinct maintained
behavior. It is only a deterministic compatibility wrapper, but its continued
presence on the current surface keeps payload capture looking like three
parallel command families (`STILL`, `AUTO`, `DETERMINISTIC`) instead of one
shared session with two maintained policies.

## What Changes

- Move the target payload backend from per-capture cold-start still sequencing
  to a persistent warm non-RAW camera session owned by `PAYLOAD_PREPARE` and
  released by `PAYLOAD_SHUTDOWN` / abort / forced mode exit.
- Freeze the current non-RAW session-level signature to `resolutionPreset` and
  non-RAW versus `RAW_SENSOR`, and reject session-level mismatches instead of
  silently re-preparing during capture.
- Keep `AUTO` and `DETERMINISTIC` as the only maintained normal still-capture
  policies on the shared non-RAW `READY` surface.
- Retire `PAYLOAD_CAPTURE_STILL` from the current public command surface and
  update maintained scripts, proofs, records, and docs to stop citing it as a
  current route.
- Add fresh hosted semantic proof plus governed target real-camera persistent
  session proof, and update the current evidence/registry/docs to reflect the
  new runtime truth.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `payload-operations`: payload prepare/capture lifecycle changes from
  per-capture stream start/stop to a persistent warm session, and
  `PAYLOAD_CAPTURE_STILL` retires from the current normal capture surface.
- `verification-evidence`: payload evidence must distinguish the new
  persistent-session proof from older single-capture warm-up evidence and
  record the `STILL` retirement boundary.
- `verification-path-registry`: maintained payload entries must register
  persistent-session target truth and stop treating compatibility `STILL` as a
  current maintained capture family.

## Impact

- Affected code: `PayloadOpsController`, `PiCameraManager`, target
  `LibcameraPiCameraDriver`, hosted stub/helper-backed payload backend surfaces,
  payload tests, and payload proof wrappers.
- Affected operator/current-doc truth: interface index, current architecture,
  verification-path registry, roadmap, and payload test records.
- Public payload semantics change: current maintained normal capture families
  become `AUTO` and `DETERMINISTIC` only; `PAYLOAD_CAPTURE_STILL` is retired
  instead of remaining a current compatibility wrapper.
