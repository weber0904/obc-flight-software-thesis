## Why

The current repository already proves one bounded reliable-transfer slice for
the current official HK `.fdp` family, but that proof is still limited to the
default S-band node-`5` path. The maintained UHF formal path now has enough
frozen transport, quiet, and file/downlink truth to support one more narrow
claim without reopening broader COMM design questions.

This change is needed now because the remaining practical no-RF COMM gap is no
longer "generic UHF support"; it is one exact missing bounded slice:
`uhf-primary-after-failover` reliable transfer for the current official HK
`.fdp` family after explicit switch from the default node-`5` baseline.

## What Changes

- Extend the bounded reliable-transfer contract from:
  - default S-band node-`5` official HK `.fdp` whole-file only
- To:
  - default S-band node-`5` official HK `.fdp` whole-file only
  - explicit-switched `uhf-primary-after-failover` node-`6` official HK `.fdp`
    whole-file only
- Keep `CommController` as the admission/policy owner and `DpCatalog` as the
  file-selection owner.
- Keep `ground_ttc_gateway` as raw relay only.
- Reuse the current helper transfer semantics unchanged:
  - `160`-byte RT `DATA` segments
  - window `2`
  - ACK timeout `1` interval
  - resend budget `3`
  - bounded `64` segments / `10,240` bytes
- Keep the UHF path bounded:
  - only `uhf-primary-after-failover`
  - only after explicit switch
  - no `uhf-backup` reliable transfer
  - no RF
  - no restart-persistent resume
  - no broad CFDP
  - no generic arbitrary-file authority redesign
- Add fresh hosted proof for:
  - happy path
  - resend-before-success
  - retry-exhausted without final artifact promotion
- Add fresh target/lab quiet-path proof for:
  - exact switched node-`6` happy path
  - probe-owned quiet override and post-proof restore to normal non-quiet
    service baseline

## Capabilities

### Modified Capabilities

- `comm-subsystem`: add the first bounded UHF reliable-transfer slice on the
  explicit-switched `uhf-primary-after-failover` node-`6` path.
- `hk-data-products`: allow the current official HK `.fdp` whole-file family
  to use the bounded reliable sidecar on either current exact allowed path.
- `interface-contract-index`: keep the helper `160`-byte RT segment ceiling
  path-local while distinguishing it from the stock UHF `243`-byte
  `Fw::FilePacket::DATA` ceiling.
- `ground-ttc-gateway`: keep the raw-relay owner boundary explicit for both
  S-band and UHF reliable-transfer proofs.
- `verification-evidence`: require hosted happy/resend/retry-exhausted proof
  plus target/lab quiet switched node-`6` happy-path proof with explicit quiet
  restore wording.
- `verification-path-registry`: register reliable-transfer boundaries adjacent
  to existing hosted node-`6` and target quiet node-`6` paths instead of
  widening those paths by implication.

## Impact

- Affected code:
  - `CommController` reliable-transfer admission and launch path
  - CommController unit tests for explicit-switched UHF admission, explicit
    non-migration, and bounded fallback
  - hosted UHF reliable-transfer proof wrapper
  - target/lab quiet switched node-`6` reliable-transfer proof wrapper
- Affected docs/specs:
  - OpenSpec change artifacts plus six delta specs listed above
  - main specs and current docs that still describe reliable transfer as
    default S-band node-`5` only
- Affected operations:
  - the repo gains one additional bounded UHF reliable-transfer slice
  - this change does not claim `uhf-backup` reliable transfer, non-quiet
    target UHF promotion, one-GDS aggregation, one-gateway multiplexing, RF,
    restart-persistent resume, or broad CFDP adoption

## Current Branch Status

- Runtime admission/path-binding changes and focused unit coverage are landed
  on this branch.
- Hosted switched-UHF reliable-transfer evidence is now honest through the
  maintained per-band launcher:
  - happy path passed
  - resend-before-success passed
  - retry exhausted without final artifact promotion passed
- Target/lab quiet switched node-`6` reliable-transfer evidence also passed on
  the exact governed path, with probe-owned quiet mode and proof-only
  overrides removed before exit.
- The six main specs and current docs are now synchronized with the landed
  scope.
- This change closes the remaining practical COMM capability gap for the
  repository's current no-RF simulation scope while keeping all broader
  non-claims explicit, so formal archive is justified.
