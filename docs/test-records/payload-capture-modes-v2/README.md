# payload-capture-modes-v2 Test Record

## Scope

- Branch: `feature/payload-ops-v2`
- OpenSpec change: `payload-capture-modes-v2`
- Date: 2026-05-21

This record captures the in-branch verification state for the payload v2
capture-surface expansion. It now includes a fresh hosted CCSDS node-`5`
sequence-wrapper PASS, but it still does not claim closeout-ready target
closure.

Current-note:

- this record is preserved as the historical branch proof that introduced the
  broader payload v2 surface
- current maintained payload semantics no longer require separate prepared
  `AUTO` versus `DETERMINISTIC` states; normal still capture now uses one
  shared camera-ready state, while `RAW_SENSOR` remains the only special
  prepare gate
- `PAYLOAD_CAPTURE_STILL` has since retired from the current public payload
  surface; any `STILL`-based examples here remain historical compatibility
  evidence only
- current target source-image validity and actual `AUTO` runtime readback are
  now governed separately by:
  [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)
- current persistent-session lifecycle and `STILL` retirement closure are now
  governed by:
  [payload-persistent-session-still-retirement-v1](../payload-persistent-session-still-retirement-v1/README.md)
- do not cite this record by itself as current authority for single-ready
  payload semantics, actual `AUTO` runtime metadata, or target real-camera
  non-black source-image validity

The code-level scope covered here is:

- new `AUTO`, `DETERMINISTIC`, and `RAW_SENSOR` session-kind vocabulary
- OFF-only `AUTO` and `DETERMINISTIC` profile update surfaces
- apply-mask-based `AUTO` and `DETERMINISTIC` capture overrides
- `.jpg + .json` dual-artifact capture output
- `PAYLOAD_GET_CAPABILITIES()` and `PAYLOAD_GET_LAST_CAPTURE_METADATA()`
- PR-stage compatibility wrappers for legacy v1 payload commands

## Commands

Commands run from `$REPO_ROOT`:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j 8
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_PayloadOpsController_ut_exe OBC_Components_CommandIngressAuthority_ut_exe -j 8
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash -n scripts/run_payload_capture_modes_v2_hosted_probe.sh
bash scripts/run_payload_capture_modes_v2_hosted_probe.sh
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate payload-capture-modes-v2
```

## Current Results

- Fresh native build: PASS
- Focused UT target build: PASS
- `OBC_Components_PayloadOpsController_ut_exe`: PASS
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS
- `bash -n scripts/run_payload_capture_modes_v2_hosted_probe.sh`: PASS
- `bash scripts/run_payload_capture_modes_v2_hosted_probe.sh`: PASS
  - canonical root: `/tmp/payload-capture-modes-v2-hosted.ROUjZ5`
- `openspec validate payload-capture-modes-v2`: PASS

## Focused Coverage

`OBC_Components_PayloadOpsController_ut_exe` currently proves:

- legacy v1 prepare/capture/shutdown wrapper behavior still functions
- `PAYLOAD_PREPARE_SESSION(AUTO)` and `PAYLOAD_CAPTURE_AUTO(...)` publish
  capture metadata and produce governed `.json` sidecars
- `PAYLOAD_GET_CAPABILITIES()` remains a readback-only surface
- `RAW_SENSOR` session preparation now propagates the prepared session kind
  correctly instead of falling back to deterministic session state

`OBC_Components_CommandIngressAuthority_ut_exe` currently proves:

- new payload v2 command opcodes remain cataloged under governed payload
  authority labels
- backup/read-status ingress still reads payload status surfaces without gaining
  payload-control authority

## Hosted Proof Status

Repository-owned hosted entry point:

```bash
bash scripts/run_payload_capture_modes_v2_hosted_probe.sh
```

Canonical path under test:

```text
fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> OBC
```

Current truth:

- the hosted probe now passes on the required CCSDS node-`5` path at
  `/tmp/payload-capture-modes-v2-hosted.ROUjZ5`
- the proof reuses the freshly re-stabilized shared sequencing/file-ingress
  spine re-proven at `/tmp/official-sequencing-system-resources-v1.oUHfai`
- direct `AUTO` session behavior is proven, including capability readback,
  `PAYLOAD_PREPARE_SESSION(AUTO)`, `PAYLOAD_CAPTURE_AUTO`,
  `PAYLOAD_GET_LAST_CAPTURE_METADATA`, governed `.jpg + .json` artifacts, and
  clean `PAYLOAD_SHUTDOWN`
- official sequence-wrapper behavior is proven for the deterministic path:
  `SEQ_VALIDATE(.sequence-staging/pcm2.bin)` is issued once as a bounded
  non-reject preflight on the same hosted node-`5` wrapper path, and
  `SEQ_RUN(.sequence-staging/pcm2.bin, WAIT)` produces the deterministic
  capture, wrapper status log, and governed artifacts
- the `SEQ_VALIDATE` step is intentionally treated as a non-reject preflight
  rather than a standalone success event because this repo's wrapper surface
  does not emit a dedicated validate-success event and long hosted runs can
  still miss `COMMAND_ENVELOPE_OBSERVED` on the external event subscriber even
  while the OBC-side wrapper path remains correct
- for this probe family, fresh hosted evidence and cleanup truth are only
  canonical when the repository-owned script runs without sandbox process-list
  restrictions; otherwise the probe can pass while leaving `fprime_gds`
  `comm` children orphaned

This record now registers a fresh hosted PASS directory as reusable evidence
for the capture-modes hosted node-`5` path, but it still does not claim target
closure.

## Non-Claims

- no target OV5647 real-image proof yet
- no target `libcamera` capability closure yet
- no target raw-register closure yet
- no payload virtual CSP node proof yet
