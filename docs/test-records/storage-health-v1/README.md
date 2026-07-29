# 測試紀錄：storage-health-v1

- 日期：2026-04-06
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - mode: hosted runtime-root observability validation
- 關聯變更：`openspec/changes/archive/2026-04-06-storage-health-v1/`
- 關聯文件：
  - `openspec/changes/archive/2026-04-06-storage-health-v1/specs/storage-health/spec.md`
  - `openspec/changes/archive/2026-04-06-storage-health-v1/specs/resource-storage/spec.md`
  - `openspec/changes/archive/2026-04-06-storage-health-v1/specs/housekeeping-archive/spec.md`
  - `openspec/changes/archive/2026-04-06-storage-health-v1/specs/verification-evidence/spec.md`
  - `docs/verification-path-registry.md`

## 1. 目標

本 change 的目標是在不先引入 retention / cleanup policy 的前提下，補齊第一版 storage observability：

- 由獨立 `storage-health` capability 擁有 governed runtime-root 掃描與 `STORAGE_*` contract
- 掃描：
  - `runtime-root/hk/`
  - `persistent-data/`
  - `staging/`
  - `runtime-root/logs/`
- 保留 missing-root 與 threshold warning 的顯式狀態
- 將 cached storage health state 納入 housekeeping archive

本次仍然**不**處理：

- 自動刪檔或 retention policy
- staging / log cleanup
- install-root payload integrity
- Raspberry Pi target-side disk health claim

## 2. 本次實作重點

- 新增 `OBC/Components/StorageHealthBridge/`
  - `StorageHealthBridge`
  - `StorageScanner`
  - bounded `HealthState` / `RootStats` runtime model
- topology 新增 `storageHealthBridge`
  - 掛到 slow rate group
  - 擁有 `STORAGE_GET_STATUS` / `STORAGE_SCAN_NOW`
- `HousekeepingSnapshotProvider` / `HousekeepingArchiveStore`
  - 新增 storage health cached state capture 與序列化
- `Main.cpp`
  - 新增 operator shell：
    - `storage get`
    - `storage scan`
- 新增：
  - `storage_scanner_unit_test`
  - `storage_health_bridge_cached_state_integration_test`
  - `housekeeping_archive_storage_health_integration_test`
  - `storage get` / `storage scan` operator shell hooks
  - `scripts/run_storage_health_hosted_probe.sh`

## 3. 驗證指令

### 3.1 跑完整本機 verification gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

### 3.2 直接執行 storage scanner unit test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/storage_scanner_unit_test
```

### 3.3 直接執行 storage bridge cached-state integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/storage_health_bridge_cached_state_integration_test
```

### 3.4 直接執行 storage-health housekeeping archive integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/housekeeping_archive_storage_health_integration_test
```

### 3.5 執行 repository-owned hosted storage-health probe

```bash
bash scripts/run_storage_health_hosted_probe.sh
```

## 4. 觀察結果

### 4.1 Governed root scope

- v1 只觀測：
  - `runtime-root/hk/`
  - `persistent-data/`
  - `staging/`
  - `runtime-root/logs/`
- 每個 root 各自保有：
  - `exists`
  - `scanOk`
  - `fileCount`
  - `totalBytes`

### 4.2 Warning / degraded model

- 第一版 warning model 是 bounded byte threshold
- threshold 只決定 warning，不會觸發清理
- missing 或 unreadable root 會進 degraded mask，不會被當成空目錄零值

### 4.3 Housekeeping archive integration

- `HousekeepingSnapshotProvider` 已可讀取 storage health cached state
- `HousekeepingArchiveStore` 已可將 storage health 狀態序列化進 archive record
- archive capture 不會重新掃描檔案系統，而是重用 `StorageHealthBridge` 的 cached state

### 4.4 Hosted probe model

- probe 以 repository-owned hosted stack 驗證：
  - `persistent-data/` 先放一個小檔
  - `staging/` 放超過 warning threshold 的大檔
  - `runtime-root/logs/` 故意不建立
- runtime 啟動後，由 slow rate group 自動觸發 storage scan 與 HK capture
- hosted probe 主要驗：
  - warning / degraded event path
  - archive file 生成
  - governed runtime-root path 可在不經 GDS 的情況下重現

## 5. 測試結果

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`: `PASS`
  - 包含 `fprime-util generate/build`
  - 包含 `fprime-util generate --ut/build --ut`
  - 包含 `fprime-util check --all`
  - 包含 `openspec validate --specs`
- `storage_scanner_unit_test`: `PASS`
  - 正常統計 bounded root file count / bytes
  - `hk/index.csv` 存在時能偵測 index
  - missing `logs/` root 會保留 degraded 狀態
- `storage_health_bridge_cached_state_integration_test`: `PASS`
  - runtime cached state 會保留 warning / degraded / hk index 狀態
- `housekeeping_archive_storage_health_integration_test`: `PASS`
  - storage health cached state 會被 serialise 進 archive record
- `run_storage_health_hosted_probe.sh`: `PASS`
  - hosted runtime 觀察到 `STORAGE_SCAN_UPDATED`
  - hosted runtime 觀察到 `STORAGE_ROOT_MISSING`
  - hosted runtime 觀察到 `STORAGE_WARNING_THRESHOLD_EXCEEDED`
  - hosted runtime 會同時產生 `HK_CAPTURE_RECORDED`、`runtime-root/hk/index.csv` 與 `hk-*.bin`

## 5.1 覆蓋面說明

- per-root file count / bytes 的細節由：
  - `storage_scanner_unit_test`
  - `storage_health_bridge_cached_state_integration_test`
  覆蓋
- hosted probe 則專注在：
  - representative root contents
  - threshold warning
  - missing root
  - housekeeping archive capture

## 6. 驗收結論

- `PASS`：第一版 `storage-health` capability 已建立，且 ownership 保持獨立於 `HousekeepingArchive` 與 `BootManager`
- `PASS`：governed runtime roots 已可用 bounded per-root statistics、warning、與 degraded state 表達
- `PASS`：storage health cached state 已可被 housekeeping archive capture
- `PASS`：本 change 建立了 hosted runtime-root storage health validation path，而未誤宣稱 target-side disk behavior

## 7. 邊界與限制

- v1 不做 retention / cleanup policy
- v1 不做 OS-level free-space governance
- 目前只有 hosted runtime-root evidence
- `Blocked-HW` / `Deferred-RPi`：尚未主張 Raspberry Pi target-side disk behavior、長時間壓力下的真實磁碟使用、或自動刪檔策略
