# 09 — Git、GitHub 與 CI 工作流

## 1. 文件目的

本文件定義第一版專案的版本控制、GitHub private repository 連接方式、tag 策略與 CI 最小要求。

## 2. Git 分支策略

### 2.1 原則

1. `main` 必須保持可建置。
2. 所有開發變更均從 feature / fix 分支進行。
3. 合併前至少通過 build 與相關測試。
4. 大型變更應拆成具語意的多個 commit。

### 2.2 分支命名

| 類型 | 格式 | 範例 |
|------|------|------|
| Feature | `feature/<scope>` | `feature/eps-simulator` |
| Fix | `fix/<scope>` | `fix/csp-timeout-handling` |
| Hotfix | `hotfix/<scope>` | `hotfix/update-rollback-check` |
| Docs | `docs/<scope>` | `docs/boot-update-spec` |

## 3. Commit 規範

### 3.1 正式格式

第一版 commit 規則與根目錄 `SKILL.md` 對齊，採 Conventional Commits：

```text
<type>(<scope>): <繁體中文主旨>
```

### 3.2 類型

| Type | 用途 |
|------|------|
| `feat` | 新功能 |
| `fix` | 修正錯誤 |
| `docs` | 文件變更 |
| `refactor` | 重構 |
| `test` | 測試 |
| `build` | 建置與依賴 |
| `ci` | CI 設定 |
| `chore` | 維護性調整 |

### 3.3 scope 建議

| scope | 說明 |
|-------|------|
| `bootstrap` | 專案建立 |
| `csp-bridge` | `CspBridge` |
| `eps-sim` | EPS 模擬器 |
| `adcs-sim` | ADCS 模擬器 |
| `comm` | 通訊次系統 |
| `boot-update` | Boot & Update Manager |
| `testing` | 測試策略與腳本 |
| `docs` | 規格文件 |

### 3.4 範例

```text
feat(eps-sim): 新增 EPS 狀態回應封包
fix(comm): 修正 UART 中斷恢復流程
docs(boot-update): 調整回滾確認流程說明
test(csp-bridge): 新增 ping timeout 測試
```

## 4. Tag 策略

### 4.1 主要版本標記

| Tag | 含義 |
|-----|------|
| `v0.1.0` | F' + CSP 基線完成 |
| `v0.2.0` | 指令 / 遙測 / 事件框架完成 |
| `v0.3.0` | EPS 模擬器整合完成 |
| `v0.4.0` | ADCS 模擬器整合完成 |
| `v0.5.0` | 通訊次系統完成 |
| `v0.6.0` | Boot & Update Manager 完成 |
| `v1.0.0` | 系統整合完成 |

### 4.2 候選版本

若實作前需先做穩定候選驗證，可使用：

- `v0.3.0-rc.1`
- `v0.6.0-rc.1`

## 5. 本地 Git 與 GitHub private repo 連接

### 5.1 建立 private repo 後新增 remote

```bash
git remote add origin git@github.com:<owner>/<repo>.git
```

或 HTTPS：

```bash
git remote add origin https://github.com/<owner>/<repo>.git
```

### 5.2 推送主分支

```bash
git push -u origin main
```

### 5.3 推送 feature 分支

```bash
git push -u origin feature/eps-simulator
```

### 5.4 建議做法

- 優先使用 SSH key
- GitHub repository 設為 private
- CI 所需 token / secret 一律放 GitHub repository secrets，不寫入 repo

## 6. Pull Request 原則

每個 PR 至少應包含：

1. 變更目的
2. 影響範圍
3. 測試摘要
4. 若有 `Blocked-HW`，需說明替代驗證證據

## 7. CI 最小配置

### 7.1 必跑項目

- build
- unit / component tests
- 基本整合測試（可模擬）

### 7.2 建議 workflow 階段

| 階段 | 內容 |
|------|------|
| `build` | F' generate / build、simulator build |
| `test-unit` | unit tests、component tests |
| `test-integration` | ZMQ / simulator 基本整合驗證 |

### 7.3 不列為第一版 CI 必跑

- 真實 Raspberry Pi 硬體測試
- 真實 UART / GPIO / I2C 測試
- 長時間 soak test

## 8. 安全與維運規則

1. 不提交憑證、金鑰、`.env`、token。
2. 不在未明確要求下進行 force push。
3. 不使用 `--no-verify` 略過 hooks，除非另有明確需求。
4. 若 CI 失敗，需修正後重新提交，不以文件註記取代修正。

## 9. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | commit 規則一致 | 與根目錄 `SKILL.md` 對齊 |
| 2 | tag 策略一致 | 使用 `v0.1.0` ~ `v1.0.0` |
| 3 | GitHub private repo 連接明確 | 有 remote、push 與安全建議 |
| 4 | CI 最小集一致 | 與 [08_testing_verification.md](./08_testing_verification.md) 一致 |
