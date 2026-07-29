# Design: payload-persistent-session-still-retirement-v1

## Context

The controller layer already exposes one normal non-RAW `READY` state and
admits `AUTO -> metadata -> DETERMINISTIC` from the same prepared window, but
the target backend still implements capture as a single-shot still sequence:
configure during prepare, then on each capture start the camera stream, drain a
warm-up set, persist one frame, and stop again. That behavior was acceptable as
the narrow black-image fix, but it is not the operator model the payload docs
now describe.

Current payload semantics are also split by a compatibility surface that no
longer carries distinct value: `PAYLOAD_CAPTURE_STILL` is just a deterministic
wrapper with legacy naming. Keeping it on the maintained surface makes the
payload family look like three equal routes instead of one persistent session
with two maintained capture policies.

## Goals / Non-Goals

**Goals:**

- Make `PAYLOAD_PREPARE` establish a persistent warm non-RAW camera session.
- Allow repeated `AUTO` and `DETERMINISTIC` captures within that session
  without redoing full cold-start warm-up.
- Keep `RAW_SENSOR` as a distinct special session and reject session-level
  mismatch instead of silently re-preparing.
- Retire `PAYLOAD_CAPTURE_STILL` from the current public surface and from
  maintained proof/documentation paths.
- Close the change with fresh hosted semantic evidence, fresh governed target
  persistent-session evidence, and synced current docs/specs.

**Non-Goals:**

- No new payload command, mode, or FPP field.
- No generic camera-control parity expansion for
  `brightness/contrast/saturation/sharpness`.
- No Route 1 full formal rerun or payload downlink redesign.
- No change to `PREPARE_SESSION(AUTO|DETERMINISTIC)` retirement in this slice;
  they remain deprecated prepare aliases.

## Decisions

### Persistent session ownership lives below the public command surface

`PayloadOpsController` keeps the same operator model:

- `PAYLOAD_PREPARE`
- `PAYLOAD_CAPTURE_AUTO`
- `PAYLOAD_CAPTURE_DETERMINISTIC`
- `PAYLOAD_GET_LAST_CAPTURE_METADATA`
- `PAYLOAD_SHUTDOWN`

The lifecycle change happens inside `PiCameraManager` and the driver contract.
The public surface continues to expose one non-RAW `READY` state, but that
state now means an active warmed session exists instead of only "prepared
configuration is available".

### Session-level versus request-level settings are frozen explicitly

The current maintained non-RAW session-level signature is:

- non-RAW versus `RAW_SENSOR`
- `resolutionPreset`

The maintained request-level controls are:

- capture policy (`AUTO` or `DETERMINISTIC`)
- `exposureUsec`
- `gainX100`
- `awbMode`
- `meteringMode`
- `evCompX100`
- preview-only `jpegQuality`
- preview-only `hflip` / `vflip`

`brightness/contrast/saturation/sharpness` remain out of the current target
claim.

### Session mismatch rejects rather than silently re-preparing

If a capture request would require a different active session signature, the
command fails with a bounded not-ready / reprepare-required path. The operator
must shut down and prepare again.

This avoids hidden target-side reconfiguration during capture and keeps the
meaning of persistent session explicit in evidence and timing.

### Warm-up becomes prepare-owned, not capture-owned

For the target `libcamera` backend:

- `prepare()` performs configure, allocation, stream start, and warm-up.
- `captureStill()` reuses the live session and captures a fresh frame under the
  requested request-level controls.
- `shutdown()` stops the session and releases resources.

Bounded convergence after a control change is still allowed during capture, but
it uses the existing `captureTimeoutMs` budget instead of a separate fixed
3-second clamp.

### `PAYLOAD_CAPTURE_STILL` is retired, not kept as current compatibility

This change removes `PAYLOAD_CAPTURE_STILL` from the current public command
surface instead of merely documenting it as deprecated.

Rationale:

- current maintained payload semantics already have `AUTO` and
  `DETERMINISTIC` as the meaningful normal still-capture policies
- keeping `STILL` active continues to confuse docs, operator flows, and thesis
  writing
- doing the runtime lifecycle convergence first gives a clean surface to retire
  `STILL` without leaving behavior split across old and new models

`PREPARE_SESSION(AUTO|DETERMINISTIC)` stays public for now because this slice
is about still-capture retirement, not wholesale compatibility deletion.

### Hosted and helper-backed semantics stay aligned

The stub driver and helper-backed protocol continue to preserve the public
payload contract and metadata model. They do not claim target sensor warm-up
behavior, but they must follow the same persistent-session semantics so hosted
tests remain a valid semantic oracle for the controller/manager contract.

## Risks / Trade-offs

- **[Risk] Persistent live session could expose stale stream state or cleanup defects** → Add focused manager tests for shutdown/abort/mode-exit and require target proof to verify bounded cleanup.
- **[Risk] Control changes may need more than one completed frame before the persisted image reflects the requested values** → Keep bounded capture-time convergence inside the configured timeout budget and verify actual metadata plus non-black source image on target.
- **[Risk] Removing `PAYLOAD_CAPTURE_STILL` can break latent maintained scripts or proofs** → Audit current scripts/records first, migrate any maintained path to deterministic capture, and keep historical records as historical only.
- **[Risk] Session-mismatch reject may reveal hidden operator assumptions** → Document the required shutdown + reprepare workflow and prove it in the new target record instead of silently reconfiguring.

## Migration Plan

1. Create the change artifacts and delta specs.
2. Implement persistent-session runtime and focused tests.
3. Refresh hosted semantic proof and target persistent-session proof.
4. Commit Phase 1 once fresh evidence is green.
5. Remove `PAYLOAD_CAPTURE_STILL`, migrate maintained wrappers/docs/records,
   and refresh evidence/registry wording.
6. Commit Phase 2.
7. Run final local verification, validate specs, archive the change, sync the
   canonical docs/records, and reach `local-ready`.

## Open Questions

None. This change fixes the retirement decision, session signature, and
operator-visible mismatch policy up front so implementation does not need to
re-decide them.
