# Test Record: onboard-state-data-fdp-parity-v1

## Verdict

- Result: `PASS`
- Date: 2026-05-06
- Archived change: `openspec/changes/archive/2026-05-06-onboard-state-data-fdp-parity-v1/`

## Scope

This record covers the hosted parity slice for official F' HK data products:

- `storage-health` now observes `<runtime-root>/data-products` as `DATA_PRODUCTS`.
- `HkTrendRecord` keeps the same product record name/id and uses a V2 payload with data-products root fields.
- `HousekeepingArchiveStore` remains a transitional fallback and bumps its binary record format to v3.
- The hosted probe proves official `.fdp` generation, `DpCatalog` build/xmit, GDS-received byte match, and received-file decode with `version = 2`.

## Out Of Scope

- RF `.fdp` parity or vendor radio behavior.
- Physical COMM `.fdp` byte-match.
- Raspberry Pi target SD/endurance behavior.
- CFDP, ARQ/NACK, packet-loss recovery, segment retry, compression, and pass/window scheduling.
- Arbitrary onboard file downlink.
- Removing `HousekeepingArchive`.

## Commands

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
```

Focused tests:

```bash
./build-fprime-automatic-native-ut/bin/Darwin/storage_scanner_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/storage_health_bridge_cached_state_integration_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_StorageHealthBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/housekeeping_archive_storage_health_integration_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_HkTrendProductProducer_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_data_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_snapshot_source_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_DpCatalogFileDownlinkGate_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/housekeeping_snapshot_provider_unit_test
```

Hosted probe:

```bash
bash scripts/run_onboard_state_data_hosted_probe.sh
```

Governance:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util check --all
openspec validate onboard-state-data-fdp-parity-v1
openspec validate --specs
python3 scripts/check_repo_consistency.py
python3 scripts/check_component_test_baseline.py
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdp-parity
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdp-parity-post-archive
```

## Evidence

Build and UT generation:

```text
fprime-util generate -f: PASS
fprime-util build: PASS
fprime-util generate --ut -f: PASS
fprime-util build --ut: PASS
```

Focused tests:

```text
storage_scanner_unit_test: PASS
storage_health_bridge_cached_state_integration_test: PASS
OBC_Components_StorageHealthBridge_ut_exe: 6 tests passed
housekeeping_archive_storage_health_integration_test: PASS
OBC_Components_HkTrendProductProducer_ut_exe: 6 tests passed
onboard_state_data_unit_test: PASS
onboard_state_snapshot_source_unit_test: PASS
OBC_Components_DpCatalogFileDownlinkGate_ut_exe: 4 tests passed
housekeeping_snapshot_provider_unit_test: PASS
```

Hosted probe:

```text
onboard-state-data-fdp-parity-v1-hosted-probe: PASS
formal-verdict=onboard-state-data-fdp-parity-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0xcceb378f
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778077743_00955172.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
fdp-received-byte-match=PASS source=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778077743_00955172.fdp received=/tmp/onboard-state-data-hosted.cAaXs3/gds-downlink/fprime-downlink/_tmp_onboard-state-data-runtime-hosted_data-products_Dp_268693505_1778077743_00955172.fdp
fdp-decode=PASS json=/tmp/onboard-state-data-hosted.cAaXs3/fdp-decode/_tmp_onboard-state-data-runtime-hosted_data-products_Dp_268693505_1778077743_00955172.json
storage-data-products-root=PASS files=2 bytes=300
legacy-catalog-absent=PASS
runtime-root=/tmp/onboard-state-data-runtime-hosted
beacon-capture=/tmp/onboard-state-data-hosted.cAaXs3/beacon-capture.bin
beacon-decoded=/tmp/onboard-state-data-hosted.cAaXs3/beacon-decoded.json
logs=/tmp/onboard-state-data-hosted.cAaXs3
```

Governance:

```text
fprime-util check --all: PASS, 41/41 tests passed
openspec validate onboard-state-data-fdp-parity-v1: PASS
openspec validate --specs: PASS, 23 passed, 0 failed
python3 scripts/check_repo_consistency.py: PASS
python3 scripts/check_component_test_baseline.py: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-fdp-parity:
  01_generate: PASS
  02_build: PASS
  03_generate_ut: PASS
  04_build_ut: PASS
  05_check_all: PASS
  06_check_repo_consistency: PASS
  07_check_component_test_baseline: PASS
  08_check_legacy_zmq_retired: PASS
  09_openspec_validate_specs: PASS
openspec archive onboard-state-data-fdp-parity-v1 --yes: PASS
post-archive openspec validate --specs: PASS, 23 passed, 0 failed
post-archive python3 scripts/check_repo_consistency.py: PASS
post-archive python3 scripts/check_component_test_baseline.py: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-fdp-parity-post-archive:
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

- `fprime-dp-write` was used as the available local decoder fallback.
- A focused rerun of `OBC_Components_StorageHealthBridge_ut_exe` emitted stale coverage merge warnings from a prior failed run, but the executable exited `0` and all tests passed. The full UT regenerate/build gate recreated the build tree before final validation.
