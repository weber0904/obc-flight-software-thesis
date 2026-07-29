# payload-e2e-downlink-closure-v1 Evidence

Date:
- `2026-06-03`
- `2026-06-04`

OpenSpec changes:
- `payload-e2e-downlink-closure-v1`
- `payload-async-capture-ack-and-target-proof-v1`

Current branch note:
- on the active branch after `payload-raw-preview-dual-artifact-v1`, the
  current payload official-delivery baseline moved to the dual-artifact
  wrappers
  `scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh` and
  `scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
- the wrapper names in this archived record remain the historical entrypoints
  used for the evidence captured here; they are no longer the preferred active
  baseline names
- this archived record predates the current dual-artifact local source
  contract; any older `.jpg + .json` wording here belongs to the historical
  branch state captured by this evidence, not to the current maintained
  payload local-artifact baseline
- current target source-image validity is now tracked separately under:
  [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)
- this archived record and its descendants remain official payload delivery
  evidence, not the current authority for target source-image content validity
- the maintained current Route `1` target wrapper was rerun again after
  `payload-public-surface-convergence-v1`; see the fresh regression import
  under
  [payload-raw-preview-dual-artifact-v1 2026-07-07 target-regression](../payload-raw-preview-dual-artifact-v1/ARTIFACTS.json)
  for the latest repo-backed corroboration that the converged payload public
  surface did not break official target Route `1` closure

## Scope

This record closes payload end-to-end official delivery on both the hosted
official path and the governed target node-`5` S-band path. Each successful
payload capture now promotes into one canonical payload `.fdp` family on the
official `DpManager -> DpWriter -> DpCatalog -> CommController ->
FileDownlink` path, while the sibling `.jpg + .json` files remain local
diagnostic residue under `persistent-data/payload/camera/`.

This record also updates the payload command model: accepted
`PAYLOAD_CAPTURE_*` commands acknowledge dispatch immediately, while final
capture success or failure is now determined by payload state, status,
telemetry, and last-capture metadata surfaces after capture plus canonical
publication complete.

This record proves:

- `PayloadOpsController` remains the only public payload owner
- the canonical payload delivery artifact is a payload `.fdp` family rather
  than the local `.jpg + .json` files
- payload readback surfaces expose the first-slice canonical `.fdp` relative
  path plus final publication status so operators can correlate local residue
  with the official artifact family
- stock `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` remain the official
  operator-owned delivery path for the payload `.fdp` family
- the hosted official path proves `.fdp` family byte-match, repo-owned decode,
  and JPEG extraction/hash parity on the default CCSDS node-`5` path
- the governed target node-`5` path proves bounded real-target payload closure
  for `PRESET_VGA_640X480`, including family byte-match, repo-owned decode,
  and JPEG extraction/hash parity
- the governed target path also records representative target size-envelope
  truth for `PRESET_HD_1280X720` local publication and `PRESET_FULL_3280X2464`
  bounded oversize rejection

This record does **not** prove:

- broad target payload size-envelope or throughput closure beyond the bounded
  `vga` downlink case, `hd` local publication case, and `full` oversize
  diagnostic
- payload-family reliable-transfer widening beyond the current official HK
  `.fdp` scope
- raw-register closure
- a physically switched EPS camera rail
- UHF nonquiet runtime stability

## Implemented Entry Points

- [scripts/run_payload_e2e_downlink_closure_v1_hosted_probe.sh](../../../scripts/run_payload_e2e_downlink_closure_v1_hosted_probe.sh)
- [scripts/run_payload_e2e_downlink_closure_v1_target_probe.sh](../../../scripts/run_payload_e2e_downlink_closure_v1_target_probe.sh)
- [scripts/payload_fdp_extract.py](../../../scripts/payload_fdp_extract.py)

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
./fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target OBC
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target OBC_Components_PayloadOpsController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
bash scripts/run_payload_e2e_downlink_closure_v1_hosted_probe.sh
bash scripts/run_payload_e2e_downlink_closure_v1_target_probe.sh
openspec validate payload-async-capture-ack-and-target-proof-v1
openspec validate --specs
```

## Results

- Fresh native OBC build: PASS
- Focused payload controller UT build: PASS
- `OBC_Components_PayloadOpsController_ut_exe`: PASS
- Hosted official payload `.fdp` family proof: PASS
- Governed target node-`5` payload `.fdp` family proof: PASS
- `openspec validate payload-async-capture-ack-and-target-proof-v1`: PASS
- `openspec validate --specs`: PASS

## Hosted Official Payload `.fdp` Family Proof

Repository-owned entrypoint:

```bash
bash scripts/run_payload_e2e_downlink_closure_v1_hosted_probe.sh
```

Official hosted proof path:

```text
PayloadOpsController
  -> DpManager/DpWriter
  -> DpCatalog
  -> CommController
  -> FileDownlink
```

Reference passing run:

| Field | Value |
|---|---|
| wrapper verdict | `payload-e2e-downlink-closure-v1-hosted: PASS` |
| probe root | `/tmp/payload-e2e-downlink-closure-v1-hosted.FWvCYT` |
| runtime root | `/tmp/pedc-hosted-rt` |
| GDS file store | `/tmp/payload-e2e-downlink-closure-v1-hosted.FWvCYT/gds-downlink` |
| summary JSON | `/tmp/payload-e2e-downlink-closure-v1-hosted.FWvCYT/payload-e2e-downlink-summary.json` |

Observed PASS excerpts:

```text
payload-e2e-downlink-closure-v1-hosted: PASS
official-path=PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink
downlink-queue=BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)
capture-case=vga resolution=PRESET_VGA_640X480 capture-id=1 jpeg-bytes=22 fdp-family-bytes=334 fdp-byte-match=PASS
capture-case=vga extract-jpeg-hash=PASS
capture-case=hd resolution=PRESET_HD_1280X720 capture-id=2 jpeg-bytes=22 fdp-family-bytes=334 fdp-byte-match=PASS
capture-case=hd extract-jpeg-hash=PASS
capture-case=full resolution=PRESET_FULL_3280X2464 capture-id=3 jpeg-bytes=22 fdp-family-bytes=334 fdp-byte-match=PASS
capture-case=full extract-jpeg-hash=PASS
```

Hosted proof proves:

- accepted `PAYLOAD_CAPTURE_*` requests return before final completion, and the
  hosted wrapper now waits on payload state plus metadata instead of treating
  the original command response as the final success oracle
- canonical payload `.fdp` family publication is part of final payload
  success on the hosted official node-`5` path
- payload readback can be correlated with the first-slice canonical `.fdp`
  relative path without adding any payload-specific browse/list/select/
  download command family
- the received `.fdp` family byte-matches the source family
- repo-owned decode tooling can extract payload metadata and JPEG bytes from
  the received `.fdp` family
- the hosted wrapper cleanup no longer leaves repo-root `.adm-*` / `.stg-*`
  sequence alias residue after exit

## Governed Target Node-`5` Payload `.fdp` Family Proof

Repository-owned entrypoint:

```bash
bash scripts/run_payload_e2e_downlink_closure_v1_target_probe.sh
```

Official target proof path:

```text
macOS fprime-gds + ground_ttc_gateway
  -> subsystem.local sband_comm_csp_node node 5
  -> SocketCAN
  -> obc.local PayloadOpsController
  -> DpManager/DpWriter
  -> DpCatalog
  -> CommController
  -> FileDownlink
```

Reference passing run:

| Field | Value |
|---|---|
| wrapper verdict | `payload-e2e-downlink-closure-v1-target: PASS` |
| probe root | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-e2e-downlink-closure-v1-target.7mKn0y/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-e2e-downlink-closure-v1-target.7mKn0y/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-e2e-downlink-closure-v1-target.7mKn0y/case/payload-e2e-downlink-target-summary.json` |

Observed PASS excerpts:

```text
payload-e2e-downlink-closure-v1-target: PASS
official-path=PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink
downlink-queue=BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)
downlink-case=vga resolution=PRESET_VGA_640X480 capture-id=1 jpeg-bytes=614400 fdp-family-bytes=617600 fdp-byte-match=PASS
downlink-case=vga extract-jpeg-hash=PASS sha256=93dfc510a01f020cbaa203c2ff2362c659bee80371821118817d97e5ac8bcf63
local-published-case=hd resolution=PRESET_HD_1280X720 capture-id=2 jpeg-bytes=1843200 fdp-family-bytes=1852480
diagnostic-case=full resolution=PRESET_FULL_3280X2464 status=bounded-oversize result=PRESULT_STORAGE_FAILED detail=14 jpeg-bytes=16242688 fdp-bytes=0
```

Target proof proves:

- the governed target node-`5` S-band baseline can execute payload capture on
  the active `libcamera` backend and publish the resulting canonical payload
  `.fdp` family under the target `data-products/` root
- the target wrapper keeps the official operator path repo-owned: stock
  `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` downlink the bounded
  `vga` payload `.fdp` family without adding any payload-specific transfer
  surface
- the GDS-received `vga` payload `.fdp` family byte-matches the target source
  family
- repo-owned decode tooling can extract the received target payload metadata
  and JPEG bytes, and the extracted JPEG SHA-256 matches the target source
  JPEG
- `PRESET_HD_1280X720` also publishes successfully on the target path as a
  larger local payload `.fdp` family, but this record does not yet elevate it
  to a governed target downlink claim
- `PRESET_FULL_3280X2464` is a bounded oversize diagnostic on the current
  `2 MiB` canonical payload-family ceiling, surfacing
  `PRESULT_STORAGE_FAILED detail 14`

## Multi-resolution Sampling Note

Hosted sampling intentionally exercised three request presets, but the hosted
stub backend writes fixed tiny JPEGs:

- `PRESET_VGA_640X480`: `22`-byte JPEG, `334`-byte `.fdp` family
- `PRESET_HD_1280X720`: `22`-byte JPEG, `334`-byte `.fdp` family
- `PRESET_FULL_3280X2464`: `22`-byte JPEG, `334`-byte `.fdp` family

Target sampling provides the bounded real-image truth that hosted cannot:

- `PRESET_VGA_640X480`: `614400`-byte JPEG, `617600`-byte `.fdp` family,
  governed node-`5` downlink PASS
- `PRESET_HD_1280X720`: `1843200`-byte JPEG, `1852480`-byte `.fdp` family,
  target local publication PASS
- `PRESET_FULL_3280X2464`: `16242688`-byte JPEG, canonical publication
  rejected as bounded oversize on the current `2 MiB` payload-family ceiling

This is the current active-baseline size-envelope truth. It does **not** yet
claim broad target throughput tuning or a wider formally admitted target
payload-family envelope.
