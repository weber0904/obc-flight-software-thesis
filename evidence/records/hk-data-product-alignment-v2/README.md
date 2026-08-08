# Test Record: hk-data-product-alignment-v2

## Verdict

- Result: `PASS`
- Date: 2026-05-08
- Archived change: `openspec/changes/archive/2026-05-08-hk-data-product-alignment-v2/`

## Scope

This record covers the final-design health/state alignment slice for official F' HK data products:

- `HkTrendRecord` keeps the same product record name/id and advances the payload schema to `version = 4`.
- V4 maps existing cached/runtime provider data into the official `.fdp` path, including ADCS attitude/magnetometer, GPS motion/time, storage root health, COMM/CSP/UART/radio state, and boot metadata.
- Missing subsystem data remains explicit through validity flags and neutral field values; missing values are not represented as valid measurements.
- `DpCatalog` remains the official `.fdp` catalog/index surface.
- `HousekeepingArchive` and `hk/index.csv` remain bounded transitional fallback surfaces and are not claimed as the official data-product catalog.
- The focused hosted probe proves `.fdp` generation, `DpCatalog` build/xmit, GDS-received `.fdp` byte match, and received `.fdp` decode on the default hosted CCSDS S-band path.

## Out Of Scope

- New `health_manifest.json`.
- Retiring the HK ring.
- Expanding the HK ring to 256 files or 50 KB.
- Persistent event/fault storage.
- `CLEAR_HEALTH_LOG` or `CLEAR_EVENT_LOG`.
- Arbitrary onboard file downlink.
- Reliable transfer, CFDP, ARQ, NACK, or retry.
- Mode transition guards.
- Command authority, session, or authentication.
- Payload, FDIR, scheduler, or OBC process/resource sources beyond already cached/runtime providers.
- RF behavior, real radio behavior, target/Pi deployment, UHF CCSDS behavior, and physical storage endurance.
- Backward decode of historical V1/V2/V3 `.fdp` files with the V4 dictionary.

## Commands

Build and UT:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util check
./fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R "onboard_state_data_unit_test|onboard_state_snapshot_source_unit_test|housekeeping_snapshot_provider_unit_test" --output-on-failure
```

Focused hosted probe:

```bash
bash scripts/run_hk_data_product_alignment_v2_probe.sh
```

Governance:

```bash
openspec validate hk-data-product-alignment-v2
openspec validate --specs
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-hk-data-product-alignment-v2-final
```

## Evidence

Build and UT:

```text
fprime-util build: PASS
fprime-util build --ut: PASS
OBC_Components_HkTrendProductProducer_ut_exe: 6 tests passed
onboard_state_data_unit_test: PASS
housekeeping_snapshot_provider_unit_test: PASS
onboard_state_snapshot_source_unit_test: PASS
```

Focused hosted CCSDS S-band `.fdp` parity probe:

```text
hk-data-product-alignment-v2-ccsds-sband-fdp-probe: PASS
formal-verdict=hk-data-product-alignment-v2
obc-binary=OBC
command-prefix=OBCApp
link-mode=hosted-sband-tcp
comm-node=5
framing=space-packet-space-data-link
scid=68
vcid=1
frame-size=1024
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
fdp-received-byte-match=PASS source=/tmp/hk-data-product-alignment-v2-runtime/data-products/Dp_268693505_1778243926_00188677.fdp received=/tmp/hk-data-product-alignment-v2.gtkkzt/gds-downlink/fprime-downlink/_tmp_hk-data-product-alignment-v2-runtime_data-products_Dp_268693505_1778243926_00188677.fdp
fdp-decode=PASS version=4 json=/tmp/hk-data-product-alignment-v2.gtkkzt/fdp-decode/_tmp_hk-data-product-alignment-v2-runtime_data-products_Dp_268693505_1778243926_00188677.json
fallback-hk-ring-official-catalog=ABSENT
legacy-catalog-absent=PASS
sband-tcp-endpoint=127.0.0.1:55624
gds-port=55619
gds-tts-port=55620
runtime-root=/tmp/hk-data-product-alignment-v2-runtime
gds-file-storage-dir=/tmp/hk-data-product-alignment-v2.gtkkzt/gds-downlink
logs=/tmp/hk-data-product-alignment-v2.gtkkzt
```

Governance:

```text
openspec validate hk-data-product-alignment-v2: PASS
openspec validate --specs: PASS, 23 passed, 0 failed
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-hk-data-product-alignment-v2-final:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
```

## Notes

- This probe intentionally uses the current default hosted CCSDS S-band path: `OBC`, `OBCApp.*`, `space-packet-space-data-link`, `ground_ttc_gateway` raw relay, S-band TCP, and `sband_comm_csp_node` node `5`.
- The direct Native GDS `.fdp` parity path remains historical adjacent evidence. This record registers the default hosted CCSDS S-band `.fdp` parity path for V4.
- `scripts/run_onboard_state_data_hosted_probe.sh` now expects V4 when reused against the current dictionary.
- The shared legacy `comm_csp_ground_gateway_probe_test` was pinned to `OBC_ComFprimeLegacy`, `OBCAppComFprimeLegacy.*`, and `fprime` framing so it remains a legacy non-CCSDS regression after the default hosted `OBC` CCSDS S-band adoption.
