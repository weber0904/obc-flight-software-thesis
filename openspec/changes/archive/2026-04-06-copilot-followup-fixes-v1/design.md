## Overview

這次變更是已合併功能的 follow-up hardening，而不是新的 capability slice。實作重點是把已存在但尚未收斂完整的 contract 補齊：

- GPS 第一版 parser 的 boundedness 要由明確上限保證，而不是只靠「目前輸入都很短」
- storage health 要把 missing root 與 unreadable / transient I/O failure 分開
- archive record 既然已加入 storage-health 欄位，就要同步更新檔案格式版本
- 已 archive 的 evidence / registry 文件要能正確追到 archive path

## Goals

- 保持既有 GPS fake/replay、storage-health hosted path 與 housekeeping archive 功能不變
- 補足 review 指到的 robustness 與 traceability 問題
- 讓修改能透過現有 unit / integration / hosted probe 路徑驗證

## Non-Goals

- 不新增 GPS live UART 硬體 bring-up
- 不改變 storage-health 的 governed roots 或 warning policy
- 不新增 archive reader migration logic
- 不重新設計 verification-path registry 結構

## Design Decisions

### 1. GPS parser 只補硬上限，不重寫成零配置 parser

第一版修正會保留現有 parser API 與 overall structure，但加入明確邊界：

- 最大 NMEA sentence 長度
- 最大 field 數量

超過任一上限時，一律回傳 `MALFORMED`。這樣可以滿足「bounded first-version parser」契約，而不需要在這個 follow-up 內把 parser 全部改寫成零配置狀態機。

### 2. Storage scan diagnostics 以「exists 語意 + error code」補強

`scanRoot_()` 會保留目前 per-root statistics 掃描模型，但增加下列語意：

- `ENOENT` / `ENOTDIR` 視為 root 不存在
- 其他 `stat()` failure 視為 root 可能存在但無法掃描
- directory traversal 若在 `opendir` / `readdir` / child `stat` 過程失敗，保留第一個有意義的 errno 作為該 root 的 failure code

bridge 發 event 時：

- `!exists` 才發 `STORAGE_ROOT_MISSING`
- `!scanOk` 則發 `STORAGE_SCAN_FAILED(root, <diagnostic-code>)`

這樣 missing 與 unreadable / transient I/O failure 不再混在一起。

### 3. Storage diagnostic code 只用於 runtime state / events，不延伸成新的 archive 欄位

這次會把 storage per-root error code 保留在 runtime/cached state，以支援 event 診斷與未來調試；但不把它們加入 archive record schema。理由：

- 這次 follow-up 的主要目的是修正 event/actionability
- 若把 per-root diagnostic code 也加入 archive schema，會讓變更範圍擴大
- 目前 archive 對 storage-health 的第一版需求仍以 bounded root statistics、warning、degraded state 為主

### 4. Archive file version 與 schema 一起演進

storage-health 欄位已經擴充 housekeeping archive record layout，因此 file header version 應同步提升。這次直接 bump `FILE_VERSION`，讓後續任何 reader 或審查工具都能透過 header 分辨 schema 版本。

### 5. 文件修正與功能修正一起收尾

這次一起修正：

- `docs/test-records/gps-subsystem-v1/README.md`
- `docs/test-records/storage-health-v1/README.md`
- `docs/verification-path-registry.md`
- `openspec/specs/verification-path-registry/spec.md`

原因是這些都是本輪 review 明確指出、且與 validation/evidence navigation 直接相關的 follow-up；把它們留到之後只會讓 review trail 持續帶著錯誤連結。

## Verification Strategy

### GPS

- 補 parser unit test：
  - oversized sentence -> `MALFORMED`
  - too-many-fields sentence -> `MALFORMED`
- 既有 `gps_support_unit_test` 繼續覆蓋 valid / no-fix / checksum failure

### Storage health

- 補 scanner unit/integration test：
  - missing root 與 unreadable root 語意區分
  - per-root diagnostic code 被保留
- hosted storage probe 仍應通過，不應因語意修正而破壞既有 warning/degraded path

### Housekeeping archive

- 既有 archive integration tests 持續通過
- 補針對 file header version 的檢查，避免之後再發生 schema 已變但 version 沒動

### Docs / evidence

- `openspec validate copilot-followup-fixes-v1`
- `openspec validate --specs`
- 直接檢查修正後 evidence README 與 registry 連結是否改為 archive / correct relative paths
