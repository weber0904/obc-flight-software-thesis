# 測試紀錄：maintenance-baseline-reconciliation-v1

- 日期：2026-04-08
- 層級：L1 / governance consistency
- 環境：
  - host: macOS development machine
  - mode: local maintenance, formal-doc reconciliation, and repo-consistency validation
- 關聯變更：`openspec/changes/archive/2026-04-08-maintenance-baseline-reconciliation-v1/`
- 關聯文件：
  - `docs/baseline-reconciliation-matrix.md`
  - `docs/baseline-reconciliation-matrix.json`
  - `scripts/check_repo_consistency.py`
  - `obc-dev-spec/08_delivery_workflow.md`

## 1. 目標

本 change 的目標是把目前 repository 的 formal docs、archived change history、main-spec set、以及 evidence/exception trail 收斂成可審查的正式對齊狀態，避免未來開發再回到「靠聊天記憶補完 repo workflow」。

本次要證明的是：

- repo 已有一份 checked-in baseline reconciliation matrix
- matrix 可把 original initial baseline queue、archived changes、current capabilities 與 evidence/exception trail 對齊
- repo-local consistency checker 能攔下：
  - main spec placeholder Purpose
  - archived change 未進 matrix
  - matrix 中不存在的 capability
  - matrix 中不存在的 evidence path

本次**不**主張：

- flight runtime 行為有任何新功能
- 新增硬體整合或新驗證路徑
- verification coverage 已因本 change 而補齊

## 2. 本次實作重點

- `docs/baseline-reconciliation-matrix.json`
  - machine-readable source of truth
  - 覆蓋 initial baseline queue、current capability set、archived change history、evidence/exception trail
- `docs/baseline-reconciliation-matrix.md`
  - 人可讀的對齊摘要
- `scripts/check_repo_consistency.py`
  - 驗證 spec purpose、archived-change coverage、capability names、evidence paths
- 文件與 spec 對齊：
  - `obc-dev-spec/08_delivery_workflow.md`
  - `docs/README.md`
  - `scripts/README.md`
  - `openspec/specs/scenario-driven-validation/spec.md`

## 3. 驗證指令

### 3.1 執行 repo consistency checker

```bash
python3 scripts/check_repo_consistency.py
```

### 3.2 驗證本 change artifacts

```bash
openspec validate maintenance-baseline-reconciliation-v1
```

### 3.3 驗證 main specs

```bash
openspec validate --specs
```

### 3.4 執行 baseline verification gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

## 4. 觀察結果

### 4.1 Matrix 覆蓋 archived changes

- matrix 覆蓋目前所有 archived changes
- original initial baseline queue 與 later governed expansions 已明確分開
- governance / follow-up fixes 若未建立 dedicated `evidence/records/<change>/` 目錄，現在都有明確例外說明

### 4.2 Placeholder Purpose 已清除

- `scenario-driven-validation` main spec 不再保留 archive 後未清掉的 placeholder purpose
- checker 會在未來再次出現 placeholder purpose 時直接 fail

### 4.3 Formal delivery docs 與 repo reality 對齊

- `08_delivery_workflow` 不再把 original queue 寫成目前 repository 的完整 change universe
- narrative docs 現在會明確指向 reconciliation matrix 與 consistency checker

## 5. 測試結果

- `python3 scripts/check_repo_consistency.py`: `PASS`
- `openspec validate maintenance-baseline-reconciliation-v1`: `PASS`
- `openspec validate --specs`: `PASS`
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`: `PASS`

## 6. 驗收結論

- `PASS`：repo 現在有正式 baseline reconciliation layer，可審查 original queue、later expansions、以及治理型例外
- `PASS`：main specs 已不再保留已知 placeholder purpose
- `PASS`：後續變更若遺漏 archived-change coverage 或引用不存在的 evidence/capability，checker 會直接攔下

## 7. 邊界與限制

- 本次只做 maintenance / reconciliation，不新增 flight behavior
- matrix 是治理與審查工具，不是第二套 formal spec system
- 本次尚未補齊 UT/L2 coverage；那會在後續 `verification-matrix-v1` 與 `ut-backfill-v1` 內處理
