## Why

最近三個已合併 PR 的 Copilot review 留下了幾個具體而且值得處理的 follow-up：GPS parser 雖然宣稱 bounded，但尚未對 sentence 長度與 field 數量做硬限制；storage health 目前會把所有 `stat()` 失敗都視為 missing root，且 `STORAGE_SCAN_FAILED` 缺少可操作的診斷碼；housekeeping archive 在擴充 storage-health 欄位後仍維持相同的檔案格式版本。這些問題都不需要推翻既有功能，但值得用一個小型 fix change 收斂掉，讓 parser、runtime observability、以及 archive format contract 更嚴謹。

## What Changes

- 收斂 GPS 第一版 bounded NMEA parser 的實際邊界，對 sentence 長度與 field 數量施加明確上限，並將超界輸入視為 `MALFORMED`
- 修正 storage health 對 root 缺失與 unreadable / I/O failure 的區分，避免把所有掃描失敗都誤報成 `STORAGE_ROOT_MISSING`
- 讓 `STORAGE_SCAN_FAILED` 攜帶具體 failure code，而不是固定常數
- 在 housekeeping archive record schema 已擴充 storage-health 欄位的情況下，更新 on-disk file header version
- 修正已合併變更留下的 evidence/registry 文件連結與 spec purpose placeholder，讓 review trail 在 archive 後仍可正確追溯

## Capabilities

### New Capabilities

### Modified Capabilities
- `gps-subsystem`: 將第一版 bounded NMEA parser 的 sentence / field 邊界寫成可驗證契約，避免無上限輸入被當成正常第一版 parser 行為
- `storage-health`: 將 missing-root 與 unreadable / scan-failure 語意分開，並讓第一版 `STORAGE_*` failure path 提供可審查的診斷碼
- `housekeeping-archive`: 當 archive record schema 發生不相容擴充時，要求 on-disk header version 一起演進

## Impact

- Affected code:
  - `simulators/gps/NmeaParser.*` 與 GPS support tests
  - `OBC/Components/StorageHealthBridge/*`、storage-health tests、hosted probe assertions
  - `OBC/Components/HousekeepingArchive/*` 與 archive integration tests
  - `evidence/records/*`、`evidence/verification-path-registry.md`、`openspec/specs/verification-path-registry/spec.md`
- Affected APIs / contracts:
  - bounded GPS parser rejection behavior
  - storage scan failure event semantics
  - housekeeping archive file header version
- Affected systems:
  - hosted GPS replay validation
  - hosted storage-health validation
  - review-time evidence navigation
