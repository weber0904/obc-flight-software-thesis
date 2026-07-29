# 測試紀錄：copilot-followup-fixes-v1

- 日期：2026-04-06
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - mode: local follow-up hardening and hosted regression validation
- 關聯變更：`openspec/changes/archive/2026-04-06-copilot-followup-fixes-v1/`
- 關聯文件：
  - `openspec/changes/archive/2026-04-06-copilot-followup-fixes-v1/specs/gps-subsystem/spec.md`
  - `openspec/changes/archive/2026-04-06-copilot-followup-fixes-v1/specs/storage-health/spec.md`
  - `openspec/changes/archive/2026-04-06-copilot-followup-fixes-v1/specs/housekeeping-archive/spec.md`
  - `docs/verification-path-registry.md`

## 1. 目標

本 change 的目標是處理已合併 PR 中由 automated review 指出的 follow-up，而不改變既有功能切片的 ownership：

- GPS parser 對第一版 bounded NMEA parsing 補上明確 sentence/field 邊界
- storage health 將 missing-root 與 existing-but-unscannable root 分開
- `STORAGE_SCAN_FAILED` 提供有意義的 failure code
- housekeeping archive record schema 已擴充 storage-health 欄位後，同步更新 file header version
- 修正 archived evidence README 與 verification-path registry 中會造成 review trail 失真的連結/placeholder

本次仍然**不**處理：

- GPS live UART 硬體 bring-up
- storage retention / cleanup policy
- archive reader migration logic
- 新增任何新的 validation path

## 2. 本次實作重點

- `simulators/gps/NmeaParser.*`
  - 新增明確的第一版 sentence-length 與 field-count 界線
  - oversized / over-field-count input 直接回 `MALFORMED`
- `OBC/Components/StorageHealthBridge/*`
  - root 掃描保留 `errorCode`
  - `ENOENT/ENOTDIR` 與其他 scan failure 分開
  - `StorageHealthBridge` 發 event 時不再把 missing-root 和 generic scan-failure 混在一起
- `OBC/Components/HousekeepingArchive/*`
  - housekeeping archive `FILE_VERSION` bump
  - integration test 檢查實際檔案 header version
- 文件收尾：
  - `docs/test-records/gps-subsystem-v1/README.md`
  - `docs/test-records/storage-health-v1/README.md`
  - `docs/verification-path-registry.md`
  - `openspec/specs/verification-path-registry/spec.md`

## 3. 驗證指令

### 3.1 跑完整本機 verification gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

### 3.2 執行 hosted GPS probe

```bash
bash scripts/run_gps_hosted_probe.sh
```

### 3.3 執行 hosted storage-health probe

```bash
bash scripts/run_storage_health_hosted_probe.sh
```

### 3.4 驗證 OpenSpec change 與 main specs

```bash
openspec validate copilot-followup-fixes-v1
openspec validate --specs
```

## 4. 觀察結果

### 4.1 GPS parser boundedness

- valid `GGA/RMC` 行為維持不變
- oversized 與 over-field-count sentence 會被視為 `MALFORMED`
- 這次 hardening 沒有改變 GPS hosted replay path 的 reviewable behavior

### 4.2 Storage-health failure semantics

- missing root 仍會顯式走 `STORAGE_ROOT_MISSING`
- existing-but-unscannable root 不再被當成 missing
- hosted storage probe 重新確認既有 `warning + missing-root + archive capture` 路徑仍可通
- per-root diagnostic code 由 unit/integration 路徑覆蓋，不靠 hosted probe 模糊判讀

### 4.3 Housekeeping archive versioning

- storage-health schema 已存在時，archive file header version 現在會跟著 layout 演進
- archive integration test 已檢查實際產生檔案的 header version

### 4.4 Evidence / registry cleanup

- GPS 與 storage-health test-record 的 change 連結已改成 archived path
- verification-path registry 的 governing evidence 連結已改成相對於 `docs/` 的正確路徑
- `verification-path-registry` main spec 不再保留 placeholder purpose

## 5. 測試結果

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`: `PASS`
  - 包含 generate/build
  - 包含 generate/build `--ut`
  - 包含 `fprime-util check --all`
  - 包含 `openspec validate --specs`
- `run_gps_hosted_probe.sh`: `PASS`
  - 既有 `GPS_FIX_ACQUIRED`
  - 既有 `GPS_FIX_LOST`
  - 既有 `GPS_PARSE_ERROR`
  - 既有 `HK_CAPTURE_RECORDED`
- `run_storage_health_hosted_probe.sh`: `PASS`
  - `STORAGE_WARNING_THRESHOLD_EXCEEDED`
  - `STORAGE_ROOT_MISSING`
  - `STORAGE_SCAN_UPDATED`
  - `HK_CAPTURE_RECORDED`
  - `runtime-root/hk/index.csv` 與 `hk-*.bin`
- `openspec validate copilot-followup-fixes-v1`: `PASS`
- `openspec validate --specs`: `PASS`

## 6. 驗收結論

- `PASS`：GPS bounded parser contract 與實作邊界已一致
- `PASS`：storage-health 對 missing-root 與其他 scan failure 的語意已更精確
- `PASS`：archive file header version 已隨 storage-health record schema 演進
- `PASS`：本次沒有新增 validation path，但已修正 review trail 中的錯誤連結與 placeholder

## 7. 邊界與限制

- 本次不新增新的 hosted / target validation path，只 harden 既有 path
- storage-health per-root diagnostic code 目前主要作為 runtime/event 診斷，不作為 archive record 的第一版欄位
- 本次不處理 GPS live UART、storage cleanup policy、或 archive backward-reader behavior
