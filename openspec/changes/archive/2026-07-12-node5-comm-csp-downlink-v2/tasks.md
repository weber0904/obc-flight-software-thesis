## 1. OpenSpec And Protocol

- [x] 1.1 Finalize change artifacts for node-`5` COMM CSP downlink `v2`.
- [x] 1.2 Add node-`5` `v2` protocol/service definitions and bounded result/status structures.

## 2. Core Implementation

- [x] 2.1 Implement node-`5` staging, commit, abort, timeout, and drain worker behavior in `CommNodeServer`.
- [x] 2.2 Implement node-`5` `v2` probe, fallback, and stream send behavior in `CommCspGroundLinkBackend`.
- [x] 2.3 Extend COMM diagnostics and stats with accepted/flushed/dropped queue-backed accounting.

## 3. Verification

- [x] 3.1 Add or refresh unit tests for backend probe/send fallback and node-`5` `v2` server semantics.
- [x] 3.2 Update hosted proof/documentation surfaces for the node-`5` transport uplift and rerun local verification.
- [x] 3.3 Run the feasible governed target/lab proof path or record the exact remaining execution gap if target proof cannot be completed in-session.

Target proof closeout notes:
- The focused local transport-only hosted proof now passes via
  `bash scripts/run_node5_comm_csp_downlink_v2_transport_hosted_probe.sh`,
  including byte-match and observable accepted-before-flushed queue gap under a
  probe-owned node-`5` drain-delay hook.
- The full hosted CCSDS S-band official `.fdp` parity rerun through
  `ground_ttc_gateway` and `fprime-gds` now passes on the current secure-auth
  branch-local probe once the hosted observation window is long enough to cover
  the stock official large-`.fdp` cadence. That official proof boundary should
  gate on byte-match/decode and `DOWNLINK_STATUS_V2` availability, not on
  branch-local queue-gap observation; queue-gap evidence remains owned by the
  separate transport-only hosted proof.
- The maintained branch-local hosted official `.fdp` proof now starts the
  bounded UHF/node-`6` surface as part of its baseline owner preflight, because
  the current hosted secure-auth runtime can fail early when the proof narrows
  itself to node-`5` only while `node-6`-dependent background transport/auth
  behavior is still active in the shared hosted stack.
- The larger hosted path still exhibits `GROUND_LINK_DOWN/UP` churn plus
  `ComQueue.QueueOverflow` while the transfer is in flight, so this session
  does not yet claim that the current official path is throughput-clean or free
  of backpressure noise.
- The governed target/lab wrapper now passes on the current live branch after
  provenance refresh:
  `bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
  -> `payload-raw-preview-dual-artifact-v1-target: PASS`
- Passing probe root:
  `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.SMhdY0/case`
- Passing target facts:
  - secure auth: `PASS`
  - target payload capture flow: `PASS`
  - bounded `full` raw oversize diagnostic: `PASS` with
    `PRESULT_STORAGE_FAILED detail 24`
  - official `BUILD_CATALOG + START_XMIT_CATALOG(NO_WAIT)` drained `11`
    `.fdp` files to GDS
  - `vga PREVIEW_JPEG` `.fdp` byte-match: `PASS`
  - `vga RAW_FRAME` `.fdp` family byte-match: `PASS`
  - preview extract hash: `PASS`
  - raw extract hash: `PASS`
- Provenance correction:
  - the earlier `2026-07-07` failure root
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/payload-raw-preview-dual-artifact-v1-target.xym94F`
    was collected before this branch was actually deployed to the live
    subsystem node-`5` binary and installed OBC release
  - after rsyncing the workspace, rebuilding both remote hosts, repackaging
    the OBC release, reinstalling `obc-deploy/current`, and rerunning the
    governed `A -> B -> C` flow, the same official target wrapper passed
  - the earlier short raw-family decode is therefore treated as a stale-
    deployment/provenance false signal rather than an open `node-5` downlink
    `v2` product defect
