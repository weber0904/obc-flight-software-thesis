# 07 — 驗證策略與證據規範

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/verification-evidence/spec.md`](../openspec/specs/verification-evidence/spec.md)。

## 1. 文件角色

本文件定義測試分層、關鍵驗證情境、證據保存與人工測試紀錄模板。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/08_testing_verification.md` | L1-L4 測試分層、CI gate、`Blocked-HW` 規則、關鍵場景 |
| `archive/legacy-v0/10_test_record_guide.md` | 人工 / 半自動測試紀錄欄位與模板 |

## 3. 測試分層

| 層級 | 範圍 | 工具 | 頻率 |
|---|---|---|---|
| L1 | 單一函式 / 物理模型 / utility | Google Test / 自訂 UT | 每次主要修改 |
| L2 | 單一 F' 元件 | F' TesterBase + GTest | 每次主要修改 |
| L3 | 多元件 + simulator | GDS、scripts、CSP test nodes | 每次整合前 |
| L4 | 全系統情境 | GDS + 多節點 + 測試紀錄 | 每個主要里程碑 |

## 4. 狀態術語

### 4.1 `Blocked-HW`

缺少真實硬體、線材、GPIO / I2C / UART 或真實裝置而無法驗證的項目。

### 4.2 `Deferred-RPi`

設計已定，但需待 Raspberry Pi 目標整合完成後才可驗證的項目。

### 4.3 必備補件

每個 `Blocked-HW` 或 `Deferred-RPi` 項目都必須記錄：

1. 原始測試目的
2. 阻塞原因
3. 替代驗證方式
4. 摘要結果
5. 解除條件

## 5. 核心驗證場景

第一批固定納入基線驗證的場景如下：

1. `CSP_INIT` / `CSP_PING`
2. `EPS_SET_PDU` 與 low-battery event
3. `ADCS_SET_MODE(DETUMBLE)` 與 pointing error 收斂
4. TCP mock 與 PTY / UART 路徑切換
5. `BOOT_PREPARE_UPDATE` → verify → activate → confirm / rollback
6. hosted OBC 對 `fprime-gds` 的 TCP ground-link 連線
7. Raspberry Pi 原生建置、target stack launch 與 boot metadata restart 驗證
8. Raspberry Pi target build 產生之 framework / project version metadata 檢查
9. Raspberry Pi install bundle 建立、安裝到固定 install root、以及 installed stack launch / clean exit 驗證
10. Raspberry Pi systemd service 安裝、status / journal 檢查、以及 OS reboot 後 installed `current` release 自動拉起驗證
11. Raspberry Pi ↔ host hardware UART probe，含 host serial peer、target serial device path 與觀察到的 radio / raw-UART 收發結果
12. libcsp integration base 回 mainline 前的 release-readiness gate，含 legacy direct-ZMQ checker、OpenSpec validation 與 shared baseline gate
13. Raspberry Pi target 上的 combined CSP + external comm baseline probe，含 OBC node `1`、EPS node `2`、ADCS node `3`、comm-owned `/dev/serial0` 與 host serial peer

補充路徑規則：

- `direct OBC -> GDS` ground path、`fprime-cli -> GDS` command path、internal libcsp foundation path、EPS internal libcsp business path、ADCS internal libcsp business path、external comm path 都必須分開記錄。
- hosted internal libcsp foundation path 只證明 OBC node bring-up、hub binding、peer reachability；不得被描述成 EPS / ADCS 已完成遷移或 ground path 已重驗。
- hosted EPS internal libcsp path 只證明 OBC/client node `1` 到 EPS simulator node `2` 的 EPS business traffic；不得被描述成 ADCS 已遷移、真實 EPS hardware 已驗證、或 ground path 已重驗。
- hosted ADCS internal libcsp path 只證明 OBC/client node `1` 到 ADCS simulator node `3` 的 ADCS business traffic；不得被描述成 ground、external comm、GPS、或真實 ADCS hardware 已驗證。
- legacy EPS / ADCS direct-ZMQ retirement evidence 只證明 active source 與 baseline gate 不再依賴退休的 project-local business transport；不得被描述成新的 hardware proof 或 ground-path proof。
- Raspberry Pi combined CSP + comm evidence 可同時證明 target-side internal CSP reachability 與 external comm UART path，但不得把 `/dev/serial0` 說成 GPS、GDS、RF、或 internal CSP transport。
- GPS live UART 若尚未有獨立 serial allocation，必須記為 `Blocked-HW`，並明確寫出 `/dev/serial0` 已由 external comm/radio path 擁有。

## 6. CI 與 gate 規則

第一版最小 gate：

1. build
2. unit / component tests
3. 基本整合測試（可模擬）

規則：

- 進入元件實作後，PR 合併前至少需通過 build 與對應的 L1 / L2 測試。
- 變更若影響整合行為，需附 L3 自動化結果或人工紀錄。
- 真機受限測試不得直接省略，必須改為 `Blocked-HW` 或 `Deferred-RPi` 證據。
- 一旦 Raspberry Pi 目標整合已執行，先前屬於 `Deferred-RPi` 的相關紀錄需回填並解除，不得繼續以延後狀態保留。
- 若 target workflow 特意省略 `.git` metadata，仍需以證據確認最終產生的 framework / project version 資訊正確，不得只接受 fallback 字串而不加說明。
- 若 target workflow 導入 install bundle，證據必須同時涵蓋 bundle manifest、install root layout、`current` 指向的 release，以及 installed stack 啟動後 runtime roots 的實際位置。
- 若 target workflow 導入 service-managed autostart，證據必須同時涵蓋 unit 安裝方式、`systemctl is-enabled` / `is-active`、journal 摘要，以及 reboot 後由 installed `current` release 重新啟動的觀察結果。
- 若 target workflow 導入真實 UART 硬體驗證，證據必須同時涵蓋 host serial device path、target serial device path、啟動命令、觀察到的 link behavior，以及剩餘 `Blocked-HW` 子案例。
- 若 comm runtime 引入可選 radio protocol adapter，證據必須記錄選用的 adapter 名稱，並清楚區分 default mock adapter 與真實 KISS / vendor protocol 驗證。
- 若 internal CSP foundation 被調整，證據必須明確寫出 hub/proxy 啟動方式、OBC node id、peer node id、所使用的 hosted ports，並說清楚這不是 ground path。
- 若 EPS internal CSP path 被調整，證據必須明確寫出 hub/proxy 啟動方式、OBC/client node id、EPS simulator node id、EPS application service ports、執行的 EPS service commands，並說清楚這不是 ADCS、external comm、GPS、或 ground path。
- 若 ADCS internal CSP path 被調整，證據必須明確寫出 hub/proxy 啟動方式、OBC/client node id、ADCS simulator node id、ADCS application service ports、執行的 ADCS service commands，並說清楚這不是 ground、external comm、GPS、或 real ADCS hardware path。
- 若 legacy direct-ZMQ retirement 被調整，證據必須包含 regression checker、active-source scan、EPS CSP smoke、ADCS CSP smoke、scenario regression 與 baseline gate，並清楚區分 libcsp ZMQHUB hosted substrate 與已退休的 project-local business transport。
- 若 libcsp integration base 準備回 mainline，證據必須列出包含的 archived CSP migration slices、release candidate branch、checker / OpenSpec / baseline gate 結果，以及未新增驗證的硬體範圍。
- 若 Raspberry Pi combined CSP + comm baseline 被調整，證據必須同時涵蓋 target CSP hub ports、node ids、host serial device、target comm serial device、CSP ping / EPS / ADCS / radio / UART raw 的觀察結果，以及仍不涵蓋的 GDS / GPS / RF / vendor control plane。

## 7. 證據保存規則

### 7.1 自動化測試

至少保存：

- 測試名稱
- 執行時間
- pass / fail
- 簡要錯誤摘要

### 7.2 人工或半自動測試

至少保存：

- 測試步驟
- 使用指令
- 預期結果
- 實際摘要結果
- 判定（Pass / Fail / Blocked-HW / Deferred-RPi）

建議保存位置：`docs/test-records/`

## 8. 紀錄模板

```markdown
# 測試紀錄：<測試名稱>

- 日期：YYYY-MM-DD HH:MM
- 層級：L3
- 環境：macOS / dev-macos
- 關聯文件：<formal spec 或 narrative source>

## 前置條件
- 已啟動必要服務

## 測試步驟

| 步驟 | 指令 / 操作 | 預期結果 | 實際摘要結果 | 判定 |
|---|---|---|---|---|
| 1 | `<command>` | `<expected>` | `<actual summary>` | Pass |

## 問題與觀察
- 無

## 結論
- 本測試通過
```

`Blocked-HW` / `Deferred-RPi` 紀錄需額外寫明阻塞原因與解除條件。
