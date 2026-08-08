# hk-trend-chunked-fdp-v1 Evidence

Status: fresh local-ready, post-review rerun with bounded-backlog follow-up, and
post-archive reconciliation
evidence captured on this branch on 2026-06-08.

- Archived change: `openspec/changes/archive/2026-06-07-hk-trend-chunked-fdp-v1/`

## Scope

This record captures the chunked official HK `.fdp` change implemented by
`hk-trend-chunked-fdp-v1`.

It covers:

- `HkTrendProductProducer` changing from one-sample-per-file emission to
  producer-side chunk accumulation with one finalized official `.fdp` per
  flushed chunk
- `HkTrendRecord` remaining record id `0` while advancing to a V6 array record
  whose elements are individual HK samples
- `HkTrendChunkMeta` record id `1` carrying `chunkSequence`, sample-sequence
  range, sample count, configured target bytes, and flush reason
- operator-facing `HK_TREND_FLUSH` and `HK_TREND_GET_STATUS` command surfaces
  plus persistent `HK_TREND_TARGET_FILE_BYTES` bounded by the current reliable
  helper ceiling
- bounded retained-sample behavior that caps repeated flush-failure backlog to
  two max-sized HK chunks and rejects newer captures once that limit is reached
- command-authority policy/catalog updates so the secure hosted command path can
  reach the new HK commands and parameter operations
- preserved stock `DpManager -> DpWriter -> DpCatalog -> FileDownlink` flow
  once a chunk is finalized
- continued hosted byte-match/decode closure for the direct onboard-state parity
  path and the default CCSDS S-band catalog/downlink path
- continued bounded hosted reliable-transfer compatibility for selected official
  HK `.fdp` files under the existing `<= 10240` byte ceiling

It does **not** cover:

- ground-selectable HK field masking or runtime-selectable sample schema
- cross-reboot resume of partially accumulated chunks
- append-in-place reuse of one already-finalized `.fdp` file
- target/lab node-`5` or switched node-`6` reliable-transfer reruns
- RF behavior, real-radio behavior, Pi deployment, or arbitrary onboard file
  downlink

## Reused Validation Paths

- Entry `34` in `evidence/verification-path-registry.md` for hosted official FDP
  parity ancestry
- Entry `44` in `evidence/verification-path-registry.md` for default hosted CCSDS
  S-band official FDP parity ancestry
- `evidence/records/reliable-transfer-v1/README.md` for the maintained bounded
  hosted node-`5` reliable-transfer path; this change revalidates only the
  happy-path current HK family compatibility

## Commands

Fresh local-ready gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

Focused hosted proofs:

```bash
bash scripts/run_onboard_state_data_hosted_probe.sh
bash scripts/run_hk_data_product_alignment_v2_probe.sh
bash scripts/run_comm_reliable_transfer_hosted_probe.sh
```

OpenSpec validation:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate hk-trend-chunked-fdp-v1
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs
```

## Acceptance

This change is accepted only when all of the following are true:

- the fresh local-ready gate passes from `generate` through
  `openspec_validate_specs`
- the fresh gate builds and runs the classic
  `OBC_Components_HkTrendProductProducer_ut_exe` harness
- the hosted onboard-state parity proof byte-matches and decodes a received HK
  `.fdp` with `version = 6`
- the hosted default CCSDS S-band proof byte-matches and decodes a received HK
  `.fdp` with `version = 6`
- the hosted reliable-transfer proof still completes successfully for the
  current chunked HK family under the bounded helper ceiling
- `openspec validate hk-trend-chunked-fdp-v1` and `openspec validate --specs`
  pass

## Fresh Evidence

Fresh local-ready gate:

- Command: `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- Verdict: `PASS`
- Summary: [build-artifacts/verification-ci-local/summary.md](../../README.md)
- Key checkpoints:
  - `01_generate`: `PASS`
  - `02_build`: `PASS`
  - `03_generate_ut`: `PASS`
  - `04_build_ut`: `PASS`
  - `05_check_all`: `PASS`
  - `06_check_repo_consistency`: `PASS`
  - `07_check_documentation_governance`: `PASS`
  - `08_check_transport_mtu_apid_contract`: `PASS`
  - `09_check_component_test_baseline`: `PASS`
  - `10_check_legacy_zmq_retired`: `PASS`
  - `11_openspec_validate_specs`: `PASS`
- Classic component harness evidence:
  - `build-artifacts/verification-ci-local/05_check_all.log`
  - `Start 59: OBC_Components_HkTrendProductProducer_ut_exe`
  - `59/69 Test #59: OBC_Components_HkTrendProductProducer_ut_exe ..........   Passed`
  - bounded backlog policy is covered by
    `HkTrendProductProducer.RetainedBacklogIsBoundedWhenFlushKeepsFailing`

Hosted onboard-state parity proof:

- Command: `bash scripts/run_onboard_state_data_hosted_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `onboard-state-data-fdp-parity-v1-hosted-probe: PASS`
  - `formal-verdict=onboard-state-data-fdp-parity-v1`
  - `hk-official-dp-files=PASS count=1 first=/tmp/osd-Ba7IM8/r/data-products/Dp_268693505_1780889402_00334234.fdp`
  - `dpcatalog-build=PASS`
  - `dpcatalog-xmit-queue=PASS`
  - `fdp-received-byte-match=PASS source=/tmp/osd-Ba7IM8/r/data-products/Dp_268693505_1780889402_00334234.fdp received=/tmp/osd-Ba7IM8/sband-ground/gds-files/fprime-downlink/_tmp_osd-Ba7IM8_r_data-products_Dp_268693505_1780889402_00334234.fdp`
  - `fdp-decode=PASS version=6 json=/tmp/onboard-state-data-hosted.Ba7IM8/fdp-decode/_tmp_osd-Ba7IM8_r_data-products_Dp_268693505_1780889402_00334234.json`
  - `storage-data-products-root=PASS files=2 bytes=529`
  - `legacy-catalog-absent=PASS`
  - `runtime-root=/tmp/osd-Ba7IM8/r`
  - `logs=/tmp/onboard-state-data-hosted.Ba7IM8`

Hosted default CCSDS S-band HK `.fdp` parity proof:

- Command: `bash scripts/run_hk_data_product_alignment_v2_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `hk-data-product-alignment-v2-ccsds-sband-fdp-probe: PASS`
  - `formal-verdict=hk-data-product-alignment-v2`
  - `obc-binary=OBC`
  - `command-prefix=OBCApp`
  - `link-mode=hosted-sband-tcp`
  - `comm-node=5`
  - `framing=space-packet-space-data-link`
  - `scid=68`
  - `vcid=1`
  - `frame-size=1024`
  - `dpcatalog-build=PASS`
  - `dpcatalog-xmit-queue=PASS`
  - `fdp-received-byte-match=PASS source=/tmp/hka-72WcQW/r/data-products/Dp_268693505_1780889444_00863338.fdp received=/tmp/hka-72WcQW/sband-ground/gds-files/fprime-downlink/_tmp_hka-72WcQW_r_data-products_Dp_268693505_1780889444_00863338.fdp`
  - `fdp-decode=PASS version=6 json=/tmp/hk-data-product-alignment-v2.72WcQW/fdp-decode/_tmp_hka-72WcQW_r_data-products_Dp_268693505_1780889444_00863338.json`
  - `fallback-hk-ring-official-catalog=ABSENT`
  - `legacy-catalog-absent=PASS`
  - `runtime-root=/tmp/hka-72WcQW/r`
  - `logs=/tmp/hk-data-product-alignment-v2.72WcQW`

Hosted bounded reliable-transfer compatibility proof:

- Command: `bash scripts/run_comm_reliable_transfer_hosted_probe.sh`
- Verdict: `PASS`
- Observed result:
  - `comm-reliable-transfer-hosted-probe: PASS`
  - `formal-verdict=reliable-transfer`
  - `probe-mode=happy`
  - `result=success`
  - `transfer-started=PASS`
  - `legacy-gds-file-storage=ABSENT`
  - `matched-source=/tmp/crt-pNIyIf/r/data-products/Dp_268693505_1780889485_00184199.fdp`
  - `received-path=/tmp/comm-reliable-transfer-hosted.pNIyIf/rt-output/Dp_268693505_1780889485_00184199.fdp`
  - `received-size=498`
  - `received-sha256=f6d43ce6933076ae394f4cb29fdb566a63597f9c5f02fe97e8d6e4fa50bbf521`
  - `comm-node=5`
  - `runtime-root=/tmp/crt-pNIyIf/r`
  - `logs=/tmp/comm-reliable-transfer-hosted.pNIyIf`

OpenSpec validation:

- `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate hk-trend-chunked-fdp-v1`
  - verdict: `PASS`
- `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`
  - verdict: `PASS`
- `python3 scripts/check_repo_consistency.py`
  - verdict: `PASS`
- `PATH="$PWD/fprime-venv/bin:$PATH" openspec archive hk-trend-chunked-fdp-v1 --yes`
  - verdict: `PASS`
  - archive path: `openspec/changes/archive/2026-06-07-hk-trend-chunked-fdp-v1/`
- post-archive `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`
  - verdict: `PASS`
- `python3 scripts/generate_reconciliation_matrix_md.py`
  - verdict: `PASS`
