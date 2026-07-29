# 測試紀錄：verification-matrix-v1

- 日期：2026-04-08
- 層級：L1 / governance inventory
- 環境：
  - host: macOS development machine
  - mode: local verification inventory and matrix alignment
- 關聯變更：`openspec/changes/archive/2026-04-08-verification-matrix-v1/`
- 關聯文件：
  - `docs/verification.md`
  - `scripts/report_verification_inventory.py`
  - `openspec/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是把 repo 目前真實存在的 tests / probes / evidence 與 capability-level coverage 統整成正式矩陣，讓後續 UT backfill 有明確起點。

## 2. 本次實作重點

- 新增 `docs/verification.md`
- 新增 `scripts/report_verification_inventory.py`
- 更新 docs / scripts 索引，讓 reviewer 可以找到矩陣與 inventory entrypoint

## 3. 驗證指令

### 3.1 執行 inventory report

```bash
python3 scripts/report_verification_inventory.py
python3 scripts/report_verification_inventory.py --json
```

### 3.2 驗證本 change artifacts

```bash
openspec validate verification-matrix-v1
```

### 3.3 驗證 main specs 與 repo consistency

```bash
python3 scripts/check_repo_consistency.py
openspec validate --specs
```

## 4. 觀察結果

- repo 目前共有 13 個 OBC components，其中 `GpsBridge`、`HousekeepingArchive`、`StorageHealthBridge` 尚未使用 classic `register_fprime_ut()` pattern
- repo 已有 baseline gate、hosted probe scripts、以及豐富的 test-record evidence tree
- 新矩陣已把 capability-level L1/L2/L3/L4 coverage 與 constrained gaps 固定成 review surface

## 5. 測試結果

- `python3 scripts/report_verification_inventory.py`: `PASS`
- `python3 scripts/report_verification_inventory.py --json`: `PASS`
- `openspec validate verification-matrix-v1`: `PASS`
- `python3 scripts/check_repo_consistency.py`: `PASS`
- `openspec validate --specs`: `PASS`

## 6. 驗收結論

- `PASS`：repo 現在有正式 verification matrix 可供後續維護與補測使用
- `PASS`：repo-local inventory tool 可重跑，能盤出目前註冊的 tests / probes / evidence directories
- `PASS`：矩陣已明確指出 `GpsBridge`、`StorageHealthBridge`、`HousekeepingArchive`、`HousekeepingSnapshotProvider` 與 transparent-framing/protocol-adapter path 的後續補強重點
