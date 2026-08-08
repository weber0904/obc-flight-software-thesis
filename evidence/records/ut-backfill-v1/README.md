# 測試紀錄：ut-backfill-v1

- 日期：2026-04-08
- 層級：L1 / L2 / L3
- 環境：
  - host: macOS development machine
  - mode: maintenance-wave targeted automated test backfill
- 關聯變更：`openspec/changes/archive/2026-04-08-ut-backfill-v1/`
- 關聯文件：
  - `openspec/changes/archive/2026-04-08-ut-backfill-v1/specs/verification-evidence/spec.md`
  - `docs/verification.md`
  - `scripts/report_verification_inventory.py`

## 1. 目標

本 change 的目標是針對 verification matrix 已明確點出的不均衡區塊補上最小且耐久的自動化測試，而不改變 flight runtime behavior：

- `GpsBridge`：補 component-level contract test
- `StorageHealthBridge`：補 component-level contract test
- `HousekeepingSnapshotProvider`：補聚合正確性 unit test
- `HousekeepingArchiveStore`：補最小 store/index/request logic test
- transparent framing：補 standalone unit test

本次仍然**刻意不做**：

- GPS live UART hardware bring-up
- storage retention / cleanup policy
- `HousekeepingArchive` 更高層 command-orchestration 全面重寫
- 任何新的 hosted / Pi validation path

## 2. 本次實作重點

- `OBC/Components/GpsBridge/test/GpsBridgeContractTest.cpp`
  - 驗證 replay source 不存在時的 runtime rejection
  - 驗證 valid / no-fix / malformed sentence 與 source-mode switching semantics
- `OBC/Components/StorageHealthBridge/test/StorageHealthBridgeContractTest.cpp`
  - 驗證未設定前掃描失敗
  - 驗證 configured scan、counter/error behavior、missing-root degraded semantics
- `OBC/Top/test/HousekeepingSnapshotProviderUnitTest.cpp`
  - 驗證 required-provider 缺失時 capture 失敗
  - 驗證 representative GPS / storage / comm / boot cached state 聚合
- `OBC/Components/HousekeepingArchive/test/HousekeepingArchiveStoreUnitTest.cpp`
  - 驗證未初始化 request rejection
  - 驗證 invalid index handling 與 empty-store request semantics
- `simulators/comm/test/TransparentLinkFramingUnitTest.cpp`
  - 驗證 round-trip escaping、bad escape、length mismatch、CRC mismatch
- `scripts/report_verification_inventory.py`
  - 補 support test 與 standalone/unit-contract test classification
- `docs/verification.md`
  - 將新增 L1/L2 coverage 反映為正式 matrix

## 3. 驗證指令

### 3.1 執行 focused backfill tests

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
ctest --test-dir build-fprime-automatic-native-ut --output-on-failure \
  -R 'gps_bridge_contract_test|storage_health_bridge_contract_test|housekeeping_snapshot_provider_unit_test|housekeeping_archive_store_unit_test|transparent_link_framing_unit_test'
```

### 3.2 執行完整本機 baseline gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

### 3.3 產出 verification inventory

```bash
python3 scripts/report_verification_inventory.py
python3 scripts/report_verification_inventory.py --json
```

### 3.4 驗證 change 與 main specs

```bash
python3 scripts/check_repo_consistency.py
openspec validate ut-backfill-v1
openspec validate --specs
```

## 4. 觀察結果

### 4.1 GpsBridge

- `GpsBridge` 現在已有專屬 contract-level test，不再只依賴 replay/integration coverage
- 仍未宣稱 live UART 路徑通過；硬體 bring-up 仍保持 `Blocked-HW`

### 4.2 StorageHealthBridge

- `StorageHealthBridge` 現在能在 component-level 驗證 pre-configuration failure 與 scan counter behavior
- storage observability 與 missing-root degraded semantics 不再只靠 integration 推論
- retention / cleanup policy 仍維持未來 scope

### 4.3 Housekeeping aggregation and archive logic

- `HousekeepingSnapshotProvider` 現在有獨立聚合 test，GPS/storage/comm/boot representative propagation 不再只有 archive integration 間接覆蓋
- `HousekeepingArchiveStore` 已補最小 store/index/request logic test
- 但更高層的 archive command orchestration 仍主要依賴 integration tests，這個弱點在 matrix 中保持顯式

### 4.4 Transparent framing

- framed transport 現在有純邏輯 unit test 覆蓋 encode/decode error semantics
- reconnect、peer restart、與 sustained transport behavior 仍保留在 integration/probe evidence 層

## 5. 測試結果

- focused `ctest` backfill set: `PASS`
  - `transparent_link_framing_unit_test`
  - `gps_bridge_contract_test`
  - `storage_health_bridge_contract_test`
  - `housekeeping_archive_store_unit_test`
  - `housekeeping_snapshot_provider_unit_test`
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`: `PASS`
- `python3 scripts/report_verification_inventory.py`: `PASS`
- `python3 scripts/report_verification_inventory.py --json`: `PASS`
- `python3 scripts/check_repo_consistency.py`: `PASS`
- `openspec validate ut-backfill-v1`: `PASS`
- `openspec validate --specs`: `PASS`

## 6. 驗收結論

- `PASS`：本 change 已補上 verification matrix 指出的最高價值自動化測試缺口
- `PASS`：後期 slice 不再全部只依賴 integration/probe，且沒有為了形式強套 classic `register_fprime_ut()`
- `PASS`：remaining weak spots 仍在 matrix 中明確保留，沒有因補測而假裝 coverage 已完全均衡

## 7. 邊界與限制

- 本次不引入任何新硬體驗證路徑
- `GpsBridge` live UART、`StorageHealth` retention/cleanup、以及 higher-level archive orchestration 仍是後續工作
- `componentsMissingRegisterFprimeUt` 報表仍會列出 `GpsBridge`、`StorageHealthBridge`、`HousekeepingArchive`；這在本 repo 已是有意識的 later-slice policy，不代表缺少自動化測試
