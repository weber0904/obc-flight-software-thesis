## 1. HK Producer And Schema

- [x] 1.1 Update `HkTrendProductProducer.fpp` to define V6 array `HkTrendRecord`, `HkTrendChunkMeta`, flush-reason enum, manual flush/status commands, and the persistent target-bytes parameter.
- [x] 1.2 Implement producer-side chunk accumulation, size-threshold rotation, manual flush, status reporting, and target-bytes normalization/persistence behavior in `HkTrendProductProducer`.
- [x] 1.3 Regenerate F' outputs and fix any interface mismatches introduced by the new command, parameter, and record definitions.

## 2. Verification And Proof Updates

- [x] 2.1 Update `HkTrendProductProducer` unit tests to cover chunk emission, sample/chunk sequence behavior, manual flush, threshold-change flush, parameter defaults, and serialize-failure buffer return.
- [x] 2.2 Update hosted official HK `.fdp` probes and bounded reliable-transfer probes to wait for a pending sample, issue `HK_TREND_FLUSH`, and validate V6 array-record plus chunk-metadata decode.
- [x] 2.3 Run targeted build and probe verification for the chunked official HK `.fdp` path and record the resulting pass/fail evidence.

## 3. Specs And Current Docs

- [x] 3.1 Sync the current HK data-product and onboard-state specs to the chunked V6 official `.fdp` contract.
- [x] 3.2 Update `docs/interfaces.md` and other current-baseline documentation that still describe one-sample-per-file V5 HK `.fdp` behavior.
- [x] 3.3 Validate the OpenSpec change and current specs after implementation updates land.
