# 08 — 交付工作流與 OpenSpec 循環

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/delivery-workflow/spec.md`](../openspec/specs/delivery-workflow/spec.md)。

## 1. 文件角色

本文件提供人類可讀的交付操作說明與範例。正式 SHALL / SHOULD 規則以 [`openspec/specs/delivery-workflow/spec.md`](../openspec/specs/delivery-workflow/spec.md) 為唯一權威來源。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/09_git_workflow.md` | commit、CI、tag 與 GitHub private repo 原則 |
| `archive/legacy-v0/11_openspec_workflow.md` | OpenSpec 標準循環、artifact 建立順序、apply / validate / archive |

## 3. 治理模型

正式治理分層如下：

1. `openspec/specs/`：正式主規格與唯一 normative workflow source
2. `obc-dev-spec/`：敘述層、操作說明與範例
3. `openspec/changes/<change>/`：正式變更工作區

是否需要 OpenSpec，採這個判準：

- 下列變更需要 formal OpenSpec change：
  - 產品功能或行為
  - 架構決策
  - 驗證路徑與驗證要求
  - CI gate 規則
  - branch / commit / release / formal workflow 規則
  - `openspec/specs/` 下的 main spec
- 下列變更可直接走一般 branch / PR：
  - 審計與報告文檔
  - README / 導覽修正
  - 非正式文案或連結修正
  - 不會改變正式流程或 traceability 要求的 docs-only 維護

## 4. Git / CI 基線

### 4.1 Branch 與 commit

- `main` 必須保持可建置
- 正式 work 一律從專用分支開始，不得直接在 `main` 上開發新功能或新修正
- 正式 OpenSpec work 的 branch 命名採：`feature/<change-name>`、`fix/<change-name>`、`docs/<change-name>`、`hotfix/<change-name>`
- 若該 PR 不需要 OpenSpec，可使用 `docs/<topic>` 或 `fix/<topic>`
- `main` 預設使用 GitHub 的 `Squash and merge`
- PR title 必須是最終 mainline commit 的 Conventional Commit 訊息
- branch 內可保留多個開發 commits
- `rebase` / `squash` branch history 是選用的 review hygiene，不是強制 gate
- 只有以下情況值得整理 branch history：
  - large PR 經過整理後能明顯提升 reviewer 可讀性
  - branch 內有大量雜訊 commits，已妨礙 reviewer 理解
  - commit 過敏感資訊、錯誤中間狀態、或需清理錯檔
- branch 已公開後，預設用 focused fix commits 回應 review；只有明顯必要時才重寫公開歷史

### 4.2 完成 checkpoint 與 push 順序

正式交付順序如下：

1. 完成程式與文件實作。
2. 通過對應的 local verification。
3. 若有 formal OpenSpec change，先執行 `openspec validate`，並用 `openspec archive` 完成封存。
4. 整理工作樹到乾淨狀態。
5. 確認 branch 已達 review-ready 邊界。
6. 此時才可回報為 `local-ready`，也就是「可 push，但尚未正式完成」。
7. 取得開發者明確確認後才 push。
8. push 後預設開 ready-for-review / non-draft PR；除非開發者明確要求 draft，否則不要用 draft PR，因為 reviewer automation 可能只在 ready PR 觸發。
9. 開 PR 後回報 PR link、pushed commit、與目前 CI 狀態；除非開發者明確要求 agent 繼續監看或處理 CI，agent 應停止主動輪詢。
10. 若 review 需要修正，預設以 focused fix commits 處理；CI 綠燈前，不得把該 change 視為正式完成。
11. release / tag 必須等 CI 綠燈後才建立。

`local-ready` 的定義：

- 已實作完成
- local verification 已通過
- 若該 work 需要 OpenSpec，change 已 archive
- worktree 乾淨
- branch 已可直接提 PR

### 4.2.1 執行權限分類

- 先判斷命令類型，再決定是否需要 unrestricted execution；不要用「先在 sandbox 跑一次，失敗再重跑」當成預設流程。
- 下列命令預設直接用 unrestricted：
  - repository-owned probes 與 stack scripts
  - `bash scripts/run_verification_ci.sh`
  - 任何會啟動 hosted runtime、bind 本地 port、建立 ZMQ / GDS / TCP listener、或使用 PTY / serial / UART / SocketCAN 類裝置的命令
  - `gh`、`ssh`、remote Raspberry Pi scripts 這類需要外部網路或遠端 session 的操作
- 下列命令可留在預設 sandbox：
  - `rg`、`sed`、`cat`、`git diff` 這類純讀檔操作
  - `apply_patch` 與其他純 workspace 內編修
  - `openspec validate ...`
  - `python3 scripts/check_repo_consistency.py` 與其他不需要 port / device / external network 的 repo-local 靜態檢查

### 4.3 PR 摘要要求（若使用 PR）

若某次交付採 PR 形式，PR 至少應包含：

1. 變更目的
2. `OpenSpec: <change-name>` 或 `OpenSpec: not required`
3. 影響範圍
4. 測試摘要
5. 若有 `Blocked-HW` 或 `Deferred-RPi`，需附替代證據
6. 風險與 rollback 說明

### 4.4 CI 最小要求

- PR required gate 維持單一 `baseline-gate`
- full gate 內容至少包含：
  - build
  - UT generate / build
  - registered tests
  - code-side component / helper coverage checks
  - retired-path guardrails
  - `openspec validate --specs`
  - repo consistency checks
- PR verification scope 由 repo-local classifier 判斷為 `full` 或 `lightweight`
- `lightweight` 僅適用於明確白名單內的 docs/governance 路徑；它可跳過 F' generate / build / UT / `fprime-util check --all`，但仍必須跑 repo consistency、component-test baseline inventory、retired-path guardrails、以及 `openspec validate --specs`
- 任何 source code、build config、CI workflow、scripts、active OpenSpec change workspace、或未知路徑都必須走 `full`
- 不使用 workflow-level `paths` / `paths-ignore` 來跳過 required workflow；`baseline-gate` job 必須仍然啟動並回報結果
- 不再對 `AGENTS.md` markdown 連結文字或 `verification-matrix.md` 同步問題做 blocking gate

### 4.5 驗證路徑治理

- 新 change 若要重用既有驗證路徑，先查 `docs/verification-path-registry.md`
- 不得只因 generic upstream F' 常識，就假設某條路在本 repository 已經是 baseline
- `OBC -> GDS` TCP adapter path、`fprime-cli -> GDS` command/uplink path、以及透明 UART / framed UART 路徑都屬於不同驗證路徑，除非已有 archive evidence 明確證明，否則不得互相代替
- 每份 evidence 若同時依賴舊路徑與驗新路徑，必須明確寫出：
  - 這次新證明的是哪一條路
  - 只是沿用 baseline 的是哪一條路

### 4.6 基線對齊與 consistency checks

- `docs/baseline-reconciliation-matrix.json` 是唯一人工維護的 reconciliation source
- `docs/baseline-reconciliation-matrix.md` 是由 JSON 生成的 reviewer surface
- 這份矩陣的角色是：
  - 列出 archived change history
  - 說明目前 main specs 已擴張到哪些正式 capability
  - 為每個 archived change 指出 evidence path 或治理型例外理由
- repo-local consistency check 由 `python3 scripts/check_repo_consistency.py` 提供，至少檢查：
  - main specs 不得保留 placeholder Purpose
  - `currentCapabilities` 與 `openspec/specs/*` 對齊
  - 每個 archived change 都必須被對齊矩陣涵蓋
  - 對齊矩陣中引用的 capability 與 evidence path 必須存在
  - checked-in `.md` 必須與 `.json` 生成結果一致

### 4.7 Agent 入口治理

- repo root 提供 `AGENTS.md` 作為未來 agent 的統一 onboarding 入口
- `AGENTS.md` 的角色是：
  - 告訴 agent 先讀哪些 repo 內正式文件
  - 指出正式 workflow、validation path、以及 repo-local skills 在哪裡
  - 明確說明它是入口索引，而不是第二套平行治理文件
- `.codex/skills/openspec-*` 若存在，視為 OpenSpec 對 Codex 生成的 tool-managed skills：
  - 用 `openspec update` 更新
  - 若要移除，先改 OpenSpec 工具或 profile 設定，再重新生成
  - 不把它們當成 repo-specific governance files 手動刪除
- `AGENTS.md` 的固定 read-first 清單應保持精簡：
  - `README.md`
  - `openspec/specs/delivery-workflow/spec.md`
  - `docs/verification-path-registry.md`
- 本文件保留給 agent 或人類在需要操作範例時再參考，不再列為固定必讀

## 5. OpenSpec 標準循環

正式流程如下：

```bash
openspec new change <change-name>
openspec status --change <change-name> --json
openspec instructions proposal --change <change-name> --json
openspec instructions specs --change <change-name> --json
openspec instructions design --change <change-name> --json
openspec instructions tasks --change <change-name> --json
openspec instructions apply --change <change-name> --json
openspec validate <change-name>
openspec archive <change-name> -y
```

規則：

1. `proposal` 先出，`design` / `specs` 依賴 proposal，`tasks` 依賴 design + specs。
2. `apply` 前至少需有 `tasks.md`。
3. 封存一律使用 `openspec archive`，不得以手動 `mv openspec/changes/...` 取代。
4. 每次 archive 前都先執行 `openspec validate`。
5. 若該 change 需要 formal archive，`archive` 完成後才可回報 `local-ready`，不可把「local pass 但尚未 archive」視為可交付狀態。
6. push 與 CI 綠燈屬於 OpenSpec 變更完成後的正式交付 gate；PR 預設應為 ready-for-review / non-draft 以觸發 reviewer automation；agent 可在回報 PR/check pending 狀態後停止主動輪詢，但 CI 綠燈前不得宣告 change 正式完成。

## 6. 任務設計規則

`tasks.md` 應優先包含：

- 程式碼實作任務
- 拓樸與配置整合任務
- L1 / L2 / L3 驗證任務
- CI 任務
- 必要文件與證據任務

治理型 OpenSpec change 可用精簡制：

- 最低要求：`tasks.md` + 對應 spec delta
- `proposal.md`：規則意圖需要說明時再補
- `design.md`：只有在 CI、自動化、或資料來源結構改動時再補

## 7. 對齊資料與審查面

- `baseline-reconciliation-matrix.json` 是 traceability source
- `baseline-reconciliation-matrix.md` 是可讀檢視面
- `verification-matrix.md` 是人工維護的 reviewer surface，不是 formal authority
- formal authority 仍是：
  - checked-in tests
  - `baseline-gate`
  - `openspec/specs/*`
  - archived evidence

## 8. 驗收錨點

1. 主規格與 change workspace 的角色清楚。
2. OpenSpec 使用真實 CLI 流程，而非抽象化或手動搬移流程。
3. Git / CI 規則、完成 checkpoint 與驗證文件一致。
4. 新 agent 能從 repo-root `AGENTS.md` 直接找到正式 workflow 與驗證路徑來源。
5. reconciliation JSON 與 Markdown review surface 不再手工漂移。
