## 1. OpenSpec And Protocol

- [x] 1.1 Add `node5-comm-csp-downlink-v3` proposal, design, tasks, and delta specs.
- [x] 1.2 Add `v3` control/data protocol definitions and repo-local libcsp sizing changes.
- [x] 1.3 Record the scoped node-`5`/`6`, port-`40` SocketCAN CAN FD target
  profile, its A-layer ownership, and the explicit BRS/data-phase non-claims.

## 2. Core Implementation

- [x] 2.1 Implement node-`5` `v3` receiver staging/commit/drain state and service handlers.
- [x] 2.2 Implement node-`5` backend `v3` probe, bulk send, resend-from-progress, and `v3 -> v1` fallback behavior.
- [x] 2.3 Extend ground-link stats and diagnostics for `v3` progress, resend, and queue-backed accounting.

## 3. Verification And Records

- [x] 3.1 Extend or replace COMM unit tests to cover `v3` probe, data/control split, resend, commit retry, and receiver idempotence.
- [x] 3.2 Add focused hosted `v3` transport proof tooling and rerun local verification.
- [x] 3.3 Complete governed target official payload `.fdp` proof and fold the final hosted/target results into docs/test-records/runbooks.
  - Current branch-local status as of 2026-07-09:
    - focused hosted `2048/2032` rerun remains requalified after the hosted-only `csp_zmqproxy` `1024`-byte capture-task fix
    - governed target secure-auth transport comparator now passes at `DOWNLINK_V3_MAX_DATA_BYTES=2032` with `COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE=1`; evidence root:
      `/tmp/target-secure-auth-v3-2032-window1-sendprobe-ownerfix`
    - governed target focused official HK comparator now passes in reuse mode with byte-match on an `8498`-byte source `.fdp`; evidence root:
      `/tmp/node5-v3-hk-target-reuse-r8`
    - later comparison established the actual carrier requirement: default
      `2032/window=3/no-CANFD` fails, `2032/window=1/no-CANFD` also fails, and
      default `2032/window=3` with scoped node-`5`/`6`, port-`40` CAN FD passes
    - branch head now assigns that maintained profile to target baseline A;
      official C verifies it and no longer installs or removes the shared
      CAN FD drop-in
    - focused hosted official payload rerun on 2026-07-12 passes in about
      `3m45s` after the isolated wrapper added the required node-`6` peer;
      START-to-catalog completion was `89.322s`, final GDS arrival `89.674s`,
      and node-`6` / FileDownlink timeouts were zero
  - The remaining open work is therefore narrower than the earlier `2032 target transport still fails` diagnosis. What remains is a fresh governed target payload official `.fdp` rerun on the A-owned profile and the final timing explanation above the now-working target V3 path.
  - Final closeout rerun on 2026-07-12:
    - installed release `v0.1.0-232-g4f6a6ad88`, F' `v4.1.0-7-g4dc6760`
    - A established the scoped COMM CAN FD profile once; the wrapper preflight
      and postflight both returned `READY/no-action-needed`
    - live target capture/publish and official payload `.fdp` proof `PASS`
    - `START_XMIT_CATALOG -> CatalogXmitCompleted = 204.929 s`
    - `START_XMIT_CATALOG -> final GDS arrival = 204.752 s`
    - node-`5` accepted/flushed `1892352/1892352`, queued/duplicate `0/0`
    - VGA preview and ten-slice raw families byte-match and extract-hash `PASS`
    - evidence root: `/private/tmp/node5-v3-pr1-official-target-final`
  - Historical debug evidence that motivated the comparators should remain recorded, but it is no longer the current branch-head truth:
    - adaptive-search rerun root:
      `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/node5-comm-csp-downlink-v3-target-search.sLKagv`
    - it captured non-prefix partial-ingress shapes such as node `5` accepting `frame-index=1` while OBC logged `frame-index=0/1/2 send-ok`
    - that evidence is now a debugging lesson and comparator ancestry, not the latest completion boundary
