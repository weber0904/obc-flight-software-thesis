# Node-5 COMM CSP Downlink V3

Status: `2048/2032` one-frame re-size implementation is rebuilt and documented
on 2026-07-09. Hosted is requalified: the focused `zmqhub` transport proof
passes after fixing the hosted-only `csp_zmqproxy` `1024`-byte capture limit,
and the maintained hosted official `.fdp` wrapper also passes with node-`5`
selecting `v3`. Target transport is now more sharply requalified as well:
the current governed secure-auth comparator proves that `2032` is not blocked
by V3 `window=3` itself, but by the carrier shape when SocketCAN stays on the
non-CAN-FD baseline. Specifically, branch-head evidence now shows:
- default target `2032` / `window=3` / no-CANFD: `FAIL`
- forced `window=1` / no-CANFD: `FAIL`
- default `window=3` plus scoped CAN FD profile for node-`5`/`6` COMM port
  `40`: `PASS`
and the governed target payload official `.fdp` wrapper now also passes in
artifact-reuse mode on that same CAN FD condition set, without re-capturing a
new photo. The current branch-head closeout focus is therefore no longer
"does target `2032` work at all?", but final fresh official payload timing and
closeout evidence.

The branch-head ownership is now explicit: scoped COMM CAN FD is a maintained
target A-layer profile, not a temporary C-owned testcase override. A applies
and verifies it on OBC/S-band/UHF; C only verifies the effective environment.
EPS/ADCS remain outside the profile. Since the libcsp SocketCAN path does not
set `CANFD_BRS`, this record proves CAN FD frame format and larger fragments,
not BRS or a measured 2 Mbit/s data phase.

One operational consequence of that branch-head truth is that the target
manual dual-GDS / Mission UI surface must call A and verify the same scoped
COMM CAN FD condition during `target-baseline-start`; it must not apply a
second manual-owned copy. Without the A-owned profile, manual S-band secure
auth can still time out waiting for `CHALLENGE` even though the underlying root
cause is the carrier requirement rather than a distinct Mission UI auth
regression.

## Scope

This record captures the regularized `node-5`-only COMM CSP downlink `v3`
transport uplift implemented by `node5-comm-csp-downlink-v3`.

It covers:

- repo-local simulator libcsp sizing for the `v3` hosted path:
  - `CSP_BUFFER_SIZE=2048`
  - `CSP_BUFFER_COUNT=24`
  - `CSP_QFIFO_LEN=32`
  - `CSP_CONN_RXQUEUE_LEN=32`
  - `CSP_CONN_MAX=16`
- stock file/downlink packet budget re-sized to fit one `v3` data frame:
  - `FW_FILE_BUFFER_MAX_SIZE=2032`
  - `DOWNLINK_V3_MAX_DATA_BYTES=2032`
- `DOWNLINK_CONTROL_V3` plus `DOWNLINK_DATA_V3` below the stock
  `DpCatalog -> CommController -> FileDownlink -> CommEgressMux ->
  GroundLinkDriver` ownership boundary
- node-`5` `stage -> commit -> drain` bytestream handling, `BEGIN/ACK_POLL/
  COMMIT/ABORT/STATUS` control behavior, and dedicated drain-worker decoupling
- OBC-side `v3` probe, data/control split, resend-from-progress, commit retry,
  and `v3 -> v1` fallback selection
- focused hosted transport proof that validates byte-match on
  `node1 backend -> node5 v3 -> external TCP sink` and demonstrates queue-backed
  accepted-before-flushed behavior through sender-side transport stats
- target-side adaptive transport comparator based on the maintained secure-auth
  command path, with probe-only `size/window/pacing` overrides for isolating
  `data-bytes/frame`, in-flight burst depth, and inter-frame pacing on the real
  SocketCAN path
- focused official target HK `.fdp` comparator that uses
  `HK_TREND_TARGET_FILE_BYTES=10240` and rejects runs whose generated `.fdp`
  stays below `8192` bytes so the debug sample still crosses multiple stock file
  packets and multiple `4096`-byte TM send units
- full hosted official `.fdp` parity through
  `ground_ttc_gateway -> fprime-gds`, including node-`5` `v3` selection and
  transport-vs-GDS timing split evidence
- governed target official payload `.fdp` parity through the maintained target
  `A -> B -> C` path remains the open closeout item; this record currently
  carries the sharper transport diagnosis and the new target comparators used to
  finish that work, not a final requalification claim

It does **not** cover:

- `node-6` migration, UHF reliable-transfer widening, or ground-side CSP
  participation
- any upgrade of the stock path into end-to-end reliable delivery

## Commands

Focused local build / UT:

```bash
./fprime-venv/bin/cmake -S . -B build-fprime-automatic-native -DBUILD_TESTING=ON
./fprime-venv/bin/cmake --build build-fprime-automatic-native --target sband_comm_csp_node comm_downlink_v3_sender comm_downlink_v3_status_probe comm_groundlink_unit_test -j4
./build-fprime-automatic-native/bin/Darwin/comm_groundlink_unit_test
```

Focused hosted proof:

```bash
bash scripts/run_node5_comm_csp_downlink_v3_transport_hosted_probe.sh
```

Full hosted official `.fdp` proof:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh
```

Governed target official `.fdp` proof:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
```

Target adaptive transport comparator:

```bash
bash scripts/run_node5_comm_csp_downlink_v3_target_adaptive_search.sh
```

Focused target official HK comparator:

```bash
bash scripts/run_node5_comm_csp_downlink_v3_hk_target_probe.sh
```

OpenSpec validation:

```bash
openspec validate node5-comm-csp-downlink-v3
```

## Fresh Evidence

Focused local build / UT:

- `./fprime-venv/bin/cmake --build build-fprime-automatic-native ...`
  - verdict: `PASS`
- `./build-fprime-automatic-native/bin/Darwin/comm_groundlink_unit_test`
  - verdict: `PASS`
  - coverage note:
    - includes branch-local checks that `COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES`
      shrinks frame size as expected
    - includes branch-local checks that `CSP_TRANSPORT=socketcan` caps the
      default in-flight window to `3`
    - includes branch-local checks that
      `COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE=1` really forces
      single-frame in-flight behavior for the target comparator

Focused hosted transport proof:

- Command: `bash scripts/run_node5_comm_csp_downlink_v3_transport_hosted_probe.sh`
- Verdict: `PASS` on the current `2048/2032` re-size rerun
- Observed result:
  - focused hosted transport reruns now pass for both the stock-file-packet
    equivalent `2019`-byte framing and the new one-frame `2032`-byte send
    buffer envelope
  - the previous hosted blocker was traced outside the V3 protocol state
    machine to `lib/libcsp/examples/zmqproxy.c`, where the proxy capture task
    used fixed `1024`-byte packet/message buffers and corrupted memory on
    larger hosted CSP frames
  - after correcting the proxy capture path to allocate a full
    `csp_packet_t` and receive dynamic ZMQ message sizes, a generic CSP smoke
    (`requestReply(PING) -> sendRaw(2019) -> requestReply(PING)`) now passes,
    and the node-`5` V3 transport proof passes at both `2019` and `2032`
  - focused hosted V3 proof with `SEND_CHUNK_BYTES=2019`:
    `PASS`, byte-match `PASS`, sender max gap `2019`
  - focused hosted V3 proof with `SEND_CHUNK_BYTES=2032`:
    `PASS`, byte-match `PASS`, sender max gap `2032`
- Evidence root:
  - generic CSP smoke after proxy fix:
    local command `csp_runtime_smoke --raw-payload-bytes 2019` with hosted
    `csp_zmqproxy + csp_service_peer`
  - focused hosted rerun `2019`:
    `/tmp/node5-v3-after-zmqproxy-2019`
  - focused hosted rerun `2032`:
    `/tmp/node5-v3-after-zmqproxy-2032`

Full hosted official `.fdp` proof:

- Command: `bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh`
- Verdict: `PASS` for the current `2048/2032` re-size slice
- Observed result:
  - hosted official wrapper now completes with `node5-transport=v3`
  - observed timings on the current `2032` envelope:
    `START_XMIT_CATALOG -> CatalogXmitCompleted = 11.638 s`,
    `START_XMIT_CATALOG -> final GDS arrival = 15.028 s`,
    `CatalogXmitCompleted -> final GDS arrival = 3.389 s`
  - hosted logs confirm the expected one-frame-per-stock-buffer shape at the
    V3 layer: repeated `byte-count=2032`, `total-bytes=4096`, `frame-count=3`
- Evidence root:
  - hosted wrapper rerun:
    `/tmp/payload-raw-preview-dual-artifact-v1-hosted.v3-resize-preview`

Governed target official `.fdp` proof:

- Command: `bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
- Verdict:
  - earlier stale-deploy / no-CANFD ancestry remains archived as diagnosis
  - current governed rerun in archived-artifact reuse mode is `PASS` on the
    scoped CAN FD condition set
  - final 2026-07-12 fresh live-capture closeout rerun is `PASS` on installed
    release `v0.1.0-232-g4f6a6ad88` with F' `v4.1.0-7-g4dc6760`
- Final closeout evidence:
  - command: `PROBE_ROOT=/tmp/node5-v3-pr1-official-target-final bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
  - evidence root: `/private/tmp/node5-v3-pr1-official-target-final`
  - source provisioning: `live-capture-and-publish`
  - wrapper preflight A/B: `READY/no-action-needed`
  - C verified `owner=target-baseline-a` for the scoped CAN FD profile and did
    not install or remove the shared profile
  - postflight A/B: `READY/no-action-needed`
  - `START_XMIT_CATALOG -> CatalogXmitCompleted = 204.929 s`
  - `START_XMIT_CATALOG -> final GDS arrival = 204.752 s`
  - final GDS arrival preceded the journal completion observation by `0.177 s`
  - node-`5` accepted/flushed `1892352/1892352`, queued frames `0`, duplicate
    frames `0`
  - VGA preview family: `60688` bytes, byte-match and extract hash `PASS`
  - VGA raw family: `618090` bytes across `10` slices, byte-match and extract
    hash `PASS`
  - full-resolution raw remained bounded-deferred with
    `PRESULT_STORAGE_FAILED detail 24`, as required
- Observed result:
  - the first rerun attempt on 2026-07-08 was invalidated because
    `$OBC_HOME/obc-deploy/current` still pointed at a stale installed
    release; target GDS/native logs still showed stock `2035`-byte file-data
    packets, proving the new sizing was not deployed
  - after packaging the fresh remote workspace build and reinstalling
    `$OBC_HOME/obc-deploy/current`, the governed `A -> B -> C` reruns
    reached the maintained target path with both node `5` and node `6`
    baseline services active, secure-auth provenance gate `PASS`, and the new
    target binary active
  - the first post-install transport diagnosis showed a real target failure:
    the maintained target packet/auth path still entered
    `GroundLinkDriver.send()` as `4096`-byte streams, and node-`5` sometimes
    accepted only a trailing or non-prefix frame, leaving `ACK_POLL` stuck at
    `contiguousFrames=0`
  - that failure is no longer the branch head conclusion; it has now been
    narrowed by two governed target reruns:
    - the secure-auth transport comparator now shows the current target
      requirement is carrier-shape dependent rather than simply
      `window`-dependent:
      default `2032/window=3/no-CANFD` fails, `2032/window=1/no-CANFD` also
      fails, and default `2032/window=3` passes only after adding the scoped
      node-`5`/`6` COMM CAN FD override
    - the focused official HK comparator passes on governed target hardware
      with byte-match on an `8498`-byte `.fdp`, proving the stock
      `DpCatalog -> FileDownlink -> ComCcsds -> GroundLinkDriver` path can
      complete through node-`5` `v3` below the slower payload proof
  - later isolation on the maintained secure-auth comparator tightened the
    actual target requirement:
    - default `2032/window=3/no-CANFD`: `FAIL`
    - forced `window=1/no-CANFD`: `FAIL`
    - default `2032/window=3` with scoped node-`5`/`6` COMM CAN FD override:
      `PASS`
  - this means the latest branch-head target root cause is not "window must be
    forced to 1", but "current `2032` official target path needs the CAN FD
    carrier shape on the governed SocketCAN route"
  - using the archived route1 target payload summary as source input, the
    maintained target wrapper now passes without re-capturing payload:
    `payload-raw-preview-dual-artifact-v1-target=PASS`
  - that rerun confirms:
    - `source-provisioning-mode=reuse-existing-summary`
    - `node5-transport=v3`
    - preview `.fdp` family byte-match `PASS`
    - raw `.fdp` family byte-match `PASS`
    - preview artifact hash `PASS`
    - raw artifact hash `PASS`
    - `START_XMIT_CATALOG -> CatalogXmitCompleted = 270.567 s`
    - `START_XMIT_CATALOG -> final GDS arrival = 277.798 s`
    - `CatalogXmitCompleted -> final GDS arrival = 7.231 s`
  - reuse-mode verification also exposed one bounded tooling compatibility gap:
    current `payload_fdp_extract.py` / `fprime-dp-write` does not reliably
    re-decode this archived payload `.fdp` family on the current branch, so the
    wrapper now falls back to archived `sourceFamilySummary` plus family-level
    SHA-256 equality for historical-source verification instead of falsely
    declaring the current target downlink broken
- Evidence root:
  - stale-deployment invalidated attempt:
    `/tmp/payload-raw-preview-dual-artifact-v1-target.v3-resize`
  - current governed rerun:
    `/tmp/payload-raw-preview-dual-artifact-v1-target.v3-resize-rerun`
  - latest governed rerun with explicit A/B baseline plus fresh `v3.5.0`
    install:
    `/tmp/payload-raw-preview-dual-artifact-v1-target.v3-sendunit-2032-final`
  - target secure-auth default failure on current no-CANFD baseline:
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.QqDAZL`
  - target secure-auth `2032/window=1/no-CANFD` failure:
    `/tmp/target-secure-auth-v3-2032-window1-no-canfd`
  - target secure-auth scoped CAN FD pass:
    `/tmp/target-secure-auth-v3-2032-window3-canfd`
  - focused official HK target comparator pass:
    `/tmp/node5-v3-hk-target-reuse-r8`

Current target debug comparators:

- `bash scripts/run_node5_comm_csp_downlink_v3_target_adaptive_search.sh`
  - purpose:
    - reuse the faster maintained target secure-auth command path to classify
      each candidate as `pass-window3`, `burst-limited-window1-pass`,
      `burst-limited-pacing-pass`, or `size-or-envelope-fail`
    - keep the current target anchors (`240` pass, `1000` fail, `2032` fail)
      while searching by aligned midpoint instead of a blind ladder
  - current behavior:
    - drives `COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES`,
      `COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE`, and
      `COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC`
    - current branch-local debug extension also supports
      `COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC` because the existing V3
      inter-frame delay only spaces whole CSP packets; it does not slow the
      back-to-back CAN fragments emitted inside one large `sendRaw()` packet
    - important phase boundary:
      this adaptive-search tool is the earlier pre-CANFD-override comparator.
      It does **not** set `COMM_CSP_SOCKETCAN_USE_CANFD`; the historical
      `240/416/1000/2032` interval therefore belongs to the original target
      slowdown / `2032`-convergence phase, not to the later recovery/proof
      phase that introduced probe-owned CAN FD override handling
    - current branch-local summaries now also retain node-`5`
      `downlink-v3-data-drop packet-length=<n>` observations so malformed
      packet shapes can be compared directly across reruns instead of only
      looking at final `PASS/FAIL`
    - writes a per-candidate summary under its own probe root so OBC/node-`5`/
      ground observations can be compared side by side
  - current claim:
    - tool is in-tree and locally syntax-checked
    - the earlier 2026-07-08 rerun tightened the working interval from the
      coarse anchors `240 pass / 1000 fail / 2032 fail` down to
      at least `240 pass / 416 fail`, and captured the sharper non-prefix
      partial-ingress shape on real SocketCAN
    - those early failures must not be retrospectively attributed to the later
      probe-owned CAN FD override owner bug; that bug belongs to a later
      secure-auth / HK recovery phase and did not exist in this original
      adaptive-search setup
    - the current branch head conclusion after the later governed reruns is
      stronger than that interval alone:
      `2032/window=3/no-CANFD` fails, `2032/window=1/no-CANFD` also fails, and
      the maintained secure-auth transport comparator passes only after adding
      the scoped node-`5`/`6` COMM CAN FD override
    - this means the earlier `2032 fail` result was not the final product
      truth; it was a debug-stage observation taken before the current deploy,
      service-owner, carrier-shape, and cleanup corrections were in place
    - the comparator remains useful for future `window/pacing` diagnosis, but
      it is no longer evidence that target `2032` transport is intrinsically
      impossible
  - evidence root:
    - adaptive search rerun:
      `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/node5-comm-csp-downlink-v3-target-search.sLKagv`
    - default secure-auth comparator failure:
      `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.QqDAZL`
    - secure-auth `window=1` / no-CANFD failure:
      `/tmp/target-secure-auth-v3-2032-window1-no-canfd`
    - secure-auth scoped CAN FD pass:
      `/tmp/target-secure-auth-v3-2032-window3-canfd`
- `bash scripts/run_node5_comm_csp_downlink_v3_hk_target_probe.sh`
  - purpose:
    - validate the stock official file path with a non-trivial `.fdp` sample
      before returning to the slower payload preview/raw family
    - force `HK_TREND_TARGET_FILE_BYTES=10240`, then reject the run unless the
      generated `.fdp` reaches at least `8192` bytes
  - current behavior:
    - keeps the maintained target `A -> B -> C` split
    - records `status-before-xmit` / `status-after-match` and requires
      `acceptedBytes` growth across the V3 status probe
  - current claim:
    - tool is in-tree and locally syntax-checked
    - the current governed rerun passes in reuse mode on a non-trivial official
      source file:
      `source-size-bytes=8498`, `received-size-bytes=8498`,
      `hk-official-byte-match=PASS`, `node5-transport=v3`
    - status snapshots also show V3 progress growth across the official file
      transfer:
      `statusBeforeXmit.acceptedBytes=28672`,
      `statusAfterMatch.acceptedBytes=94208`
    - this comparator is now an exercised target proof, not just an unrun next
      step
  - evidence root:
  - focused official HK target pass:
    `/tmp/node5-v3-hk-target-reuse-r8`
  - default secure-auth comparator failure on current target baseline:
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.QqDAZL`
  - forced `window=1` without CAN FD:
    `/tmp/target-secure-auth-v3-2032-window1-no-canfd`
  - default `window=3` with scoped CAN FD:
    `/tmp/target-secure-auth-v3-2032-window3-canfd`
  - governed payload official target reuse pass:
    `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3`

## Errors Encountered And Handling

Time ordering matters in this section:

- the original `Node-5 COMM_CSP V3 Target Slowdown / 2032 收斂重訂計劃`
  phase used the adaptive-search comparator above and was **pre-CANFD-override**
- the later secure-auth / HK recovery phase introduced optional
  `COMM_CSP_SOCKETCAN_USE_CANFD` probe-owned override handling
- therefore the later `CAN FD override owner` bug must not be used to explain
  the original `240/416/1000/2032` adaptive-search failures

### 1. Hosted large-frame proof was invalidated by a hidden `csp_zmqproxy` `1024`-byte limit

- Symptom:
  - hosted `2032`-byte V3 transport looked flaky or crashed even though the V3
    protocol logic itself appeared correct
- Actual cause:
  - `lib/libcsp/examples/zmqproxy.c` hard-coded `malloc(1024)` and
    `zmq_msg_init_size(..., 1024)` in its capture path, so larger hosted CSP
    frames corrupted memory
- Handling:
  - replace the fixed `1024` capture assumption with a bounded dynamic receive
    path sized against full `csp_packet_t`
  - re-run the generic CSP smoke and the focused hosted V3 proof at both
    `2019` and `2032`
- Rule going forward:
  - do not treat a hosted-only transport crash as protocol failure before
    checking repo-local proxy/capture helpers for stale magic limits

### 2. The first target `2032` failure was polluted by stale deployment, not pure transport truth

- Symptom:
  - target logs still showed stock `2035` file-data payload behavior after the
    branch had already been re-sized to `2032`
- Actual cause:
  - `$OBC_HOME/obc-deploy/current` still pointed to a stale installed
    release, so the new branch binary was not actually running on target
- Handling:
  - rebuild, package, reinstall the fresh branch bundle, and verify the current
    install pointer before trusting any target transport conclusion
- Rule going forward:
  - if target evidence still looks like old sizing or old behavior, check the
    installed release pointer and service provenance before changing product
    logic

### 3. Probe-owned CAN FD override was initially applied too broadly

- Symptom:
  - target transport reruns produced inconsistent behavior and unnecessary
    interference outside the actual S-band / UHF COMM services under test
- Phase boundary:
  - this is a later recovery/proof issue, not part of the original
    pre-CANFD-override adaptive-search phase
- Actual cause:
  - the probe-owned SocketCAN override was being applied to unrelated services
    such as EPS / ADCS, not just the COMM path that owns this proof
- Handling:
  - narrow the override owner to OBC COMM plus
    `subsystem-sband-csp.service` and `subsystem-uhf-csp.service`
- Rule going forward:
  - probe-owned transport overrides must only touch the services under test;
    otherwise the probe perturbs shared baseline behavior and invalidates its
    own conclusions

### 4. Partial subsystem rebuild was not sufficient for the governed service surface

- Symptom:
  - target reruns still looked like old node-`5` behavior even after source
    edits were present in the workspace
- Actual cause:
  - the governed subsystem service surface was not fully refreshed by a narrow
    partial rebuild / install flow
- Handling:
  - rebuild and deploy the full required native binary surface for the governed
    subsystem services, not only the most obvious one edited file target
- Rule going forward:
  - when target service behavior disagrees with workspace source, verify the
    actual deployed binary set, not just the edited code tree

### 5. HK official comparator delay was initially misread as transport slowness

- Symptom:
  - the HK target proof looked unreasonably slow before any downlink even
    started
- Actual cause:
  - `HkTrendProductProducer` uses `DEFAULT_PERIOD_TICKS = 30` with a `1 Hz`
    data-group cadence, so waiting for a fresh `>= 8192` byte official `.fdp`
    can take many minutes
- Handling:
  - add reuse mode and require a non-trivial existing source file instead of
    regenerating from scratch every run
- Rule going forward:
  - separate "source product generation cadence" from "downlink cadence" before
    concluding the transport is slow

### 6. HK reuse mode needed catalog-state cleanup and a narrower oracle

- Symptom:
  - reuse-mode HK runs could end with `CatalogXmitCompleted ... 0 bytes transmitted`
    or choose the wrong remote `.fdp`
- Actual cause:
  - stale alternate source files and retained `DpState.dat` polluted the
    catalog state, and `SendingProduct` journal text was too strong as the only
    `NO_WAIT` source-selection oracle
- Handling:
  - list existing candidates, choose the largest eligible source, prune remote
    `data-products/` to that retained file, remove `DpState.dat`, and rely on
    later byte-match plus V3 status growth instead of forcing `SendingProduct`
    journal text on the reuse path
- Rule going forward:
  - when reusing official `.fdp` inputs, clear stale catalog state and control
    the remote source pool; otherwise `DpCatalog` can report valid completion on
    the wrong file set or transmit nothing

### 7. Cleanup must be proven by rerunning the A-layer baseline

- Symptom:
  - proof-owned `57-csp-socketcan-canfd.conf` overrides could be left behind and
    silently contaminate the next target run
- Actual cause:
  - cleanup ownership for the probe-only override was incomplete
- Handling:
  - add explicit proof cleanup and then verify it by rerunning
    `bash scripts/ensure_target_comm_lab_baseline.sh` until it returns
    `target-comm-lab-baseline: READY` with `no-action-needed=yes`
- Rule going forward:
  - probe cleanup is not a claim until the A-layer baseline says the shared lab
    is already clean without needing another repair pass

### 8. Historical payload `.fdp` reuse must not depend on current re-decode of archived source families

- Symptom:
  - the target payload wrapper reached secure-auth and official downlink, but
    reuse-mode verification still failed while trying to decode the archived
    route1 source `.fdp` family with the current branch extractor
- Actual cause:
  - current `payload_fdp_extract.py` / `fprime-dp-write` does not reliably
    re-decode this historical payload family, even though the archived target
    summary already contains successful source-family hashes and extracted
    artifact metadata from the original passing run
- Handling:
  - add `PAYLOAD_TARGET_REUSE_SUMMARY_JSON` so the wrapper can stage the
    archived source `.fdp` family back into the target runtime, reuse the
    archived `sourceFamilySummary`, and fall back to family-level SHA-256
    equality when current re-decode of the historical source family is not a
    stable oracle
- Rule going forward:
  - when reusing historical payload `.fdp` evidence, do not let current decode
    tooling incompatibility falsely fail a current target downlink rerun if the
    archived family metadata and family-level hashes already close the claim

### 9. `FileDownlink.Run` on the slow rate group created a fake transport slowdown

- Symptom:
  - after target `2032` V3 plus scoped COMM CAN FD had already passed, the
    official payload reuse path still needed about `270 s` for
    `START_XMIT_CATALOG -> CatalogXmitCompleted`
- Actual cause:
  - this was not all V3 transport time
  - `DpCatalog` emitted `SendingProduct`, but `FileHandling.fileDownlink.Run`
    was still scheduled on the slow rate group while
    `FileHandlingConfig::DownlinkConfig` still assumed `cycleTime=1000` and
    `cooldown=1000`
  - with `rateGroup2` divisor `5` and `1 Hz` base tick, the effective
    dequeue/cooldown cadence stretched to about `5 s` per `Run_handler`, which
    inflated `SendingProduct -> SendStarted` to roughly `10..14 s` per raw
    `.fdp` slice
- Handling:
  - move `FileHandling.fileDownlink.Run` from `rateGroup2Comp` to
    `rateGroup3Comp` so the actual run cadence matches the intended `1 Hz`
    file-downlink contract
  - repackage, reinstall, and rerun the governed target archived-artifact
    payload proof on the same scoped COMM CAN FD condition set
- Verified result:
  - current rerun root:
    `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-runfix`
  - wrapper still passes in reuse mode with `node5-transport=v3`
  - `START_XMIT_CATALOG -> CatalogXmitCompleted` drops from `270.567 s` to
    `176.091 s`
  - `CatalogXmitCompleted -> final GDS arrival` drops from `7.231 s` to
    `0.214 s`
  - raw-slice `SendingProduct -> SendStarted` drops from about `10.8..14.1 s`
    to about `2.0..3.0 s`
  - raw-slice `SendStarted -> FileSent` remains about `8.9..21.7 s`, which
    means the remaining dominant cost is now above V3, in the official
    CCSDS/TM synchronous send cadence
- Rule going forward:
  - when official file/payload downlink looks slow, measure
    `SendingProduct -> SendStarted` separately from `SendStarted -> FileSent`
    before blaming the transport
  - a rate-group/cycle-time mismatch can produce minutes of fake slowdown even
    after the lower transport is already fixed

## Current Boundary

This record supports the narrow claim that:

- the `2048/24` libcsp pool re-size, `FW_FILE_BUFFER_MAX_SIZE=2032`, and
  `DOWNLINK_V3_MAX_DATA_BYTES=2032` implementation are in the tree
- local rebuild and COMM unit coverage are refreshed against the new sizing
- the hosted `csp_zmqproxy` path no longer imposes the hidden `1024`-byte
  capture-task limit that previously invalidated large-frame hosted CSP proofs
- the focused hosted `node1 -> node5 v3 -> external sink` transport proof is
  requalified at both `2019` and `2032` send-buffer sizing
- the maintained target secure-auth transport comparator now proves the current
  target runtime requirement is not merely `window=1`, but scoped COMM CAN FD:
  default `2032/window=3/no-CANFD` fails, `window=1/no-CANFD` fails, and
  `window=3` with scoped node-`5`/`6` COMM CAN FD override passes
- the focused official HK target comparator now passes on a reused `8498`-byte
  source `.fdp` with byte-match and `node5-transport=v3`
- the governed target payload official `.fdp` wrapper now passes in
  archived-artifact reuse mode on that same scoped CAN FD condition set, with
  preview/raw family byte-match and artifact-hash closure
- the current governed target payload reuse rerun with `FileDownlink.Run`
  restored to a `1 Hz` data-group cadence further reduces
  `START_XMIT_CATALOG -> CatalogXmitCompleted` from `270.567 s` to `176.091 s`
  on the same official path
- the maintained stock ownership boundary remains intact

This record does **not** yet claim that:

- the current payload target path timing problem is fully eliminated
- the official target path above V3 no longer bottlenecks on the CCSDS/TM
  `4096`-byte synchronous send cadence
- the current transport fixes automatically improve every payload proof timing
  without further upper-layer analysis
