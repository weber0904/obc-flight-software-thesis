## Context

The current repo-local reliable-transfer helper is already implemented and
proven for the default S-band node-`5` path, but the active admission and
proof wrappers still hard-code that path. The maintained UHF formal path
already has frozen adjacent truth that this change must reuse rather than
reopen:

- `uhf-primary-after-failover` is the only current official UHF file/downlink
  path
- UHF primary packet quiet and beacon suppress semantics are already frozen
- the stock current UHF file/downlink path admits `243` file-data bytes per
  `Fw::FilePacket::DATA` packet
- the helper still uses its own bounded `160`-byte RT `DATA` segment ceiling

This design therefore adds one exact UHF reliable-transfer slice without
claiming that the S-band proof generalizes automatically.

## Goals / Non-Goals

**Goals**

- admit reliable transfer on exactly two current paths:
  - default S-band node-`5`
  - explicit-switched `uhf-primary-after-failover` node-`6`
- keep the artifact family bounded to the current official HK `.fdp`
  whole-file requests
- bind an admitted transfer to its exact path at start and prevent in-flight
  migration across switch or failover
- preserve current UHF packet quiet and beacon suppress semantics unchanged
- produce hosted proof for happy/resend/retry-exhausted plus target/lab quiet
  switched node-`6` happy-path proof

**Non-Goals**

- no `uhf-backup` reliable transfer
- no RF / over-the-air closure
- no restart-persistent resume
- no broad CFDP platform adoption
- no one-GDS aggregation
- no one-gateway multiplexer
- no generic simultaneous dual-link closure
- no non-quiet target UHF promotion

## Decisions

### 1. Reliable-transfer admission stays `CommController`-owned and path-bound

`CommController` remains the only admission owner. A request may use the
bounded helper only when all of the following are true:

- the request is whole-file (`offset == 0`, `length == 0`)
- the selected artifact is the current official HK `.fdp` family
- the current file path is one of the exact allowed paths
- the repo-owned receiver surface is intentionally enabled for the proof/runtime

The admitted transfer is path-bound at start:

- S-band transfers configure the helper for node `5`
- explicit-switched UHF transfers configure the helper for node `6`
- switching or failover does not migrate the active transfer context; the role
  change only aborts or fails it boundedly

### 2. UHF reliable transfer is explicit-switch-only in v1

This change does not promote any generic "UHF primary means reliable transfer"
rule. The new bounded UHF slice is admitted only on the explicit-switched
`uhf-primary-after-failover` branch. Automatic failover to UHF remains outside
the reliable-transfer claim for this change.

### 3. UHF launch delay semantics remain unchanged

The current UHF primary file launch quiesce stays in place. UHF reliable
transfer does not bypass that delay. Instead, deferred launch now chooses
between stock `FileDownlink` and the bounded helper after the existing quiesce
window expires.

### 4. Helper semantics are reused, not redesigned

The helper transport unit and retry semantics stay unchanged:

- `160`-byte RT `DATA` segment payload
- window `2`
- ACK timeout `1`
- resend budget `3`
- bounded `64` segments / `10,240` bytes
- one active transfer
- duplicate observation ignored in-context
- no restart-persistent resume

The stock UHF `243`-byte `Fw::FilePacket::DATA` ceiling remains the current
official file/downlink path-local baseline and is not rewritten by this change.

### 5. Hosted and target/lab proof boundaries stay separate and honest

Hosted proof must cover:

- happy path
- resend-before-success
- retry-exhausted with no final artifact promotion

Target/lab proof must cover:

- exact quiet switched node-`6` happy-path reliable transfer
- probe-owned `DIAGNOSTIC_QUIET_PACKET_EGRESS=1`
- removal of proof-only overrides and restart back to normal non-quiet service
  baseline after the proof

The quiet target proof is diagnostic-path only. It is not a nominal non-quiet
UHF reliable-transfer promotion.

## PASS / FAIL Boundary

| Case | Required result |
|---|---|
| Hosted switched UHF happy path | byte-matched RT output, no stock GDS `.fdp` output |
| Hosted switched UHF resend case | final success plus observed resend |
| Hosted switched UHF retry exhaustion | no final artifact promotion |
| Target/lab quiet switched node-`6` happy path | byte-matched RT output, no stock GDS `.fdp` output |
| Target/lab override cleanup | quiet and RT proof overrides removed; services back on normal non-quiet baseline |

## Comparison With `reliable-transfer-v1`

**Reused**

- helper wire protocol and resend semantics
- `CommController` ownership
- `DpCatalog` file-selection ownership
- raw-relay `ground_ttc_gateway` boundary
- official HK `.fdp` family and whole-file completion truth

**Changed**

- target-node selection is no longer implicitly node `5`
- deferred UHF launch now reaches the helper on the exact switched UHF path
- proof boundary now includes one additional bounded hosted and target/lab UHF
  slice

**Still Not Claimed**

- `uhf-backup` reliable transfer
- automatic failover-to-UHF reliable transfer
- non-quiet target UHF reliable transfer promotion
- RF, restart-persistent resume, broad CFDP, one-GDS, one-gateway, or generic
  simultaneous closure

## Open Questions

- None. This change intentionally locks to the first bounded UHF slice only.

## Implementation Status

- Implemented:
  - `CommController` admission now distinguishes default S-band, explicit
    switched UHF primary, and automatic failover-to-UHF
  - helper target-node selection is explicit for node `5` vs node `6`
  - in-flight transfer contexts abort instead of migrating across path changes
  - focused UT coverage now exercises switched-UHF admission, bounded fallback,
    and UHF-originated abort behavior
  - hosted switched-UHF proof wrapper now runs on the maintained per-band
    stock-stack launcher instead of ad hoc stack bring-up
  - target/lab quiet switched node-`6` proof wrapper now records proof-owned
    override cleanup plus restoration to the normal non-quiet baseline
- Closed proof boundary:
  - hosted happy-path switched-UHF reliable transfer passed
  - hosted resend-before-success passed
  - hosted retry-exhausted with no final artifact promotion passed
  - target/lab quiet switched node-`6` happy-path reliable transfer passed
- Consequence:
  - the new UHF claim is no longer residual-only
  - main specs and current docs can sync honestly
  - formal archive is justified because the bounded claim now lands with fresh
    hosted and target evidence
