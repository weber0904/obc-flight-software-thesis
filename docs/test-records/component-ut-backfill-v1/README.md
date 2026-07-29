# 測試紀錄：component-ut-backfill-v1

- 日期：2026-04-09
- 層級：L1 / L2 / L3
- 環境：
  - host: macOS development machine
  - mode: classic F' component harness backfill and regression verification
- 關聯變更：`openspec/changes/archive/2026-04-09-component-ut-backfill-v1/`
- 關聯文件：
  - `OBC/Components/GpsBridge/test/ut/`
  - `OBC/Components/StorageHealthBridge/test/ut/`
  - `OBC/Components/HousekeepingArchive/test/ut/`
  - `docs/verification-matrix.md`
  - `openspec/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是補回目前 repo 中所有仍缺 classic F' component harness 的真實 component：

- `GpsBridge`
- `StorageHealthBridge`
- `HousekeepingArchive`

本次要證明的是：

- 三個 component 都已有 `register_fprime_ut()` + generated `TesterBase` / `GTestBase`
- helper/support direct tests 與既有 integration tests 保持不變且仍然綠燈
- 新 classic harness 測到 command / event / telemetry / scheduler / output-port contract，而不是只做到能編譯

## 2. 本次實作重點

- `GpsBridge`
  - 新增 classic harness tests covering:
    - `GPS_GET_STATE`
    - `GPS_SET_SOURCE_MODE`
    - fix/no-fix transitions
    - parse/source error event behavior
    - scheduler polling path
- `StorageHealthBridge`
  - 新增 classic harness tests covering:
    - `STORAGE_GET_STATUS`
    - `STORAGE_SCAN_NOW`
    - `schedIn` cadence
    - root-missing / scan-failed / warning-threshold events
    - telemetry publication through the component interface
- `HousekeepingArchive`
  - 新增 classic harness tests covering:
    - `HK_CAPTURE_NOW`
    - `HK_DOWNLINK_INDEX`
    - `HK_DOWNLINK_SLOT`
    - command-response mapping
    - periodic capture
    - downlink request emission via `fileOut`
- 保留既有 helper/support 與 integration coverage：
  - `gps_support_unit_test`
  - `storage_scanner_unit_test`
  - `transparent_link_framing_unit_test`
  - `housekeeping_snapshot_provider_unit_test`
  - `housekeeping_archive_store_unit_test`
  - 既有 GPS / storage / archive integration tests

## 3. 驗證指令

### 3.1 編譯 UT deployment

```bash
PATH="$PWD/fprime-venv/bin:$PATH" ./fprime-venv/bin/fprime-util build --ut
```

### 3.2 執行新的 classic component harness

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
  "$PWD/fprime-venv/bin/ctest" \
  --test-dir build-fprime-automatic-native-ut \
  --output-on-failure \
  -R 'OBC_Components_(GpsBridge|StorageHealthBridge|HousekeepingArchive)_ut_exe'
```

### 3.3 執行受影響 helper / integration regressions

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
  "$PWD/fprime-venv/bin/ctest" \
  --test-dir build-fprime-automatic-native-ut \
  --output-on-failure \
  -R '(gps_support_unit_test|gps_bridge_cached_state_integration_test|storage_scanner_unit_test|storage_health_bridge_cached_state_integration_test|housekeeping_archive_store_unit_test|housekeeping_archive_integration_test|housekeeping_archive_gps_integration_test|housekeeping_archive_storage_health_integration_test|transparent_link_framing_unit_test|housekeeping_snapshot_provider_unit_test)'
```

### 3.4 執行完整 baseline gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/component-test-baseline-recovery
```

## 4. 觀察結果

- `GpsBridge` 現在不再只依賴 replay/helper/integration coverage；component 本身已有 classic F' L2 harness
- `StorageHealthBridge` 的 component contract 現在能直接驗：
  - pre-configuration failure
  - warning/degraded event emission
  - cached telemetry publication contract
- `HousekeepingArchive` 現在不再只靠 store/helper 或 integration；component command/orchestration surface 已有 classic harness
- helper/support direct tests 仍保留，而且與 component harness 並存，不互相替代

## 5. 測試結果

- `PATH="$PWD/fprime-venv/bin:$PATH" ./fprime-venv/bin/fprime-util build --ut`: `PASS`
- focused classic harness set: `PASS`
  - `OBC_Components_GpsBridge_ut_exe`
  - `OBC_Components_StorageHealthBridge_ut_exe`
  - `OBC_Components_HousekeepingArchive_ut_exe`
- focused helper / integration regression set: `PASS`
  - `gps_support_unit_test`
  - `gps_bridge_cached_state_integration_test`
  - `storage_scanner_unit_test`
  - `storage_health_bridge_cached_state_integration_test`
  - `housekeeping_archive_store_unit_test`
  - `housekeeping_archive_integration_test`
  - `housekeeping_archive_gps_integration_test`
  - `housekeeping_archive_storage_health_integration_test`
  - `transparent_link_framing_unit_test`
  - `housekeeping_snapshot_provider_unit_test`
- `bash scripts/run_verification_ci.sh build-artifacts/component-test-baseline-recovery`: `PASS`
- `openspec validate component-ut-backfill-v1`: `PASS`
- `openspec validate --specs`: `PASS`

## 6. 驗收結論

- `PASS`：所有現有真實 component 都已具備 classic F' L2 harness
- `PASS`：helper/support direct tests 與 integration evidence 仍保持有效，未被新 harness 取代
- `PASS`：repo 現在對 later true components 的測試風格已重新與前期 classic F' pattern 對齊
