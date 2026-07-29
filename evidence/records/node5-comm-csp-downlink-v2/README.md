# Node-5 COMM CSP Downlink V2

Status: hosted plus governed target/lab evidence captured on this branch on
2026-07-07.

## Scope

This record captures the regularized `node-5`-only COMM CSP downlink `v2`
transport uplift implemented by `node5-comm-csp-downlink-v2`.

It covers:

- parallel `v2` node-`5` transport services added below the existing stock
  `DpCatalog -> CommController -> FileDownlink -> CommEgressMux` ownership
  boundary
- node-`5` staging, final-chunk commit, drain-worker decoupling, duplicate
  reply caching, and `ABORT_V2` behavior
- OBC-side `DOWNLINK_STATUS_V2` probe, `v2` fallback policy, and per-send
  stream chunking from `CommCspGroundLinkBackend`
- local transport-only hosted proof that demonstrates accepted-before-flushed
  queue separation below the stock official downlink path
- full hosted official `.fdp` parity through
  `fprime-gds -> ground_ttc_gateway -> sband_comm_csp_node(node 5) -> CSP ->
  OBC -> FileDownlink`
- the current hosted proof-owner correction that starts the bounded UHF/node-`6`
  surface as baseline preflight because the current secure-auth runtime can fail
  early when the proof narrows itself to node `5` only
- `GroundLinkDriver` default uplink poll timeout updated from `100 ms` to
  `1000 ms`

It does **not** cover:

- node-`6` migration, UHF reliable-transfer widening, or generic all-node
  COMM-CSP migration
- end-to-end reliable delivery, ARQ, CFDP, or external-link persistence beyond
  node-`5` in-memory staging/commit semantics
- throughput-clean closure of the official hosted path

## Commands

Focused local build / UT:

```bash
./fprime-venv/bin/cmake --build build-fprime-automatic-native --target OBC comm_downlink_v2_status_probe sband_comm_csp_node ground_ttc_gateway csp_zmqproxy -j4
./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target comm_groundlink_unit_test -j4
```

Focused hosted proofs:

```bash
bash scripts/run_node5_comm_csp_downlink_v2_transport_hosted_probe.sh
bash scripts/run_node5_comm_csp_downlink_v2_hosted_probe.sh
```

Adjacent official hosted CCSDS S-band `.fdp` control proof:

```bash
bash scripts/run_hk_data_product_alignment_v2_probe.sh
```

OpenSpec validation:

```bash
openspec validate node5-comm-csp-downlink-v2
openspec validate --specs
```

## Acceptance For This Record

This record is accepted when all of the following are true:

- local build / UT complete for the touched native binaries and
  `comm_groundlink_unit_test`
- the focused transport-only hosted proof passes with byte-match and observable
  accepted-before-flushed queue gap
- the full hosted official `.fdp` parity proof passes through the maintained
  `ground_ttc_gateway -> fprime-gds` path with `DOWNLINK_STATUS_V2` available
- the hosted official proof explicitly starts the bounded node-`6` surface as
  baseline preflight
- the governed target/lab node-`5` official payload `.fdp` wrapper passes on
  live `obc.local` / `subsystem.local` hardware after provenance is refreshed
- `openspec validate node5-comm-csp-downlink-v2` passes

This record remains intentionally narrower than full closeout:

- the hosted official path still shows `ComQueue.QueueOverflow` on the S-band
  telemetry queue during file downlink, so this record does **not** claim the
  path is throughput-clean

## Fresh Evidence

Focused local build / UT:

- `./fprime-venv/bin/cmake --build build-fprime-automatic-native --target OBC ...`
  - verdict: `PASS`
- `./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut --target comm_groundlink_unit_test -j4`
  - verdict: `PASS`

Focused transport-only hosted proof:

- Command: `bash scripts/run_node5_comm_csp_downlink_v2_transport_hosted_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `status-max-gap-bytes=2866`
  - `status-max-queued-slots=12`
  - `sender-max-gap-bytes=2866`
  - byte-match: `PASS`
  - queue-gap observation: `PASS`
- Evidence root:
  - `/tmp/node5-comm-csp-downlink-v2-transport-hosted.QirW9W`

Full hosted official `.fdp` parity proof:

- Command: `bash scripts/run_node5_comm_csp_downlink_v2_hosted_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `node5-comm-csp-downlink-v2-hosted-probe: PASS`
  - `formal-verdict=node5-comm-csp-downlink-v2 hosted`
  - `downlink-v2-status=PASS connected=1 accepted=110592 flushed=110592 dropped=0`
  - `pipeline-gap=UNOBSERVED max-gap-bytes=0 max-queued-slots=0 samples=184`
  - `fdp-byte-match=PASS`
  - source / received `.fdp` SHA-256:
    `cab621bcdb00fa12e9fc41bfb12be0e8f3be669f9c331cf8b1cdfa8b466467e5`
- Evidence roots:
  - logs: `/tmp/node5-comm-csp-downlink-v2-hosted.NaTqzL`
  - stack: `/tmp/node5v2-NaTqzL`
- Baseline-owner correction observed on this passing run:
  - `groundLinkDriver.GROUND_LINK_UP` present
  - `uhfGroundLinkDriver.GROUND_LINK_UP` present
  - no repeating `CSP owner timeout node 6 port 30 timeout 1000 ms` pre-auth
    failure shape

Adjacent official hosted CCSDS S-band `.fdp` control proof:

- Command: `bash scripts/run_hk_data_product_alignment_v2_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `hk-data-product-alignment-v2-ccsds-sband-fdp-probe: PASS`
- Evidence root:
  - `/tmp/hk-data-product-alignment-v2.NaSuIM`

OpenSpec validation:

- `openspec validate node5-comm-csp-downlink-v2`
  - verdict: `PASS`
- `openspec validate --specs`
  - not rerun after this record-only update in this session

Governed target/lab proof:

- Command: `bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `payload-raw-preview-dual-artifact-v1-target: PASS`
  - secure-auth bootstrap: `PASS`
  - target payload capture flow: `PASS`
  - bounded `full` raw oversize diagnostic: `PASS` with
    `PRESULT_STORAGE_FAILED detail 24`
  - stock `BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)`: drained `11`
    official `.fdp` files to GDS
  - received preview-family decode: `PASS`
  - received raw-family decode: `PASS`
  - source/received `vga PREVIEW_JPEG` `.fdp` byte-match: `PASS`
  - source/received `vga RAW_FRAME` `.fdp` family byte-match: `PASS`
  - extracted preview JPEG hash: `PASS`
  - extracted raw artifact hash: `PASS`
  - repo-owned wrapper emitted final summary and cleanup records
- Evidence root:
  - `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.SMhdY0/case`
- Provenance correction on this passing rerun:
  - the earlier `2026-07-07` target failure root
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.xym94F`
    was collected before this branch was actually deployed to the live
    `subsystem-sband-csp.service` workspace binary and the installed
    `obc-comm-csp-stack.service` release
  - after refreshing live provenance, the same governed wrapper passed on the
    current branch, so the earlier short raw-family decode was an environment/
    stale-deployment false signal rather than a confirmed `node-5` downlink
    `v2` product defect

## Known Residuals

1. The hosted official path still records
   `ComCcsds.comQueue.QueueOverflow` on `COM_QUEUE (0) queue at index 2`.
   - `index 2` is the S-band telemetry queue, not the file queue.
   - This residual predates the `GroundLinkDriver` `1000 ms` default change and
     remains deferred in this change.
2. The hosted official proof now demonstrates functional parity, but it does
   not prove the path is free of telemetry backpressure noise under concurrent
   file downlink.
3. The earlier `2026-07-07` target false negative is now classified as a
   provenance error, not an open product defect.
   - the stale-deployment run on root `...xym94F` produced short received raw
     family members such as `550122 / 614400`
   - the refreshed live-branch rerun on root `...SMhdY0/case` completed the
     same governed official path with full byte-match and extract-hash `PASS`

## Outcome

This change is locally and hostedly proven enough to say:

- the regularized node-`5` COMM CSP downlink `v2` transport is implemented
- the stock official `.fdp` hosted path still works end-to-end in the current
  flight-software branch
- the governed target/lab node-`5` official payload `.fdp` path also works
  end-to-end on the current branch once live deployment provenance is refreshed
- the transport uplift remains below stock `FileDownlink` ownership

This record still does **not** claim the official path is throughput-clean,
because hosted S-band telemetry queue overflow noise remains deferred.
