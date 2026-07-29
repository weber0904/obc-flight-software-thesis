# payload-persistent-session-still-retirement-v1 Evidence

Date:
- `2026-07-07`

Originating OpenSpec change:
- [payload-persistent-session-still-retirement-v1](../../../openspec/changes/archive/2026-07-06-payload-persistent-session-still-retirement-v1)

## Scope

This record closes the current payload lifecycle convergence slice:

- one shared persistent non-RAW `PAYLOAD_PREPARE` session
- `AUTO -> PAYLOAD_GET_LAST_CAPTURE_METADATA -> DETERMINISTIC` on that same
  session
- repeated deterministic follow-up capture without a second prepare
- session-level mismatch rejection instead of implicit reprepare
- `PAYLOAD_CAPTURE_STILL` retirement from the current public payload surface

It proves:

- current hosted semantics and current target real-camera behavior agree on the
  shared non-RAW `READY` contract
- current maintained normal still-capture policies are `AUTO` and
  `DETERMINISTIC` only
- target runtime no longer returns to a per-capture cold-start lifecycle
  between maintained non-RAW captures

It does **not** prove:

- Route 1 official payload `.fdp` downlink closure
- raw-register closure
- full generic target camera-control parity for `brightness`, `contrast`,
  `saturation`, or `sharpness`
- any claim that `PREPARE_SESSION(AUTO|DETERMINISTIC)` remains on the current
  maintained public surface; those operator-facing commands are now historical
  only

## Canonical Artifact Root

- canonical artifact root:
  [2026-07-07-local-ready](ARTIFACTS.json)
- previous retained local-ready root:
  [2026-07-06-local-ready](ARTIFACTS.json)
- hosted imported root:
  [hosted/probe-root](ARTIFACTS.json)
- hosted imported stack root:
  [hosted/stack-root](ARTIFACTS.json)
- canonical target proof root reused from the adjacent dedicated record:
  [payload-target-capture-sanity-v1 canonical root](../payload-target-capture-sanity-v1/ARTIFACTS.json)

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target OBC_Components_PayloadOpsController_ut_exe OBC_Components_CommandIngressAuthority_ut_exe -- -j4
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash scripts/run_payload_persistent_session_hosted_probe.sh
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/run_payload_target_capture_sanity_v1_target_probe.sh
```

## Results

- Fresh payload/controller focused UT build: `PASS`
- `OBC_Components_PayloadOpsController_ut_exe`: `PASS`
- `OBC_Components_CommandIngressAuthority_ut_exe`: `PASS`
- Hosted persistent-session semantic proof: `PASS`
- Governed target persistent-session payload proof: `PASS`

## What To Read

Read in this order:

1. [hosted/manifest.json](ARTIFACTS.json)
   - hosted original tmp root, command, timestamp, and formal verdict
2. [payload-target-capture-sanity-v1 manifest.json](../payload-target-capture-sanity-v1/ARTIFACTS.json)
   - canonical target proof import reused by this lifecycle record
3. hosted summary:
   [summary.log](ARTIFACTS.json)
4. hosted stack corroboration:
   [stack-root](ARTIFACTS.json)
   - hosted GDS / OBC logs referenced by the hosted summary JSON
5. target summary:
   [summary.log](../payload-target-capture-sanity-v1/ARTIFACTS.json)
6. target full machine-readable result:
   [payload-target-capture-sanity-summary.json](../payload-target-capture-sanity-v1/ARTIFACTS.json)
7. target command breadcrumbs:
   [checkpoints.jsonl](../payload-target-capture-sanity-v1/ARTIFACTS.json)

## Proven Current Lifecycle Facts

- current public normal still-capture surface is:
  - `PAYLOAD_SET_CAMERA_DEFAULTS`
  - `PAYLOAD_SET_AUTO_DEFAULTS`
  - `PAYLOAD_SET_DETERMINISTIC_DEFAULTS`
  - `PAYLOAD_PREPARE`
  - `PAYLOAD_CAPTURE_AUTO`
  - `PAYLOAD_GET_LAST_CAPTURE_METADATA`
  - `PAYLOAD_CAPTURE_DETERMINISTIC`
  - `PAYLOAD_SHUTDOWN`
- hosted proof confirms:
  - one `PAYLOAD_PREPARE`
  - `AUTO`
  - actual-metadata readback
  - `DETERMINISTIC`
  - same-session reuse
  - resolution-mismatch reject
  - one fresh hosted rerun still reports `prepare-ready-count=1`
- target proof confirms:
  - secure-auth on the governed node-`5` path
  - `PAYLOAD_PREPARE`
  - `AUTO`
  - metadata readback
  - `DETERMINISTIC`
  - deterministic follow-up capture in the same prepared session
  - busy rejection when `PAYLOAD_SET_CAMERA_DEFAULTS` tries to change
    resolution while that shared non-RAW session is still prepared
  - `PAYLOAD_SHUTDOWN`
  - after shutdown, the proof re-applies shared camera defaults and performs a
    fresh prepare before the flipped deterministic corroboration case
- `PAYLOAD_CAPTURE_STILL` is no longer part of the current maintained public
  surface; treat older `STILL`-based wrappers and records as historical only

## Source / Ground Distinction

- `hosted/probe-root/...`
  - hosted semantic proof artifacts from the local stub/helper-backed runtime
  - these prove controller/manager contract semantics, not real sensor output
- `hosted/stack-root/...`
  - hosted runtime/GDS logs used by the hosted proof
  - these corroborate hosted semantic flow only; they are not target evidence
- `payload-target-capture-sanity-v1 .../probe-root/source-artifacts/...`
  - onboard artifacts created on `obc.local`, then copied back by SSH
  - these prove target-side generation, not ground receipt
- `payload-target-capture-sanity-v1 .../probe-root/source-decode/...`
  - macOS-side decode/extract of target source artifacts or target-side source
    `.fdp`
  - these are still source-side corroboration, not payload downlink evidence
- `payload-target-capture-sanity-v1 .../probe-root/sband-ground/...`
  - macOS ground stack logs/captures for the governed node-`5` secure-auth
    command path reused by the target proof
  - these prove command/readback flow for this slice, not Route 1 payload
    delivery closure

## Relationship To Adjacent Records

- [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)
  remains the current authority for target source-image validity, actual
  `AUTO` exposure/gain/AWB readback, and bounded preview-orientation
  corroboration
- [payload-raw-preview-dual-artifact-v1](../payload-raw-preview-dual-artifact-v1/README.md)
  now also carries the fresh `2026-07-07` target Route `1` regression import
  showing that the converged public payload surface did not break maintained
  preview/raw publication and downlink closure
- [payload-ops-contract-v1](../payload-ops-contract-v1/README.md) and
  [payload-capture-modes-v2](../payload-capture-modes-v2/README.md) remain
  historical records and should not be cited as current `STILL`-maintained
  payload truth
