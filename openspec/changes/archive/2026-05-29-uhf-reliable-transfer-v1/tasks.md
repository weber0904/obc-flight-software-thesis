## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `comm-subsystem`, `hk-data-products`, `interface-contract-index`,
  `ground-ttc-gateway`, `verification-evidence`, and
  `verification-path-registry`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate uhf-reliable-transfer-v1`.

## 2. Runtime And Tests

- [x] 2.1 Extend `CommController` admission so the bounded helper may run on:
  - default S-band node-`5`
  - explicit-switched `uhf-primary-after-failover` node-`6`
- [x] 2.2 Keep `uhf-backup` out of reliable-transfer scope and keep automatic
  failover-to-UHF outside the new claim.
- [x] 2.3 Make target-node selection explicit and keep in-flight transfer
  contexts non-migrating across switch or failover.
- [x] 2.4 Preserve the current UHF launch quiesce and reuse helper resend
  semantics unchanged.
- [x] 2.5 Add focused CommController UT coverage for switched-UHF admission,
  explicit bounded fallback, and UHF-originated abort-on-role-change behavior.

## 3. Hosted Proof

- [x] 3.1 Add a dedicated hosted switched-UHF reliable-transfer wrapper that
  starts the current two-stack shared-runtime baseline and enables the RT
  receiver on node `6` only.
- [x] 3.2 Prove hosted happy-path switched-UHF reliable transfer.
- [x] 3.3 Prove one hosted resend-before-success case.
- [x] 3.4 Prove one hosted retry-exhausted case with no final artifact
  promotion.

## 4. Target/Lab Proof

- [x] 4.1 Extend the governed target CAN matrix helper so `reliable-transfer`
  works on both:
  - default node-`5`
  - quiet switched node-`6`
- [x] 4.2 Add a dedicated target/lab wrapper for the quiet switched
  `uhf-primary-after-failover` happy path.
- [x] 4.3 Keep quiet mode probe-owned, remove RT proof overrides after the run,
  and restore the services to the normal non-quiet baseline.

## 5. Specs, Registry, And Docs

- [x] 5.1 Sync the six main specs so they no longer conflict with the new UHF
  slice.
- [x] 5.2 Add a dedicated evidence record for `uhf-reliable-transfer-v1`.
- [x] 5.3 Update `evidence/verification-path-registry.md` with distinct hosted and
  target/lab UHF reliable-transfer boundaries adjacent to existing node-`6`
  entries.
- [x] 5.4 Update current docs and runbooks so the active COMM queue names the
  bounded UHF reliable-transfer slice explicitly and keeps RF / quiet-rescue /
  broad simultaneous work out of the practical queue.

## 6. Verification

- [x] 6.1 Run shell and Python syntax checks for touched wrappers/helpers.
- [x] 6.2 Run focused CommController / reliable-transfer unit tests.
- [x] 6.3 Run `bash scripts/run_comm_uhf_reliable_transfer_hosted_probe.sh`.
- [x] 6.4 Run `bash scripts/run_comm_csp_socketcan_uhf_reliable_transfer_probe.sh`.
- [x] 6.5 Run `openspec validate uhf-reliable-transfer-v1` and
  `openspec validate --specs`.

## Closure State

- Runtime admission, explicit node selection, non-migrating abort behavior,
  focused UT coverage, hosted proof wrappers, and target helper/wrapper logic
  are implemented on this branch.
- Hosted switched-UHF proof is complete through the maintained per-band
  launcher:
  - happy path passed
  - resend-before-success passed
  - retry-exhausted without final artifact promotion passed
- Target/lab quiet switched node-`6` proof passed on the exact governed path
  and recorded quiet/proof-override cleanup plus restoration to the normal
  non-quiet baseline.
- The six main specs, current docs, and verification-path registry are synced.
- The change is ready for formal archive.
