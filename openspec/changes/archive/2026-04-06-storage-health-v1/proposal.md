## Why

專案目前已明確區分 `runtime-root/hk/`、`persistent-data/`、`staging/`、`logs/` 等 storage roles，也已經有 housekeeping archive、boot staging 與 installed/runtime layout 等功能，但 OBC 本身仍缺少一個 repository-owned storage health view。現在正適合先補上第一版 storage observability，讓 flight software 可以看見 runtime storage 狀態、warning threshold 與缺失 root，而不必先把 retention / cleanup policy 一起做進來。

## What Changes

- 新增第一版 `storage-health` capability，提供 governed runtime storage roots 的觀測與 cached state
- 新增 `StorageHealthBridge`，負責掃描 `hk/`、`persistent-data/`、`staging/`、`logs/` 等 runtime roots，發布 `STORAGE_*` telemetry / event / command
- 第一版只做 health / observability，不做自動刪檔、retention policy、或 destructive cleanup
- 將 storage health cached state 納入 housekeeping archive snapshot，使 storage 狀態可與其他 subsystem cached state 一起保存
- 補齊 hosted validation 與 evidence，明確區分 hosted runtime-root storage health path 與尚未主張的 target-side disk behavior

## Capabilities

### New Capabilities
- `storage-health`: 定義第一版 governed runtime-root storage observability、threshold warning、cached storage state、與 hosted validation 邊界

### Modified Capabilities
- `resource-storage`: 將既有 storage monitoring threshold 與 runtime-root observability 的 requirements 收斂成第一版可驗證能力
- `housekeeping-archive`: 將 storage health cached state 納入 archive snapshot，而不新增另一條 archive-time filesystem policy path
- `verification-evidence`: 為 hosted storage health validation 與 deferred target-side disk behavior 增加可審查證據邊界
- `verification-path-registry`: 將第一版 hosted storage health validation path 登記為可重用 baseline

## Impact

- Affected code:
  - `OBC/Components/` 新增 storage health component family 與 runtime scan support
  - `OBC/Top/` topology、snapshot provider、runtime wiring
  - `OBC/Components/HousekeepingArchive/` snapshot schema
  - `scripts/` hosted validation probe
- Affected APIs:
  - 新增 `STORAGE_*` telemetry / event / command families
  - housekeeping snapshot schema 會增加 storage health 欄位
- Affected systems:
  - hosted runtime validation
  - runtime-root observability for `hk/`, `persistent-data/`, `staging/`, and `logs/`
