# payload-target-backend-hardening-v1 Test Record

## Scope

- Branch: `feature/payload-ops-v2`
- OpenSpec change: `payload-target-backend-hardening-v1`
- Date: 2026-05-21

This record captures the in-branch verification state for payload backend
helper isolation and target-truth closure.

The code-level scope covered here is:

- helper-backed `IPiCameraDriver` as the default payload backend boundary
- `payload_camera_backend_helper` process and pipe-based request/reply protocol
- parent-owned timeout, abort, and helper-kill behavior
- hosted regression over the helper-backed compatibility path
- target build truth for `LIBCAMERA_FOUND`
- target proof for real OV5647 capture and explicit raw-register non-claim

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native --target payload_camera_backend_helper OBC payload_csp_probe_main -j 8
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target payload_backend_helper_client_unit_test OBC_Components_PayloadOpsController_ut_exe -j 8
./build-fprime-automatic-native-ut/bin/Darwin/payload_backend_helper_client_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
bash scripts/run_payload_target_backend_hardening_v1_hosted_probe.sh
ssh operator@<private-lab-host> 'cd $OBC_HOME/lab/fprime/v0 && source fprime-venv/bin/activate && cmake --build build-fprime-automatic-native --target payload_camera_backend_helper OBC -j 4'
bash scripts/run_payload_target_backend_hardening_v1_target_probe.sh
openspec validate payload-target-backend-hardening-v1
openspec validate --specs
```

## Current Results

- Fresh native helper build: PASS
- Focused helper/component UT:
  - `payload_backend_helper_client_unit_test`: PASS
  - `OBC_Components_PayloadOpsController_ut_exe`: PASS
- Hosted helper-backed regression wrapper: PASS
  - canonical PASS root:
    `/tmp/payload-target-backend-hardening-v1-hosted.DK9u62`
- Raspberry Pi target build truth:
  - OV5647 is enumerated by `rpicam-hello --list-cameras`
  - target build `CMakeCache.txt` reports `LIBCAMERA_FOUND:INTERNAL=1`
- Raspberry Pi target proof:
  - PASS
  - canonical PASS root:
    `/tmp/payload-target-backend-hardening-v1-target.4VOj7j`
- `openspec validate payload-target-backend-hardening-v1`: PASS
- `openspec validate --specs`: PASS

## Focused Coverage

`payload_backend_helper_client_unit_test` currently proves:

- helper binary discovery from the OBC sibling executable directory
- helper request/reply round-trips for capabilities and stub register access
- timeout/abort path keeps the helper boundary bounded from the parent side

`OBC_Components_PayloadOpsController_ut_exe` currently proves:

- helper-backed runtime still honors session transitions and metadata writeback
- unsupported raw-register paths return governed `PRESULT_REJECTED_UNSUPPORTED`
- compatibility wrappers keep the v1 payload surface reviewable while v2 is in
  flight

## Hosted Helper Regression

Repository-owned hosted entry point:

```bash
bash scripts/run_payload_target_backend_hardening_v1_hosted_probe.sh
```

Hosted proof path currently reuses two bounded sub-proofs:

```text
1. helper-backed payload-ops compatibility on the default hosted CCSDS node-5 path
2. helper-backed payload CSP shim proof on the hosted internal CSP runtime
```

Canonical current PASS root:

```text
/tmp/payload-target-backend-hardening-v1-hosted.DK9u62
```

Current hosted truth:

- the main OBC runtime now talks to the payload backend through the helper
  client boundary even on hosted
- hosted proof still uses the stub backend for capture artifacts
- the hosted helper regression remains distinct from target real-camera closure
- the hosted wrapper now reuses the same ground-link quiet gating discipline as
  the compat-v1 hosted sequencing probe; without that bounded `GROUND_LINK_UP`
  quiet window, `SESSION_OPEN` and early `MODE_SET` ingress can flake on the
  comm-managed node-`5` path

## Target Helper-Backed Real-Camera Proof

Repository-owned target entry point:

```bash
bash scripts/run_payload_target_backend_hardening_v1_target_probe.sh
```

Canonical current PASS root:

```text
/tmp/payload-target-backend-hardening-v1-target.4VOj7j
```

Current target truth:

- the active target OBC runtime talks to the payload backend through the
  helper-client boundary, not through direct in-process `libcamera` calls
- the target proof uses the authenticated Pi-local `OBC -> GDS` adapter path
  for payload commands
- `PAYLOAD_CAPTURE_AUTO` produced a real OV5647-backed JPEG artifact at
  `persistent-data/payload/camera/capture-1-1.jpg`
- `PAYLOAD_CAPTURE_DETERMINISTIC` produced a second real OV5647-backed JPEG
  artifact at `persistent-data/payload/camera/capture-1-2.jpg`
- metadata sidecars for both captures recorded backend `libcamera`, camera
  model `OV5647`, and the requested/applied masks
- deterministic target proof now uses a direct command path rather than
  `SEQ_VALIDATE/SEQ_RUN`; official sequencing proof remains governed by the
  hosted payload-capture and payload-ops evidence paths
- raw sensor register access remains an explicit bounded non-claim on the real
  backend; the probe enters `RAW_SENSOR` session and confirms register read is
  rejected as unsupported
- payload power remains a lab proxy only: EPS simulator PDU channel `3`
  toggles while the camera remains OBC-powered on the Raspberry Pi CSI path

## Target Truth And Investigation

Confirmed directly on the OBC Pi:

- `rpicam-hello --list-cameras` enumerates `ov5647`
- Linux media graph exposes the OV5647 sensor on the Raspberry Pi camera path
- `libcamera-dev` is installed and the branch target build now detects
  `LIBCAMERA_FOUND=1`

Current raw-register truth:

- hosted fake backend supports a stub register map for contract proof
- the real `libcamera` backend currently reports `rawRegisterSupported=false`
- this branch therefore records **explicit bounded non-claim** for real
  OV5647 register read/write closure unless a target-native sensor-control path
  is implemented later in the PR

Current power-control truth:

- payload power remains a lab proxy on EPS simulator PDU channel `3`
- this record does not yet claim any maintainable Raspberry Pi camera physical
  rail on/off path

## Non-Claims

- no physical EPS-switched payload rail closure
- no target node-`7` split payload process yet
- no real OV5647 raw-register round-trip claim yet
- no final helper-restart operator command surface
- no target official sequencing closure in this change-local target record; the
  target direct deterministic proof intentionally avoids coupling helper-backed
  camera closure to the Pi-local sequence-ingress flake
