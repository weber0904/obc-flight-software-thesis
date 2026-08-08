# 測試紀錄：agent-onboarding-governance-v1

- 日期：2026-04-08
- 層級：L1 / governance consistency
- 環境：
  - host: macOS development machine
  - mode: local agent-onboarding governance and entrypoint validation
- 關聯變更：`openspec/changes/archive/2026-04-08-agent-onboarding-governance-v1/`
- 關聯文件：
  - `AGENTS.md`
  - `scripts/check_agent_entrypoint.py`
  - `obc-dev-spec/08_delivery_workflow.md`
  - `openspec/specs/delivery-workflow/spec.md`

## 1. 目標

本 change 的目標是把未來 agent 進 repo 時的第一入口正式化，讓新的 agent 不必再靠聊天歷史重建 workflow、validation path 與 repo-local skills 的位置。

本次要證明的是：

- repo root 存在 `AGENTS.md`
- `AGENTS.md` 會導向正式 workflow、validation-path、以及 repo-local skill sources
- repo-local onboarding checker 能攔下必要引用遺失

本次**不**主張：

- `AGENTS.md` 取代 formal specs
- 新增任何 flight runtime 或 hardware behavior
- 建立第二套平行 `CLAUDE.md` / agent-specific rulebook

## 2. 本次實作重點

- 新增 repo-root `AGENTS.md`
- 新增 `scripts/check_agent_entrypoint.py`
- 更新入口文件：
  - `README.md`
  - `.github/README.md`
  - `docs/README.md`
  - `scripts/README.md`
  - `obc-dev-spec/08_delivery_workflow.md`

## 3. 驗證指令

### 3.1 執行 onboarding checker

```bash
python3 scripts/check_agent_entrypoint.py
```

### 3.2 驗證本 change artifacts

```bash
openspec validate agent-onboarding-governance-v1
```

### 3.3 驗證 main specs

```bash
openspec validate --specs
```

### 3.4 驗證 repo consistency

```bash
python3 scripts/check_repo_consistency.py
```

## 4. 觀察結果

- `AGENTS.md` 現在可單獨作為 repo-root onboarding 入口
- 入口文件不直接重寫第二套 workflow，而是回指正式來源
- onboarding checker 能驗證關鍵引用與 skill entrypoints 仍存在

## 5. 測試結果

- `python3 scripts/check_agent_entrypoint.py`: `PASS`
- `openspec validate agent-onboarding-governance-v1`: `PASS`
- `openspec validate --specs`: `PASS`
- `python3 scripts/check_repo_consistency.py`: `PASS`

## 6. 驗收結論

- `PASS`：未來 agent 已有 repo-root 入口可讀
- `PASS`：入口文件可導向正式 workflow / validation / skill sources
- `PASS`：repo 內已有可重跑的 onboarding checker，避免入口文件默默 drift

## 7. 邊界與限制

- 本 change 不建立 `CLAUDE.md` 或其他平行 agent 規範文件
- `AGENTS.md` 是入口索引，不是取代 formal specs 的第二套治理文件
