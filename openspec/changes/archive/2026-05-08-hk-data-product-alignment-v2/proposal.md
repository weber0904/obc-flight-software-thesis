## Why

The current baseline already writes official F' HK `.fdp` products, uses `DpCatalog`, proves direct hosted `.fdp` received-byte parity, and advances the current HK payload to `HkTrendRecordV3` / `version = 3` for mode-model-v2 semantics. The remaining gap is final-design health/state alignment: several cached runtime fields already available to the fallback HK ring are not yet present in the official `.fdp` path, and the strongest existing `.fdp` parity evidence still uses the direct Native GDS path rather than the current default hosted CCSDS S-band path.

This change makes official F' HK `.fdp` products the primary stored HK/state history surface for the cached fields already available in the runtime baseline. It keeps `HousekeepingArchive` and `hk/index.csv` as a bounded transitional fallback and does not add a parallel manifest or a broad file-transfer feature.

## What Changes

- Migrate the stable `HkTrendRecord` product record to a V4 payload while preserving product record name and id.
- Map cached/provider-backed health/state gaps into the official `.fdp` payload, including ADCS attitude details, GPS time/motion fields, all governed storage roots, COMM band, CSP counters, radio/UART link stats, and boot metadata.
- Keep missing subsystem values explicit through validity flags and quality masks instead of presenting fallback numeric defaults as valid measurements.
- Add a focused hosted CCSDS S-band `.fdp` parity probe for the current default `OBC` path through `sband_comm_csp_node` node `5`.
- Record evidence and registry boundaries that distinguish official `.fdp` catalog/downlink from the fallback HK ring and from arbitrary file downlink or reliable transfer.

## Capabilities

### New Capabilities

### Modified Capabilities

- `hk-data-products`: Adds V4 HK trend product content, version semantics, and default hosted CCSDS S-band `.fdp` parity criteria.
- `onboard-data-products-and-live-beacon`: Updates the official HK trend data-product boundary to reference V4 while leaving BeaconV1 unchanged.

## Impact

- Affected components: `HkTrendProductProducer` and topology snapshot sources.
- Affected support logic: `OnboardStateData` snapshot/reduction structures, hosted probe decode helpers, current dictionary/decode evidence, and OpenSpec/test-record documentation.
- Affected evidence: `evidence/records/hk-data-product-alignment-v2/README.md` and `evidence/verification-path-registry.md`.
- Explicitly out of scope: `health_manifest.json`, HK ring retirement or capacity expansion, persistent event/fault storage, clear-log commands, arbitrary onboard file downlink, CFDP/ARQ/NACK/retry, mode transition guard, command authority/session/auth, payload/FDIR/scheduler data-source expansion, UHF CCSDS, RF, target/Pi `.fdp` parity, and historical `.fdp` backward decode.
