# 測試紀錄：component-test-governance-v1

- 日期：2026-04-09
- 層級：L1 / governance consistency
- 環境：
  - host: macOS development machine
  - mode: local workflow-governance and repo-checker validation
- 關聯變更：`openspec/changes/archive/2026-04-09-component-test-governance-v1/`
- 關聯文件：
  - `AGENTS.md`
  - `docs/verification.md`
  - `scripts/check_component_test_baseline.py`
  - `scripts/report_verification_inventory.py`
  - `openspec/specs/delivery-workflow/spec.md`
  - `openspec/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是把 repo 對 F' component 與 helper/support module 的最低測試要求正式化，避免未來再出現：

- 真正的 F' component 沒有 classic `register_fprime_ut()` harness
- helper 的 direct L1 tests 被誤當成 component harness 的替代品
- verification matrix 把 component 與 helper 混在同一個 coverage bucket

本次要證明的是：

- repo 已有正式的 component-test baseline checker
- workflow / verification specs 已明確區分 component 與 helper 的測試義務
- repo-root entrypoints 已把這個規則寫成未來 agent / 開發者必讀規則

## 2. 本次實作重點

- 新增 `scripts/check_component_test_baseline.py`
  - 枚舉 `OBC/Components/` 下真實的 `*ComponentBase` 類別
  - 驗證每個 real component 都有 classic F' L2 harness
  - 驗證 `docs/verification.md` 已分成 `Component Coverage` 與 `Helper/Support Coverage`
- 新增 `scripts/verification_inventory_lib.py`
  - 提供 component/helper inventory 的 shared source of truth
- 更新：
  - `scripts/run_verification_ci.sh`
  - `AGENTS.md`
  - `.codex/skills/change-closeout/SKILL.md`
  - `docs/verification.md`
- delta specs：
  - `component-test-governance-v1`
  - `delivery-workflow`
  - `verification-evidence`

## 3. 驗證指令

### 3.1 執行 component baseline checker

```bash
python3 scripts/check_component_test_baseline.py
```

### 3.2 執行 verification inventory

```bash
python3 scripts/report_verification_inventory.py
python3 scripts/report_verification_inventory.py --json
```

### 3.3 驗證本 change 與 main specs

```bash
openspec validate component-test-governance-v1
openspec validate --specs
```

### 3.4 執行完整 baseline gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/component-test-baseline-recovery
```

## 4. 觀察結果

- repo 現在有一個正式的 component-test baseline checker，不再只靠 reviewer 肉眼發現缺失
- `AGENTS.md` 與 `change-closeout` skill 已把 component vs helper 的測試規則寫成顯式 guardrail
- baseline gate 現在會自動執行：
  - verification inventory report
  - repo consistency checker
  - agent entrypoint checker
  - component-test baseline checker

## 5. 測試結果

- `python3 scripts/check_component_test_baseline.py`: `PASS`
- `python3 scripts/report_verification_inventory.py`: `PASS`
- `python3 scripts/report_verification_inventory.py --json`: `PASS`
- `openspec validate component-test-governance-v1`: `PASS`
- `openspec validate --specs`: `PASS`
- `bash scripts/run_verification_ci.sh build-artifacts/component-test-baseline-recovery`: `PASS`

## 6. 驗收結論

- `PASS`：repo 已把 classic F' component harness 視為正式 workflow gate，而不是習慣性建議
- `PASS`：helper/support direct tests 的角色已明確保留，但不再被允許替代 component L2 harness
- `PASS`：未來若新增或修改真實 component 卻缺少 classic F' tester，baseline gate 會直接 fail
