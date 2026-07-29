# 11 — OpenSpec 自動化開發工作流

## 1. 目的

本文件定義本專案使用 OpenSpec 的標準開發循環，確保「需求整理 -> 正式規格 -> 程式實作 -> 驗證 -> 封存」可重複執行。

## 2. 三層文件責任

1. `obc-dev-spec/`
- 文字敘述版需求與設計討論來源。
- 可不完全結構化，但必須可追溯。

2. `openspec/specs/`
- 主規格（正式基準）。
- 定義系統目前應有行為。

3. `openspec/changes/<change>/`
- 變更工作區。
- 包含 proposal/design/specs/tasks。
- 真正的 apply 實作來源是 `tasks.md`。

## 3. 標準指令流程

### 3.1 建立或選擇變更

```bash
openspec list --json
openspec new change <change-name>
openspec status --change "<change-name>" --json
```

### 3.2 補齊 artifacts

```bash
openspec instructions proposal --change "<change-name>" --json
openspec instructions design --change "<change-name>" --json
openspec instructions specs --change "<change-name>" --json
openspec instructions tasks --change "<change-name>" --json
```

### 3.3 開始 apply 迭代

```bash
openspec instructions apply --change "<change-name>" --json
```

執行原則：
- 每次迭代 2~5 個任務。
- 完成任務立即勾選 `- [x]`。
- 每次迭代都跑對應範圍檢查（build/test/docs consistency）。

### 3.4 驗證與同步

```bash
openspec status --change "<change-name>" --json
```

- 任務完成後，先做一致性檢查。
- delta specs 同步到主規格時採合併語義，不可整檔覆蓋。

### 3.5 封存

```bash
mkdir -p openspec/changes/archive
mv openspec/changes/<change-name> openspec/changes/archive/YYYY-MM-DD-<change-name>
```

## 4. 任務設計規則（避免只改文件）

`tasks.md` 應優先包含：
- 程式碼實作任務
- 拓樸與配置整合任務
- 測試任務（L1/L2 優先）
- CI 任務
- 必要文件/紀錄任務

不建議把整個變更都寫成 docs-only 任務，除非該變更明確是文件改版。

## 5. 迭代完成判定

一次迭代完成需同時滿足：
1. 指定批次任務已勾選
2. 對應檢查已執行並記錄結果
3. apply 進度有前進

## 6. 異常處理

- 若 apply 顯示 blocked（缺 artifact），先補齊 artifact 再繼續，不停在中途。
- 若出現需求歧義且影響安全/功能正確性，使用互動式問題詢問後再繼續。
- 若僅是非阻塞歧義，記錄假設並持續推進。
