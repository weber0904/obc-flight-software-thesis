# payload-raw-preview-dual-artifact-v1 Evidence

Date:
- `2026-06-04`

OpenSpec change:
- `payload-raw-preview-dual-artifact-v1`

Current-note:

- the 2026-06-28 Chapter 5 Route `1` target payload preview/downlink raw tree
  remains retained historical transport/hash/downlink evidence under:
  [chapter5 route1 historical root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json)
- the thesis-backed sequence campaign on 2026-07-20 retained functional PASS
  observations for the Route 1 sequence, deterministic preview downlink, and
  SoC fallback. The 2026-07-20 observation is not governed A/B/C authority
  because C restarted the shared OBC service and complete contemporaneous
  revision provenance is absent.
  Its frozen repo-backed evidence and decoded-JSON canonicalization manifest
  live under
  [2026-07-20-route1-target-abc-rerun](../chapter5-integrated-route-closure-v1/artifacts/2026-07-20-route1-target-abc-rerun/README.md).
  This observation does not replace this record's detailed dual-artifact
  contract or broaden its transport claims.
  The 2026-07-12 Route 1 proof is historical functional evidence, not current target authority,
  because its required remote workspace/build/install provenance artifacts
  were not retained. Route 1 target requalification remains pending.
- after the later payload public-surface convergence, a fresh target rerun of
  the maintained Route `1` wrapper still passed on `2026-07-07`; the minimal
  repo-backed regression import lives under:
  [2026-07-07-public-surface-regression/target-regression](ARTIFACTS.json)
- after the `node5-comm-csp-downlink-v2` transport uplift was actually
  deployed onto the governed live target baseline, the same maintained target
  wrapper passed again on `2026-07-07` at:
  `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.SMhdY0/case`
- after the later `node5-comm-csp-downlink-v3` branch-local reruns on
  `2026-07-08`, the maintained hosted wrapper passed again at:
  `/tmp/payload-raw-preview-dual-artifact-v1-hosted.v3-default`
  and the governed target wrapper passed again at:
  `/private/tmp/payload-raw-preview-dual-artifact-v1-target.v3-mtu240/case`
- after the follow-up `node5-comm-csp-downlink-v3` target recovery on
  `2026-07-09`, the maintained governed target wrapper passed again in
  archived-artifact reuse mode at:
  `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3/case`
  using the Chapter 5 route1 target summary and source families from:
  [chapter5 route1/target canonical root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json)
- this record remains the dedicated payload preview/downlink explanation
  surface; it is not the current sequence Route 1 closure entry
- current target source-image validity is now covered separately by:
  [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)
- the 2026-06-28 Route 1 target closure here still proves transport/hash/
  downlink truth; it should not be cited by itself as proof that the target
  onboard source image content was already non-black
- the 2026-07-07 regression rerun likewise proves maintained
  transport/hash/downlink closure after the public-surface rename and command
  migration; it still does not supersede the separate source-image validity
  proof

## Scope

This record closes the payload dual-artifact branch on the active official
delivery path:

```text
PayloadOpsController
  -> DpManager/DpWriter
  -> DpCatalog
  -> CommController
  -> FileDownlink
```

The branch changes payload capture from a single local `.jpg` story to a
dual-artifact model:

- local `raw` capture stored as `persistent-data/payload/camera/PIC%02X.bin`
- local preview JPEG stored as `persistent-data/payload/camera/PIC%02X.jpg`
- preview JPEG auto-published as the official payload `.fdp` artifact
- raw official payload `.fdp` published later only when explicitly requested
  by `PAYLOAD_PUBLISH_CAPTURE(captureIndex, RAW_FRAME)`

This record is intended to prove:

- accepted `PAYLOAD_CAPTURE_*` commands now include a ground-chosen
  `captureIndex: U8`
- each capture stores one deterministic local raw artifact and one deterministic
  local preview JPEG for the same capture index, independent of later official
  raw publication
- official payload delivery remains payload `.fdp`, but the payload product
  family is now artifact-aware rather than JPEG-only
- preview JPEG remains the default official path after capture, while raw
  publication is an explicit follow-up operator action
- `PAYLOAD_GET_LAST_CAPTURE_METADATA()` remains a current-runtime convenience
  readback rather than a cross-restart persistent "last pointer"; post-restart
  operator recovery still depends on a ground-known `captureIndex`
- `FULL` raw official publish remains intentionally bounded out of scope on the
  current `2 MiB` payload data-product ceiling
- the current global transport uplift is part of the active branch truth:
  `FW_FILE_BUFFER_MAX_SIZE = 2032`, `ComCfg::TmFrameFixedSize = 4096`,
  `commsFileBuffSize = 4096`, and `commsFileBuffCount = 32`
- governed target reruns for this path inherit the target `A -> B -> C`
  default, so preflight and postflight readiness must require both
  `subsystem-sband-csp.service` and `subsystem-uhf-csp.service` unless a
  separately documented narrower exception is being proven

This record does **not** intend to prove:

- broad target full-resolution raw official downlink closure
- payload-specific browse/list/select/download command families
- QoS shaping or token-bucket downlink control
- UHF throughput broadening
- any claim that local preview JPEG replaces raw as the only payload artifact

## Implemented Entry Points

- [scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh](../../../scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh)
- [scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh](../../../scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh)
- [scripts/payload_fdp_extract.py](../../../scripts/payload_fdp_extract.py)

## Commands

Commands run from `$REPO_ROOT` unless noted otherwise:

```bash
./fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target OBC
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target OBC_Components_PayloadOpsController_ut_exe OBC_Components_CommandIngressAuthority_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe --gtest_filter='PayloadOpsController.CaptureStillWritesDualArtifactsAndAutoPublishesPreview:PayloadOpsController.PublishRawPromotesStoredCapture:PayloadOpsController.PublishRawRejectsFullResolution:PayloadOpsController.ReusingCaptureIndexOverwritesLocalArtifacts:PayloadOpsController.PreviewPublishIgnoresUnrelatedDpWriterNotifications'
bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
openspec validate payload-raw-preview-dual-artifact-v1
openspec validate --specs
```

## Results

- Fresh native OBC build: `PASS`
- Focused payload/controller authority UT build: `PASS`
- Focused payload controller UT run: `PASS`
- Hosted dual-artifact official proof: `PASS`
- Governed target node-`5` dual-artifact official proof: `PASS`
- `openspec validate payload-raw-preview-dual-artifact-v1`: `PASS`
- `openspec validate --specs`: `PASS`

## Hosted Official Dual-Artifact Proof

Repository-owned entrypoint:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh
```

Reference passing run:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-hosted=PASS` |
| probe root | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.QaBL7i` |
| runtime root | `/tmp/pedc-hosted-rt` |
| GDS file store | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.QaBL7i/gds-downlink` |
| summary JSON | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.QaBL7i/payload-raw-preview-dual-artifact-summary.json` |

Hosted proof is expected to cover:

- `VGA`: local raw + local preview JPEG, preview auto-publish, raw explicit
  publish, and governed payload `.fdp` byte-match/decode/hash-match on the
  stock downlink path
- `HD`: local raw + local preview JPEG, preview auto-publish, raw explicit
  publish, plus source-side payload `.fdp` decode/hash proof; governed hosted
  downlink stays bounded to preview-only on the stock path
- `FULL`: preview path only if it fits the current payload-family ceiling, plus
  explicit bounded rejection for `RAW_FRAME`

Hosted passing facts:

- `vga` local preview bytes `22`, local raw bytes `614400`
- `hd` local preview bytes `22`, local raw bytes `1843200`
- `full` local preview bytes `22`, local raw bytes `16163840`
- governed downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `320`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `617880`, byte-match `PASS`
  - `hd` `PREVIEW_JPEG` `.fdp` family bytes `320`, byte-match `PASS`
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
  - `hd` preview JPEG extract hash `PASS`
- `full` raw publish remained the intended bounded non-claim:
  `PRESULT_STORAGE_FAILED detail 24`

## Governed Target Node-`5` Dual-Artifact Proof

Repository-owned entrypoint:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
```

Reference passing run:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-target=PASS` |
| probe root | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.1Md11X/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.1Md11X/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.1Md11X/case/payload-raw-preview-dual-artifact-target-summary.json` |

Current rerun on the active `node5-comm-csp-downlink-v3` branch:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-target=PASS` |
| probe root | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.u8ppwU/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.u8ppwU/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.u8ppwU/case/payload-raw-preview-dual-artifact-target-summary.json` |

Current rerun facts:

- official stock path remained
  `PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`
- target proof closed with `node5-transport=v3`
- target secure-auth closed before payload work, and the real SocketCAN path
  showed repeated `BEGIN -> DATA -> ACK_POLL -> COMMIT = OK`
- target timing on this rerun:
  - `START_XMIT_CATALOG` requested at `2026-07-07T22:32:04.509057Z`
  - `CatalogXmitCompleted` observed at `2026-07-07T22:50:18.111694Z`
    (`1093.603 s`)
  - final GDS arrival observed at `2026-07-07T22:50:19.963926Z`
    (`1095.455 s`)
  - `CatalogXmitCompleted -> final GDS arrival` was `1.852 s`
- node-`5` v3 status:
  - before xmit: `accepted=798720 flushed=798720`
  - at catalog complete: `accepted=2379776 flushed=2379776 queued-frames=0 duplicate-frames=0`
  - after final GDS match: `accepted=2400256 flushed=2400256 queued-frames=0 duplicate-frames=0`
- `vga` local preview bytes `47598`, local raw bytes `614400`
- `hd` local preview bytes `142484`, local raw bytes `1843200`
- `full` local preview bytes `1126835`, local raw bytes `16242688`
- governed downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `47917`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `618090`, byte-match `PASS`
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
- `hd` preview remained source-side decode/hash `PASS`
- `full` preview remained source-side decode/hash `PASS`

Current `2026-07-09` governed target artifact-reuse rerun on the same branch:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-target=PASS` |
| probe root | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3/case/payload-raw-preview-dual-artifact-target-summary.json` |

Current target artifact-reuse rerun facts:

- source provisioning mode was `reuse-existing-summary`
- reused summary input:
  `$REPO_ROOT/docs/test-records/chapter5-integrated-route-closure-v1/artifacts/2026-06-28-formal-rerun/route1/target/external-roots/payload-raw-preview-dual-artifact-v1-target.xw9FPi/case/payload-raw-preview-dual-artifact-target-summary.json`
- the wrapper staged the archived payload `.fdp` family back into the governed
  target runtime, then still used the stock official path
  `BUILD_CATALOG -> START_XMIT_CATALOG(NO_WAIT)` through
  `PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`
- this rerun used the branch-head target `2032` transport condition that
  currently matters on real hardware:
  scoped node-`5`/`6` COMM CAN FD override enabled
- target proof closed with `node5-transport=v3`
- target timing on this rerun:
  - `START_XMIT_CATALOG -> CatalogXmitCompleted = 270.567 s`
  - `START_XMIT_CATALOG -> final GDS arrival = 277.798 s`
  - `CatalogXmitCompleted -> final GDS arrival = 7.231 s`
- governed downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `5709`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `617880`, byte-match `PASS`
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
- reuse-mode verification note:
  - current `payload_fdp_extract.py` / `fprime-dp-write` did not reliably
    re-decode this archived source family on the current branch, so the wrapper
    used archived `sourceFamilySummary` plus family-level SHA-256 equality as
    the historical-source oracle
- `full` raw publish remained the intended bounded non-claim:
  `PRESULT_STORAGE_FAILED detail 24`

Current `2026-07-09` governed target artifact-reuse rerun after moving
`FileDownlink.Run` onto the `1 Hz` data group:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-target=PASS` |
| probe root | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-runfix/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-runfix/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-runfix/case/payload-raw-preview-dual-artifact-target-summary.json` |

Current target artifact-reuse rerun facts after the scheduler fix:

- source provisioning mode remained `reuse-existing-summary`
- reused summary input remained:
  `$REPO_ROOT/docs/test-records/chapter5-integrated-route-closure-v1/artifacts/2026-06-28-formal-rerun/route1/target/external-roots/payload-raw-preview-dual-artifact-v1-target.xw9FPi/case/payload-raw-preview-dual-artifact-target-summary.json`
- the wrapper still used the stock official path
  `BUILD_CATALOG -> START_XMIT_CATALOG(NO_WAIT)` through
  `PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`
- this rerun kept the same branch-head target transport condition that
  currently matters on real hardware:
  scoped node-`5`/`6` COMM CAN FD override enabled
- target proof still closed with `node5-transport=v3`
- target timing on this rerun:
  - `START_XMIT_CATALOG -> CatalogXmitCompleted = 176.091 s`
  - `START_XMIT_CATALOG -> final GDS arrival = 176.305 s`
  - `CatalogXmitCompleted -> final GDS arrival = 0.214 s`
- event timing proved the scheduler fix removed a separate upstream delay:
  - raw `64647`-byte slices now show `SendingProduct -> SendStarted` around
    `2.0..3.0 s`
  - raw `64647`-byte slices still show `SendStarted -> FileSent` around
    `8.9..21.7 s`
  - therefore the remaining dominant cost is the official path above V3, not
    node-`5` queue drain
- current official CCSDS/TM packing math on this branch:
  - `FW_FILE_BUFFER_MAX_SIZE = 2032`
  - `ComCfg::AggregationSize = 4081`, so two file buffers plus two
    `6`-byte space-packet headers fit into one `4096`-byte TM frame
  - one `64647`-byte raw slice therefore maps to about `32` file buffers and
    about `16` synchronous `GroundLinkDriver.send()` calls
  - with the observed `SendStarted -> FileSent` timings above, the implied
    remaining synchronous send cost is about `0.56..1.36 s` per TM frame,
    average about `0.856 s`
- governed downlink set remained closed:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `5709`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `617880`, byte-match `PASS`
- extractor/hash results remained closed:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
- node-`5` status stayed drained at completion:
  - before xmit: `accepted=36864 flushed=36864`
  - at catalog complete: `accepted=1015808 flushed=1015808 queued-frames=0 duplicate-frames=0`
  - after final GDS match: `accepted=1024000 flushed=1024000 queued-frames=0 duplicate-frames=0`
- `full` raw publish remained the intended bounded non-claim:
  `PRESULT_STORAGE_FAILED detail 24`

Current `2026-07-08` hosted rerun on the same branch:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-hosted=PASS` |
| probe root | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.v3-default` |
| runtime root | `/tmp/pedc-hosted-rt-v3-default` |
| GDS file store | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.v3-default/gds-downlink` |
| summary JSON | `/tmp/payload-raw-preview-dual-artifact-v1-hosted.v3-default/payload-raw-preview-dual-artifact-summary.json` |

Current hosted rerun facts:

- the branch default hosted path still passed with `node5-transport=v3`
  without forcing a reduced `COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES`
- hosted timing on this rerun:
  - `START_XMIT_CATALOG` requested at `2026-07-08T05:53:19.498778Z`
  - `CatalogXmitCompleted` observed at `2026-07-08T06:02:49.872945Z`
    (`570.374 s`)
  - final GDS arrival observed at `2026-07-08T06:02:53.415862Z`
    (`573.917 s`)
  - `CatalogXmitCompleted -> final GDS arrival` was `3.543 s`
- node-`5` v3 status:
  - before xmit: `accepted=430080 flushed=430080`
  - at catalog complete: `accepted=1531904 flushed=1531904 queued-frames=0 duplicate-frames=0`
  - after final GDS match: `accepted=1536000 flushed=1536000 queued-frames=0 duplicate-frames=0`
- hosted downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `341`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `618090`, byte-match `PASS`
  - `hd` `PREVIEW_JPEG` `.fdp` family bytes `341`, byte-match `PASS`
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
  - `hd` preview JPEG extract hash `PASS`

Current `2026-07-08` governed target rerun on the same branch:

| Field | Value |
|---|---|
| wrapper verdict | `payload-raw-preview-dual-artifact-v1-target=PASS` |
| probe root | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.v3-mtu240/case` |
| target runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| GDS file store | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.v3-mtu240/case/sband-ground/gds-runtime/gds-files` |
| summary JSON | `/private/tmp/payload-raw-preview-dual-artifact-v1-target.v3-mtu240/case/payload-raw-preview-dual-artifact-target-summary.json` |

Current target rerun facts:

- this rerun required `COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES=240` on the
  governed target path; the hosted rerun above still passed on the branch
  default sizing
- official stock path remained
  `PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`
- target proof closed with `node5-transport=v3`
- target timing on this rerun:
  - `START_XMIT_CATALOG` requested at `2026-07-08T05:29:25.116074Z`
  - `CatalogXmitCompleted` observed at `2026-07-08T05:46:51.156486Z`
    (`1046.04 s`)
  - final GDS arrival observed at `2026-07-08T05:46:53.069798Z`
    (`1047.954 s`)
  - `CatalogXmitCompleted -> final GDS arrival` was `1.913 s`
- node-`5` v3 status:
  - before xmit: `accepted=667648 flushed=667648`
  - at catalog complete: `accepted=1769472 flushed=1769472 queued-frames=0 duplicate-frames=0`
  - after final GDS match: `accepted=1769472 flushed=1769472 queued-frames=0 duplicate-frames=0`
- governed downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `49308`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `618090`, byte-match `PASS`
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
- this target rerun also showed an extra non-payload HK trend `.fdp` entering
  the official catalog alongside the selected payload family, which explains
  why the observed GDS file count during transfer exceeded the payload-only
  family-slice count
- `full` raw publish remained the intended bounded non-claim:
  `raw-publish-status=bounded-full-raw-deferred`
  with diagnostic `PRESULT_STORAGE_FAILED detail 24`
- provenance correction:
  - an earlier same-day target false negative was collected before the branch
    was actually deployed to the live node-`5` subsystem binary and installed
    OBC release
  - after refreshing live provenance, the same governed wrapper closed `PASS`,
    so that earlier short raw-family decode was not treated as the canonical
    product result

Target proof is expected to cover:

- official `PREVIEW_JPEG` delivery on the governed node-`5` path
- bounded official `RAW_FRAME` delivery for `PRESET_VGA_640X480`
- source-side `HD` and `FULL` preview capture plus payload `.fdp` decode truth
- explicit `FULL` raw bounded non-claim

Target passing facts:

- `vga` local preview bytes `9727`, local raw bytes `614400`
- `hd` local preview bytes `15209`, local raw bytes `1843200`
- `full` local preview bytes `1262568`, local raw bytes `16242688`
- governed downlink set:
  - `vga` `PREVIEW_JPEG` `.fdp` family bytes `10025`, byte-match `PASS`
  - `vga` `RAW_FRAME` `.fdp` family bytes `617880`, byte-match `PASS`
- target downlink timing on this reference run:
  - `START_XMIT_CATALOG` accepted at `2026-06-04 22:24:31`
  - `vga` preview `.fdp` first arrived at GDS at `2026-06-04 22:24:54`
    (about `23 s`)
  - `vga` raw `.fdp` family first slice arrived at GDS at
    `2026-06-04 22:26:10` (about `99 s`)
  - `vga` raw `.fdp` family final slice arrived at GDS at
    `2026-06-04 22:37:32` (about `781 s`, roughly `13.0 min`)
  - for comparison, the earlier target payload `.fdp` closure proof needed
    about `1731 s` (roughly `28.9 min`) to land the full `vga` family, so the
    current transport uplift is measurably better but still slow
- extractor/hash results:
  - `vga` preview JPEG extract hash `PASS`
  - `vga` raw extract hash `PASS`
  - `hd` preview JPEG source-side decode/hash `PASS`
  - `full` preview JPEG source-side decode/hash `PASS`
- `hd` raw remained a non-claim on the governed target path in this reference
  run; only `vga` raw was formally promoted and downlinked
- `full` raw publish remained the intended bounded non-claim:
  `PRESULT_STORAGE_FAILED detail 24`
- cleanup status under the proof root reported `PASS`

## Transport Budget Note

This branch intentionally raises the file-path packet budget before any new
throughput broadening claim:

- `FW_FILE_BUFFER_MAX_SIZE = 2032`
- `ComCfg::TmFrameFixedSize = 4096`
- `ComCcsds` / `OBCComCcsds` file-buffer size `4096`
- `ComCcsds` / `OBCComCcsds` file-buffer count `32`

The branch also widens queue depth on the data-product and stock file-handling
active components so multi-slice payload publication can proceed without
asserting on default queue-size `10` assumptions. This is a stability fix for
the current official path, not yet a formal throughput-optimization claim.

The slow official raw downlink problem is **not** resolved by this branch.
What this change proves is:

- preview JPEG becomes small enough that governed preview delivery is practical
- the same stock official path can now carry bounded `vga` raw correctly
  without the older near-`29 min` behavior

What remains true on the active baseline is:

- governed raw payload-family downlink is still slow on the stock
  `DpCatalog -> CommController -> FileDownlink` path
- the reference `vga` raw family still took about `13 min` to land fully at
  GDS, which is too slow to treat the transport problem as solved
