## Context

The active target payload backend already exposes the correct public payload
contract, but its current `libcamera` implementation starts the camera,
submits one request, and saves the first completed frame. Fresh evidence shows
that this sequence can yield a black raw frame and black preview JPEG even when
the camera hardware itself is healthy and a direct `rpicam-still --shutter
30000 --gain 4 --width 640 --height 480 -n` capture on `obc.local` produces a
lit image.

That means this change is not a Route 1 downlink problem and not a generic
camera-hardware problem. It is a target backend sequencing defect plus a proof
gap: the repository currently proves transport/hash/downlink closure without a
bounded target source-image validity oracle.

## Goals / Non-Goals

**Goals**

- Fix target `AUTO` and `DETERMINISTIC` capture so the saved onboard image is
  not the first cold-start frame.
- Keep the existing payload command/FPP surface unchanged.
- Add a bounded, repo-owned target proof that checks source artifact validity
  without requiring full Route 1 downlink reruns.
- Preserve a strict evidence split between target onboard source artifacts and
  ground/downlink artifacts.

**Non-Goals**

- No full generic camera-control parity for `metering`, `evComp`,
  `brightness`, `contrast`, `saturation`, `sharpness`, `hflip`, or `vflip`.
- No preview/still dual-role reconfigure surface or new operator-facing knob.
- No reclassification of historical Route 1 PASS evidence as retroactive FAIL.
- No full Route 1 formal rerun in this change.

## Decisions

### Use a bounded warm-up loop inside `captureStill()`

The target backend will keep `prepare()` as pure camera/config/allocation
setup. `captureStill()` will start the camera, keep the chosen controls
active, drain a bounded number of completed frames, and only persist the first
post-warm-up completed frame.

Chosen policy:

- `minCompletedFrames = 6`
- `warmupTimeoutMs = 3000`

Alternative considered: add a separate preview/still reconfigure path like the
full Raspberry Pi `rpicam-jpeg` application flow. Rejected for this slice
because it would broaden scope into a larger target backend redesign.

### Keep session behavior explicit but unchanged at the public boundary

`AUTO` still means `AeEnable=true` and `AwbEnable=true`.
`DETERMINISTIC` still means `AeEnable=false`, `AwbEnable=false`, and explicit
`ExposureTime`/`AnalogueGain` derived from the current payload settings.

The only semantic change is which completed frame is persisted, not which
public command or session kind operators must use.

### Add a source-image luma oracle instead of trusting artifact existence

The new target proof must reject near-black raw frames even if:

- local `.bin` and `.jpg` exist
- metadata is present
- Route 1-style transport/downlink/hash checks would otherwise pass

The oracle will compute raw-frame luma stats for current supported formats
(`YUYV`, `UYVY`, `NV12`, `RGB888`, `BGR888`) and require both:

- `meanLuma >= 1.0`
- `p95Luma >= 8`

Alternative considered: JPEG-only brightness checking. Rejected because the
raw frame is the canonical source artifact and the JPEG path is derivative.

### Add a new current target proof instead of reopening Route 1

This change adds a small A/B/C target proof for two VGA cases:

- `AUTO`
- `DETERMINISTIC`

It proves:

- current secure-auth command path
- `PAYLOAD` mode entry
- onboard source artifacts exist
- metadata identifies `libcamera` and `OV5647`
- image-content sanity passes

It does **not** claim ground/downlink closure. Route 1 current formal rerun
remains a separate transport/downlink record.

## Risks / Trade-offs

- [Risk] Warm-up count may still be too short on some boots.  
  Mitigation: bounded timeout plus a proof oracle; if it still fails, the
  change fails closed instead of silently producing another black artifact.

- [Risk] Longer capture latency could affect operator expectations.  
  Mitigation: keep the bounded window small (`<= 3s`) and document that this
  slice changes only target backend sequencing, not the public payload command
  surface.

- [Risk] Current target backend still ignores several generic settings-model
  fields.  
  Mitigation: document that parity work remains separate; this slice closes
  black-image usability first.

## Migration Plan

1. Add the OpenSpec deltas and tasks for target payload black-image closure.
2. Implement bounded warm-up capture behavior in the target `libcamera`
   backend.
3. Add focused luma-oracle tests and any small helper tests needed to keep the
   warm-up logic reviewable.
4. Add and run the new A/B/C target payload capture sanity proof.
5. Update payload and Chapter 5 records with current-note guidance that
   separates transport/downlink closure from source-image validity.

Rollback is straightforward: revert the warm-up logic and the new sanity proof,
which returns the repo to the previous transport-only Route 1 truth.

## Open Questions

None for this slice. The open parity question for the remaining generic camera
settings stays explicitly deferred to later target backend follow-up work.
