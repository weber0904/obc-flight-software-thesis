# payload-target-capture-sanity-v1 Evidence

Date:
- `2026-07-07`

Originating OpenSpec changes:
- [2026-07-06-payload-single-ready-auto-actual-metadata-v1](../../../openspec/changes/archive/2026-07-06-payload-single-ready-auto-actual-metadata-v1)
- [2026-07-06-payload-target-black-image-fix-v1](../../../openspec/changes/archive/2026-07-06-payload-target-black-image-fix-v1)

## Scope

This record closes the current target real-camera source-image validity slice
for the Raspberry Pi OV5647 `libcamera` backend.

Current-note:

- this record remains the current authority for target source-image validity,
  actual `AUTO` exposure/gain/AWB readback, and bounded target preview
  `hflip/vflip` corroboration
- the newer persistent-session lifecycle and `PAYLOAD_CAPTURE_STILL`
  retirement boundary are now governed separately by
  [payload-persistent-session-still-retirement-v1](../payload-persistent-session-still-retirement-v1/README.md)
- do not cite this record by itself as the sole authority for current public
  `STILL` retirement or one-prepare multi-capture lifecycle semantics

It proves a narrower claim than Route 1:

- current governed target `A -> B -> C` secure-auth payload path can enter
  `PAYLOAD` mode and execute real-camera capture on `obc.local`
- source-side onboard artifacts for VGA `AUTO` and VGA `DETERMINISTIC`
  captures are generated successfully
- the saved source image is no longer the cold-start black frame previously
  seen on the target path
- source-image validity is now backed by a repo-owned raw-frame luma oracle

It does **not** prove:

- Route 1 official `.fdp` downlink closure
- ground receipt of the payload preview product
- broad target payload throughput or admitted size-envelope closure
- full generic camera-control parity for `metering`, `evComp`,
  `brightness`, `contrast`, `saturation`, or `sharpness`
- any claim that raw `.bin` orientation is transformed; the current proof only
  closes bounded preview-JPEG `hflip/vflip`

## Canonical Artifact Root

- canonical artifact root:
  [2026-07-07-target-formal-rerun](ARTIFACTS.json)
- previous retained rerun root:
  [2026-07-06-target-formal-rerun](ARTIFACTS.json)
- imported probe root:
  [probe-root](ARTIFACTS.json)
- imported manifest:
  [manifest.json](ARTIFACTS.json)

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
./fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target payload_camera_backend_helper OBC
python3 scripts/test_payload_image_sanity.py
python3 -m py_compile scripts/comm_verification/lib/payload_image_sanity.py scripts/comm_verification/lib/run_payload_target_capture_sanity_v1_target_probe.py
bash -n scripts/run_payload_target_capture_sanity_v1_target_probe.sh
bash scripts/run_payload_target_capture_sanity_v1_target_probe.sh
```

## Results

- Fresh native payload/OBC build: `PASS`
- Focused payload image-sanity tests: `PASS`
- Python compile sanity: `PASS`
- Wrapper shell syntax: `PASS`
- Governed target real-camera payload capture sanity proof: `PASS`

## Reference Passing Run

Repository-owned entrypoint:

```bash
bash scripts/run_payload_target_capture_sanity_v1_target_probe.sh
```

Reference imported run:

| Field | Value |
|---|---|
| wrapper verdict | `payload-target-capture-sanity-v1: PASS` |
| original probe root | `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-target-capture-sanity-v1.HwwSDM` |
| imported probe root | `evidence/records/payload-target-capture-sanity-v1/artifacts/2026-07-07-target-formal-rerun/probe-root` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| summary JSON | [payload-target-capture-sanity-summary.json](ARTIFACTS.json) |
| status log | [status.log](ARTIFACTS.json) |
| wrapper summary | [summary.log](ARTIFACTS.json) |

Observed PASS summary:

```text
payload-target-capture-sanity-v1: PASS
capabilities-line=... backend libcamera supported 63 offOnly 15 autoMutable 0 deterministicMutable 48 raw 0 real 1
same-session-capture-count=3
set-camera-defaults-busy-reject=PRESULT_REJECTED_BUSY detail=0
case=auto policy=CAPTURE_AUTO capture-id=1 capture-index=0x30 pixel-format=PIXEL_YUYV resolution=640x480 mean-luma=175.137 p95-luma=217 preview-bytes=44500 raw-bytes=614400
case=deterministic policy=CAPTURE_DETERMINISTIC capture-id=2 capture-index=0x31 pixel-format=PIXEL_YUYV resolution=640x480 mean-luma=173.974 p95-luma=216 preview-bytes=44987 raw-bytes=614400
case=deterministic-followup policy=CAPTURE_DETERMINISTIC capture-id=3 capture-index=0x32 pixel-format=PIXEL_YUYV resolution=640x480 mean-luma=174.125 p95-luma=216 preview-bytes=44851 raw-bytes=614400
case=deterministic-flipped policy=CAPTURE_DETERMINISTIC capture-id=4 capture-index=0x33 pixel-format=PIXEL_YUYV resolution=640x480 mean-luma=173.862 p95-luma=216 preview-bytes=17293 raw-bytes=614400
case=deterministic-flipped preview-flip baseline-diff=0.9830 corrected-diff=0.0397 ratio=24.743
```

## What To Read

Read in this order:

1. [manifest.json](ARTIFACTS.json)
   - command, timestamp, imported root, and formal verdict
2. [summary.log](ARTIFACTS.json)
   - high-level AUTO/DETERMINISTIC result lines
3. [payload-target-capture-sanity-summary.json](ARTIFACTS.json)
   - full per-case metadata, luma stats, artifact paths, and reference capture
4. per-case source artifacts / decodes
   - AUTO preview:
     [PIC30.jpg](ARTIFACTS.json)
   - AUTO raw:
     [PIC30.bin](ARTIFACTS.json)
   - AUTO decoded preview:
     [Dp_268673025_1783382245_00075615.jpg](ARTIFACTS.json)
   - DETERMINISTIC preview:
     [PIC31.jpg](ARTIFACTS.json)
   - DETERMINISTIC raw:
     [PIC31.bin](ARTIFACTS.json)
   - DETERMINISTIC decoded preview:
     [Dp_268673025_1783382294_00073015.jpg](ARTIFACTS.json)
   - DETERMINISTIC follow-up decoded preview:
     [Dp_268673025_1783382349_00073368.jpg](ARTIFACTS.json)
   - DETERMINISTIC flipped decoded preview:
     [Dp_268673025_1783382502_00072983.jpg](ARTIFACTS.json)
5. target-side command/retry breadcrumbs
   - [checkpoints.jsonl](ARTIFACTS.json)

## Proven Source-Image Facts

- secure-auth established on the governed target node-`5` path before either
  capture case
- target capabilities reported:
  - backend `libcamera`
  - camera model `OV5647`
  - supported mask `63`
  - off-only mask `15`
  - auto mutable mask `0`
  - deterministic mutable mask `48`
- AUTO case:
  - capture policy `CAPTURE_AUTO`
  - resolution `PRESET_VGA_640X480`
  - preview JPEG quality `90`
  - pixel format `PIXEL_YUYV`
  - raw bytes `614400`
  - preview bytes `44500`
  - actual exposure `48152 usec`
  - actual gain `400`
  - actual AWB valid `true`
  - actual AWB color temperature `2916 K`
  - actual AWB color gains `red=1093`, `blue=1951`
  - `meanLuma=175.137`, `p95Luma=217`
- DETERMINISTIC case:
  - capture policy `CAPTURE_DETERMINISTIC`
  - resolution `PRESET_VGA_640X480`
  - requested mask `48` (`EXPOSURE_USEC | GAIN_X100`)
  - preview JPEG quality `90`
  - requested exposure `48152 usec`
  - actual exposure `48152 usec`
  - requested gain `400`
  - actual gain `400`
  - pixel format `PIXEL_YUYV`
  - raw bytes `614400`
  - preview bytes `44987`
  - actual AWB valid `false`
  - `meanLuma=173.974`, `p95Luma=216`
- DETERMINISTIC follow-up case:
  - same shared non-RAW prepared session as the prior deterministic capture
  - requested exposure `48152 usec`
  - actual exposure `48152 usec`
  - requested gain `400`
  - actual gain `400`
  - preview bytes `44851`
  - `meanLuma=174.125`, `p95Luma=216`
- DETERMINISTIC flipped case:
  - performed after `PAYLOAD_SHUTDOWN`, then fresh
    `PAYLOAD_SET_CAMERA_DEFAULTS(...)` and `PAYLOAD_PREPARE`
  - preview JPEG quality `55`
  - preview `hflip=true`, `vflip=false`
  - preview bytes `17293`
  - `meanLuma=173.862`, `p95Luma=216`
  - preview flip oracle `PASS`
    - baseline diff `0.9830`
    - corrected diff `0.0397`
    - improvement ratio `24.743`
- same-session camera-default mismatch guard:
  - `PAYLOAD_SET_CAMERA_DEFAULTS(PRESET_HD_1280X720, ...)` while prepared
    rejects with `PRESULT_REJECTED_BUSY detail=0`

Both cases pass the current source-image oracle:

- `meanLuma >= 1.0`
- `p95Luma >= 8`

## Operator Workflow Proven Here

The current target operator workflow closed by this record is:

1. `MODE_SET PAYLOAD`
2. `PAYLOAD_SET_CAMERA_DEFAULTS(...)`
3. `PAYLOAD_SET_AUTO_DEFAULTS(...)`
4. `PAYLOAD_SET_DETERMINISTIC_DEFAULTS(...)`
5. `PAYLOAD_PREPARE`
6. `PAYLOAD_CAPTURE_AUTO(...)`
7. `PAYLOAD_GET_LAST_CAPTURE_METADATA()`
8. inspect actual `ExposureTime` / `AnalogueGain` / AWB result
9. `PAYLOAD_CAPTURE_DETERMINISTIC(...)`
   - same shared non-RAW `READY` window
   - no extra prepare between AUTO and DETERMINISTIC
10. optional same-session deterministic follow-up and preview-orientation check
    without tearing the non-RAW session down first

This is the current proof that `AUTO` and `DETERMINISTIC` are capture-policy
families on one shared `camera-ready` surface, not separate prepare-exclusive
states.

## Provenance / Source Distinction

Use these rules when citing this record:

- `probe-root/source-artifacts/...`
  - onboard artifacts created on target, then copied back by SSH
  - these prove target-side generation, not ground receipt
- `probe-root/source-decode/...`
  - macOS-side decode/extract of the target source artifacts or target-side
    source `.fdp`
  - these still belong to the source side, not to the ground-received path
- `probe-root/sband-ground/...`
  - macOS ground stack logs and captures from this proof
  - this proof reuses secure-auth and command flow, but does not make a
    payload downlink claim
- [reference-capture/rpicam-reference-vga.jpg](ARTIFACTS.json)
  - direct `rpicam-still --shutter 30000 --gain 4 --width 640 --height 480 -n`
    reference on `obc.local`, fetched back to macOS
  - used only as hardware-path corroboration, not as the flight-software
    artifact under test

## Relationship To Route 1

- Route 1 target formal evidence from `2026-06-28` still proves:
  - payload preview `.fdp` transport
  - target/source-vs-ground byte match
  - extract/hash/downlink closure
- This new record proves the missing adjacent claim:
  - source-side target image content is no longer effectively black for the
    current `AUTO` and `DETERMINISTIC` VGA paths

So after this change:

- Route 1 current target closure stays a transport/downlink authority
- `payload-target-capture-sanity-v1` becomes the current source-image-validity
  authority for target real-camera capture
