## Why

The current official HK trend path emits one `.fdp` file for every 30-second
snapshot, which creates thousands of tiny files, repeats per-file overhead, and
forces operators to treat long-duration HK history as a large catalog of
single-sample artifacts. We need to keep the active official F' data-product
infrastructure while using it in the intended record-container style: aggregate
multiple HK samples into one finalized `.fdp`, then rotate at a governed size
ceiling that stays inside the current bounded reliable-transfer scope.

## What Changes

- Change `HkTrendProductProducer` from immediate one-sample emission to
  in-memory chunk accumulation with one finalized official `.fdp` per emitted
  chunk.
- Advance the HK trend payload from V5 to V6 while preserving the public
  `HkTrendRecord` record name and id `0`.
- Change `HkTrendRecord` from a single sample record into an array record whose
  elements are individual HK samples.
- Add `HkTrendChunkMeta` record id `1` so each emitted `.fdp` carries chunk
  sequence, sample range, sample count, configured target bytes, and flush
  reason.
- Add manual `HK_TREND_FLUSH` and `HK_TREND_GET_STATUS` command surfaces plus a
  persistent `HK_TREND_TARGET_FILE_BYTES` parameter with default `8192` and
  bounded ceiling `10240`.
- Keep the stock `DpManager -> DpWriter -> DpCatalog -> FileDownlink` flow
  unchanged after a chunk is finalized.
- Update hosted proofs, decode checks, and operator documentation so they
  explicitly flush pending HK samples before catalog build and downlink when
  freshness matters.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `hk-data-products`: official HK `.fdp` generation changes from one snapshot
  per file to chunked multi-sample official `.fdp` files with V6 decode,
  manual flush, size-threshold rotation, and bounded reliable compatibility.
- `onboard-data-products-and-live-beacon`: the official HK trend boundary
  changes from single-record immediate emission to scheduled sample capture plus
  finalized chunk emission, while keeping `HK_TREND_PRODUCT_WRITTEN` as the
  operator-facing success event.
- `interface-contract-index`: `docs/interfaces.md` must describe the current
  chunked HK `.fdp` mission-history contract, V6 payload identity, and the new
  manual flush / target-size control surfaces.

## Impact

- Affected code: `OBC/Components/HkTrendProductProducer/*`, its unit tests, and
  the `TopCcsds` runtime command/parameter surface generated from FPP.
- Affected proofs: hosted official HK `.fdp` generation, decode, catalog, and
  bounded reliable-transfer probes must flush pending chunks explicitly before
  file assertions.
- Affected docs/specs: HK data-product specs, onboard-state data-product
  narrative, interface index, and current-baseline documentation.
- Downstream operators keep the same official file/downlink chain, but the
  operational unit changes from "one file per sample" to "one file per HK
  chunk".
