## Context

The current baseline has three related but distinct paths. `HousekeepingArchive` writes repo-owned binary HK ring files under `<runtime-root>/hk/`; `HkTrendProductProducer` writes official F' `.fdp` files under `<runtime-root>/data-products/`; `DpCatalog` can build a catalog and queue those `.fdp` files through `FileDownlink`. Current evidence proves generated `.fdp` files and catalog queueing only, not GDS-received byte match or decode.

Storage-health currently observes `hk`, `persistent-data`, `staging`, and `logs`. Once official HK `.fdp` files become the target mission-history path, `data-products` needs the same health visibility and a first-slice policy surface without implementing destructive cleanup.

## Goals / Non-Goals

**Goals:**

- Make official F' HK `.fdp` files the primary HK/state mission-history target.
- Add `data-products` to storage-health cached state, telemetry, warning/degraded masks, and root events.
- Migrate the HK trend product payload to V2 while preserving the product record name and id.
- Prove hosted GDS-received `.fdp` byte match and decode for current V2 products.
- Keep the HK ring as a transitional fallback with an explicit file-format bump.

**Non-Goals:**

- No `HousekeepingArchive` deletion or retention cleanup.
- No live beacon wire-format change.
- No RF, physical COMM `.fdp` parity, CFDP, ARQ/NACK, segment retry, packet-loss recovery, or arbitrary file downlink claim.
- No backward decode promise for pre-change V1 `.fdp` files with the new dictionary.

## Decisions

### Storage policy fields are observe-only

`RootStats` gains `quotaBytes`, `watermarkBytes`, `quotaStatus`, and `retentionStatus`. `quotaStatus` values are `0=NOT_CONFIGURED`, `1=OK`, `2=OVER_QUOTA`, and `3=UNAVAILABLE` when a configured quota cannot be evaluated because the root scan failed; `retentionStatus=0` means observe-only. The scanner never deletes files in this change. This keeps policy visibility available to operators without introducing retention risk.

### `data-products` uses bit 4 in existing U8 masks

Existing root ids remain stable: HK=0, PERSISTENT=1, STAGING=2, LOGS=3. `DATA_PRODUCTS=4` uses bit 4 in the existing `U8` warning and degraded masks. This avoids a mask-width migration while leaving three spare bits.

### HK trend V2 keeps product record name and id

The product record remains `HkTrendRecord` id `0`, but its FPP type becomes `HkTrendRecordV2` and the payload `version` field is set to `2`. New V2 `.fdp` files are decoded with the current dictionary. Old V1 `.fdp` backward decode is not promised by this change.

### Hosted parity proves received-file behavior only for direct GDS path

The existing hosted probe already uses direct Native GDS command/downlink and `DpCatalogFileDownlinkGate`. It will be extended to locate GDS-received `.fdp` files, byte-match them with the source runtime file, and decode the received file. The evidence will register this as hosted official `.fdp` parity, not as physical COMM, RF, CFDP, ARQ, or packet-loss recovery.

## Risks / Trade-offs

- V2 record migration changes the dictionary shape for `HkTrendRecord`; mitigate by documenting that old V1 `.fdp` backward decode is out of scope.
- `data-products` missing at first scan can raise degraded bit 4 if the runtime setup did not create the root; mitigate by keeping data-product directory creation in topology setup and validating it in the hosted probe.
- The hosted byte-match path depends on GDS file-storage behavior; mitigate by using an isolated GDS storage directory, recursive search, and exact content comparison.
