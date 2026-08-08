# 測試紀錄：verification-matrix-repair-v1

- 日期：2026-04-09
- 層級：L1 / governance inventory
- 環境：
  - host: macOS development machine
  - mode: verification inventory repair and matrix restructuring
- 關聯變更：`openspec/changes/archive/2026-04-09-verification-matrix-repair-v1/`
- 關聯文件：
  - `docs/verification.md`
  - `scripts/report_verification_inventory.py`
  - `scripts/verification_inventory_lib.py`
  - `scripts/check_component_test_baseline.py`

## 1. 目標

本 change 的目標是修正 verification matrix / inventory 對 repo 現況的分類方式，讓 checked-in coverage summary 不再把：

- 真正的 F' component
- helper / support / topology service

混在同一個 bucket 裡追蹤。

## 2. 本次實作重點

- 重寫 `docs/verification.md`
  - 明確拆成：
    - `Component Coverage`
    - `Helper/Support Coverage`
  - capability matrix 中同步反映新的 classic component harness
- 重寫 `scripts/report_verification_inventory.py`
  - 改為輸出 component/helper split 的 report
- 新增 `scripts/verification_inventory_lib.py`
  - 作為 component/helper inventory 的 shared source of truth
- `scripts/check_component_test_baseline.py`
  - 驗證 matrix 不會把 helper 誤報成 component，也不會把 real component 放進 helper section

## 3. 驗證指令

### 3.1 產出 inventory report

```bash
python3 scripts/report_verification_inventory.py
python3 scripts/report_verification_inventory.py --json
```

### 3.2 執行 matrix / baseline checker

```bash
python3 scripts/check_component_test_baseline.py
python3 scripts/check_repo_consistency.py
```

### 3.3 驗證本 change 與 main specs

```bash
openspec validate verification-matrix-repair-v1
openspec validate --specs
```

## 4. 觀察結果

- inventory report 現在會列出：
  - 13 個 real F' components
  - 5 個 helper/support modules
- `componentsMissingClassicL2` 現在為空，已不再保留舊的 later-slice policy 假設
- verification matrix 的 remaining gaps 已改成真正尚未完成的範圍：
  - hardware-constrained GPS live UART
  - storage retention / cleanup
  - transport reconnect / sustained exchange semantics

## 5. 測試結果

- `python3 scripts/report_verification_inventory.py`: `PASS`
- `python3 scripts/report_verification_inventory.py --json`: `PASS`
- `python3 scripts/check_component_test_baseline.py`: `PASS`
- `python3 scripts/check_repo_consistency.py`: `PASS`
- `openspec validate verification-matrix-repair-v1`: `PASS`
- `openspec validate --specs`: `PASS`

## 6. 驗收結論

- `PASS`：checked-in verification matrix 已不再混淆 component 與 helper coverage
- `PASS`：repo-local inventory tooling 與 matrix 現在對真實 component baseline 有一致的分類
- `PASS`：未來若 matrix 再把 helper 寫進 component section，checker 會直接 fail
