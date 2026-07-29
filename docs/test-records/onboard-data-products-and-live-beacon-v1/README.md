# 測試紀錄：onboard-data-products-and-live-beacon-v1

## Verdict

- 結果：`PASS`
- 日期：2026-05-06
- 關聯變更：`openspec/changes/archive/2026-05-05-onboard-data-products-and-live-beacon-v1/`

## Scope

本紀錄證明 hosted v0 runtime 的 replacement baseline：

- `OnboardStateMonitor` 從既有 cached subsystem state 產生 `ReducedStateV1`，source failure 會 invalidate cached reduced state。
- `BeaconPublisher` 每 17 scheduler ticks 透過 COMM-facing sink 輸出 `BeaconV1`，sequence 只在 sink 成功後遞增。
- `BeaconV1` 含 cached subsystem raw/near-raw critical fields，debug receiver 只作驗證捕捉，不建立 mission `BEACON_HISTORY`。
- `HkTrendProductProducer -> DpManager -> DpWriter` 產生官方 F' `.fdp` HK trend data product。
- `DpCatalog.BUILD_CATALOG` 可掃描官方 `.fdp`，`DpCatalog.START_XMIT_CATALOG` 透過 `DpCatalogFileDownlinkGate` 把 pending product queue 到 stock `FileDownlink`。
- `DpCatalogFileDownlinkGate` 以 `SendFileResponse.context` 過濾共用 `FileDownlink.FileComplete` callback，避免 housekeeping 檔案完成通知推進 data-product catalog 狀態。
- runtime baseline 不再產生 repo-local `data-products/catalog.csv`、OPD1 product files 或 `beacon-history-*.bin`。

## Out Of Scope

- 真實 RF 鏈路或 vendor radio framing。
- Raspberry Pi target-side SD card endurance / retention behavior。
- packet-loss 下的 reliable file transfer、NACK/ARQ、CFDP、重傳策略。
- GDS 端 byte-matched received `.fdp` 檔案完整性；本次只證明 `DpCatalog -> FileDownlink` queueing。
- `MissionExecutive` / LOW_POWER / DETUMBLE / load shedding。
- `TlmPacketizer` telemetry packet/rate design。

## Commands

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util generate -f
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util generate --ut -f
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build --ut
```

Focused tests:

```bash
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_data_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_snapshot_source_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_OnboardStateMonitor_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BeaconPublisher_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_HkTrendProductProducer_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_DpCatalogFileDownlinkGate_ut_exe
```

Hosted probe and decode:

```bash
bash scripts/run_onboard_state_data_hosted_probe.sh
scripts/decode_beacon_v1.py /tmp/onboard-state-data-hosted.1zFE5E/beacon-capture.bin --format json
```

Governance:

```bash
openspec validate onboard-data-products-and-live-beacon-v1
openspec archive onboard-data-products-and-live-beacon-v1 --yes
openspec validate --specs
python3 scripts/check_repo_consistency.py
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-gate
```

## Evidence

Focused tests:

```text
onboard_state_data_unit_test: PASS
onboard_state_snapshot_source_unit_test: PASS
OBC_Components_OnboardStateMonitor_ut_exe: 3 tests passed
OBC_Components_BeaconPublisher_ut_exe: 5 tests passed
OBC_Components_HkTrendProductProducer_ut_exe: 4 tests passed
OBC_Components_DpCatalogFileDownlinkGate_ut_exe: 4 tests passed
```

Hosted probe:

```text
onboard-data-products-and-live-beacon-v1-hosted-probe: PASS
formal-verdict=onboard-data-products-and-live-beacon-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0xe01f9ad0
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778003278_00825015.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
legacy-catalog-absent=PASS
runtime-root=/tmp/onboard-state-data-runtime-hosted
beacon-capture=/tmp/onboard-state-data-hosted.1zFE5E/beacon-capture.bin
beacon-decoded=/tmp/onboard-state-data-hosted.1zFE5E/beacon-decoded.json
logs=/tmp/onboard-state-data-hosted.1zFE5E
```

Beacon decoder:

```text
type=BeaconV1 sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=3760167632
```

OpenSpec and shared gate:

```text
openspec validate onboard-data-products-and-live-beacon-v1: PASS
openspec archive onboard-data-products-and-live-beacon-v1 --yes: archived as 2026-05-05-onboard-data-products-and-live-beacon-v1
openspec validate --specs: 23 passed, 0 failed
python3 scripts/check_repo_consistency.py: PASS
scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-gate:
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

- The old PR #41 repo-local `StateDataArchive` / `ProductCatalog` / `catalog.csv` baseline is replaced by official F' data products.
- The original F' telemetry/event path remains unchanged: components still emit normal telemetry/events through `TlmChan`, `events`, `ComFprime`, and Native GDS.
- The existing `HousekeepingArchive` ring/index/downlink capability remains separate and valid.

## PR Review Follow-Up: 2026-05-06

Additional review fixes were made after PR #42 CI passed:

- `UartDriver` runtime beacon send now returns success only after the configured byte-stream transport accepts the frame; UART poll, exchange, send, transport configuration, and stats reads share a runtime mutex.
- `BeaconV1` encoding uses a fixed-size 108-byte array instead of `std::vector`.
- `BeaconV1` encoding sanitizes non-finite floating-point source fields before scaled integer conversion.
- `BeaconV1` decoding validates `TimeBase`, `SatMode`, `BootSlot`, and boolean wire fields before constructing typed values.
- `OnboardStateSnapshotSource` preserves caller-provided high-resolution timestamps and only falls back to workstation time when the timestamp is zero.
- CRC32 lookup table was moved out of `crc32()` into an anonymous-namespace constant.

Additional verification:

```text
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build --ut: PASS
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_UartDriver_ut_exe: 6 tests passed
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_data_unit_test: PASS
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_snapshot_source_unit_test: PASS
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BeaconPublisher_ut_exe: 5 tests passed
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util check --all: 41 passed, 0 failed
openspec validate onboard-data-products-and-live-beacon: PASS
openspec validate --specs: 23 passed, 0 failed
bash scripts/run_onboard_state_data_hosted_probe.sh: PASS
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-review-followup: PASS
```

Hosted follow-up probe:

```text
onboard-data-products-and-live-beacon-v1-hosted-probe: PASS
formal-verdict=onboard-data-products-and-live-beacon-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0x960a9d28
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778038419_00020239.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
legacy-catalog-absent=PASS
```

Verification CI follow-up:

```text
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

## PR Review Follow-Up: Nonblocking Beacon Handoff And Decode Robustness

Additional PR review findings were handled in a later PR review pass:

- `UartBeaconSink` now hands beacon frames to `UartDriver::queueSendForRuntime` instead of running byte-stream writes on the data rate group caller path.
- `UartDriver` owns a bounded runtime send queue and drains it from `UartDriver.schedIn`; disconnected transport and queue overflow reject the sink send so `BeaconPublisher` does not advance sequence.
- TCP and serial byte-stream transport writes now poll for write readiness using the configured timeout and return `TIMEOUT`/`IO_ERROR` instead of using an unbounded write loop.
- `appendF32Scaled` now clamps with double-precision bounds before casting to `I32`, avoiding the `I32::max` rounding edge.
- `decodeBeaconV1` now decodes into a temporary object and updates caller output only after all validation passes.
- `OnboardStateMonitor::publishState_` no longer carries an unused `ok` parameter.

Additional verification:

```text
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build --ut: PASS
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_data_unit_test: PASS
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_OnboardStateMonitor_ut_exe: 3 tests passed
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_UartDriver_ut_exe: 9 tests passed
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util check --all: 41 passed, 0 failed
openspec validate onboard-data-products-and-live-beacon: PASS
openspec validate --specs: 23 passed, 0 failed
python3 scripts/check_repo_consistency.py: PASS
python3 scripts/check_component_test_baseline.py: PASS
bash scripts/run_onboard_state_data_hosted_probe.sh: PASS
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-review-followup-4: PASS
```

Hosted follow-up probe:

```text
onboard-data-products-and-live-beacon-v1-hosted-probe: PASS
formal-verdict=onboard-data-products-and-live-beacon-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0x1dedb6de
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778050696_00947079.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
legacy-catalog-absent=PASS
```

Verification CI follow-up:

```text
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

## PR Review Follow-Up: Beacon Canonical Length And Cleanup

Additional PR review findings were handled in a later PR review pass:

- `decodeBeaconV1` now rejects non-canonical frame lengths before CRC validation. Extended frames are rejected even if they include an internally consistent CRC.
- The duplicated ADCS rate norm helper was consolidated into `OBC::StateData::adcsRateNorm`.
- The unused `ReducedStateV1::dataProductBacklogBytes` field was removed from the V1 in-memory model.
- The `dpBufferManager.setup(300U, 0U, ...)` review finding was triaged as a false positive: in F' v4.1.0 the second argument is `memId`, not memory size. Buffer memory is computed from configured bins, and hosted probes validated successful data product generation.

Additional verification:

```text
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build --ut: PASS
./build-fprime-automatic-native-ut/bin/Darwin/onboard_state_data_unit_test: PASS
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BeaconPublisher_ut_exe: 5 tests passed
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_HkTrendProductProducer_ut_exe: 5 tests passed
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util check --all: 41 passed, 0 failed
openspec validate onboard-data-products-and-live-beacon: PASS
openspec validate --specs: 23 passed, 0 failed
python3 scripts/check_repo_consistency.py: PASS
python3 scripts/check_component_test_baseline.py: PASS
bash scripts/run_onboard_state_data_hosted_probe.sh: PASS
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-review-followup-2: PASS
```

Hosted follow-up probe:

```text
onboard-data-products-and-live-beacon-v1-hosted-probe: PASS
formal-verdict=onboard-data-products-and-live-beacon-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0xb0677c19
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778044886_00798683.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
legacy-catalog-absent=PASS
```

Verification CI follow-up:

```text
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

## PR Review Follow-Up: DP Buffer Return

An additional PR review finding identified that `HkTrendProductProducer` returned early on HK trend serialization failure without returning the allocated data product buffer. The producer now exposes `productBufferReturnOut`, connects it to `dpBufferManager.bufferSendIn`, and returns the allocated buffer before publishing `SERIALIZE_ERROR`.

Additional verification:

```text
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util build --ut: PASS
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_HkTrendProductProducer_ut_exe: 5 tests passed
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ./fprime-venv/bin/fprime-util check --all: 41 passed, 0 failed
openspec validate onboard-data-products-and-live-beacon: PASS
openspec validate --specs: 23 passed, 0 failed
python3 scripts/check_repo_consistency.py: PASS
python3 scripts/check_component_test_baseline.py: PASS
bash scripts/run_onboard_state_data_hosted_probe.sh: PASS
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-pr42-dp-buffer-return: PASS
```

Hosted follow-up probe:

```text
onboard-data-products-and-live-beacon-v1-hosted-probe: PASS
formal-verdict=onboard-data-products-and-live-beacon-v1
live-beacon-capture=PASS sequence=0 size=108 battery_voltage=8.06 battery_current=0.34 crc=0x3572e0d7
hk-official-dp-files=PASS count=1 first=/tmp/onboard-state-data-runtime-hosted/data-products/Dp_268693505_1778043231_00272776.fdp
dpcatalog-build=PASS
dpcatalog-xmit-queue=PASS
legacy-catalog-absent=PASS
```

Verification CI follow-up:

```text
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
