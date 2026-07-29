## Context

The active HK mission-history path already uses official F' data-product
services: `HkTrendProductProducer -> DpManager -> DpWriter -> DpCatalog ->
FileDownlink`. The current producer, however, emits one finalized container for
every 30-second sample, so the official file path is being used as a
single-sample snapshot stream instead of a record-container stream. That
creates excessive file count, repeated container/file overhead, and a poor
operator experience for long-duration HK history.

This change must stay inside the current maintained topology, the stock F'
data-product/file model, and the current bounded reliable-transfer scope for
selected official HK `.fdp` files. It must not reintroduce the retired HK ring
archive or require a custom file appender beneath `DpWriter`.

## Goals / Non-Goals

**Goals:**

- Aggregate many 30-second HK samples into one finalized official `.fdp` file.
- Keep `DpManager`, `DpWriter`, `DpCatalog`, `FileDownlink`, and the bounded
  reliable-transfer chain unchanged after a chunk is finalized.
- Preserve the public `HkTrendRecord` identity (`record name = HkTrendRecord`,
  `id = 0`) while advancing the payload version to V6.
- Keep sample sequence and chunk sequence separate.
- Add operator controls for manual flush, status inspection, and persistent
  target file bytes using the standard parameter DB.
- Keep all legal operator-configured sizes within the current `10,240`-byte
  reliable-transfer ceiling.

**Non-Goals:**

- Ground-selectable HK field masking or runtime-selectable sample schema.
- Cross-reboot resume of partially accumulated in-memory HK chunks.
- Long-lived append to the same `.fdp` file after `DpWriter` has finalized it.
- New DP buffer families, new catalog semantics, or reliable-transfer protocol
  redesign.

## Decisions

### Accumulate samples in the producer, not in `DpWriter`

The producer keeps an in-memory pending chunk of HK samples and emits one
finalized official container only when the chunk must flush. This keeps the
stock F' contract intact: one finalized container becomes one `.fdp` file.

Alternatives considered:

- Append to one `.fdp` file inside `DpWriter`.
  Rejected because current `DpWriter` semantics are finalized-container to
  finalized-file, and `DpCatalog` operates on complete files rather than open
  append streams.
- Reintroduce the retired HK ring/archive path.
  Rejected because the active baseline already standardized on the official F'
  data-product infrastructure.

### Use array `HkTrendRecord` plus explicit chunk metadata

`HkTrendRecord` changes from a single record to an array record of V6 per-sample
elements, and the same container also carries `HkTrendChunkMeta` record id `1`.
This preserves the long-term public record identity while making chunk decode
self-describing.

Alternatives considered:

- Create a brand-new top-level record name for chunked HK.
  Rejected because preserving `HkTrendRecord` record id `0` avoids unnecessary
  downstream identity churn.
- Encode chunk metadata only in filenames or external catalog state.
  Rejected because each `.fdp` must remain self-describing after transfer or
  offline inspection.

### Keep sample sequence and chunk sequence separate

Each scheduled sample increments `sampleSequence`, regardless of whether a file
is emitted. Each emitted `.fdp` increments `chunkSequence` once. This preserves
the semantic meaning of the original per-sample sequence while introducing an
explicit per-file identity.

Alternatives considered:

- Reuse the old `sequence` field as the chunk/file sequence.
  Rejected because it would lose per-sample continuity and make partial chunk
  flushes harder to reason about.

### Define the operator target as final `.fdp` packet size

`HK_TREND_TARGET_FILE_BYTES` represents the final on-disk official `.fdp`
packet size ceiling, not raw payload bytes. The producer estimates packet size
before appending a sample, flushes the current non-empty chunk if the next
append would exceed the target, and then starts the next chunk with that
sample.

Alternatives considered:

- Configure target by sample count only.
  Rejected because the user-facing requirement is a file-size policy and future
  schema growth should not silently widen file size beyond the reliable ceiling.
- Configure target by payload-only bytes.
  Rejected because operators care about the actual transferred/stored file size,
  including official container/file overhead.

### Default to `8192` and cap at `10240`

The default `8192`-byte target reduces file count substantially while staying
comfortably inside the current bounded reliable-transfer scope. The maximum
legal value is `10240` so all legal operator settings preserve the current
reliable helper claim. The minimum legal value is the computed one-sample packet
size so the parameter never allows an impossible target.

### Use standard F' parameter persistence

`HK_TREND_TARGET_FILE_BYTES` is a component parameter loaded through the
existing topology `loadParameters()` path and persisted through the existing
`FileHandling.prmDb` flow. This avoids introducing a second configuration store
for one bounded sizing control.

### Flush immediately on shrink past the pending chunk

If an operator applies a smaller target and the pending chunk already exceeds
that target, the producer emits the current chunk immediately with flush reason
`THRESHOLD_CHANGE`. This makes the new policy effective without waiting for
another scheduled capture.

### Require explicit flush for freshness-sensitive probes and operators

Freshness-sensitive hosted proofs should wait for at least one pending sample,
invoke `HK_TREND_FLUSH`, then build and transmit the catalog. This keeps the
runtime chunking policy honest and avoids reintroducing implicit "one sample
equals one file" assumptions in probes.

## Risks / Trade-offs

- [Unexpected reset loses the pending in-memory chunk] → Accept this in the
  current change and keep the loss boundary explicit in specs and operator flow.
- [Decode and probe churn from V6 array records] → Update hosted proofs, unit
  tests, and interface docs together so the new decode contract is reviewable.
- [Incorrect size estimation could violate the `10240` bound] → Estimate using
  official container packet sizing, clamp operator input, and test threshold
  behavior around one-sample and near-limit cases.
- [Longer interval before a file appears could surprise operators] → Add manual
  flush and status commands, publish pending counts/bytes telemetry, and update
  runbooks/probes to use explicit flush when they need immediate artifacts.

## Migration Plan

1. Add V6 chunked HK record definitions, metadata record, commands, parameter,
   telemetry, and producer accumulation logic.
2. Regenerate F' code and update unit tests for chunking, manual flush,
   threshold changes, and parameter load behavior.
3. Update hosted probes so official HK `.fdp` generation waits for a pending
   sample and then flushes before asserting on files, catalog, or transfer.
4. Update current specs and interface documentation to describe the V6 chunked
   contract.
5. Verify generated `.fdp` decode, byte-match downlink, and bounded reliable
   compatibility under the preserved `<= 10240` rule.

Rollback is straightforward: revert the change set and return to the prior V5
single-sample emission behavior. No persistent migration of stored files is
required because the active decode claim remains "decode files generated by the
current dictionary".

## Open Questions

No open questions block this implementation. Ground-adjustable HK field content
selection remains a separate follow-on change.
